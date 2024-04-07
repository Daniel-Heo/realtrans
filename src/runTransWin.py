#!/bin/env python
#-*- coding: utf-8 -*-
import sys
import os

# 현재 디렉토리의 절대 경로를 구합니다.
current_directory = os.path.dirname(os.path.abspath(__file__))
## lib 폴더의 경로를 구합니다.
lib_path = os.path.join(current_directory, 'lib')

# # lib 폴더의 경로를 sys.path에 추가합니다.
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
import librosa
import ctrans_manager

# 글로벌 변수
exit_flag = False
transcript = ""
cuda_dev = None
use_en2ko = False
cmdCheckTime = time.time()
use_trans = True # 번역 사용 여부
trans_lang = None # 번역 언어 : ALL일 경우에 사용한다.
isDecoding = False # print에서 decoding을 사용할 경우에 사용한다.
audio_info = None

# 파일 읽기 : 파일이 존재하면 파일을 읽어서 내용을 반환, 파일이 존재하지 않으면 None을 반환
def read_file_if_exists(file_path):
    # 파일이 존재하는지 체크
    if os.path.exists(file_path):
        # 파일이 존재하면, 파일을 열고 내용을 읽은 후 반환
        with open(file_path, 'r') as file:
            return file.read()
    else:
        #print(f"{file_path} 파일이 존재하지 않습니다.")
        return None

# Low Level 사운드 입력을 처리하는 클래스
class LoopbackAudio(threading.Thread):
    def __init__(self, callback, device):
        threading.Thread.__init__(self)
        self.callback = callback
        self.samplerate = 16000
        self.block_size = 20 # 20ms
        self.frame_size = int(self.samplerate * self.block_size / 1000)
        self.mics = sc.all_microphones(include_loopback=True)
        self.mic_index = device
        self.stop_event = threading.Event()

    def run(self):
        if self.mic_index == None:
            mic = sc.default_microphone()
        else:
            mic = self.mics[self.mic_index]
        with mic.recorder(samplerate=self.samplerate) as recorder:
            while not self.stop_event.is_set():
                transposed_data = np.transpose(recorder.record(numframes=self.frame_size))
                self.callback((librosa.to_mono(transposed_data)*32768).astype(np.int16))

    def stop(self):
        self.stop_event.set()
        
# 음성 입력장치 제어 클래스
class Audio(object):
    RATE_PROCESS = 16000
    CHANNELS = 1
    BLOCKS_PER_SECOND = 20 # 20ms

    def __init__(self, callback=None, device=None):
        global ARGS
        def proxy_callback(in_data):
            callback(in_data)
        if callback is None:
            def callback(in_data): 
                return self.buffer_queue.put(in_data)
            
        self.buffer_queue = queue.Queue()
        self.device = device

        self.soundcard_reader = LoopbackAudio(
            callback=proxy_callback, device=self.device)
        self.soundcard_reader.daemon = True
        self.soundcard_reader.start()

    def read(self):
        return self.buffer_queue.get()

    def destroy(self):
        self.soundcard_reader.stop()
        self.soundcard_reader.join()

    frame_duration_ms = BLOCKS_PER_SECOND

voiced_duration = 0  # 음성 프레임의 지속 시간을 계산 : ms 단위
silence_duration = 0  # 비음성(정적) 지속 시간을 추적 : ms 단위

# 음성 활동 감지 클래스 : 3초 이상 음성 감지시 10ms의 정적 후 음성 자르기, 4초는 그대로 음성 자르기
class VADAudio(Audio):

    def __init__(self, aggressiveness=3, device=None):
        super().__init__(device=device)
        self.vad = webrtcvad.Vad(aggressiveness)

    def frame_generator(self):
        while True:
            yield self.read()

    def vad_collector(self, padding_ms=500, ratio=0.80, frames=None):
        global voiced_duration
        global silence_duration

        if frames is None:
            frames = self.frame_generator()
        num_padding_frames = padding_ms // self.frame_duration_ms # 300ms // 20ms = 15 frames : //(나눗셈의 정수형)
        ring_buffer = collections.deque(maxlen=num_padding_frames) 
        triggered = False # 음성 감지 여부 초기화(감지 안된 상태)

        for frame in frames: 
            if len(frame) < 320:  # 프레임 유효성 검사 : 1 block_size 이상인지 확인(크지 않으면 처리할 가치가 없음을 의미)
                return

            # 현재 프레임이 음성인지 비음성인지를 판단합니다. 이 메서드는 VAD 알고리즘을 사용하여 결정하며, 반환값은 True 또는 False입니다.
            is_speech = self.vad.is_speech(frame, 16000) # 32000으로 고정 : 16000으로 하면 인식이 떨어지는듯.

            # 기존 프레임이 음성이 아닌 상태에서 음성이 감지되면, 링 버퍼를 초기화하고 음성 감지를 시작합니다. f:frame, s:is_speech
            if not triggered:
                ring_buffer.append((frame, is_speech)) # 링버퍼에 샘플과 음성 여부를 추가
                num_voiced = len([f for f, speech in ring_buffer if speech]) # 링버퍼에 있는 음성 프레임의 개수를 계산

                if num_voiced > ratio * ring_buffer.maxlen: # 링버퍼에 있는 음성 프레임의 비율이 설정된 ratio를 초과하면 음성 감지를 시작
                    voiced_duration = num_voiced * self.frame_duration_ms
                    silence_duration = 0
                    triggered = True # 음성 감지 상태로 변경
                    for f, s in ring_buffer:
                        if s:
                            yield f
                    ring_buffer.clear()
            else:
                if is_speech:
                    voiced_duration += self.frame_duration_ms
                    silence_duration = 0  # 음성이 감지되면 정적 지속 시간을 리셋
                    if voiced_duration > 5000: # 5초 이상 음성 감지시 그대로 음성 자르기
                        voiced_duration = 0  # 정적 지속 시간을 리셋
                        silence_duration = 0  # 정적 지속 시간을 리셋
                        yield None 
                else:
                    voiced_duration += self.frame_duration_ms
                    silence_duration += self.frame_duration_ms
                    if voiced_duration > 3000 and silence_duration > 10:  # 4초 이상 음성 감지시 10ms의 정적 후 음성 자르기
                        voiced_duration = 0  # 정적 지속 시간을 리셋
                        silence_duration = 0  # 정적 지속 시간을 리셋
                        yield None 
                    elif voiced_duration > 5000: # 5초 이상 음성 감지시 그대로 음성 자르기
                        voiced_duration = 0  # 정적 지속 시간을 리셋
                        silence_duration = 0  # 정적 지속 시간을 리셋
                        yield None 

                if is_speech:
                    yield frame
                ring_buffer.append((frame, is_speech))
                num_unvoiced = len(
                    [f for f, speech in ring_buffer if not speech])
                if num_unvoiced > ratio * ring_buffer.maxlen:
                    voiced_duration = 0  # 정적 지속 시간을 리셋
                    silence_duration = 0  # 정적 지속 시간을 리셋
                    triggered = False
                    yield None
                    ring_buffer.clear()

# ctranslate2 모델 로드
def load_ct_model():
    translator, sp_model = ctrans_manager.load_model("huge", device=cuda_dev) # huge, large, medium, small

    return translator, sp_model

# JSON 파일을 읽어서 번역 언어를 설정 : 번역 언어가 ALL인 경우에 사용
def find_keys_with_value(json_data, target_value):
    # JSON 데이터를 순회하며 키와 값을 확인합니다.
    for key, value in json_data.items():
        # 찾고자 하는 값과 일치하는 경우 키를 리턴합니다.
        if value == target_value:
            return key
        # 값이 딕셔너리인 경우, 재귀적으로 함수를 호출해 내부에서도 검색합니다.
        #elif isinstance(value, dict):
        #    keys_found.extend(find_keys_with_value(value, target_value))
    
    return None

# 음성에서 추출된 텍스트로 번역 작업과 관련된 것들을 처리
def transSound(ARGS):
    global transcript
    global exit_flag
    global use_en2ko
    global use_trans
    global trans_lang
    global isDecoding
    global lock
    global whisper_lang_map

    isLoadModel = False
    translator = None
    sp_model = None

    src_lang = None
    transText = ""
    while True:
        # 종료 체크
        if exit_flag:
            return 0
        
        # 번역을 하지 않는다면, 다음 루프로 이동
        if use_trans == False:
            sleep(0.3)
            continue

        if isLoadModel == False:
            translator, sp_model = load_ct_model()
            isLoadModel = True
        
        try:
            # whisper_lang_map json에서 번역 언어를 찾아서 src_lang을 설정
            src_lang = find_keys_with_value(whisper_lang_map, trans_lang)
            if( src_lang == None ):
                src_lang = "eng_Latn"
        except Exception as e:
            src_lang = "eng_Latn"

        if len(transcript) > 1:
            lock.acquire()
            transText = transcript
            transcript=""
            lock.release()

            if len(transText) > 1:
                outPut = '-' + transText
                try:
                    if isDecoding==True:
                        print(outPut.encode('utf-8', errors='ignore').decode('utf-8'))
                    else :
                        print(outPut.encode('utf-8', errors='ignore'))
                except Exception as e:
                    pass  # 에러가 발생해도 아무런 행동을 취하지 않음

                if ARGS.target_lang is None:
                    continue

                if src_lang == ARGS.target_lang:
                    continue
                
                outPut = ' '
                outPut += ctrans_manager.translate(transText, translator=translator, sp_model=sp_model, source_lang=src_lang, target_lang=ARGS.target_lang, lang_map=whisper_lang_map)

                try:
                    if isDecoding==True:
                        print(outPut.encode('utf-8', errors='ignore').decode('utf-8'))
                    else :
                        print(outPut.encode('utf-8', errors='ignore'))
                except Exception as e:
                    pass  # 에러가 발생해도 아무런 행동을 취하지 않음
        
        sleep(0.1)

    exit_flag = True
    torch.cuda.empty_cache()

import json

# JSON 파일을 열고 그 내용을 읽어서 파이썬 객체로 변환하는 함수입니다.
def load_json_file(file_path):
    if os.path.exists(file_path):
        with open(file_path, 'r', encoding='UTF-8') as file:
            data = json.load(file)
            return data
    else: 
        #print(f"{file_path} 파일이 존재하지 않습니다.")
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
            if len(matches) >= 2 and (len(pattern)*len(matches)*2)>=len(src_txt): return True
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
    
# 음성 입력을 처리하는 메인 함수
def main(ARGS):
    global transcript
    global exit_flag
    global cuda_dev
    global cmdCheckTime
    global lock
    global use_en2ko
    global whisper_lang_map
    global use_trans
    global trans_lang

    use_recognize = True  # 음성인식 사용 여부

    # Whisper 모델 환상 제거용 필터 
    file_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "hallucination_filter.json")
    json_filter = load_json_file(file_path)

    devNo=GetDevNo(ARGS)
    if devNo<0:
        exit_flag = True
        return 0

    vad_audio = VADAudio(aggressiveness=ARGS.webRTC_aggressiveness,
                         device=devNo)

    frames = vad_audio.vad_collector()

    # Whisper 모델 로드
    #import whisper
    #whisper_model = whisper.load_model("large-v3") # large, medium, small, base
    #whisper_model.to(cuda_dev)

    # Faster Whisper 모델 로드
    import realtransc
    whisper_model = realtransc.load_model("large-v3", device=cuda_dev, compute_type="float16") # tiny, base, small, medium, large-v1, large-v2, large-v3, large, distil-large-v2, distil-medium, distil-small


    wav_data = bytearray()
    for frame in frames:
        if exit_flag:
            exit
        
        # 시스템 명령 체크 ( 번역에 관한 변경사항 적용 ) : 3초에 한번
        if time.time() > (cmdCheckTime+3):
            cmdCheckTime = time.time()
            work=load_json_file(os.getcwd()+"/pymsg.json")
            if work != None:
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
                else: 
                    ARGS.source_lang = work["src_lang"]
                    use_recognize = True
                
                if ARGS.source_lang=="eng_Latn" and ARGS.target_lang=="kor_Hang":
                    use_en2ko = True
                else:
                    use_en2ko = False
                os.remove(os.getcwd()+"/pymsg.json")

        if use_recognize == False:
            sleep(1)
            continue

        if frame is not None:
            wav_data.extend(frame)
        else:
            # 오디오의 총 샘플 수를 샘플 레이트로 나누어 오디오의 길이(초 단위)를 계산합니다.
            audio_length_seconds = len(wav_data) >> 14 # 2^4*1024

            if len(wav_data) > 0 and audio_length_seconds > 0.5:
                npAudioInt16 = np.frombuffer(wav_data, dtype=np.int16)
                
                # 오디오 데이터를 부동소수점으로 변환
                audio_float16 = Int2Float(npAudioInt16, dtype=np.float16)
                #print("audio_float16 : ", audio_float16.shape)
                audio_float16 = np.append(audio_float16, np.zeros(320, dtype=np.float16))

                # Whisper 모델 사용
                # if( audio_length_seconds < 3 and trans_lang != None): # 3초 이내의 음성은 기존 language를 사용한다.
                #     srcText = whisper_model.transcribe(audio=audio_float16, language=trans_lang, fp16=True, condition_on_previous_text=False)
                # else:
                #     if( ARGS.source_lang == "ALL"):
                #         srcText = whisper_model.transcribe(audio=audio_float16, fp16=True, condition_on_previous_text=False)
                #         trans_lang = srcText['language']
                #     else:
                #         trans_lang = whisper_lang_map[ARGS.source_lang]
                #         srcText = whisper_model.transcribe(audio=audio_float16, language=trans_lang, fp16=True, condition_on_previous_text=False)
                         #srcText = whisper_model.transcribe(audio=audio_float32, language="en", fp16=True)
                
                # Faster Whisper 모델 사용
                if( audio_length_seconds < 3 and trans_lang != None): # 3초 이내의 음성은 기존 language를 사용한다.
                    result = whisper_model.transcribe(audio=audio_float16, language=trans_lang )
                else:
                    if( ARGS.source_lang == "ALL"):
                        result = whisper_model.transcribe(audio=audio_float16 )
                        trans_lang = result['language']
                    else:
                        trans_lang = whisper_lang_map[ARGS.source_lang]
                        result = whisper_model.transcribe(audio=audio_float16, language=trans_lang )

                srcText = {'text': ''}
                for segment in result["segments"]:
                    srcText['text'] += segment['text']
                
                if len(srcText['text'])>1:
                    tmp = srcText['text'][1:].encode('utf-8', errors='ignore').decode('utf-8')
                    try:
                        hallucination_filter = json_filter[trans_lang]
                    except Exception as e:
                        hallucination_filter = None
                        trans_lang = "en"

                    if hallucination_filter != None:
                        if halu_filter(tmp, hallucination_filter) == False:
                            lock.acquire()
                            if ARGS.view != None:
                                transcript += " "+trans_lang+"> "+tmp
                            else :
                                transcript += " "+tmp
                            lock.release()
                    else:
                        lock.acquire()
                        if ARGS.view != None:
                            transcript += " "+trans_lang+"> "+tmp
                        else :
                            transcript += " "+tmp
                        
                        lock.release()

                    # use_trans가 False이면 이곳에서 출력
                    if use_trans==False and len(transcript) > 1:
                        try:
                            tmp = '-'+transcript
                            if isDecoding==True:
                                print(tmp.encode('utf-8', errors='ignore').decode('utf-8'))
                            else :
                                print(tmp.encode('utf-8', errors='ignore'))
                            transcript = ""
                        except Exception as e:
                            pass  # 에러가 발생해도 아무런 행동을 취하지 않음

                frame = bytearray()
                wav_data = bytearray()

    exit_flag = True
    # GPU 메모리 해제
    torch.cuda.empty_cache()
    

# 음성 입력장치의 번호를 반환
def GetDevNo(ARGS):
    if ARGS.device is None:
        return -1

    mics = sc.all_microphones(include_loopback=True)

    for i in range(len(mics)):
        if mics[i].name.find(ARGS.device)>=0:
            return i

    return -1

# int16 -> float로 변환
def Int2Float(data, dtype=np.float32):
    # 최대 정수 값을 구함 (np.iinfo를 사용하여 입력 데이터 타입에 대한 정보를 얻음)
    max_int_value = np.iinfo(data.dtype).max
    # 정수형 데이터를 부동소수점으로 변환하고, -1.0과 1.0 사이의 값으로 정규화
    return data.astype(dtype) / max_int_value

# 오디오 데이터를 저장하는 함수
def save_wav(file_name, audio_data, sample_rate=16000):
    import wave
    print(f"Saving audio to {os.getcwd()}{file_name}")
    with wave.open(file_name, 'wb') as wf:
        wf.setnchannels(1)  # 모노 채널
        wf.setsampwidth(2)  # 16비트 샘플
        wf.setframerate(sample_rate)
        wf.writeframes(audio_data.tobytes())
    return file_path

# 초기 작업처리
if __name__ == '__main__':

    import argparse
    parser = argparse.ArgumentParser(description="Stream from speacker to whisper and translate")
    parser.add_argument('-d', '--device', type=str, default=None,
                        help="Device active sound input name")
    parser.add_argument('-r', '--sample_rate', type=int, default=48000,
                        help="Sampling rate : Default 16000")
    parser.add_argument('-s', '--source_lang', type=str, default=None, # eng_Latn, ALL
                        help="Voice input Language : eng_Latn, kor_Hang")
    parser.add_argument('-t', '--target_lang', type=str, default=None, # kor_Hang
                        help="Transfer Language : eng_Latn, kor_Hang, xx")
    parser.add_argument('-v', '--view', type=str, default=None,
                        help="Debug mode : None, not None")
    parser.add_argument('-w', '--webRTC_aggressiveness', type=int, default=3,
                        help="Set aggressiveness of webRTC: an integer between 0 and 3, 0 being the least aggressive about filtering out non-speech, 3 the most aggressive. Default: 3")      

    ARGS = parser.parse_args()

    # Check Input parameter
    if ARGS.device is None:
        p = pyaudio.PyAudio()
        audio_info = p.get_default_output_device_info()
        ARGS.device = audio_info['name']
        p.terminate()

    if( ARGS.source_lang == None):
        ARGS.source_lang = "eng_Latn"
        isDecoding = True

    if ARGS.target_lang == "xx" :
        ARGS.target_lang = None
        use_trans = False # 번역을 하지 않는다.
    else:
        if( ARGS.target_lang == None):
            ARGS.target_lang = "kor_Hang"
        
    if ARGS.source_lang=="eng_Latn" and ARGS.target_lang=="kor_Hang":
        use_en2ko = True

    # Whisper 지원 언어 목록
    file_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "whisper_lang_map.json")
    whisper_lang_map = load_json_file(file_path)

    #cuda activate : Translation processing with CPU
    cuda_dev = 'cuda' if torch.cuda.is_available() else 'cpu'
    torch.device(cuda_dev)
    print('Translation processing with ' + cuda_dev)
    
    #thread
    # t1 : voice -> text
    # t2 : english text -> korean text
    lock = threading.Lock()
    t1 = threading.Thread(target=main, args=(ARGS,))
    t2 = threading.Thread(target=transSound, args=(ARGS,))

    t1.start()
    t2.start()
    t1.join()
    t2.join()