import sentencepiece as spm
import ctranslate2
from huggingface_hub import snapshot_download
import os

model_list = {
    # "nllb_3.3B": ["JustFrederik/nllb-200-3.3B-ct2-float16","sentencepiece.bpe.model"], # 6.69GB
    # "nllb_1.3B": ["JustFrederik/nllb-200-distilled-1.3B-ct2-int8","sentencepiece.bpe.model"], # 1.38GB
    # "nllb_1.3B_fp16": ["JustFrederik/nllb-200-1.3B-ct2-float16","sentencepiece.bpe.model"], # 2.74GB
    # "nllb_600M": ["JustFrederik/nllb-200-distilled-600M-ct2-int8","sentencepiece.bpe.model"], # 0.63GB
    # "m2m_418M": ["JustFrederik/m2m_100_418m_ct2_int8","spm.128k.model"], # 0.49GB
    # "m2m_1.2B": ["JustFrederik/m2m_100_1.2b_ct2_int8","spm.128k.model"], # 1.25GB
    "huge": ["JustFrederik/nllb-200-3.3B-ct2-float16","sentencepiece.bpe.model"], # 6.69GB
    "large": ["JustFrederik/nllb-200-1.3B-ct2-float16","sentencepiece.bpe.model"], # 2.74GB
    "medium": ["JustFrederik/nllb-200-distilled-1.3B-ct2-int8","sentencepiece.bpe.model"], # 1.38GB
    "small": ["JustFrederik/nllb-200-distilled-600M-ct2-int8","sentencepiece.bpe.model"], # 0.63GB
}

def download_model(model_name, save_directory):
    global current_dir
    save_directory= os.path.join(current_dir,save_directory)

    # 모델 리스트에 존재하면 해당 이름을 가져온다.
    if model_name in model_list:
        model_name = model_list[model_name][0]

    # /가 들어간 것을 _로 변경
    model_savename = model_name.replace("/", "_")

    # 모델 및 토크나이저의 저장 경로 확인
    model_path = os.path.join(save_directory, model_savename)
    #print(f"Model path : {model_path}")
    
    # 모델 메인 디렉토리가 존재하지 않으면 생성
    if not os.path.exists(save_directory):
        os.makedirs(save_directory)
    
    # 모델이 이미 다운로드되어 있는지 확인
    if not os.path.exists(model_path):
        print(f"{model_name} Model {model_path} Downloading.")
        # Huggingface에서 직접 다운로드
        snapshot_download(model_name, revision="main", local_dir=model_path)
        print("Download Success!")

model_type = ""
current_dir = ""
lang_map = ""
def load_model(model_name, device="cpu", save_directory="models"):
    # 현재 작업 디렉토리를 전역변수에 저장
    global current_dir, model_type, lang_map

    current_dir = os.getcwd()
    save_directory= os.path.join(current_dir,save_directory)

    if model_name in model_list:
        hf_name = model_list[model_name][0]
    else:
        print("Model name is not in the list")
        return None, None

    ct_model_path = hf_name
    sp_model_name = model_list[model_name][1]


    # 모델 확인 및 다운로드
    download_model(ct_model_path, save_directory=save_directory)

    # 모델 및 토크나이저의 저장 경로 확인
    model_path = os.path.join(save_directory, ct_model_path.replace("/", "_"))
    translator = ctranslate2.Translator(model_path, device)    # or "cuda" for GPU
    sp_model = spm.SentencePieceProcessor(model_path+"/"+sp_model_name)

    # ct_model_path에 nllb가 들어있으면 model_type을 nllb로 설정, m2m이면 m2m으로 설정, 그 외는 None
    if "nllb" in ct_model_path:
        model_type = "nllb"
    elif "m2m" in ct_model_path:
        model_type = "m2m"  
    else:
        model_type = None

    return translator, sp_model


def translate(source, translator, sp_model, source_lang, target_lang, lang_map=None):
    global model_type
    if model_type == "m2m":
        source_lang = "__"+lang_map[source_lang]+"__"
        target_lang = "__"+lang_map[target_lang]+"__"

    source_sents = [source]
    source_sentences = [sent.strip() for sent in source_sents]
    target_prefix = [[target_lang]] * len(source_sentences)

    # Subword the source sentences
    source_sents_subworded = sp_model.encode_as_pieces(source_sentences)
    source_sents_subworded = [[source_lang] + sent + ["</s>"] for sent in source_sents_subworded]
    
    # Translate the source sentences
    translations_subworded = translator.translate_batch(source_sents_subworded, batch_type="tokens", max_batch_size=2024, beam_size=4, target_prefix=target_prefix)
    translations_subworded = [translation.hypotheses[0] for translation in translations_subworded]
    for translation in translations_subworded:
        if target_lang in translation:
            translation.remove(target_lang)

    # Desubword the target sentences
    translations = sp_model.decode(translations_subworded)
    translation = translations[0]
  
    return translation

