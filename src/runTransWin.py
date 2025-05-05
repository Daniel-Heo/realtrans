#!/bin/env python
#-*- coding: utf-8 -*-
import sys
import os

# 현재 디렉토리의 절대 경로를 구합니다.
current_directory = os.path.dirname(os.path.abspath(__file__))
# lib 폴더의 경로를 구합니다.
lib_path = os.path.join(current_directory, 'lib')
# lib 폴더의 경로를 sys.path에 추가합니다.
if lib_path not in sys.path:
    sys.path.append(lib_path)

import soundcard as sc
import time
from time import sleep
import webrtcvad
from halo import Halo
import threading
import collections, queue
import numpy as np
import torch
import pyaudio
import json
import librosa
import ctrans_manager

# Low Level 사운드 입력을 처리하는 클래스
class LoopbackAudio(threading.Thread):
    def __init__(self, callback, device):
        threading.Thread.__init__(self)
        self.callback = callback
        self.samplerate = 16000
        self.block_size = 20 # 20ms
        self.frame_size = int(self.samplerate * self.block_size / 1000)
        self.stop_event = threading.Event()
        self.is_paused = True
        self.work_idx = 0 # 0~9 : LoopbackAudio의 처리
        self.audio_data = [None] * 10

        p = pyaudio.PyAudio()
        audio_info = ""
        if device == "speaker":
            audio_info = p.get_default_output_device_info()
            self.snd = sc.all_microphones(include_loopback=True)
        else:
            audio_info = p.get_default_input_device_info()
            self.snd = sc.all_microphones(include_loopback=False)
        for i in range(len(self.snd)):
            if self.snd[i].name.find(audio_info['name'])>=0:
                self.snd_index = i
                self.snd_channels = self.snd[i].channels
                break
        p.terminate()

    def run(self):
        dev = self.snd[self.snd_index]

        with dev.recorder(samplerate=self.samplerate) as recorder:
            while not self.stop_event.is_set():
                if self.is_paused == False:
                    self.audio_data[self.work_idx] = recorder.record(numframes=self.frame_size)
                    self.work_idx += 1
                    if self.work_idx > 9:
                        self.work_idx = 0

    def stop(self):
        self.stop_event.set()

    def pause(self):
        self.is_paused = True
    
    def resume(self):
        self.is_paused = False
    
    def is_paused(self):
        return self.is_paused

class VADAudio(threading.Thread):
    frame_duration_ms = 20 # 20ms

    def __init__(self, aggressiveness=3, device=None):
        threading.Thread.__init__(self)
        self.device = device
        self.stop_event = threading.Event()
        self.work_idx = 0
        self.soundcard_reader = LoopbackAudio(callback=None, device=self.device)
        self.soundcard_reader.daemon = True
        self.soundcard_reader.start()

        self.voiced_duration = 0  # 음성 프레임의 지속 시간을 계산 : ms 단위
        self.silence_duration = 0  # 비음성(정적) 지속 시간을 추적 : ms 단위
    
        self.vad = webrtcvad.Vad(aggressiveness)
        self.collect_audio = []
        self.speech = None

        self.daemon = True
        self.start()

    def run(self):
        padding_ms=600
        ratio=0.60

        num_padding_frames = padding_ms // self.frame_duration_ms # 300ms // 20ms = 15 frames : //(나눗셈의 정수형)
        ring_buffer = collections.deque(maxlen=num_padding_frames) 
        triggered = False

        while not self.stop_event.is_set():
            if self.work_idx != self.soundcard_reader.work_idx and not self.soundcard_reader.is_paused:
                transposed_data = np.transpose(self.soundcard_reader.audio_data[self.work_idx])
                if transposed_data.ndim == 1:
                    frame = (transposed_data * 65536).astype(np.int16)
                else:
                    frame = (librosa.to_mono(transposed_data[0]) * 65536).astype(np.int16)
                self.work_idx += 1
                if self.work_idx > 9:
                    self.work_idx = 0
                
                if len(frame) < 320:
                    continue
                is_speech = self.vad.is_speech(frame, 16000)

                if not triggered:
                    ring_buffer.append((frame, is_speech))
                    num_voiced = sum(1 for _, speech in ring_buffer if speech)
                    if num_voiced > ratio * len(ring_buffer):
                        self.voiced_duration = num_voiced * self.frame_duration_ms
                        self.silence_duration = 0
                        triggered = True
                        self.collect_audio = [f for f,s in ring_buffer if s]
                        ring_buffer.clear()
                else:
                    if is_speech:
                        self.collect_audio.append(frame)
                        self.voiced_duration += self.frame_duration_ms
                        self.silence_duration = 0  # 음성이 감지되면 정적 지속 시간을 리셋
                        if self.voiced_duration > 5000: # 5초 이상 음성 감지시 그대로 음성 자르기
                            self.voiced_duration = 0  # 정적 지속 시간을 리셋
                            self.silence_duration = 0  # 정적 지속 시간을 리셋
                            self.speech = np.concatenate(self.collect_audio) if self.collect_audio else np.array([])
                            self.collect_audio = []
                    else:
                        self.voiced_duration += self.frame_duration_ms
                        self.silence_duration += self.frame_duration_ms
                        if self.voiced_duration > 3000 and self.silence_duration > 10:  # 4초 이상 음성 감지시 10ms의 정적 후 음성 자르기
                            self.voiced_duration = 0  # 정적 지속 시간을 리셋
                            self.silence_duration = 0  # 정적 지속 시간을 리셋
                            self.speech = np.concatenate(self.collect_audio) if self.collect_audio else np.array([])
                            self.collect_audio = []
                        elif self.voiced_duration > 5000: # 5초 이상 음성 감지시 그대로 음성 자르기
                            self.voiced_duration = 0  # 정적 지속 시간을 리셋
                            self.silence_duration = 0  # 정적 지속 시간을 리셋
                            self.speech = np.concatenate(self.collect_audio) if self.collect_audio else np.array([])
                            self.collect_audio = []

                    ring_buffer.append((frame, is_speech))
                    num_unvoiced = sum(1 for _, speech in ring_buffer if not speech)
                    if num_unvoiced > ratio * len(ring_buffer):
                        triggered = False
                        self.voiced_duration = 0  # 정적 지속 시간을 리셋
                        self.silence_duration = 0  # 정적 지속 시간을 리셋
                        self.speech = np.concatenate(self.collect_audio) if self.collect_audio else np.array([])
                        self.collect_audio = []
                        ring_buffer.clear()
            else:
                time.sleep(0.01)
        
    def get_audio_data(self):
        if self.speech is not None:
            speech = self.speech
            self.speech = None
            return speech

    def destroy(self):
        self.soundcard_reader.stop()
        self.soundcard_reader.join()

    def stop(self):
        self.stop_event.set()

    def pause(self):
        self.soundcard_reader.pause()
    
    def resume(self):
        self.soundcard_reader.resume()

    def is_paused(self):
        return self.soundcard_reader.is_paused()

    
# JSON 파일을 읽어서 번역 언어를 설정 : 번역 언어가 ALL인 경우에 사용
def find_keys_with_value(json_data, target_value):
    # JSON 데이터를 순회하며 키와 값을 확인합니다.
    for key, value in json_data.items():
        # 찾고자 하는 값과 일치하는 경우 키를 리턴합니다.
        if value == target_value:
            return key
    return None

# 번역 처리 클래스
class Translator:
    def __init__(self, ARGS):
        self.isDecoding = ARGS.isDecoding
        self.lock = threading.Lock()
        self.lang_map = ARGS.lang_map
        self.cuda_dev = ARGS.cuda_dev
        self.work_path = ARGS.work_path
        self.ctrans = None
        self.file_size = 0

        self.initialize(ARGS)

    def initialize(self, ARGS):
        self.ctrans = ctrans_manager.CTrans(base_model=ARGS.model_size, source_lang=ARGS.source_lang, target_lang=ARGS.target_lang, device=self.cuda_dev, lang_map=self.lang_map)

    def translate(self, text, source_lang, target_lang, ARGS):
        source_lang = find_keys_with_value(self.lang_map, source_lang)
        self.ctrans.check_model(ARGS.source_lang, target_lang)
        if source_lang is None:
            source_lang = "eng_Latn"

        if target_lang is None or source_lang == target_lang:
            return

        outPut = ''
        outPut += self.ctrans.translate(text+' ', source_lang=source_lang, target_lang=target_lang, lang_map=self.lang_map)
        # 처음에 -나 ' '가 있으면 제거
        outPut = outPut.strip()
        if len(outPut)>0:
            if outPut[0] == '-':
                if len(outPut) > 1:
                    outPut = outPut[1:]
                    outPut = outPut.strip()
                else :
                    outPut = ""

            try:
                if self.isDecoding:
                    print(outPut.encode('utf-8', errors='ignore').decode('utf-8'))
                else:
                    print(outPut.encode('utf-8', errors='ignore'))
            except Exception as e:
                pass

    def translate_file(self, text, source_lang, target_lang):
            self.ctrans.check_model(source_lang, target_lang)
            outPut = ''
            work_pos = 0
            source_sentences = [text[i:i+2000] for i in range(0, len(text), 2000)]
            total = len(source_sentences)
            for i in range(len(source_sentences)):
                txt = source_sentences[i]
                outPut += self.ctrans.translate_file(txt, source_lang=source_lang, target_lang=target_lang, lang_map=self.lang_map)

                work_pos += 1
                percent = 10+int(work_pos/total*90)
                print(f'[{percent}]')

            print(f'[100]')
            # 파일 저장
            with open(self.work_path+"/translate_out.txt", 'w', encoding='UTF-8') as file:
                file.write(outPut)

    def load_translate_file(self, file_path):
        if os.path.exists(file_path):
            # 파일 사이즈를 가져온다.
            self.file_size = os.path.getsize(file_path)
            with open(file_path, 'r', encoding='UTF-8') as file:
                # 텍스트 파일을 읽어서 내용을 반환
                data = file.read()
                # 첫줄에서 ->를 기준으로 이전은 src_lang, 이후는 tgt_lang로 분리
                first_line = data.split('\n')[0]
                langs = first_line.split('->')
                if len(langs) == 2:
                    langs = [lang.strip() for lang in langs]
                    outData = {"src_lang":langs[0], "tgt_lang":langs[1]}
                else:
                    return None
                
                # data의 2번째 줄부터는 번역할 텍스트
                outData["data"] = data.split('\n')[1:]
                return outData
        return None

# JSON 파일을 열고 그 내용을 읽어서 파이썬 객체로 변환하는 함수입니다.
def load_json_file(file_path):
    if os.path.exists(file_path):
        with open(file_path, 'r', encoding='UTF-8') as file:
            data = json.load(file)
            return data
    else: 
        return None

import re
def halu_filter(src_txt, filter_list):    
    for filter_txt in filter_list:
        if filter_txt.startswith('('):
            pattern = filter_txt[1:-1]
            # [text] 형식의 필터 포함
            if pattern in src_txt and (len(src_txt)-len(pattern)) <= 5: return True
            # (text) 형식의 필터: 2회 이상 반복되고 반복문자의 비중이 50%를 넘는 문자열 검색
            try:
                matches = re.findall(pattern, src_txt)
            except:
                matches = []
            if len(matches) >= 2 and ((len(pattern)*len(matches))<<1)>=len(src_txt): return True
        elif filter_txt.startswith('['):
            # [text] 형식의 필터: 포함이 되고 나머지 문자의 길이가 2 이하인 경우
            pattern = filter_txt[1:-1]
            if pattern in src_txt and (len(src_txt)-len(pattern)) <= 2: return True
        elif filter_txt.startswith('*'):
            # *text* 형식의 필터: text가 포함되는 경우
            pattern = filter_txt[1:-1]
            if pattern in src_txt: return True
        else:
            # 일치여부 검사
            if src_txt == filter_txt: return True
    return False

default_asr_options =  {
    "vad_filter": True ,
    "beam_size": 4, # 한 번에 beam_size만큼 탐색하고 가장 좋은 단어 연결을 선택. 각 탐색 단계에서 유지되는 최대 후보 시퀀스의 수를 결정합니다. 빔 서치는 특정 순간에 가장 가능성 있는 후보들만을 유지하여 계산 효율을 높이는 방식으로 작동
    "best_of": 2, # 5 : 생성할 시퀀스 수입니다. 생성 과정에서 고려할 총 후보 시퀀스의 수를 지정합니다. 생성된 모든 시퀀스 중에서 최종적으로 반환할 시퀀스의 질을 높이기 위해 더 많은 후보를 평가. best_of >= num_return_sequences(생성된 것에서 제일 좋은 것을 리턴한다.)
    "patience": 0.8, # 1 : 빔 서치 매개변수입니다. 인내도 계수입니다. 1.0이면 최상의 결과를 찾으면 탐색을 중단합니다. 0.5면 50%에서 탐색을 중단합니다. float 1 - 아니요
    "repetition_penalty": 2, # 1 : 반복 토큰에 대한 패널티 요소.
    "log_prob_threshold": -2, # -1.0(36%) 평균 로그 확률이 이 값보다 낮으면 디코딩이 실패로 간주됩니다. Optional[float] -1.0  log(p) 확률 70%(0.7)이면 log(0.7) = -0.35, 90%이면 log(0.9) = -0.10, 30%이면 log(0.3) = -1.20, 50%이면 log(0.5) = -0.69, 10%이면 log(0.1) = -2.30
    "no_repeat_ngram_size": 1, # 0 : 반복을 피하기 위한 n-그램의 크기.
    
    "length_penalty": 1, # 1 : 빔 서치 매개변수입니다. 생성되는 시퀀스의 길이에 대한 패널티를 설정합니다. 1보다 작으면 긴 시퀀스가 선호되기 쉽습니다. float 1 - 아니요
    "compression_ratio_threshold": 2.4, # gzip 압축률이 이 값보다 높으면 디코딩된 문자열이 중복되어 실패로 간주됩니다. Optional[float] 2.4 
    "no_speech_threshold": 0.4, # 0.6 토큰 확률이 이 값보다 높고 'logprob_threshold'로 인해 디코딩이 실패한 경우, 세그먼트를 무음으로 간주합니다. 
    "condition_on_previous_text": True, # True : True인 경우 모델의 이전 출력을 다음 윈도우의 프롬프트로 지정하여 일관된 출력이 가능합니다. False로 하면 텍스트의 일관성이 없어질 수 있지만 모델이 비정상 루프에 빠지는 것을 방지할 수 있습니다.  
    #"prompt_reset_on_temperature": 0.4, # 0.5 프롬프트를 재설정하기 위한 온도 임계값. -- condition_on_previous_text가 True인 경우에만 사용됩니다.
    #"initial_prompt": None, # 모델의 초기 윈도우 프롬프트로 제공할 선택적 텍스트입니다. 예: 의학 Optional[str] None 
    
    #"prefix": None, # 오디오의 초기 윈도우 접두사로 제공할 선택적 텍스트입니다. Optional[str] None
    "suppress_blank": True, # True : 샘플링 시작 시 빈 출력을 억제합니다. bool True - 아니요
    "suppress_tokens": [-1], # 억제할 토큰 ID 목록입니다. -1은 model.config.json 파일에 정의된 기본 기호 집합을 억제합니다. Optional[List[int]] [-1] - 아니요
    "without_timestamps": True, # 생성된 텍스트에 타임스탬프를 제외할지 여부
    "max_initial_timestamp": 0.0, # 오디오의 초기 타임스탬프가 이 값보다 늦지 않도록 지정합니다. 오디오의 처음 부분이 무음 또는 불필요한 소리인 경우 타임스탬프 지정을 제한할 수 있습니다.
    "word_timestamps": False, # 단어 수준에서 타임스탬프를 포함할지 여부.
    #"prepend_punctuations": "", #"prepend_punctuations": "\"'“¿([{-", # 생성된 텍스트 앞에 붙일 구두점.
    #"append_punctuations": "", #"append_punctuations": "\"'.。,，!！?？:：”)]}、", # 생성된 텍스트 뒤에 붙일 구두점.
    #"max_new_tokens": None, # 440, # 생성할 새 토큰의 최대 수.
    #"hallucination_silence_threshold": None, # 생성된 텍스트에서 환각 감지를 위한 임계값. 최소 2초의 침묵이 발생한 후 속삭임이 환각에 대해 경계하게 만드는 것을 사용. 0.5초 미만은 대화를 잠시 쉬는것이라 의미 없음. vad_filter=True로 설정해서 무음을 제거하면 무음 할루시네이션을 쉽게 제거 가능.
    #"clip_timestamps": "0", #  자체 임계값 매개변수를 사용하여 등록하는 데 필요한 무음 기간을 지정.
}

# 음성 입력을 처리하는 메인 함수
def main(ARGS):
    global default_asr_options
    cmdCheckTime = time.time()
    use_recognize = True  # 음성인식 사용 여부
    use_trans = ARGS.use_trans  # 번역 사용 여부
    trans_lang = None # 번역 언어 : ALL인 경우 사용
    transcript = ""  # 음성 인식 결과를 저장하는 변수
    

    # Whisper 모델 환상 제거용 필터 
    file_path = os.path.join(ARGS.exec_path, "hallucination_filter.json")
    json_filter = load_json_file(file_path)

    vad_audio = VADAudio(aggressiveness=ARGS.webRTC_aggressiveness,
                         device=ARGS.device)

    # Faster Whisper 원본 모델 로드
    from faster_whisper import WhisperModel
    if ARGS.model_size == "large":
        model_size = "large-v3"
    else: model_size = ARGS.model_size

    ctrans_manager.check_fwmodel(model_size)

    # cuda 사용 여부에 따라 데이터 타입 변경 : CPU는 float32, GPU는 float16
    if ARGS.cuda_dev == 'cpu' or ARGS.proc == "nvidia float32" or ARGS.proc == "amd float32":
        data_type = "float32"
    else:
        data_type = "float16"
    
    whisper_model = WhisperModel(model_size, device=ARGS.cuda_dev, compute_type=data_type)
    # 번역 클래스 생성
    translator = Translator(ARGS)

    # 음성 입력 시작
    vad_audio.resume()

    wav_data = bytearray()
    while True:
        
        # 시스템 명령 체크 ( 번역에 관한 변경사항 적용 ) : 0.3초에 한번
        if time.time() > (cmdCheckTime+0.2):
            cmdCheckTime = time.time()
            work=load_json_file(ARGS.work_path+"/pymsg.json")
            if work != None:
                # 종료 체크
                if work["tgt_lang"]=="exit":
                    vad_audio.destroy()
                    os.remove(ARGS.work_path+"/pymsg.json")
                    break

                # 번역 설정
                if work["tgt_lang"]=="xx": 
                    ARGS.target_lang = None
                    use_trans = False
                else: 
                    ARGS.target_lang = work["tgt_lang"]
                    use_trans = True

                #음성인식 설정
                if work["src_lang"]=="xx": 
                    # 기존 유지
                    use_recognize= False
                    vad_audio.pause()
                else: 
                    ARGS.source_lang = work["src_lang"]
                    use_recognize = True
                    vad_audio.resume()
                
                os.remove(ARGS.work_path+"/pymsg.json")

            trans_data=translator.load_translate_file(ARGS.work_path+"/translate.txt")
            if trans_data != None:
                pause_stats = vad_audio.is_paused
                if pause_stats == False: vad_audio.pause()
                translator.translate_file(text=trans_data["data"], source_lang=trans_data["src_lang"], target_lang=trans_data["tgt_lang"])
                os.remove(ARGS.work_path+"/translate.txt")
                if pause_stats == False: vad_audio.resume()

        if use_recognize == False:
            sleep(0.01)
            continue

        wav_data = vad_audio.get_audio_data()

        if wav_data is None: 
            sleep(0.01)

        if wav_data is not None:
            # 오디오의 총 샘플 수를 샘플 레이트로 나누어 오디오의 길이(초 단위)를 계산합니다.
            audio_length_seconds = len(wav_data) >> 14 # 2^4*1024

            if len(wav_data) > 0:
                npAudioInt16 = np.frombuffer(wav_data, dtype=np.int16)
                # 오디오 데이터를 부동소수점으로 변환
                audio_float16 = Int2Float(npAudioInt16, dtype=np.float16)

                # Faster Whisper 원본 모델 사용
                segments = None
                if( audio_length_seconds < 3 and trans_lang != None): # 3초 이내의 음성은 기존 language를 사용한다.
                    segments, info = whisper_model.transcribe(audio=audio_float16, language=trans_lang, **default_asr_options)
                else:
                    if( ARGS.source_lang == "ALL"):
                        segments, info = whisper_model.transcribe(audio=audio_float16, **default_asr_options )
                        trans_lang = info.language
                    else:
                        trans_lang = ARGS.lang_map[ARGS.source_lang]
                        segments, info = whisper_model.transcribe(audio=audio_float16, language=trans_lang, **default_asr_options )

                srcText = {'text': ''}
                for segment in segments:
                    srcText['text'] += segment.text

                # srcText['text']에서 ⁇ 문자 제거
                srcText['text'] = srcText['text'].replace("⁇", "")
                
                tmp = ""
                if len(srcText['text'])>1:
                    tmp = srcText['text'][0:].encode('utf-8', errors='ignore').decode('utf-8')
                    try:
                        hallucination_filter = json_filter[trans_lang]
                    except Exception as e:
                        hallucination_filter = None
                        trans_lang = "en"

                    transcript = ""
                    if hallucination_filter != None:
                        if halu_filter(tmp, hallucination_filter) == False:
                            if ARGS.view != None:
                                transcript = " "+trans_lang+"> "+tmp
                            else :
                                transcript = " "+tmp
                    else:
                        if ARGS.view != None: # 언어코드 출력 ( 디버깅용 )
                            transcript = " "+trans_lang+"> "+tmp
                        else :
                            transcript = " "+tmp

                    # use_trans가 False이면 이곳에서 출력
                    if len(transcript) > 1:
                        try:
                            outText = '-'+transcript
                            if ARGS.isDecoding==True:
                                print(outText.encode('utf-8', errors='ignore').decode('utf-8'))
                            else :
                                print(outText.encode('utf-8', errors='ignore'))
                        except Exception as e:
                            pass  # 에러가 발생해도 아무런 행동을 취하지 않음

                        if use_trans: 
                            translator.translate(text=transcript, source_lang=trans_lang, target_lang=ARGS.target_lang, ARGS=ARGS)

            wav_data = bytearray()

    # GPU 메모리 해제
    if ARGS.cuda_dev == 'cuda': # cuda / rocm
        torch.cuda.empty_cache()
    else:
        pass  # CPU 사용 시 메모리 해제 불필요

# int16 -> float로 변환
def Int2Float(data, dtype=np.float32):
    # 최대 정수 값을 구함 (np.iinfo를 사용하여 입력 데이터 타입에 대한 정보를 얻음)
    max_int_value = np.iinfo(data.dtype).max
    # 정수형 데이터를 부동소수점으로 변환하고, -1.0과 1.0 사이의 값으로 정규화
    return data.astype(dtype) / max_int_value

# 초기 작업처리
if __name__ == '__main__':

    import argparse
    parser = argparse.ArgumentParser(description="Stream from speacker to whisper and translate")
    parser.add_argument('-d', '--device', type=str, default="speaker",
                        help="Device active sound input name")
    parser.add_argument('-m', '--model_size', type=str, default="small",
                        help="Model size : large, turbo, medium, small")
    parser.add_argument('-r', '--sample_rate', type=int, default=48000,
                        help="Sampling rate : Default 48000")
    parser.add_argument('-s', '--source_lang', type=str, default=None, # eng_Latn, ALL
                        help="Voice input Language : eng_Latn, kor_Hang")
    parser.add_argument('-t', '--target_lang', type=str, default=None, # kor_Hang
                        help="Transfer Language : eng_Latn, kor_Hang, xx")
    parser.add_argument('-p', '--proc', type=str, default="nvidia float16",
                        help="Process Method : nvidia float16, nvidia float32, amd float16, amd float32, CPU")
    parser.add_argument('-v', '--view', type=str, default=None,
                        help="Debug mode : None, not None")
    parser.add_argument('-w', '--webRTC_aggressiveness', type=int, default=3,
                        help="Set aggressiveness of webRTC: an integer between 0 and 3, 0 being the least aggressive about filtering out non-speech, 3 the most aggressive. Default: 3")      

    ARGS = parser.parse_args()

    if( ARGS.source_lang == None):
        ARGS.source_lang = "eng_Latn"
        ARGS.isDecoding = True
    else:
        ARGS.isDecoding = False

    if ARGS.target_lang == "xx" :
        ARGS.target_lang = None
        ARGS.use_trans = False # 번역을 하지 않는다.
    else:
        if( ARGS.target_lang == None):
            ARGS.target_lang = "kor_Hang"
        ARGS.use_trans = True # 번역을 한다.

    # medium -> turbo 매핑
    if ARGS.model_size=="medium":
        ARGS.model_size="turbo"

    # 시작 정보로 Path 지정 : work_path가 움직이는 것을 방지
    ARGS.exec_path = os.path.dirname(os.path.abspath(__file__))
    ARGS.work_path = os.getcwd()

    # Whisper 지원 언어 목록
    file_path = os.path.join(ARGS.exec_path, "whisper_lang_map.json")
    ARGS.lang_map = load_json_file(file_path)

    #cuda activate : Translation processing with CPU
    ARGS.cuda_dev = 'cuda' if torch.cuda.is_available() else 'cpu'
    if ARGS.proc == "CPU" or ARGS.cuda_dev == 'cpu':
        ARGS.cuda_dev = 'cpu'
    else:
        # AMD GPU 지원 확인
        if torch.cuda.is_available():
            ARGS.cuda_dev = 'cuda'
        elif hasattr(torch, 'hip') and torch.hip.is_available():
            ARGS.cuda_dev = 'cuda'  # AMD HIP/ROCm
        else:
            ARGS.cuda_dev = 'cpu'

    if os.path.exists(ARGS.work_path+"/pymsg.json"):
        os.remove(ARGS.work_path+"/pymsg.json") # 기존에 처리되지 못한 명령 삭제

    torch.device(ARGS.cuda_dev)
    print('#Translation processing with ' + ARGS.cuda_dev)

    # UTF-8 출력 설정
    sys.stdout.reconfigure(encoding='utf-8')

    # 메인 함수 호출
    t1 = threading.Thread(target=main, args=(ARGS,))
    t1.start()
    t1.join()