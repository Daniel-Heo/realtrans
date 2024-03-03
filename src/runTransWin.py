#!/bin/env python
#-*- coding: utf-8 -*-
import soundcard as sc
import time
from time import sleep
import webrtcvad
from halo import Halo
import threading
import collections, queue
import whisper
import os
import numpy as np
import torch
import threading
from transformers import AutoTokenizer, AutoModelForSeq2SeqLM

# 글로벌 변수
exit_flag = False
transcript = ""
testDir = ""

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

# 음성 입력장치 제어 클래스
class Audio(object):
    RATE_PROCESS = 16000
    CHANNELS = 1
    BLOCKS_PER_SECOND = 50

    def __init__(self, callback=None, device=None, input_rate=RATE_PROCESS):
        def proxy_callback(in_data):
            callback(in_data)

        if callback is None:
            def callback(in_data): return self.buffer_queue.put(in_data)
        self.buffer_queue = queue.Queue()
        self.device = device
        self.input_rate = input_rate # 16000
        self.sample_rate = self.RATE_PROCESS # 16000
        self.block_size = int(self.RATE_PROCESS / float(self.BLOCKS_PER_SECOND)) # 320

        self.soundcard_reader = LoopbackAudio(
            callback=proxy_callback, device=self.device, samplerate=self.sample_rate)
        self.soundcard_reader.daemon = True
        self.soundcard_reader.start()

    def read(self):
        return self.buffer_queue.get()

    def destroy(self):
        self.soundcard_reader.stop()
        self.soundcard_reader.join()

    frame_duration_ms = property( lambda self: 1000 // self.BLOCKS_PER_SECOND ) # 프레임 지속 시간 : 20ms

voiced_duration = 0  # 음성 프레임의 지속 시간을 계산 : ms 단위
silence_duration = 0  # 비음성(정적) 지속 시간을 추적 : ms 단위

# 음성 활동 감지 클래스 : 3초 이상 음성 감지시 10ms의 정적 후 음성 자르기, 4초는 그대로 음성 자르기
class VADAudio(Audio):
    """Filter & segment audio with voice activity detection."""

    def __init__(self, aggressiveness=3, device=None, input_rate=None):
        super().__init__(device=device, input_rate=input_rate)
        self.vad = webrtcvad.Vad(aggressiveness)

    def frame_generator(self):
        """Generator that yields all audio frames from microphone."""
        if self.input_rate == self.RATE_PROCESS:
            while True:
                yield self.read()
        else:
            raise Exception("Resampling required")
    
    def vad_collector(self, padding_ms=200, ratio=0.80, frames=None):
        global voiced_duration
        global silence_duration

        if frames is None:
            frames = self.frame_generator()
        num_padding_frames = padding_ms // self.frame_duration_ms # 100ms / 20ms = 5 frames
        ring_buffer = collections.deque(maxlen=num_padding_frames) # 5 frames
        triggered = False # 음성 감지 여부 초기화(감지 안된 상태)

        for frame in frames: # 5개의 프레임이 든 frames에서 한개씩 가져옴.
            if len(frame) < 640:  # 프레임 유효성 검사 : 1 block_size 이상인지 확인(크지 않으면 처리할 가치가 없음을 의미)
                return
            
            # 2 channel float data를 mono int16 data로 변환
            mono_frame = np.mean(frame, axis=1)
            frame = np.int16(mono_frame * 32768)

            # 현재 프레임이 음성인지 비음성인지를 판단합니다. 이 메서드는 VAD 알고리즘을 사용하여 결정하며, 반환값은 True 또는 False입니다.
            # 320개의 샘플을 가진 프레임을 VAD에 전달하여 음성 여부를 판단합니다.
            is_speech = self.vad.is_speech(frame, self.sample_rate)

            # 기존 프레임이 음성이 아닌 상태에서 음성이 감지되면, 링 버퍼를 초기화하고 음성 감지를 시작합니다. f:frame, s:is_speech
            if not triggered:
                ring_buffer.append((frame, is_speech)) # 링버퍼에 320개의 샘플과 음성 여부를 추가
                num_voiced = len([f for f, speech in ring_buffer if speech]) # 링버퍼에 있는 음성 프레임의 개수를 계산

                if num_voiced > ratio * ring_buffer.maxlen: # 링버퍼에 있는 음성 프레임의 비율이 설정된 ratio를 초과하면 음성 감지를 시작
                    voiced_duration = num_voiced * self.frame_duration_ms
                    silence_duration = 0
                    triggered = True # 음성 감지 상태로 변경
                    for f, s in ring_buffer:
                        yield f
                    ring_buffer.clear()
            else:
                if is_speech:
                    voiced_duration += self.frame_duration_ms
                    silence_duration = 0  # 음성이 감지되면 정적 지속 시간을 리셋
                else:
                    voiced_duration += self.frame_duration_ms
                    silence_duration += self.frame_duration_ms
                    if voiced_duration > 3000 and silence_duration > 10:  # 3초 이상 음성 감지시 10ms의 정적 후 음성 자르기
                        voiced_duration = 0  # 정적 지속 시간을 리셋
                        silence_duration = 0  # 정적 지속 시간을 리셋
                        yield None 
                    elif voiced_duration > 4000: # 4초 이상 음성 감지시 그대로 음성 자르기
                        voiced_duration = 0  # 정적 지속 시간을 리셋
                        silence_duration = 0  # 정적 지속 시간을 리셋
                        yield None 

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

SAMPLE_RATE = 16000

# Low Level 사운드 입력을 처리하는 클래스
class LoopbackAudio(threading.Thread):
    def __init__(self, callback, device, samplerate=SAMPLE_RATE):
        threading.Thread.__init__(self)
        self.callback = callback
        self.samplerate = samplerate
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
                data = recorder.record(numframes=640)
                self.callback(data)

    def stop(self):
        self.stop_event.set()

# text to text 번역 모델 로드
def load_ko2en_model():
    # 모델과 토크나이저 초기화
    tokenizer = AutoTokenizer.from_pretrained("NHNDQ/nllb-finetuned-en2ko")
    model = AutoModelForSeq2SeqLM.from_pretrained("NHNDQ/nllb-finetuned-en2ko")
    return model, tokenizer

# text to text 번역
def translate_text_with_e2ko(model, token, text: str):
    inputs = token(text, return_tensors="pt", padding=True, truncation=True, max_length=512)
    outputs = model.generate(inputs["input_ids"], attention_mask=inputs["attention_mask"], do_sample=False, num_beams=2, no_repeat_ngram_size=2, max_length=512)
    
    # 출력 텍스트 디코딩
    translated_text = token.decode(outputs[0], skip_special_tokens=True)
    return translated_text

# 음성에서 추출된 텍스트로 번역 작업과 관련된 것들을 처리
def transSound(ARGS):
    global transcript
    global exit_flag
    global testDir

    isLoadModel = False
    e2ko_model = None
    e2ko_token = None
    cmdCheckTime = time.time()
    
    decText = ""
    while True:
        # 종료 체크
        if exit_flag:
            return 0
        
        if transcript != "":
            lock.acquire()
            tmpText = transcript
            transcript=""
            lock.release()

            try:
                outPut = '-'+tmpText.encode('utf-8', errors='ignore').decode('utf-8')
                print(outPut)
            except Exception as e:
                pass  # 에러가 발생해도 아무런 행동을 취하지 않음
            
            # 시스템 명령 체크 ( 번역을 할지 말지 ) : 3초에 한번
            if time.time() > (cmdCheckTime+3):
                cmdCheckTime = time.time()
                work=read_file_if_exists(testDir+"childcmd.temp")
                #print("Read file: "+ ("None" if work is None else work) )
                if work != None:
                    if( work.find("ko")>=0 ):
                        ARGS.translang = work
                    else:
                        if e2ko_model is not None:
                            del e2ko_model
                        if e2ko_token is not None:
                            del e2ko_token
                        isLoadModel = False
                        ARGS.translang = None

                    os.remove(testDir+"childcmd.temp")

            if ARGS.translang is None:
                continue

            if isLoadModel is False:
                e2ko_model, e2ko_token = load_ko2en_model()
                isLoadModel = True

            outPut = translate_text_with_e2ko(e2ko_model, e2ko_token, tmpText)
            try:
                outPut = outPut.encode('utf-8', errors='ignore').decode('utf-8')
                print(outPut)
            except Exception as e:
                pass  # 에러가 발생해도 아무런 행동을 취하지 않음
        
        sleep(0.3)

    exit_flag = True

# 음성 입력을 처리하는 메인 함수
def main(ARGS):
    global transcript
    global exit_flag

    # Whisper 모델 환상 제거용 필터
    filter_list = {".", "you", "Thank you.", "Thanks for watching!", "Thanks you for watching.", "We'll be right back.", "We'll see you next week.", "www.mooji.org", "BANG!"}

    devNo=getDevNo(ARGS)
    if devNo<0:
        exit_flag = True
        return 0

    vad_audio = VADAudio(aggressiveness=ARGS.webRTC_aggressiveness,
                         device=devNo,
                         input_rate=ARGS.rate)

    frames = vad_audio.vad_collector()

    spinner = None
    if not ARGS.nospinner:
        spinner = Halo(spinner='line')

    whisper_model = whisper.load_model("large") # base, large
    #print("Voice Model loaded")

    wav_data = bytearray()
    for frame in frames:
        if exit_flag:
            exit
            
        if frame is not None:
            wav_data.extend(frame)
        else:
            if len(wav_data) > 0:
                newsound = np.frombuffer(wav_data, np.int16)
                audio_float32 = Int2Float(newsound)

                srcText = whisper_model.transcribe(audio=audio_float32, language=ARGS.voicelang, fp16=False)
                
                if len(srcText['text'])>1:
                    tmp = srcText['text'][1:].encode('utf-8', errors='ignore').decode('utf-8')
                    if tmp not in filter_list:
                        lock.acquire()
                        transcript += " "+tmp
                        lock.release()

                frame = bytearray()
                wav_data = bytearray()

    exit_flag = True

# 음성 입력장치의 번호를 반환
def getDevNo(ARGS):
    if ARGS.device is None:
        return -1

    mics = sc.all_microphones(include_loopback=True)

    for i in range(len(mics)):
        if mics[i].name.find(ARGS.device)>=0:
            return i

    return -1

# int16 -> float32로 변환
def Int2Float(sound):
    _sound = np.copy(sound)  #
    abs_max = np.abs(_sound).max()
    _sound = _sound.astype('float32')
    if abs_max > 0:
        _sound *= 1/abs_max
    audio_float32 = torch.from_numpy(_sound.squeeze())
    return audio_float32

# 초기 작업처리
if __name__ == '__main__':
    DEFAULT_SAMPLE_RATE = 16000

    import argparse
    parser = argparse.ArgumentParser(description="Stream from microphone to webRTC and silero VAD")
    parser.add_argument('--nospinner', action='store_true', help="Disable spinner")
    parser.add_argument('-d', '--device', type=str, default=None,
                        help="Device active sound input name")
    parser.add_argument('-name', '--silaro_model_name', type=str, default="silero_vad",
                        help="select the name of the model. You can select between 'silero_vad',''silero_vad_micro','silero_vad_micro_8k','silero_vad_mini','silero_vad_mini_8k'")
    parser.add_argument('-v', '--voicelang', type=str, default="en",
                        help="Voice input Language : en, ko, ja")
    parser.add_argument('-t', '--translang', type=str, default="ko",
                        help="Transfer Language : en, ko, xx")
    parser.add_argument('-i', '--origintext', type=str, default=None,
                        help="Original text output : true, false")
    parser.add_argument('-w', '--webRTC_aggressiveness', type=int, default=3,
                        help="Set aggressiveness of webRTC: an integer between 0 and 3, 0 being the least aggressive about filtering out non-speech, 3 the most aggressive. Default: 3")      

    ARGS = parser.parse_args()
    ARGS.rate=DEFAULT_SAMPLE_RATE

    # Check Input parameter
    if ARGS.device is None:
        import pyaudio

        p = pyaudio.PyAudio()
        default_output = p.get_default_output_device_info()
        ARGS.device = default_output['name']

    if ARGS.translang != "en" and ARGS.translang != "ko":
        ARGS.translang = None

    #cuda activate : Translation processing with CPU
    cuda_dev = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    print('Translation processing with ' + 'cuda' if torch.cuda.is_available() else 'cpu')

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