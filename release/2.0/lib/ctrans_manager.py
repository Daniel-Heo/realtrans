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
    def __init__(self, base_model, source_lang, target_lang,device="cpu", save_directory="models", lang_map=None):
        self.base_model = base_model # 기본 모델 : large, medium, small
        self.model_name = "" # 입력받은 번역 모델 이름 : ALL, en2ko ... 
        self.device = device # cpu, cuda
        self.save_directory = save_directory # 모델 저장 디렉토리
        self.is_loaded = False # 모델 로드 여부
        self.load_model_name = "" # 현재 로드한 모델 이름 
        self.model_type = "" # 번역 모델 타입 : nllb, m2m
        self.current_dir = "" # 현재 디렉토리
        self.translator = None # 번역기
        self.sp_model = None # SentencePiece 모델
        self.lang_map = lang_map # 언어 코드 매핑
        self.tokenizer = None # 토크나이저
        self.check_model(source_lang, target_lang)
    
    def is_loaded(self):
        return self.is_loaded

    def _download_model(self, model_name, save_directory="models", resume=False):
        save_directory = os.path.join(self.current_dir, self.save_directory)

        # Check if the model exists in the model list
        if model_name in model_list:
            model_name = model_list[model_name][0]

        # Replace "/" with "_" in the model name
        model_savename = model_name.replace("/", "_")

        # Check the path for the model and tokenizer
        model_path = os.path.join(save_directory, model_savename)

        # Create the main model directory if it doesn't exist
        if not os.path.exists(save_directory) :
            os.makedirs(save_directory)

        # Check if the model is already downloaded
        if not os.path.exists(model_path) or resume is True:
            print(f"{model_name} Model {model_path} Downloading. Please wait Download Success Message... \nUsually 3GB is downloaded. It may take several minutes or more.")
            # Download directly from Huggingface
            snapshot_download(model_name, revision="main", local_dir=model_path, resume_download=resume)
            print("Download Success!")

    def load_model(self):
        if self.model_name == self.load_model_name:
            return None
        
        if self.translator is not None:
            del self.translator
            #del self.sp_model

        print(f"Loading {self.model_name} model...")
        self.load_model_name = self.model_name

        self.current_dir = os.getcwd()
        save_directory = os.path.join(self.current_dir, self.save_directory)

        if model_list.get(self.model_name) is not None:
            hf_name = model_list[self.model_name][0]
            #sp_model_name = model_list[self.model_name][1]
        else:
            hf_name = model_list[self.base_model][0]
            #sp_model_name = model_list["large"][1]

        ct_model_path = hf_name

        # Check and download the model
        self._download_model(ct_model_path, save_directory=save_directory)

        # Check the path for the model and tokenizer
        model_path = os.path.join(save_directory, ct_model_path.replace("/", "_"))

        resumable = False
        try:
            self.translator = ctranslate2.Translator(model_path, self.device)
            #self.sp_model = spm.SentencePieceProcessor(model_path + "/" + sp_model_name)
            self.tokenizer = AutoTokenizer.from_pretrained(model_path)
        except:
            # resume download
            print("Model download failed. Try to resume download.")
            resumable = True
            pass

        # 모델 로딩 오류시 resume 다운로드
        if resumable:
            self._download_model(ct_model_path, save_directory=save_directory, resume=True)
            self.translator = ctranslate2.Translator(model_path, self.device)
            #self.sp_model = spm.SentencePieceProcessor(model_path + "/" + sp_model_name)
            self.tokenizer = AutoTokenizer.from_pretrained(model_path)

        if "nllb" in ct_model_path:
            self.model_type = "nllb"
        elif "m2m" in ct_model_path:
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