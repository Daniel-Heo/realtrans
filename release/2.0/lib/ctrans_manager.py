#import sentencepiece as spm
import ctranslate2
from huggingface_hub import snapshot_download
import os
from transformers import AutoTokenizer

model_list = {
    # "huge": ["JustFrederik/nllb-200-3.3B-ct2-float16","sentencepiece.bpe.model"], # 6.69GB
    "large": ["skywood/nllb-200-distilled-1.3B-ct2-float16","sentencepiece.bpe.model"], # 2.68GB
    "medium": ["skywood/nllb-200-distilled-1.3B-ct2-int8","sentencepiece.bpe.model"], # 1.35GB
    "small": ["skywood/nllb-200-distilled-600M-ct2-int8","sentencepiece.bpe.model"], # 0.6GB
    "en2ko": ["skywood/NHNDQ-nllb-finetuned-en2ko-ct2-float16","sentencepiece.bpe.model"], # 1.23GB - small인 경우 활성화되지 않는다.
    "ko2en": ["skywood/NHNDQ-nllb-finetuned-ko2en-ct2-float16","sentencepiece.bpe.model"], # 1.23GB - small인 경우 활성화되지 않는다.
    #"en2ko": ["skywood/NHNDQ-nllb-finetuned-en2ko-ct2-int8","sentencepiece.bpe.model"], # 1.23GB - small인 경우 활성화되지 않는다.
    #"ko2en": ["skywood/NHNDQ-nllb-finetuned-ko2en-ct2-int8","sentencepiece.bpe.model"], # 1.23GB - small인 경우 활성화되지 않는다.
}

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
    
    def is_loaded(self):
        return self.is_loaded

    def _download_model(self, repo_id, resume=False):
        # 저장할 모델이름으로 변환 : / -> _
        model_savename = repo_id.replace("/", "_")

        # 다운로드 모델 디렉토리 경로
        model_path = os.path.join(self.models_directory, model_savename)

        # 다운로드 체크 : 모델 디렉토리가 있고 resume이 False인 경우 다운로드하지 않는다.
        if os.path.exists(model_path) and resume is False:
            return None

        # 모델 메인 디렉토리가 없으면 생성
        if not os.path.exists(self.models_directory) :
            os.makedirs(self.models_directory)

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
            "local_dir": model_path,
            "local_dir_use_symlinks": False,
            "cache_dir": None,
            #"tqdm_class": disabled_tqdm,
        }

        import warnings
        # huggingface_hub 관련 경고를 무시하기 위한 경고 필터 설정 : 심볼릭링크를 사용하려면 관리자 권한이 필요하다. 사용자에게 관리자 권한을 요구할만한 사항은 아니라 경고를 무시한다.
        warnings.filterwarnings("ignore", message="`huggingface_hub` cache-system uses symlinks")

        print(f"Model {model_path} Downloading.\nPlease wait... It may take several minutes or more.")
        # Download directly from Huggingface
        try:
            if( resume == False):
                snapshot_download(repo_id, **kwargs)
            else:
                snapshot_download(repo_id, resume_download=resume,**kwargs)
            print("Download Success!")
        except:
            # resume download
            print("Model download failed. Try to resume download.")
            kwargs["local_files_only"] = True
            if( resume == False):
                snapshot_download(repo_id, **kwargs)
            else:
                snapshot_download(repo_id, resume_download=resume, **kwargs)
            print("Download Success!")

        return model_path

    def load_model(self):
        if self.model_name == self.load_model_name:
            return None
        
        if self.translator is not None:
            del self.translator

        print(f"Loading {self.model_name} model...")
        self.load_model_name = self.model_name

        if model_list.get(self.model_name) is not None:
            repo_id = model_list[self.model_name][0]
        else:
            repo_id = model_list[self.base_model][0]

        # 모델 다운로드 체크 및 다운로드 
        self._download_model(repo_id)

        # 모델 저장 경로
        model_path = os.path.join(self.models_directory, repo_id.replace("/", "_"))

        resumable = False
        try:
            self.translator = ctranslate2.Translator(model_path, self.device)
            self.tokenizer = AutoTokenizer.from_pretrained(model_path)
        except:
            # resume download
            print("Model loading failed. Resume download start.")
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

        source_sents = [source]
        source_sentences = [sent.strip() for sent in source_sents]
        target_prefix = [[target_lang]] * len(source_sentences)

        # transformer tokenizer
        if True:
            self.tokenizer.src_lang = source_lang
            source = [self.tokenizer.convert_ids_to_tokens(self.tokenizer.encode(source))]
            results = self.translator.translate_batch(source, batch_type="tokens", max_batch_size=1, beam_size=4, target_prefix=target_prefix)
            translation = self.tokenizer.decode(self.tokenizer.convert_tokens_to_ids(results[0].hypotheses[0]), skip_special_tokens=True)
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
            load_trans_model_name = self.base_model
        else:
            load_trans_model_name = self.lang_map[src_lang]+"2"+self.lang_map[tgt_lang]

        return load_trans_model_name



def print_list(data):
    if data is not None: print((' '.join(map(str,data))).encode('utf-8').decode('utf-8'))