import ctranslate2
from huggingface_hub import snapshot_download
import os
from transformers import AutoTokenizer
import re
from tqdm.auto import tqdm

model_list = {
    # "huge": ["JustFrederik/nllb-200-3.3B-ct2-float16","sentencepiece.bpe.model"], # 6.69GB
    "large": ["skywood/nllb-200-distilled-1.3B-ct2-float16","sentencepiece.bpe.model"], # 2.68GB
    "medium": ["skywood/nllb-200-distilled-1.3B-ct2-int8","sentencepiece.bpe.model"], # 1.35GB
    "small": ["skywood/nllb-200-distilled-600M-ct2-int8","sentencepiece.bpe.model"], # 0.6GB
    "en2ko": ["skywood/NHNDQ-nllb-finetuned-en2ko-ct2-float16","sentencepiece.bpe.model"], # 1.23GB - small인 경우 활성화되지 않는다.
    "ko2en": ["skywood/NHNDQ-nllb-finetuned-ko2en-ct2-float16","sentencepiece.bpe.model"], # 1.23GB - small인 경우 활성화되지 않는다.
}

class disabled_tqdm(tqdm):
    def __init__(self, *args, **kwargs):
        kwargs["disable"] = True # True: disable, False: enable
        super().__init__(*args, **kwargs)

class CTrans:
    def __init__(self, base_model, source_lang, target_lang,device="cpu", lang_map=None):
        self.base_model = base_model # 기본 모델 : large, medium, small
        self.model_name = "" # 입력받은 번역 모델 이름 : ALL, en2ko ... 
        self.device = device # cpu, cuda
        self.current_dir = os.getcwd() # 현재 디렉토리
        self.models_directory = os.path.join(self.current_dir, "models") # 모델 저장 디렉토리
        self.is_loaded = False # 모델 로드 여부
        self.load_model_name = "" # 현재 로드한 모델 이름 
        self.model_type = "" # 번역 모델 타입 : nllb, m2m
        self.translator = None # 번역기
        self.sp_model = None # SentencePiece 모델
        self.lang_map = lang_map # 언어 코드 매핑
        self.tokenizer = None # 토크나이저
        self.check_model(source_lang, target_lang)

        self.default_trans_options =  {
            "max_batch_size": 2048, # 최대 배치 크기입니다. 입력 수보다 크면 max_batch_size입력은 길이별로 정렬되고 max_batch_size예제 청크로 분할되어 패딩 위치 수가 최소화됩니다.
            "batch_type":  'tokens',  # max_batch_size"examples" 또는 "tokens"의 개수인지 여부입니다. "tokens"의 경우 입력이 토큰 수로 정렬됩니다.

            "beam_size": 3, #int = 2, # 한 번에 beam_size만큼 탐색하고 가장 좋은 단어 연결을 선택 beam_size>=num_hypotheses>=sampling_topk
            "patience": 1, #float = 1, # 빔 서치 매개변수입니다. 인내도 계수입니다. 1.0이면 최상의 결과를 찾으면 탐색을 중단합니다. 0.5면 50%에서 탐색을 중단합니다. float 1 - 아니요
            "sampling_topk": 1, #int = 1, # 빔 한개당 각 단계별로 상위 후보로부터 가져오는 결과합니다. sampling_topk>=num_hypotheses
            "sampling_topp": 1, #float = 1, # 누적 확률이 이 값을 초과하는 가장 가능성이 높은 토큰을 유지합니다.
            "num_hypotheses": 1, #int = 1, # 빔 한개당 각 단계별 최종 결과 수입니다. beam_size보다 작거나 같아야 합니다. beam_size보다 작으면 확률이 높은 것만 반환됩니다.
            "sampling_temperature": 0.8, #float = 1, # 생성 과정에서 확률적 요소를 조절하여 결과의 다양성과 예측 가능성 사이의 균형을 조정하는 데 사용. 모델의 출력 로짓(softmax 전의 값)에 sampling_temperature를 사용하는 기본적인 식. adjusted logits= logits(출력로짓)/temperature(sampling_temperature). 조정된 로짓을 softmax 함수에 입력하면 최종 확률 분포를 얻을 수 있다. 0.1 ( 높은확률이지만 다양성 부족), 1 (기본), 10 (낮은확률이지만 다양성 높음)
            "repetition_penalty": 2, #float = 1, # 반복 토큰에 대한 패널티 요소.
            "no_repeat_ngram_size": 1, #int = 0, # 0 : 반복을 피하기 위한 n-그램의 크기.
            "length_penalty": 0.8, #float = 1, # 빔 서치 매개변수입니다. 생성되는 시퀀스의 길이에 대한 패널티를 설정합니다. 1보다 작으면 긴 시퀀스가 선호되기 쉽습니다. float 1 - 아니요
            
            "coverage_penalty": 0.4, #float = 0, # 빔 검색 중에 적용되는 커버리지 페널티 가중치입니다. 원본 단어가 과거에 번역된 경우 다시 번역될 가능성이 적으므로 더 낮은 정렬 확률을 할당
            "disable_unk": False, #bool = False, # 알 수 없는 토큰 생성을 비활성화합니다.
            "suppress_sequences": None, #Optional[List[List[str]]] = None,  # 일부 토큰 시퀀스 생성을 비활성화합니다.
            "end_token": None, #Optional[Union[str, List[str], List[int]]] = None, # 이러한 토큰 중 하나에 대한 디코딩을 중지합니다(기본값은 모델 EOS 토큰입니다).
            "return_end_token": False, # bool = False, # 결과에 종료 토큰을 포함합니다.
            "prefix_bias_beta": 0, #float = 0, # 주어진 접두사 쪽으로 번역을 바이어스하기 위한 매개변수입니다.
            "max_input_length": 510, #int = 1024, # 이 많은 토큰 이후의 입력을 자릅니다(비활성화하려면 0으로 설정).
            "max_decoding_length": 255, #int = 256, # 최대 예측 길이.
            "min_decoding_length": 1, #int = 1, # 최소 예측 길이.
            "use_vmap": False, #bool = False, # 이 모델에 저장된 어휘 매핑 파일을 사용합니다.
            "return_scores": True, #bool = False, # 출력에 점수를 포함합니다.
            "return_attention": False, #bool = False, # 출력에 주의 벡터를 포함합니다.
            "return_alternatives": False, #bool = False, # 단일 결과 대신 여러 개의 가능한 결과를 반환할 수 있습니다. 이 옵션을 사용하려면 num_hypotheses를 설정해야 합니다. 사용자에게 선택권을 주기위해 사용할 수 있다. 
            "min_alternative_expansion_prob": 0, #float = 0, # 대안을 확장할 최소 초기 확률입니다.
            "replace_unknowns": False, #bool = False, # 알 수 없는 대상 토큰을 관심도가 가장 높은 소스 토큰으로 교체합니다.
            "callback": None, # Callable[[GenerationStepResult], bool] = None # 1일 때 생성된 각 토큰에 대해 호출되는 선택적 함수입니다 beam_size. 콜백 함수가 를 반환하면 True이 배치에 대한 디코딩이 중지됩니다. beam_size. 콜백 함수가 True를 반환하면 배치에 대한 디코딩이 중지됩니다.
            "asynchronous": False, # bool = False, # AsyncTranslator를 사용하여 번역을 비동기식으로 수행합니다.
        }

        self.low_trans_options =  {
            "max_batch_size": 2048, # 최대 배치 크기입니다. 입력 수보다 크면 max_batch_size입력은 길이별로 정렬되고 max_batch_size예제 청크로 분할되어 패딩 위치 수가 최소화됩니다.
            "batch_type":  'tokens',  # max_batch_size"examples" 또는 "tokens"의 개수인지 여부입니다. "tokens"의 경우 입력이 토큰 수로 정렬됩니다.

            "beam_size": 4, #int = 2, # 한 번에 beam_size만큼 탐색하고 가장 좋은 단어 연결을 선택 beam_size>=num_hypotheses>=sampling_topk
            "patience": 1, #float = 1, # 빔 서치 매개변수입니다. 인내도 계수입니다. 1.0이면 최상의 결과를 찾으면 탐색을 중단합니다. 0.5면 50%에서 탐색을 중단합니다. float 1 - 아니요
            "sampling_topk": 1, #int = 1, # 빔 한개당 각 단계별로 상위 후보로부터 가져오는 결과합니다. sampling_topk>=num_hypotheses
            "sampling_topp": 0.8, #float = 1, # 누적 확률이 이 값을 초과하는 가장 가능성이 높은 토큰을 유지합니다.
            "num_hypotheses": 1, #int = 1, # 빔 한개당 각 단계별 최종 결과 수입니다. beam_size보다 작거나 같아야 합니다. beam_size보다 작으면 확률이 높은 것만 반환됩니다.
            "sampling_temperature": 0.8, #float = 1, # 생성 과정에서 확률적 요소를 조절하여 결과의 다양성과 예측 가능성 사이의 균형을 조정하는 데 사용. 모델의 출력 로짓(softmax 전의 값)에 sampling_temperature를 사용하는 기본적인 식. adjusted logits= logits(출력로짓)/temperature(sampling_temperature). 조정된 로짓을 softmax 함수에 입력하면 최종 확률 분포를 얻을 수 있다. 0.1 ( 높은확률이지만 다양성 부족), 1 (기본), 10 (낮은확률이지만 다양성 높음)
            "repetition_penalty": 2, #float = 1, # 반복 토큰에 대한 패널티 요소.
            "no_repeat_ngram_size": 1, #int = 0, # 0 : 반복을 피하기 위한 n-그램의 크기.
            "length_penalty": 1, #float = 1, # 빔 서치 매개변수입니다. 생성되는 시퀀스의 길이에 대한 패널티를 설정합니다. 1보다 작으면 긴 시퀀스가 선호되기 쉽습니다. float 1 - 아니요
            
            "coverage_penalty": 0, #float = 0, # 빔 검색 중에 적용되는 커버리지 페널티 가중치입니다. 원본 단어가 과거에 번역된 경우 다시 번역될 가능성이 적으므로 더 낮은 정렬 확률을 할당
            "disable_unk": False, #bool = False, # 알 수 없는 토큰 생성을 비활성화합니다.
            "suppress_sequences": None, #Optional[List[List[str]]] = None,  # 일부 토큰 시퀀스 생성을 비활성화합니다.
            "end_token": None, #Optional[Union[str, List[str], List[int]]] = None, # 이러한 토큰 중 하나에 대한 디코딩을 중지합니다(기본값은 모델 EOS 토큰입니다).
            "return_end_token": False, # bool = False, # 결과에 종료 토큰을 포함합니다.
            "prefix_bias_beta": 0, #float = 0, # 주어진 접두사 쪽으로 번역을 바이어스하기 위한 매개변수입니다.
            "max_input_length": 510, #int = 1024, # 이 많은 토큰 이후의 입력을 자릅니다(비활성화하려면 0으로 설정).
            "max_decoding_length": 255, #int = 256, # 최대 예측 길이.
            "min_decoding_length": 1, #int = 1, # 최소 예측 길이.
            "use_vmap": False, #bool = False, # 이 모델에 저장된 어휘 매핑 파일을 사용합니다.
            "return_scores": True, #bool = False, # 출력에 점수를 포함합니다.
            "return_attention": False, #bool = False, # 출력에 주의 벡터를 포함합니다.
            "return_alternatives": False, #bool = False, # 단일 결과 대신 여러 개의 가능한 결과를 반환할 수 있습니다. 이 옵션을 사용하려면 num_hypotheses를 설정해야 합니다. 사용자에게 선택권을 주기위해 사용할 수 있다. 
            "min_alternative_expansion_prob": 0, #float = 0, # 대안을 확장할 최소 초기 확률입니다.
            "replace_unknowns": False, #bool = False, # 알 수 없는 대상 토큰을 관심도가 가장 높은 소스 토큰으로 교체합니다.
            "callback": None, # Callable[[GenerationStepResult], bool] = None # 1일 때 생성된 각 토큰에 대해 호출되는 선택적 함수입니다 beam_size. 콜백 함수가 를 반환하면 True이 배치에 대한 디코딩이 중지됩니다. beam_size. 콜백 함수가 True를 반환하면 배치에 대한 디코딩이 중지됩니다.
            "asynchronous": False, # bool = False, # AsyncTranslator를 사용하여 번역을 비동기식으로 수행합니다.
        }

        self.file_trans_options =  {
            "max_batch_size": 2048, # 최대 배치 크기입니다. 입력 수보다 크면 max_batch_size입력은 길이별로 정렬되고 max_batch_size예제 청크로 분할되어 패딩 위치 수가 최소화됩니다.
            "batch_type":  'tokens',  # max_batch_size"examples" 또는 "tokens"의 개수인지 여부입니다. "tokens"의 경우 입력이 토큰 수로 정렬됩니다.

            "beam_size": 4, #int = 2, # 한 번에 beam_size만큼 탐색하고 가장 좋은 단어 연결을 선택 beam_size>=num_hypotheses>=sampling_topk
            "patience": 1, #float = 1, # 빔 서치 매개변수입니다. 인내도 계수입니다. 1.0이면 최상의 결과를 찾으면 탐색을 중단합니다. 0.5면 50%에서 탐색을 중단합니다. float 1 - 아니요
            "sampling_topk": 1, #int = 1, # 빔 한개당 각 단계별로 상위 후보로부터 가져오는 결과합니다. sampling_topk>=num_hypotheses
            "sampling_topp": 0.8, #float = 1, # 누적 확률이 이 값을 초과하는 가장 가능성이 높은 토큰을 유지합니다.
            "num_hypotheses": 1, #int = 1, # 빔 한개당 각 단계별 최종 결과 수입니다. beam_size보다 작거나 같아야 합니다. beam_size보다 작으면 확률이 높은 것만 반환됩니다.
            "sampling_temperature": 0.8, #float = 1, # 생성 과정에서 확률적 요소를 조절하여 결과의 다양성과 예측 가능성 사이의 균형을 조정하는 데 사용. 모델의 출력 로짓(softmax 전의 값)에 sampling_temperature를 사용하는 기본적인 식. adjusted logits= logits(출력로짓)/temperature(sampling_temperature). 조정된 로짓을 softmax 함수에 입력하면 최종 확률 분포를 얻을 수 있다. 0.1 ( 높은확률이지만 다양성 부족), 1 (기본), 10 (낮은확률이지만 다양성 높음)
            "repetition_penalty": 2, #float = 1, # 반복 토큰에 대한 패널티 요소.
            "no_repeat_ngram_size": 1, #int = 0, # 0 : 반복을 피하기 위한 n-그램의 크기.
            "length_penalty": 0.8, #float = 1, # 빔 서치 매개변수입니다. 생성되는 시퀀스의 길이에 대한 패널티를 설정합니다. 1보다 작으면 긴 시퀀스가 선호되기 쉽습니다. float 1 - 아니요
            
            "coverage_penalty": 0.3, #float = 0, # 빔 검색 중에 적용되는 커버리지 페널티 가중치입니다. 원본 단어가 과거에 번역된 경우 다시 번역될 가능성이 적으므로 더 낮은 정렬 확률을 할당
            "disable_unk": False, #bool = False, # 알 수 없는 토큰 생성을 비활성화합니다.
            "suppress_sequences": None, #Optional[List[List[str]]] = None,  # 일부 토큰 시퀀스 생성을 비활성화합니다.
            "end_token": None, #Optional[Union[str, List[str], List[int]]] = None, # 이러한 토큰 중 하나에 대한 디코딩을 중지합니다(기본값은 모델 EOS 토큰입니다).
            "return_end_token": False, # bool = False, # 결과에 종료 토큰을 포함합니다.
            "prefix_bias_beta": 0, #float = 0, # 주어진 접두사 쪽으로 번역을 바이어스하기 위한 매개변수입니다.
            "max_input_length": 510, #int = 1024, # 이 많은 토큰 이후의 입력을 자릅니다(비활성화하려면 0으로 설정).
            "max_decoding_length": 255, #int = 256, # 최대 예측 길이.
            "min_decoding_length": 1, #int = 1, # 최소 예측 길이.
            "use_vmap": False, #bool = False, # 이 모델에 저장된 어휘 매핑 파일을 사용합니다.
            "return_scores": True, #bool = False, # 출력에 점수를 포함합니다.
            "return_attention": False, #bool = False, # 출력에 주의 벡터를 포함합니다.
            "return_alternatives": False, #bool = False, # 단일 결과 대신 여러 개의 가능한 결과를 반환할 수 있습니다. 이 옵션을 사용하려면 num_hypotheses를 설정해야 합니다. 사용자에게 선택권을 주기위해 사용할 수 있다. 
            "min_alternative_expansion_prob": 0, #float = 0, # 대안을 확장할 최소 초기 확률입니다.
            "replace_unknowns": False, #bool = False, # 알 수 없는 대상 토큰을 관심도가 가장 높은 소스 토큰으로 교체합니다.
            "callback": None, # Callable[[GenerationStepResult], bool] = None # 1일 때 생성된 각 토큰에 대해 호출되는 선택적 함수입니다 beam_size. 콜백 함수가 를 반환하면 True이 배치에 대한 디코딩이 중지됩니다. beam_size. 콜백 함수가 True를 반환하면 배치에 대한 디코딩이 중지됩니다.
            "asynchronous": False, # bool = False, # AsyncTranslator를 사용하여 번역을 비동기식으로 수행합니다.
        }
    
    
    def is_loaded(self):
        return self.is_loaded

    def _download_model(self, repo_id, resume=False):
        # 다운로드 목록
        allow_patterns = [
            "config.json",
            "shared_vocabulary.json",
            "model.bin",
            "special_tokens_map.json",
            "tokenizer.json",
            "tokenizer_config.json",
        ]

        # 다운로드 옵션
        kwargs = {
            "local_files_only": False,
            "allow_patterns": allow_patterns,
            "tqdm_class": disabled_tqdm,
        }

        print(f"#Model loading. Please wait... It may take several minutes or more at first.")
        # Download directly from Huggingface
        model_path = None
        try:
            if( resume == False):
                model_path = snapshot_download(repo_id, **kwargs)
            else:
                model_path = snapshot_download(repo_id, resume_download=resume,**kwargs)
            print("#Loading Success!")
        except:
            # resume download
            #print("#Model download failed. Try to resume download.")
            kwargs["local_files_only"] = True
            if( resume == False):
                model_path = snapshot_download(repo_id, **kwargs)
            else:
                model_path = snapshot_download(repo_id, resume_download=resume, **kwargs)
            print("#Loading Success!")

        return model_path

    def load_model(self):
        if self.model_name == self.load_model_name:
            return None
        
        if self.translator is not None:
            del self.translator

        #print(f"Loading {self.model_name} model...")
        self.load_model_name = self.model_name

        if model_list.get(self.model_name) is not None:
            repo_id = model_list[self.model_name][0]
        else:
            repo_id = model_list[self.base_model][0]

        # 모델 다운로드 체크 및 다운로드 
        model_path = self._download_model(repo_id)
        #print("rf_dir:",model_path)

        # 모델 저장 경로
        #model_path = os.path.join(self.models_directory, repo_id.replace("/", "_"))

        resumable = False
        try:
            self.translator = ctranslate2.Translator(model_path, self.device)
            self.tokenizer = AutoTokenizer.from_pretrained(model_path)
        except:
            # resume download
            print("#Model loading failed. Resume download start.")
            resumable = True
            pass

        # 모델 로딩 오류시 resume 다운로드
        if resumable:
            self._download_model(repo_id, resume=True)
            self.translator = ctranslate2.Translator(model_path, self.device)
            self.tokenizer = AutoTokenizer.from_pretrained(model_path)

        if "nllb" in repo_id:
            self.model_type = "nllb"
        elif "m2m" in repo_id:
            self.model_type = "m2m"
        else:
            self.model_type = None

    def translate(self, source, source_lang, target_lang, lang_map=None):
        if self.model_type == "m2m":
            source_lang = "__" + lang_map[source_lang] + "__"
            target_lang = "__" + lang_map[target_lang] + "__"

        # "…、。" 이 문자들이 있으면 번역이 끊긴다. ( 끊기는 이유가 토큰때문? 아니면 모델 안에서? 파악해봐야. )
        translate_table = str.maketrans({
            '…': '.',
            '。': '.',
            '、': ',',
            '¿': '?',
            '\ufeff': '',
            '\u200b': '',
            '\u200c': '',
            '\u200d': '',
            '\u200e': '',
            '\u200f': '',
            '\r': '',
            '\n': ''
        })
        source = source.translate(translate_table)
        input_list = []
        start_pos = 0
        for i in range(len(source)):
            if source[i] == '.' or source[i] == '!' or source[i] == '?':
                if i+1 < len(source) and source[i+1]==' ' and (i-start_pos)>3 and source[i-1]!='.': # not source[i+1].isdigit() and
                    if (i-start_pos) > 0:
                        input_list.append(source[start_pos:i+1])
                    start_pos = i+1
                    continue
            # 마지막 처리        
            if i == (len(source)-1) and (i-start_pos) > 0:
                input_list.append(source[start_pos:i+1])

        source_sentences = [sent.strip() for sent in input_list]
        target_prefix = [[target_lang]] * len(source_sentences)

        self.tokenizer.src_lang = source_lang
        for i in range(len(source_sentences)):
            source_sentences[i] = self.tokenizer.convert_ids_to_tokens(self.tokenizer.encode(source_sentences[i]))

        # transformer tokenizer
        translation = ""
        if True:
            #if source_lang in ["jpn_Jpan"]: results = self.translator.translate_batch(source_sentences, target_prefix=target_prefix, **self.#low_trans_options)
            #else: results = self.translator.translate_batch(source_sentences, target_prefix=target_prefix, **self.default_trans_options)
            results = self.translator.translate_batch(source_sentences, target_prefix=target_prefix, **self.default_trans_options)
            for result in results:
                translation += self.tokenizer.decode(self.tokenizer.convert_tokens_to_ids(result.hypotheses[0]), skip_special_tokens=True)+" "
        # spm tokenizer
        else:
            # Subword the source sentences
            #print_list(source_sentences)  
            source_sents_subworded = self.sp_model.encode_as_pieces(source_sentences) # spm tokenizer 사용
            # print_list(source_sents_subworded)  
            # print('------------------------------------')
            if self.model_type == "m2m":
                source_sents_subworded = [[source_lang] + sent for sent in source_sents_subworded]
            else:
                source_sents_subworded = [[source_lang] + sent + ["</s>"] for sent in source_sents_subworded]
            print_list(source_sents_subworded) 
            print(source_sents_subworded)

            # Translate the source sentences
            translations_subworded = self.translator.translate_batch(source_sents_subworded, batch_type="tokens", max_batch_size=2024, beam_size=4, target_prefix=target_prefix)
            translations_subworded = [translation.hypotheses[0] for translation in translations_subworded]
            for translation in translations_subworded:
                if target_lang in translation:
                    translation.remove(target_lang)

            # Desubword the target sentences
            translations = self.sp_model.decode(translations_subworded)
            tmp_translations = ""
            for tgt_sent in translations:
                #print(src_sent, tgt_sent, sep="\n• ")
                tmp_translations += tgt_sent
            #translation = translations[0]
            translation = tmp_translations

        return translation
    
    def translate_file(self, source, source_lang, target_lang, lang_map=None):
        translate_table = str.maketrans({
            '…': '.',
            '。': '.',
            '、': ',',
            '¿': '?',
            '\ufeff': '',
            '\u200b': '',
            '\u200c': '',
            '\u200d': '',
            '\u200e': '',
            '\u200f': '',
            '\r': '',
            '\n': ''
        })
    
        text = source
        target_prefix = [[target_lang]] * len(text)

        self.tokenizer.src_lang = source_lang
        contents = []
        for k in range(len(text)):
            #print(text[k])
            text[k] = text[k].strip()
            text[k] = text[k].translate(translate_table)
            if len(text[k]) == 0: continue
            contents.append(self.tokenizer.convert_ids_to_tokens(self.tokenizer.encode(text[k])))

        translation = ""
        # transformer tokenizer
        results = self.translator.translate_batch(contents, target_prefix=target_prefix, **self.file_trans_options)
        for result in results:
            translation += self.tokenizer.decode(self.tokenizer.convert_tokens_to_ids(result.hypotheses[0]), skip_special_tokens=True)+"\r\n"

        return translation
    
    # 모델 변경 체크
    def check_model(self, source_lang, target_lang):
        tmp_model_name = self.get_model_name(source_lang, target_lang)
        
        if tmp_model_name == self.model_name: return # 변경사항이 없는 경우

        # model_list에 있는 경우 
        if model_list.get(tmp_model_name) is not None:
            # large, medium, small 모델 변경은 자유롭게 가능 : 외부 변경 가능
            # 언어 모델은 base_model이 'small'인 경우 변경 불가능 : 저용량 유지를 위해
            if self.base_model == 'small' and self.load_model_name != self.base_model: # small 모델이 로드되지 않은 경우만
                self.model_name = self.base_model
                self.load_model()
            elif self.base_model != 'small' or tmp_model_name in {'large', 'medium', 'small'}:
                self.model_name = tmp_model_name
                self.load_model()
        # model_list에 없는 경우 기본 모델로 변경
        else:
            if self.load_model_name != self.base_model: # 기본 모델이 로드되지 않은 경우만
                self.model_name = self.base_model
                self.load_model()

    # 로딩할 모델 이름 만들기
    def get_model_name(self, src_lang, tgt_lang):
        if src_lang == 'ALL':
            return self.base_model

        # src_lang, tgt_lang가 lang_map에 있는지 확인
        if src_lang not in self.lang_map or tgt_lang not in self.lang_map:
            return self.base_model

        load_trans_model_name = self.lang_map[src_lang]+"2"+self.lang_map[tgt_lang]

        return load_trans_model_name


def print_list(data):
    if data is not None: print((' '.join(map(str,data))).encode('utf-8').decode('utf-8'))