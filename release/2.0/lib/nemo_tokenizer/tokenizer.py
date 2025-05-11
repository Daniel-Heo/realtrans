"""
Python interface for NemoTokenizer
"""

import os
import sys
import importlib.util
from typing import List, Union, Dict, Any, Optional


# Attempt to load the C++ extension module
def _load_extension():
    try:
        # Try standard import first
        from .nemo_tokenizer_core import NemoTokenizerCore
        return NemoTokenizerCore
    except ImportError:
        # Try to manually locate the module
        try:
            # Directory path of the current file
            module_dir = os.path.dirname(os.path.abspath(__file__))
            
            # Try various extensions (depends on platform)
            for ext in ['.so', '.pyd', '.dll', '.dylib']:
                module_path = os.path.join(module_dir, f"nemo_tokenizer_core{ext}")
                if os.path.exists(module_path):
                    spec = importlib.util.spec_from_file_location("nemo_tokenizer_core", module_path)
                    if spec:
                        module = importlib.util.module_from_spec(spec)
                        spec.loader.exec_module(module)
                        return module.NemoTokenizerCore
            
            # Module not found
            raise ImportError(
                "Unable to load NemoTokenizer C++ extension module. "
                "Module not found at path: {}. "
                "Please ensure the package is installed correctly. "
                "Error: {}".format(module_dir, str(sys.exc_info()[1]))
            )
        except Exception as e:
            # Detailed error message for debugging
            raise ImportError(
                "An error occurred while loading the NemoTokenizer C++ extension module. "
                "Please ensure the package is installed correctly. "
                "Error: {}".format(str(e))
            )


# Attempt to load the C++ extension module
NemoTokenizerCore = _load_extension()


class NemoTokenizer:
    """
    NemoTokenizer class - Supports SentencePiece and WordPiece tokenization.
    
    Provides a Python interface wrapping the C++ implementation.
    """
    
    def __init__(self, tokenizer_file: Optional[str] = None):
        """
        Initialize the NemoTokenizer
        
        Args:
            tokenizer_file: Path to the tokenizer JSON file (optional)
        """
        self._tokenizer = NemoTokenizerCore()
        
        if tokenizer_file is not None:
            if not os.path.exists(tokenizer_file):
                raise FileNotFoundError(f"Tokenizer file not found: {tokenizer_file}")
            self.load_tokenizer(tokenizer_file)
    
    def load_tokenizer(self, tokenizer_file: str) -> None:
        """
        Load tokenizer JSON file
        
        Args:
            tokenizer_file: Path to the tokenizer JSON file
        """
        if not os.path.exists(tokenizer_file):
            raise FileNotFoundError(f"Tokenizer file not found: {tokenizer_file}")
        
        self._tokenizer.loadTokenizer(tokenizer_file)
    
    def batch_tokenize(self, texts: List[str], add_special_tokens: bool = True) -> List[List[str]]:
        """
        Tokenize multiple texts at once
        
        Args:
            texts: List of texts to tokenize
            add_special_tokens: Whether to add special tokens
            
        Returns:
            List of token lists for each text
        """
        return self._tokenizer.batch_tokenize(texts, add_special_tokens)
    
    def tokenize(self, text: str, add_special_tokens: bool = True) -> List[str]:
        """
        Split text into tokens
        
        Args:
            text: Text to tokenize
            add_special_tokens: Whether to add special tokens
            
        Returns:
            List of tokens
        """
        return self._tokenizer.tokenize(text, add_special_tokens)
    
    def encode(self, text: str, add_special_tokens: bool = True) -> List[int]:
        """
        Convert text to token IDs
        
        Args:
            text: Text to encode
            add_special_tokens: Whether to add special tokens
            
        Returns:
            List of token IDs
        """
        return self._tokenizer.encode(text, add_special_tokens)
    
    def decode(self, ids: List[int], skip_special_tokens: bool = True) -> str:
        """
        Convert token IDs to text
        
        Args:
            ids: List of token IDs to decode
            skip_special_tokens: Whether to remove special tokens
            
        Returns:
            Reconstructed text
        """
        return self._tokenizer.decode(ids, skip_special_tokens)
    
    def convert_tokens_to_ids(self, tokens: List[str], add_special_tokens: bool = True) -> List[int]:
        """
        Convert tokens to IDs
        
        Args:
            tokens: List of tokens to convert
            add_special_tokens: Whether to add special tokens
            
        Returns:
            List of token IDs
        """
        return self._tokenizer.convert_tokens_to_ids(tokens, add_special_tokens)
    
    def convert_ids_to_tokens(self, ids: List[int], skip_special_tokens: bool = True) -> List[str]:
        """
        Convert IDs to tokens
        
        Args:
            ids: List of IDs to convert
            skip_special_tokens: Whether to remove special tokens
            
        Returns:
            List of tokens
        """
        return self._tokenizer.convert_ids_to_tokens(ids, skip_special_tokens)
    
    def convert_tokens_to_text(self, tokens: List[str], skip_special_tokens: bool = True) -> str:
        """
        Convert tokens back to original text
        
        Args:
            tokens: List of tokens to convert
            skip_special_tokens: Whether to remove special tokens
            
        Returns:
            Reconstructed text
        """
        return self._tokenizer.convert_tokens_to_text(tokens, skip_special_tokens)
