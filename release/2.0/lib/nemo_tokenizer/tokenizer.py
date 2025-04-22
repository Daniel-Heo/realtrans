"""
NemoTokenizer의 Python 인터페이스
"""

import os
import sys
import importlib.util
from typing import List, Union, Dict, Any, Optional


# C++ 확장 모듈 로딩 시도
def _load_extension():
    try:
        # 먼저 일반적인 임포트 시도
        from .nemo_tokenizer_core import NemoTokenizerCore
        return NemoTokenizerCore
    except ImportError:
        # 모듈 경로 직접 찾기 시도
        try:
            # 현재 파일의 디렉토리 경로
            module_dir = os.path.dirname(os.path.abspath(__file__))
            
            # 다양한 확장자 시도 (플랫폼에 따라 달라질 수 있음)
            for ext in ['.so', '.pyd', '.dll', '.dylib']:
                module_path = os.path.join(module_dir, f"nemo_tokenizer_core{ext}")
                if os.path.exists(module_path):
                    spec = importlib.util.spec_from_file_location("nemo_tokenizer_core", module_path)
                    if spec:
                        module = importlib.util.module_from_spec(spec)
                        spec.loader.exec_module(module)
                        return module.NemoTokenizerCore
            
            # 모듈을 찾을 수 없음
            raise ImportError(
                "NemoTokenizer C++ 확장 모듈을 가져올 수 없습니다. "
                "파일 경로: {} 에서 모듈을 찾을 수 없습니다. "
                "패키지를 올바르게 설치했는지 확인하세요. "
                "Error: {}".format(module_dir, str(sys.exc_info()[1]))
            )
        except Exception as e:
            # 디버깅을 위한 자세한 오류 메시지
            raise ImportError(
                "NemoTokenizer C++ 확장 모듈을 로드하는 중 오류가 발생했습니다. "
                "패키지를 올바르게 설치했는지 확인하세요. "
                "Error: {}".format(str(e))
            )


# C++ 확장 모듈 로딩 시도
NemoTokenizerCore = _load_extension()



class NemoTokenizer:
    """
    NemoTokenizer 클래스 - SentencePiece 및 WordPiece 토큰화를 지원합니다.
    
    C++ 구현을 감싸는 Python 인터페이스를 제공합니다.
    """
    
    def __init__(self, tokenizer_file: Optional[str] = None):
        """
        NemoTokenizer 초기화
        
        Args:
            tokenizer_file: 토크나이저 JSON 파일 경로 (선택사항)
        """
        self._tokenizer = NemoTokenizerCore()
        
        if tokenizer_file is not None:
            if not os.path.exists(tokenizer_file):
                raise FileNotFoundError(f"토크나이저 파일을 찾을 수 없습니다: {tokenizer_file}")
            self.load_tokenizer(tokenizer_file)
    
    def load_tokenizer(self, tokenizer_file: str) -> None:
        """
        토크나이저 JSON 파일 로드
        
        Args:
            tokenizer_file: 토크나이저 JSON 파일 경로
        """
        if not os.path.exists(tokenizer_file):
            raise FileNotFoundError(f"토크나이저 파일을 찾을 수 없습니다: {tokenizer_file}")
        
        self._tokenizer.loadTokenizer(tokenizer_file)
    
    def batch_tokenize(self, texts: List[str], add_special_tokens: bool = True) -> List[List[str]]:
        """
        여러 텍스트를 한번에 토큰화
        
        Args:
            texts: 토큰화할 텍스트 리스트
            add_special_tokens: 특수 토큰 추가 여부
            
        Returns:
            각 텍스트에 대한 토큰 리스트의 리스트
        """
        return self._tokenizer.batch_tokenize(texts)
    
    def tokenize(self, text: str, add_special_tokens: bool = True) -> List[str]:
        """
        텍스트를 토큰으로 분리
        
        Args:
            text: 토큰화할 텍스트
            add_special_tokens: 특수 토큰 추가 여부
            
        Returns:
            토큰 리스트
        """
        return self._tokenizer.tokenize(text)
    
    def encode(self, text: str, add_special_tokens: bool = True) -> List[int]:
        """
        텍스트를 토큰 ID로 변환
        
        Args:
            text: 인코딩할 텍스트
            add_special_tokens: 특수 토큰 추가 여부
            
        Returns:
            토큰 ID 리스트
        """
        return self._tokenizer.encode(text, add_special_tokens)
    
    def decode(self, ids: List[int], skip_special_tokens: bool = True) -> str:
        """
        토큰 ID를 텍스트로 변환
        
        Args:
            ids: 디코딩할 토큰 ID 리스트
            skip_special_tokens: 특수 토큰 삭제 여부
            
        Returns:
            복원된 텍스트
        """
        return self._tokenizer.decode(ids)
    
    def convert_tokens_to_ids(self, tokens: List[str], add_special_tokens: bool = True) -> List[int]:
        """
        토큰을 ID로 변환
        
        Args:
            tokens: 변환할 토큰 리스트
            add_special_tokens: 특수 토큰 추가 여부
            
        Returns:
            토큰 ID 리스트
        """
        return self._tokenizer.convert_tokens_to_ids(tokens)
    
    def convert_ids_to_tokens(self, ids: List[int], skip_special_tokens: bool = True) -> List[str]:
        """
        ID를 토큰으로 변환
        
        Args:
            ids: 변환할 ID 리스트
            skip_special_tokens: 특수 토큰 삭제 여부
            
        Returns:
            토큰 리스트
        """
        return self._tokenizer.convert_ids_to_tokens(ids)
    
    def convert_tokens_to_text(self, tokens: List[str], skip_special_tokens: bool = True) -> str:
        """
        토큰을 원본 텍스트로 변환
        
        Args:
            tokens: 변환할 토큰 리스트
            skip_special_tokens: 특수 토큰 삭제 여부
            
        Returns:
            복원된 텍스트
        """
        return self._tokenizer.convert_tokens_to_text(tokens)