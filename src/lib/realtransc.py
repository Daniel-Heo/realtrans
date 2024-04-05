import os
import numpy as np
from functools import lru_cache
from typing import List, Union, Optional, NamedTuple, TypedDict
import torch
from transformers import Pipeline
from transformers.pipelines.pt_utils import PipelineIterator
import torch.nn.functional as F
import ctranslate2
import faster_whisper

# 처리 오디오 설정값
F_SAMPLE_RATE = 16000 # 샘플링레이트 16000Hz ( 음성인식 학습 데이터 기본 단위 : 고정 )
F_HOP_LENGTH = 160 # 160 : STFT에서 인접한 윈도우 간의 겹치는 부분의 크기를 나타냅니다. 이는 STFT를 계산할 때 윈도우를 얼마나 이동시킬지 결정합니다. 이 값이 작을수록 STFT의 시간 해상도가 높아지지만 주파수 해상도가 낮아지며, 반대로 이 값이 클수록 주파수 해상도가 높아지지만 시간 해상도가 낮아집니다.
F_CHUNK_LENGTH = 8 # 10 : 오디오 입력을 10s 길이의 청크로 나누는 데 사용되는 시간 길이입니다. 이는 오디오 입력을 처리할 때 한 번에 처리할 수 있는 최대 길이를 제한하는 데 사용됩니다.
F_SAMPLES = F_CHUNK_LENGTH * F_SAMPLE_RATE  # 10*16000 = 160000 samples in a 10-second chunk
F_FRAMES = F_SAMPLES/F_HOP_LENGTH  # 160000/160 = 1000 frames in a mel spectrogram input in a 10-second chunk
F_SAMPLES_PER_TOKEN = F_HOP_LENGTH * 2  # 160*2 = 320 - 컨볼루션의 스트라이드 초기값 :  2 ( 2pixel씩 이동하면서 컨볼루션 수행 )
FRAMES_PER_SECOND = F_SAMPLE_RATE/F_HOP_LENGTH  # 16000/160 = 100ms per audio frame
TOKENS_PER_SECOND = F_SAMPLE_RATE/F_SAMPLES_PER_TOKEN  # 16000/320 = 50ms
MEL_FFT = 400 # FFT 크기는 400으로 고정. 이는 STFT(Short-Time Fourier Transform)를 수행할 때 사용되는 FFT 윈도우의 크기를 나타냅니다. FFT 크기가 작을수록 주파수 해상도가 낮아지지만 시간 해상도가 높아지며, 반대로 FFT 크기가 클수록 주파수 해상도가 높아지지만 시간 해상도가 낮아집니다.

@lru_cache(maxsize=None)
def load_mel_filters(n_mels: int, device) -> torch.Tensor:
    """
    Load or compute Mel filterbank.
    """
    assert n_mels in [80, 128], f"Unsupported n_mels: {n_mels}"
    path = os.path.join(os.path.dirname(__file__), "assets", f"mel_{n_mels}_filters.npz")
    with np.load(path) as f:
        return torch.from_numpy(f[f"mel_{n_mels}"]).to(device)
    
def compute_log_mel_spectrogram(
    audio: Union[str, np.ndarray, torch.Tensor],
    n_mels: int = 80,
    padding: int = 0,
    device: Optional[Union[str, torch.device]] = None,
) -> torch.Tensor:
    """
    Compute the log-Mel spectrogram of an audio signal with an optimized single function.
    """
    if not torch.is_tensor(audio):
        audio = torch.from_numpy(audio)

    # Ensure audio is on the correct device
    if device is not None:
        audio = audio.to(device)
    
    # Pad audio if necessary
    if padding > 0:
        audio = F.pad(audio, (0, padding))
    
    # Create Hann window
    window = torch.hann_window(MEL_FFT).to(audio.device)
    
    # Compute STFT
    stft = torch.stft(audio, MEL_FFT, F_HOP_LENGTH, window=window, return_complex=True)
    power_spec = stft[..., :-1].abs() ** 2 # 파워 스펙트럼 계산
    
    # Load Mel filters
    mel_filters = load_mel_filters(n_mels, audio.device)
    
    # Apply Mel filters
    mel_spec = mel_filters @ power_spec
    
    # Compute log spectrogram
    log_spec = torch.log10(torch.clamp(mel_spec, min=1e-10))
    log_spec = torch.maximum(log_spec, log_spec.max() - 8.0)
    log_spec = (log_spec + 4.0) / 4.0
    
    return log_spec

class SingleSeg(TypedDict):
    start: float
    end: float
    text: str

class TranscribeResult(TypedDict):
    segments: List[SingleSeg]
    language: str

# 토크나이저에서 숫자나 특정 기호를 포함하는 토큰을 찾아내는 함수
# 숫자나 %$£ 기호를 포함하는 토큰을 찾아내어 반환합니다. - suppress_numerals 분석용
def find_numeral_symbol_tokens(tokenizer):
    numeral_symbol_tokens = []
    for i in range(tokenizer.eot):
        token = tokenizer.decode([i]).removeprefix(" ")
        has_numeral_symbol = any(c in "0123456789%$£" for c in token)
        if has_numeral_symbol:
            numeral_symbol_tokens.append(i)
    return numeral_symbol_tokens

class WhisperModel(faster_whisper.WhisperModel):
    '''
    FasterWhisperModel은 더 빠른 속삭임에 대한 일괄 추론을 제공합니다.
     현재는 비타임스탬프 모드에서만 작동하며 일괄 처리의 모든 샘플에 대한 고정 프롬프트가 있습니다.
    '''

    """
    이 함수는 음성 인식 및 변환 작업을 배치 처리하는 데 사용되며, Faster Whisper와 같은 음성 인식 모델의 일부로 사용될 수 있습니다. 기능적으로, 입력된 음성 특성(features)을 텍스트로 변환합니다. 함수의 주요 구성 요소 및 흐름은 다음과 같습니다:

    입력 매개변수:

    features: 음성 데이터의 특성을 나타내는 NumPy 배열입니다. 이 배열은 모델이 인식을 수행할 음성 데이터를 포함합니다.
    tokenizer: 텍스트를 토큰으로 변환하고, 그 반대도 수행하는 토크나이저 객체입니다.
    options: 트랜스크립션 옵션을 포함하는 객체로, 초기 프롬프트, 타임스탬프 처리 방식, 출력 접두사 등을 설정할 수 있습니다.
    encoder_output: 선택적으로, 인코더의 출력을 직접 전달할 수 있습니다. 기본값은 None이며, 이 경우 함수 내에서 features를 이용해 인코딩을 수행합니다.
    배치 크기 설정: 입력된 features 배열의 첫 번째 차원을 사용하여 배치 크기를 설정합니다.

    초기 프롬프트 처리: 사용자가 지정한 초기 프롬프트가 있는 경우, 이를 토큰화하여 all_tokens 리스트에 추가합니다.

    프롬프트 생성: self.get_prompt 메서드를 사용하여 모델에 전달할 프롬프트를 생성합니다. 이때, 초기 프롬프트 또는 이전 토큰, 타임스탬프 처리 방식, 출력 접두사 등 옵션에 따라 프롬프트가 조정됩니다.

    인코딩: self.encode 메서드를 사용하여 입력된 features로부터 인코더 출력을 생성합니다. 사용자가 encoder_output을 제공한 경우, 이 단계는 생략됩니다.

    모델 생성 호출: self.model.generate를 사용하여 텍스트 생성 작업을 수행합니다. 이때, 인코더 출력과 생성된 프롬프트, 빔 크기(beam size), 인내심(patience), 길이 패널티(length penalty), 최대 길이, 공백 억제 및 토큰 억제 옵션이 사용됩니다.

    결과 처리: 생성된 결과에서 토큰 시퀀스를 추출하여 배치 처리합니다.

    배치 디코드: decode_batch 함수를 정의하고 사용하여 토큰 배치를 텍스트로 디코드합니다. 이 과정에서 tokenizer.eot보다 작은 토큰만을 유효한 것으로 간주하여 디코드합니다.

    텍스트 반환: 최종적으로 디코드된 텍스트를 반환합니다.

    이 함수는 복잡한 음성 인식 작업을 처리하며, 다양한 옵션을 통해 결과의 세밀한 조정이 가능합니다. 배치 처리를 통해 효율성을 높이고, 사용자 지정 옵션으로 출력을 최적화할 수 있습니다.
    """
    def generate_segment_batched(self, features: np.ndarray, tokenizer: faster_whisper.tokenizer.Tokenizer, options: faster_whisper.transcribe.TranscriptionOptions, encoder_output = None):
        if hasattr(features, 'shape'):
            batch_size = features.shape[0]
        else:
            batch_size = 1
        all_tokens = []
        prompt_reset_since = 0
        if options.initial_prompt is not None:
            initial_prompt = " " + options.initial_prompt.strip()
            initial_prompt_tokens = tokenizer.encode(initial_prompt)
            all_tokens.extend(initial_prompt_tokens)
        previous_tokens = all_tokens[prompt_reset_since:]
        prompt = self.get_prompt(
            tokenizer,
            previous_tokens,
            without_timestamps=options.without_timestamps,
            prefix=options.prefix,
        )

        encoder_output = self.encode(features)

        max_initial_timestamp_index = int(
            round(options.max_initial_timestamp / self.time_precision)
        )

        result = self.model.generate(
                encoder_output,
                [prompt] * batch_size,
                beam_size=options.beam_size,
                patience=options.patience,
                length_penalty=options.length_penalty,
                max_length=self.max_length,
                suppress_blank=options.suppress_blank,
                suppress_tokens=options.suppress_tokens,
            )

        tokens_batch = [x.sequences_ids[0] for x in result]

        def decode_batch(tokens: List[List[int]]) -> str:
            res = []
            for tk in tokens:
                res.append([token for token in tk if token < tokenizer.eot])
            # text_tokens = [token for token in tokens if token < self.eot]
            return tokenizer.tokenizer.decode_batch(res)

        text = decode_batch(tokens_batch)

        return text

    """
    encode 함수는 음성 인식 모델의 일부로서, 입력된 음성 특성(features)을 모델이 이해할 수 있는 인코딩된 형태로 변환하는 역할을 합니다. 이 과정은 모델의 인코더 부분에서 수행되며, 특히 여러 GPU에서 모델을 실행할 때의 처리를 고려한 구현이 포함되어 있습니다. 

    다중 GPU 처리: 모델이 CUDA(즉, NVIDIA GPU)를 사용하고 여러 개의 GPU(self.model.device_index의 길이가 1보다 큰 경우)에서 실행될 때, 인코더의 출력을 CPU로 이동시키는 것이 필요합니다. 이는 후속 작업에서 어떤 GPU가 작업을 처리할지 모르기 때문입니다. to_cpu 변수는 이러한 상황에서 True가 되어, 인코딩된 출력을 CPU로 이동시키라는 지시를 나타냅니다.

    배치 크기 확인 및 조정: 입력된 features 배열의 차원을 확인하여, 배치 크기가 1인 경우(즉, features의 차원이 2인 경우) np.expand_dims를 사용하여 배치 차원을 추가합니다. 이는 모델이 일관된 배치 처리 인터페이스를 통해 작업을 수행할 수 있도록 하기 위함입니다.

    CTranslate2 저장소 변환: faster_whisper.transcribe.get_ctranslate2_storage 함수를 사용하여 NumPy 배열 형태의 features를 CTranslate2(딥러닝 모델 최적화 및 변환을 위한 라이브러리)의 StorageView로 변환합니다. 이 변환은 모델이 입력 데이터를 효율적으로 처리할 수 있게 해주며, CTranslate2를 사용하는 이유는 속도와 메모리 사용 최적화 때문입니다.

    인코딩 실행: 마지막으로, self.model.encode 메서드를 호출하여 변환된 features를 인코딩합니다. 여기서 to_cpu 인자는 앞서 계산된 대로, 모델의 출력을 CPU로 이동할지 여부를 결정합니다.

    이 함수의 반환값은 ctranslate2.StorageView 객체로, 이는 인코딩된 음성 특성을 나타냅니다. 이러한 인코딩된 출력은 모델의 디코더 부분에서 사용되어 최종적인 텍스트 변환 결과를 생성하는 데 기여합니다. 다중 GPU 환경에서의 세심한 처리를 통해, 이 함수는 음성 인식 모델의 효율성과 확장성을 보장하는 역할을 합니다.
    """
    def encode(self, features: np.ndarray) -> ctranslate2.StorageView:
        to_cpu = self.model.device == "cuda" and len(self.model.device_index) > 1
        if hasattr(features, 'shape'):
            if len(features.shape) == 2:  # 배치 크기가 1인 경우
                features = np.expand_dims(features, 0) # 배치 차원 추가
        else:
            print("Features has no shape attribute.")
            return None
        features = faster_whisper.transcribe.get_ctranslate2_storage(features) # CTranslate2 저장소 변환

        return self.model.encode(features, to_cpu=to_cpu)

class FasterWhisperPipeline(Pipeline):
    def __init__(
            self,
            model,
            options : NamedTuple,
            tokenizer=None,
            device: Union[int, str, "torch.device"] = -1,
            framework = "pt",
            language : Optional[str] = None,
            suppress_numerals: bool = False,
    ):
        self.model = model
        self.tokenizer = tokenizer
        self.options = options
        self.preset_language = language
        self.suppress_numerals = suppress_numerals
        self._num_workers = 1
        self._preprocess_params, self._forward_params, self._postprocess_params = self._sanitize_parameters()
        self.call_count = 0
        self.framework = framework
        if self.framework == "pt":
            if isinstance(device, torch.device):
                self.device = device
            elif isinstance(device, str):
                self.device = torch.device(device)
            elif device < 0:
                self.device = torch.device("cpu")
            else:
                self.device = torch.device(f"cuda:{device}")
        else:
            self.device = device

        super(Pipeline, self).__init__()

    # 입력된 매개변수를 사전 처리하여 반환합니다.
    def _sanitize_parameters(self):
        return {}, {}, {}
    
    # 입력된 데이터를 전처리, 모델에 전달, 결과를 후처리하여 반환합니다.
    def get_iterator(
        self, inputs, num_workers: int, batch_size: int, preprocess_params, forward_params, postprocess_params
    ):
        dataset = PipelineIterator(inputs, self.preprocess, preprocess_params)

        def stack(items):
            return {'inputs': torch.stack([x['inputs'] for x in items])}
        dataloader = torch.utils.data.DataLoader(dataset, num_workers=num_workers, batch_size=batch_size, collate_fn=stack)
        model_iterator = PipelineIterator(dataloader, self.forward, forward_params, loader_batch_size=batch_size)
        final_iterator = PipelineIterator(model_iterator, self.postprocess, postprocess_params)
        return final_iterator

    # 입력된 데이터를 전처리하여 반환합니다.
    # 처리 내용 : 입력된 오디오 데이터를 Mel 스펙트로그램으로 변환합니다.
    def preprocess(self, audio):
        if 'inputs' in audio:
            audio = audio['inputs']
        else:
            print("Key 'inputs' not found in audio.")
            return {'inputs': None}
        model_n_mels = self.model.feat_kwargs.get("feature_size") # 모델 멜 스펙트로그램의 크기
        features = compute_log_mel_spectrogram(
            audio,
            n_mels=model_n_mels if model_n_mels is not None else 80, # Mel 필터의 수 
            padding=F_SAMPLES - audio.shape[0], # 최대 입력길이에서 입력길이를 뺀 값 :10초(160000) - 입력길이
        )
        return {'inputs': features}

    # 입력된 데이터를 모델에 전달하여 결과를 반환합니다.
    def _forward(self, model_inputs):
        if 'inputs' in model_inputs:
            model_inputs = model_inputs['inputs']
        else:
            print("Key 'inputs' not found in model_inputs.")
            return {'text': None}
        outputs = self.model.generate_segment_batched(model_inputs, self.tokenizer, self.options)
        return {'text': outputs}

    # 모델의 결과를 후처리하여 반환합니다.
    def postprocess(self, model_outputs):
        return model_outputs

    # 입력된 데이터를 전처리, 모델에 전달, 결과를 후처리하여 반환합니다.
    def transcribe(
        self, audio: Union[str, np.ndarray], batch_size=None, num_workers=0, language=None, task=None
    ) -> TranscribeResult:

        def data(audio):
            yield {'inputs': audio[:]}

        if self.tokenizer is None:
            if language is None:
                language = self.detect_language(audio)
            task = task or "transcribe"
            self.tokenizer = faster_whisper.tokenizer.Tokenizer(self.model.hf_tokenizer,
                                                                self.model.model.is_multilingual, task=task,
                                                                language=language)
        else:
            language = language or self.tokenizer.language_code
            task = task or self.tokenizer.task
            if task != self.tokenizer.task or language != self.tokenizer.language_code:
                self.tokenizer = faster_whisper.tokenizer.Tokenizer(self.model.hf_tokenizer,
                                                                    self.model.model.is_multilingual, task=task,
                                                                    language=language)

        segments: List[SingleSeg] = []
        batch_size = batch_size or 1
        for idx, out in enumerate(self.__call__(data(audio), batch_size=batch_size, num_workers=num_workers)):
            text = out['text']
            if batch_size in [0, 1, None]:
                text = text[0]
            segments.append(
                {
                    "text": text,
                    "start": 0.0,
                    "end": round(len(audio)/F_SAMPLE_RATE, 3)
                }
            )

        # revert the tokenizer if multilingual inference is enabled
        if self.preset_language is None:
            self.tokenizer = None

        return {"segments": segments, "language": language}

    # 입력된 오디오 데이터의 언어를 감지하여 반환합니다.
    def detect_language(self, audio: np.ndarray):
        model_n_mels = self.model.feat_kwargs.get("feature_size")
        segment = compute_log_mel_spectrogram(audio[: F_SAMPLES],
                                      n_mels=model_n_mels if model_n_mels is not None else 80,
                                      padding=0 if audio.shape[0] >= F_SAMPLES else F_SAMPLES - audio.shape[0])
        encoder_output = self.model.encode(segment)
        results = self.model.model.detect_language(encoder_output)
        language_token, language_probability = results[0][0]
        language = language_token[2:-2]
        return language

# 입력된 매개변수를 사용하여 FasterWhisperPipeline 인스턴스를 생성하고 반환합니다.
def load_model(model_name,
               device, # cuda or cpu 
               device_index=0, # 0
               compute_type="float16", # float16, float32, int16, int8
               language : Optional[str] = None, # recognition language
               download_root=None, # model download root
               threads=4): # number of threads

    # model_name에 '.'이 있으면 그 뒤의 문자열을 language로 지정합니다.
    if "." in model_name:
        model_name, language = model_name.split(".", 1)
    # model_name이 없으면 language를 model_name으로 지정합니다.
    if model_name is None:
        model_name = language

    model = WhisperModel(model_name,
                         device=device, # cpu or gpu
                         device_index=device_index, # 0
                         compute_type=compute_type, # float16
                         download_root=download_root, # model download root
                         cpu_threads=threads)
    
    default_asr_options =  {
        "beam_size": 5, # 생성 중에 사용할 빔의 수.
        "best_of": 5, # 생성 후 유지할 빔의 수.
        "patience": 1, # 조기 중단 전에 개선을 위해 기다릴 횟수.
        "length_penalty": 1, # 더 긴 시퀀스에 대한 패널티 요소.
        "repetition_penalty": 1, # 반복 토큰에 대한 패널티 요소.
        "no_repeat_ngram_size": 0, # 반복을 피하기 위한 n-그램의 크기.
        "temperatures": [0.0, 0.2, 0.4, 0.6, 0.8, 1.0], # 샘플링을 위한 온도 값의 목록.
        "compression_ratio_threshold": 2.4, # 압축된 토큰을 필터링하기 위한 임계값.
        "log_prob_threshold": -1.0, # 낮은 확률 토큰을 필터링하기 위한 임계값.
        "no_speech_threshold": 0.6, # 낮은 신뢰도의 음성 토큰을 필터링하기 위한 임계값.
        "condition_on_previous_text": False, # 이전 텍스트에 대한 생성 조건 여부.
        "prompt_reset_on_temperature": 0.5, # 프롬프트를 재설정하기 위한 온도 임계값.
        "initial_prompt": None, # 텍스트 생성을 위한 초기 프롬프트.
        "prefix": None, # 텍스트 생성을 위한 접두사.
        "suppress_blank": True, # 빈 토큰을 억제할지 여부.
        "suppress_tokens": [-1], # 생성 중에 억제할 토큰의 목록.
        "without_timestamps": True, # 생성된 텍스트에 타임스탬프를 포함할지 여부.
        "max_initial_timestamp": 0.0, # 생성된 텍스트에 대한 최대 초기 타임스탬프.
        "word_timestamps": False, # 단어 수준에서 타임스탬프를 포함할지 여부.
        "prepend_punctuations": "",#"prepend_punctuations": "\"'“¿([{-", # 생성된 텍스트 앞에 붙일 구두점.
        "append_punctuations": "",#"append_punctuations": "\"'.。,，!！?？:：”)]}、", # 생성된 텍스트 뒤에 붙일 구두점.
        "max_new_tokens": None, # 생성할 새 토큰의 최대 수.
        "clip_timestamps": None, # 타임스탬프를 특정 범위로 클립할지 여부.
        "hallucination_silence_threshold": None, # 생성된 텍스트에서 환각 감지를 위한 임계값.
    }

    default_asr_options = faster_whisper.transcribe.TranscriptionOptions(**default_asr_options)

    return FasterWhisperPipeline(
        model=model,
        options=default_asr_options,
        language=language,
    )