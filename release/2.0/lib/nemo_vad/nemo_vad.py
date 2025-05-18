"""
Python interface for NemoVad
"""

import os
import sys
import importlib.util
import numpy as np
from numpy.typing import NDArray
from typing import List, Union, Dict, Any, Optional, Callable


# C++ 확장 모듈 로딩 시도
def _load_extension():
    try:
        # 표준 import 시도
        from . import nemo_vad_core
        return nemo_vad_core
    except ImportError:
        try:
            # 현재 파일의 디렉토리 경로
            module_dir = os.path.dirname(os.path.abspath(__file__))
            
            # Python 버전 정보
            python_version = f"cp{sys.version_info.major}{sys.version_info.minor}"
            
            # Windows 전용 모듈 파일명들
            possible_names = [
                f"nemo_vad_core.{python_version}-win_amd64.pyd",
                f"nemo_vad_core.pyd",
                "nemo_vad_core.dll"
            ]
            
            # 실제 존재하는 파일 찾기
            for name in possible_names:
                print(f"파일: {name}")
                module_path = os.path.join(module_dir, name)
                if os.path.exists(module_path):
                    # 모듈 로딩 시도
                    spec = importlib.util.spec_from_file_location("nemo_vad_core", module_path)
                    if spec and spec.loader:
                        module = importlib.util.module_from_spec(spec)
                        spec.loader.exec_module(module)
                        return module
            
            # 디버그 정보 출력
            print(f"디렉토리: {module_dir}")
            print("디렉토리 내 파일들:")
            for file in os.listdir(module_dir):
                print(f"  - {file}")
            
            raise ImportError(
                f"Unable to load NemoVad C++ extension module. "
                f"Module not found at path: {module_dir}. "
                f"Please ensure the package is installed correctly."
            )
        except Exception as e:
            raise ImportError(
                f"An error occurred while loading the NemoVad C++ extension module. "
                f"Error: {str(e)}"
            )


# C++ 확장 모듈 로딩
nemo_vad_core = _load_extension()

# VADOptions 클래스를 외부에 노출
VADOptions = nemo_vad_core.VADOptions

class NemoVad:
    def __init__(self, 
                 aggressiveness: int = 2,
                 device: str = "speaker",
                 sample_rate: int = 16000,
                 min_speech_duration: float = 0.3,
                 mid_speech_duration: float = 3.0,
                 max_speech_duration: float = 5.0,
                 min_silence_duration: float = 0.3,
                 ratio:float = 0.6):
        """
        Args:
            device: 오디오 장치 타입 ("speaker" 또는 "mic")
            sample_rate: 샘플 레이트 (Hz)
            aggressiveness: VAD 적극성 (0-3, 높을수록 더 엄격)
            min_speech_duration: 최소 음성 지속 시간 (초)
            max_speech_duration: 최대 음성 지속 시간 (초)
            min_silence_duration: 최소 무음 지속 시간 (초)
        """
        self._vad = nemo_vad_core.NemoVAD(aggressiveness, device, sample_rate)
        self._setup_options(min_speech_duration, mid_speech_duration, max_speech_duration, min_silence_duration, ratio)
        self._speech_callback = None
        
    def _setup_options(self, min_speech: float, mid_speech: float, max_speech: float, min_silence: float, ratio: float):
        """VAD 옵션 설정"""
        options = VADOptions()
        options.voiced_duration_min_ms = int(min_speech * 1000)
        options.voiced_duration_mid_ms = int(mid_speech * 1000)
        options.voiced_duration_max_ms = int(max_speech * 1000)
        options.silence_duration_min_ms = int(min_silence * 1000)
        options.ratio = ratio 
        self._vad.set_options(options)
    
    def start(self):
        """음성 감지 시작"""
        return self._vad.start()
    
    def stop(self):
        """음성 감지 중지"""
        return self._vad.stop()
    
    def pause(self):
        """음성 감지 일시 중지"""
        return self._vad.pause()
    
    def resume(self):
        """음성 감지 재개"""
        return self._vad.resume()
    
    def set_options(self, options: VADOptions):
        """VAD 옵션 설정"""
        self._vad.set_options(options)
    
    def has_new_speech(self) -> bool:
        """새로운 음성 데이터가 있는지 확인 (has_speech의 별칭)"""
        return self._vad.has_new_speech()
    
    def get_speech_data(self, dtype=np.float32):
        """
        다양한 데이터 타입으로 음성 데이터를 가져옵니다.
        
        Args:
            dtype: 원하는 NumPy 데이터 타입 (np.float16, np.float32 또는 np.int16). 기본값은 np.float32.
        
        Returns:
            NDArray: 지정된 데이터 타입의 NumPy 배열로 된 음성 데이터.
        """
        if dtype not in (np.float16, np.float32, np.int16):
            raise ValueError("지원되지 않는 dtype입니다. np.float16, np.float32 또는 np.int16 중 하나여야 합니다.")
        
        # 데이터 타입에 따라 적절한 함수 호출
        if dtype == np.float16:
            return self._vad.get_speech_data_float16()
        elif dtype == np.float32:
            return self._vad.get_speech_data_float32() 
        else:  
            return self._vad.get_speech_data_int16()
  
    @property
    def is_running(self) -> bool:
        """음성 감지기가 실행 중인지 확인"""
        return self._vad.is_running()
    
    @property
    def is_paused(self) -> bool:
        """음성 감지기가 일시 중지 상태인지 확인"""
        return self._vad.is_paused()