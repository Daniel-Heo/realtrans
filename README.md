# RealTrans

![](thumbnail.png?raw=true)

실시간 음성 변환을 위한 프로그램입니다. 
현재의 소리 출력 장치에서 들리는 소리를 입력으로 받아, 영어 문장을 만들어 한국어로 번역을 합니다.
Whisper 모델을 사용하여 음성에서 영어 문장을 만들고, NHNDQ/nllb-finetuned-en2ko 모델을 사용하여 영어 문장을 한국어로 번역합니다.
추출된 영어 문장을 기반으로 API를 통해 요약하는 기능도 가지고 있습니다.
영어에서 한국어로 번역하는 부분은 DeepL API를 사용하여 번역을 할 수 있는 기능도 제공합니다.

Showcase: Not available yet

Setup Guide: Not available yet

## Setup

### Prerequisites

#### Install Git

Follow the instructions [here](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git) to install Git on your computer.

#### Install 7-Zip

Download and Install the 7-Zip application from [here](https://www.7-zip.org/download.html).

This is used to extract the zipped RVC WebUI application after it has been downloaded.

### Install Nvidia CUDA Toolkit

If you have an Nvidia GPU, you should install the Nvidia CUDA Toolkit to enable GPU acceleration for training voice models. If you do not have an Nvidia GPU, you can skip this step.

Download and install the Nvidia CUDA Toolkit from [here](https://developer.nvidia.com/cuda-toolkit-archive).


## Usage

실행하기전에 Nvidia GPU가 있는 경우, Nvidia CUDA Toolkit을 설치해야 합니다.
이 프로그램은 Cuda Toolkit 12.x을 사용하여 개발되었습니다.

realtrans.exe 파일을 실행하면 프로그램이 실행됩니다.
실행후에는 현재의 소리 출력 장치에서 들리는 소리를 입력으로 받아, 영어 문장을 만들어 한국어로 번역을 합니다.

## Warning

Nvidia GPU가 없는 경우 환경에서 테스트 되지 않았습니다.

Nvidia GPU는 1660 Super, 3080을 사용하여 테스트 되었습니다. 
1660 Super에서 정상적으로 작동하는 것을 확인하였습니다. 그 이하에서는 테스트되지 않았습니다.

총 VRAM 사용량 : 5.4GB ( 최소 6GB 이상의 VRAM이 필요합니다. )
Fast Whisper Large Model : 3GB VRAM
NHNDQ/nllb-finetuned-en2ko Model : 2.4GB VRAM

- VRAM이 작은 경우에는  Whisper small, base, tiny 모델을 사용하여 사용량을 줄일 수 있습니다. ( 이 경우는 소스 수정 필요 )
  small 모델은 인식 품질이 떨어질 수 있습니다.

* 프로그램 실행시에 실행중인 Python 프로그램이 종료됩니다. ( 비정상 종료시에 남아있는 프로세스를 종료시키기 위하여 )
* 게임등과 같이 사용할 경우에 VRAM이 부족하여 프로그램이 종료되거나 시스템이 마비상태가 되어 제어가 어려운 상황에 놓일 수 있습니다. 
  절대 게임과 같이 사용하지 말아주세요.

## Terms of Use

The use of the source for the following purposes is prohibited.

* MIT License와 해당 국가의 법률에 위배되는 행위

## Disclaimer

I am not liable for any direct, indirect, consequential, incidental, or special damages arising out of or in any way connected with the use/misuse or inability to use this software.
