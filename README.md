# RealTrans ( English )

![](thumbnail.png?raw=true)

![](setting.png?raw=true)

![](summary.png?raw=true)

![](text_trans.png?raw=true)

This is a program for real-time voice translation. Recognizes and translates the voice heard from the current sound output device. Create sentences from speech using the Faster Whisper model, and translate the sentences using the nllb model. It also has a function to summarize through API based on spoken sentences. The translation part also provides the ability to translate using the DeepL API. Text translation is also supported. Translation is possible after entering data directly or importing data through file input and selecting a language. Based on Nvidia 3080, approximately 300 lines are translated per second.

- Support API : OpenAI API ( Summary ), DeepL ( real time translation )
- Support OS : Windows 10, 11 ~

### Install

> 1. Install Python
> 2. Run install.bat in the RealTrans folder
> 3. Install CuDNN
> 4. Run realtrans.exe in the RealTrans folder

#### 1. Install Python ( Require 3.12.x ) : https://www.python.org/downloads/ ( *** Make sure to select run Admin and add PATH during installation. *** ) - Supports only up to Python 3.12. (Recommended version: 3.12.8 Windows installer (64-bit) ) *Important*
( Caution: It is recommended to install the Python installation folder in the simple format of C:\python\. If the path is complex, contains spaces, or contains non-English characters, it may not run sometimes. )

> How to check installation result
- Run cmd in Windows to create a console window
- Run python --version (If it runs and the python version is printed, it is normal, so enter exit() to exit)
* If Python does not run normally, Python may have been installed multiple times. In this case, remove all previously installed Pythons and reinstall them. If it is not a duplicate, the python path may be missing from the environment variables. In this case, you should reinstall it or select Path in Windows' Advanced System Settings/Environment Variables/System Variables, edit it, add the python path, and apply it.

#### 2. run realtrans install batch file in the RealTrans folder ( install python package )
( Caution: It is recommended to place the RealTrans folder in the simple format of C:\realtrans\. If the path is complex, contains spaces, or contains non-English characters, it may not run sometimes. )

> Run install.bat
* Method 1: Open Windows File Explorer, go to the realtrans folder, and click on the install.bat file to run it.
* Method 2: When installing from cmd, run install.bat from the realtrans folder (downloaded realtrans folder)

- install.bat must be completed without error. When an error occurs, you must respond to the error message.
- Run python runTransWin.py. If the Translation processing with cuda message appears, it is normal.
- If you play something like YouTube and make audio come out, the message will be translated into English and Korean in the console window you run.
- If you have succeeded up to this point, run RealTrans.exe and it will run normally.
  * When running for the first time, downloading the model may take several minutes. If an error message appears and a symlink-related error appears, set administrator privileges or developer mode only when downloading the model to download normally.
  * Note) If install.bat is not executed properly, a no module error may occur when running.

### 3. Install CuDNN
If not installed, you may encounter the following error:

Could not locate cudnn_ops64_9.dll. Please make sure it is in your library path!

CuDNN Installation : https://developer.nvidia.com/cudnn-downloads 

After extracting or installing the downloaded file, you will find three folders: bin, include, and lib.

Installation Path Example:
C:\Program Files\NVIDIA\CUDNN\v9.5\bin\12.6

Copy the .dll files from this path to the realtrans folder.

* If other programs also require CuDNN, you can add the directory containing the .dll files to the system environment variables.
Edit the system environment variables, click Path under User Variables, and add the following:
C:\Program Files\NVIDIA\CUDNN\v9.5\bin\12.6.

#### 4. Final Windows program execution
> realtrans.exe

  - If execution is successful, you only need to run realtrans.exe from next time.

* Optional: Installing Cuda Toolkit and CuDNN will reduce GPU and CPU loads. It is not recommended to install them when installing for the first time. Install them later when you need performance. You must install both of them when installing.
  
   Install Cuda Toolkit: https://developer.nvidia.com/cuda-toolkit (You can install the Windows version. Improves floating point calculation and PyTorch performance)

   Install CuDNN: https://developer.nvidia.com/cudnn-downloads (performance can be improved by about 20%)
	When you unzip or install the downloaded file, there are three folders: bin, include, and lib.
	In the folder where you installed the CUDA Toolkit, there is a folder with the same name as the three folders above. (CUDA Toolkit directory: C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6)
	Add the files in the cuDNN bin, include, and lib folders to the bin, include, and lib folders in the folder in the CUDA Toolkit.

## Usage

When you run the realtrans.exe file, the program will run.
After execution, it receives the sound heard from the current sound output device as input, creates an English sentence, and translates it into Korean.

How to use the program
  - Click the settings gear in the top left corner to select the voice language you are inputting and select the language you want to translate to.
  - When translating, the PC has the function of translating from its own computer to AI, and the API sends voice-recognized text through the API KEY of an external cloud service and receives and displays the translated text.
  - If your computer's performance is slow, activate only voice recognition and use an external API for translation to reduce the load on your computer.
  - The summary sends the contents of the text window to OpenAI using the API KEY, receives the summarized results, and displays them. Because OpenAI response is slow, there may be a lot of waiting time when processing a large amount of data.
  - If you select speaker for the voice input in the settings, voice input will be received from the speaker output. If you select mic, input will be received from the microphone.
  - The processing method of the settings is cuda float16 by default. If an error occurs with an old nvidia graphics card, try using cuda float32. If you are running it on a CPU, use the small version.

* It is recommended to run runTransWin.py once when running for the first time. The model download status may not appear in Windows programs.
*Model downloading may take a long time. (Usually it takes about 10 minutes for small, 30 minutes for medium, and 1 hour for large. If the network conditions are bad, it may take twice as long.)
* Feeling by model size: The small model is recognized, but the quality feels a lot lower. The medium model has somewhat good recognition and translation. The large model has a slightly better recognition and translation than the medium model.

* * To improve the recognition speed in the medium model, you can use the turbo model. In this case, you can modify the relevant parts in the two files below.

	python\Lib\site-packages\faster_whisper\util.py 20 lines

	#"medium": "Systran/faster-whisper-medium",

	"medium": "mobiuslabsgmbh/faster-whisper-large-v3-turbo",

	Lib\ctrans_manager.py 385 lines

	#"medium": "Systran/faster-whisper-medium",

	"medium": "mobiuslabsgmbh/faster-whisper-large-v3-turbo",

## Hallucination Filter (Filters out results that are unrelated to the voice.)

File ) hallucination_filter.json

ex) en : English base https://www.andiamo.co.uk/resources/iso-language-codes/

    "en": [
         "you", -> Filter sentences that match exactly.
         "*Thank*", -> Filters sentences containing Thank.
         "(Oh)", -> Filtered when Oh appears repeatedly and takes up more than 50% of the sentence.
         "[www.rrr.org]", -> If there is a difference of about 2 characters between www.rrr.org, it will be filtered.
	]

## Support Language
 
UI : Korean, English, German, Spanish, French, Italian, Japanese, Portuguese, Russian, Chinese, Arabic, Hindi

Voice recognition language : ALL ( automatic recognition ), Korean, English, German, French, Italian, Arabic, Hindi, Polish, Russian, Turkish, Dutch, Spanish, Thai, Portuguese, Indonesian, Swedish, Czech, Romanian
                            ,Catalan, Hungarian, Ukrainian, Greek, Bulgarian, Serbian, Macedonian, Latvian, Slovenian, Galician, Danish, Urdu, Slovak, Hebrew, Finnish, Azerbaijani Latn/Arab, Lithuanian, Estonian
                            ,Nynorsk, Welsh, Punjabi Guru, Basque, Vietnamese, Bengali, Nepali, Marathi, Belarusian, Kazakh, Armenian, Swahili, Tamil, Albanian, Mandarin(TW), Cantonese(HK), Mandarin(CN), Cantonese(CN), Japanese
			    
Translation language : 

	A total of 186 languages
	Korean, Hangul
	English, Latin 
 	...
	

## Warning

If you do not have an Nvidia GPU, you can use an appropriately small model depending on your computer specifications.

Recommended graphics card: Most NVidia graphics cards

VRAM requirements by model size: large 5.68GB, medium 2.85GB, small 1.07GB

|	|faster whisper	|ctranslate2	|total require|
|-------|---------------|---------------|-------------|
|large 	|3	        |2.68	        |5.68GB       |
|medium	|1.5	        |1.35	        |2.85GB       |
|small 	|0.47	        |0.6	        |1.07GB       |

ctranslate2 usage model

large : facebook nllb-200-distilled-1.3B ct2 float16

medium : nllb-200-distilled-1.3B ct2 int8

small : nllb-200-distilled-600M ct2 int8

en2ko : NHNDQ/nllb-finetuned-en2ko ct2 float16

ko2en : NHNDQ/nllb-finetuned-ko2en ct2 float16

- If your VRAM is small, you can reduce usage by using Whisper small, base, and tiny models. (In this case, source modification is required)
  
   Small models may have lower recognition quality.

* When the program is run, the running Python program is terminated. (To terminate remaining processes in case of abnormal termination)

## Terms of Use

The use of the source for the following purposes is prohibited.

* Any act that violates the MIT License and the laws of your country. 

## Disclaimer

I am not liable for any direct, indirect, consequential, incidental, or special damages arising out of or in any way connected with the use/misuse or inability to use this software.

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/realtrans)

## Frequently Asked Questions

1. What are the graphic card requirements?
Small can be run with CPU. From the medium model, it can be run normally without slowdown on Nvidia 1660Super, 2060 or higher.


## Error message explanation

- Requested float16 compute type, but the target device or backend do not support efficient float16 computation.
  
The error above means that float16 is not supported. If your nvidia graphics card is old, this error may occur. Then you need to change it to float32. Set the processing unit to cuda float32 in the settings.

- Could not locate cudnn_ops64_9.dll. Please make sure it is in your library path!
  
The above error is because the dll file of cudnn cannot be found. You need to read the installation manual on the Internet and place the dll files in the appropriate location.

- If only "ython" is displayed on the screen
  
This is because you did not check the with path option when installing Python or Python cannot be found for other reasons.

- ModuleNotFoundError: No module named 'soundcard'
  
This happens if you don't run the install.bat file of the realtrans compressed file.

- Fetching 4 files: If it stops at 25%
  
When downloading a model, it takes a long time to complete. It can take 10 minutes for a small model, 30 minutes for a medium model, and more than an hour for a large model.

- runTransWin.py: error: argument -d/--device: expected one argument
  
This is a compatibility error that occurs when you overwrite a new version of RealTrans with an older version of the config.json file. Try saving the settings in Settings and then restarting.

- File "C:\python\Lib\site-packages\faster_whisper\transcribe.py", line 419, in transcribe
    language = max(
               ^^^^
ValueError: max() iterable argument is empty

 When the language is set to ALL, there are cases where the language cannot be detected in non-speech audio. In such cases, an error occurs in faster-whisper. Modifying the faster-whisper source code as shown below prevents this issue.

python/Lib/site-packages/faster_whisper/transcribe.py Let’s modify the file starting from line 419.

Before)

                    language = max(
                        detected_language_info,
                        key=lambda lang: len(detected_language_info[lang]),
                    )
                    language_probability = max(detected_language_info[language])

After) 

                    if( not detected_language_info ):
                        language = "en"
                        language_probability = 1
                    else:
                        language = max(
                            detected_language_info,
                            key=lambda lang: len(detected_language_info[lang]),
                        )
                        language_probability = max(detected_language_info[language])

--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# RealTrans ( 한국어 )

실시간 음성 번역을 위한 프로그램입니다. 현재의 소리 출력 장치에서 들리는 음성을 인식하고 번역을 합니다. Faster Whisper 모델을 사용하여 음성에서 문장을 만들고, nllb 모델을 사용하여 문장을 번역합니다. 음성 문장을 기반으로 API를 통해 요약하는 기능도 가지고 있습니다. 번역하는 부분은 DeepL API를 사용하여 번역을 할 수 있는 기능도 제공합니다. 텍스트 번역도 지원합니다. 직접 입력하거나 파일입력으로 데이터를 가져와서 언어를 선택한 후에 번역이 가능합니다. Nvidia 3080기준 초당 300줄 정도 번역이됩니다.

- 지원 API : OpenAI API ( 요약 ), DeepL ( 실시간 번역 )
- 지원 OS : Windows 10, 11 ~

### 설치

> 1. 파이선 설치
> 2. RealTrans 폴더의 install.bat 실행
> 3. CuDNN 설치
> 4. RealTrans 폴더의 realtrans.exe 실행

#### 1. 파이선 설치 ( https://wikidocs.net/8 - *** 설치중 run Admin과 PATH 추가를 꼭 선택하세요. *** ) - 파이선 3.12까지만 지원. ( 권장버젼 : 3.12.8 Windows installer (64-bit) ) *중요*
( 주의 : 파이선 설치폴더는 간단하게 C:\python\로 설치하시는 것을 추천합니다. 경로가 복잡하거나 스페이스가 들어가거나 영문외의 글자가 들어갈 경우에 가끔 실행이 안되는 경우가 있습니다. )

> 정상 설치 확인 방법
- window에서 cmd를 실행하여 console창을 만듬
- python --version 실행 ( 실행이 되어서 python version이 출력되면 정상임으로 exit()를 입력하여 빠져나옴 )
* 파이선이 정상적으로 실행이 되지 않을 경우에 파이선이 중복으로 설치되었을 수 있으며 이 경우에는 기존에 설치된 파이선들을 다 제거하고 다시 설치해보세요. 중복이 아닌 경우에는 환경변수에 python path가 누락되어있을 수 있습니다. 이 경우에는 재설치를 하시거나 윈도우의 고급 시스템 설정/환경변수/시스템 변수에서 Path를 선택하고 편집해서 python 경로를 추가해주시고 적용을 해주셔야합니다.

#### 2. Realtrans 설치 : RealTrans 폴더의 install.bat 실행
( 주의 : RealTrans 폴더도 간단하게 C:\realtrans\ 형식의 폴더에 넣으시는걸 추천드립니다. 경로가 복잡하거나 스페이스가 들어가거나 영문외의 글자가 들어갈 경우에 가끔 실행이 안되는 경우가 있습니다. )

> install.bat 실행
* 방법 1 : 윈도우 파일 탐색기 열어서 해당 realtrans 폴더로 가셔서 install.bat 파일을 눌러서 실행하세요.
* 방법 2 : cmd에서 설치시에는 해당 realtrans 폴더에서 install.bat 실행 (down 받은 realtrans 폴더 )

 - install.bat이 오류 없이 완료가 되어야함. 오류 발생시 해당 오류 메시지에 대한 대처를 해야됩니다.
 - python runTransWin.py를 실행해봄 Translation processing with cuda 메시지가 나오면 정상
 - 유튜브 같은 것을 플레이해서 음성이 나오게 하면 실행한 콘솔창에서 영한번역이 되어서 메시지가 출력됩니다.
 - 여기까지 성공하셨으면 RealTrans.exe를 실행하면 정상적으로 수행되게됩니다.
   * 처음 실행시 모델 다운로드가 몇분 걸릴수도 있습니다. 에러메시지가 나올 경우에 symlink관련 오류가 뜨면 모델 다운로드시에만 관리자권한이나 개발자모드를 설정하시면 정상적으로 다운로드가됩니다.
   * 참고) 정상적으로 install.bat이 실행되지 않은 경우 실행시 no module 에러가 발생할 수 있습니다.
  
### 3. CuDNN 설치

 설치하지 않을 경우 다음과 같은 에러가 발생합니다. 

  - Could not locate cudnn_ops64_9.dll. Please make sure it is in your library path!

CuDNN 설치 : https://developer.nvidia.com/cudnn-downloads 

다운로드한 파일을 압축 해제하거나 설치를 하면 bin, include, lib 3개의 폴더가 존재한다.

설치형 위치 : C:\Program Files\NVIDIA\CUDNN\v9.5\bin\12.6

해당 위치에서 dll파일들을 realtrans폴더에 카피를 한다.

* 다른 프로그램에서도 CuDNN을 사용할 경우에는 시스템환경변수 편집에서 환경변수를 클릭하고 사용자 변수의 Path에 dll파일이 있는 C:\Program Files\NVIDIA\CUDNN\v9.5\bin\12.6를 추가한다.

#### 4. 최종 윈도우 프로그램 실행
> realtrans.exe 실행

 - 실행이 정상적으로 되면 다음부터는 realtrans.exe만 실행하면 됩니다.
  

## 사용법
realtrans.exe 파일을 실행하면 프로그램이 실행됩니다.
실행후에는 현재의 소리 출력 장치에서 들리는 소리를 입력으로 받아, 영어 문장을 만들어 한국어로 번역을 합니다.

프로그램 사용법
 - 왼쪽 제일 위에 톱니바퀴의 설정을 눌러서 입력하는 음성 언어를 선택하시고 번역할 언어를 선택하시면 됩니다.
 - 번역시에 PC는 자체 컴퓨터에서 AI로 번역하는 기능이고, API는 외부 클라우드 서비스의 API KEY를 통해 음성인식한 텍스트를 보내고 번역된 텍스트를 받아서 표시합니다.
 - 컴퓨터의 성능이 느릴 경우에는 음성인식만 활성화시킨후에 번역은 외부 API를 사용하시면 컴퓨터에 부하가 적어집니다.
 - 요약은 OpenAI에 API KEY로 텍스트 창의 내용을 보내서 요약한 결과를 받아서 표시합니다. OpenAI 응답이 느리기 때문에 많은 양을 처리할 경우에 대기시간이 많이 걸릴 수 있습니다.
 - 설정의 음성입력은 speaker로 선택하시면 스피커 출력에서 음성입력을 받습니다. mic 선택시 마이크에서 입력받습니다.
 - 설정의 처리방식은 cuda float16이 기본이며, 예전 nvidia 그래픽카드일 경우 에러가 발생하면 cuda float32을 사용해보세요. CPU로 돌릴 경우에는 small버젼을 사용하세요.

* 처음 실행시에는 runTransWin.py를 한번 실행하는 것이 좋습니다. 모델 다운로드 상태가 윈도우 프로그램에서 나타나지 않을 수 있습니다.
* 모델 다운로드 시간이 많이 걸릴 수 있습니다.  ( 보통 small은 10분, mediaum은 30분, large는 1시간 정도 소요되며 네트웍 사정이 안좋을 경우에는 2배의 시간이 걸릴 수 있습니다. )
* 모델 사이즈별 느낌 : small 모델은 인식은 되는데 품질은 많이 떨어지는 느낌. medium은 인식과 번역이 어느정도 쓸만하다는 느낌. large는 인식과 번역이 medium보다는 조금 잘되는 느낌.

* medium 모델에서 인식 속도 개선을 위해서는 turbo 모델을 사용할 수 있습니다. 이 경우 아래 2개의 파일에 해당 부분을 수정해주시면됩니다.

	python\Lib\site-packages\faster_whisper\util.py 20 lines

	#"medium": "Systran/faster-whisper-medium",

	"medium": "mobiuslabsgmbh/faster-whisper-large-v3-turbo",

	Lib\ctrans_manager.py 385 lines

	#"medium": "Systran/faster-whisper-medium",

	"medium": "mobiuslabsgmbh/faster-whisper-large-v3-turbo",

## 할루시네이션 필터 ( 음성과 상관없는 결과가 나오는 것을 필터합니다. )

파일 : hallucination_filter.json

예제) en : 영어 ( 참고 : https://www.andiamo.co.uk/resources/iso-language-codes/ )

    "en": [
        "you",   -> 정확하게 맞는 문장을 필터합니다.
        "*Thank*", -> Thank가 들어간 문장을 필터합니다.
        "(Oh)",  -> Oh가 반복적으로 나와 문장의 50%이상을 차지할 때 필터됩니다.
        "[www.rrr.org]",  -> www.rrr.org가 들어가면서 2문자 정도 차이가 날 경우에 필터가 됩니다.
	]

## 지원 언어
 
UI : 한국어, 영어, 독일어, 스페인어, 프랑스어, 이탈리아어, 일본어, 포르투갈어, 러시아어, 중국어, 아랍어, 힌디어

음성인식 언어 : ALL(자동인식), 한국어, 영어, 독일어, 프랑스어, 이탈리아어, 아랍어, 힌디어, 폴란드어, 러시아어, 터키어, 네덜란드어, 스페인어, 태국어, 포르투갈어, 인도네시아어, 스웨덴어, 체코어, 루마니아어
                             ,카탈로니아어, 헝가리어, 우크라이나어, 그리스어, 불가리아어, 세르비아어, 마케도니아어, 라트비아어, 슬로베니아어, 갈리시아어, 덴마크어, 우르두어, 슬로바키아어, 히브리어, 핀란드어, 아제르바이잔어 라트/아랍어, 리투아니아어, 에스토니아어
                             ,뉘노르스크, 웨일스어, 펀자브 구루, 바스크어, 베트남어, 벵골어, 네팔어, 마라티어, 벨로루시어, 카자흐어, 아르메니아어, 스와힐리어, 타밀어, 알바니아어, 북경어(TW), 광둥어(HK), 북경어(CN), 광둥어(CN), 일본어
			     
번역 언어:

	총 186개 언어
	한국어, 한글
	영어, 라틴어
 	...

## 주의

Nvidia GPU가 없는 경우 컴퓨터 사양에 따라서 적당히 작은 모델을 사용하시면 됩니다.

권장 그래픽카드 : 대부분의 NVidia 그래픽카드

모델 크기별 VRAM 요구사항 : large 5.68GB, medium 2.85GB, small 1.07GB

|	|faster whisper	|ctranslate2	|total require|
|-------|---------------|---------------|-------------|
|large 	|3	        |2.68	        |5.68GB       |
|medium	|1.5	        |1.35	        |2.85GB       |
|small 	|0.47	        |0.6	        |1.07GB       |

ctranslate2 usage model

large : facebook nllb-200-distilled-1.3B ct2 float16

medium : nllb-200-distilled-1.3B ct2 int8

small : nllb-200-distilled-600M ct2 int8

en2ko : NHNDQ/nllb-finetuned-en2ko ct2 float16

ko2en : NHNDQ/nllb-finetuned-ko2en ct2 float16

- VRAM이 작은 경우에는  Whisper small, base, tiny 모델을 사용하여 사용량을 줄일 수 있습니다. ( 이 경우는 소스 수정 필요 )
  
  small 모델은 인식 품질이 떨어질 수 있습니다.

* 프로그램 실행시에 실행중인 Python 프로그램이 종료됩니다. ( 비정상 종료시에 남아있는 프로세스를 종료시키기 위하여 )

## 이용 약관

다음과 같은 목적으로 소스를 사용하는 것을 금지합니다.

* MIT License와 해당 국가의 법률에 위배되는 행위

## 부연 설명

이 소프트웨어의 사용으로 발생하는 직접적, 간접적, 결과적, 우발적 또는 특별한 손해에 대해 책임을 지지 않습니다.

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/realtrans)

## 자주 묻는 질문

1. 그래픽 카드 요구사항은?

small은 CPU로 돌려도 돌아갑니다. medium 모델부터는 Nvidia 1660Super, 2060이상 정도에서 정상적으로 느려지지 않게 돌아갑니다.

​

## 에러메시지 설명

- Requested float16 compute type, but the target device or backend do not support efficient float16 computation.
  
위의 에러는 float16을 지원하지 않는다는 것입니다. nvidia 그래픽카드인데 오래된 것은 해당 에러가 발생할 수 있습니다. 그러면 float32로 변경하셔야합니다. 설정에서 처리장치를 cuda float32로 설정하세요.

- Could not locate cudnn_ops64_9.dll. Please make sure it is in your library path!
  
위의 에러는 cudnn의 dll 파일을 찾지못해서 그렇습니다. 인터넷의 해당 설치메뉴얼을 읽으시고 해당하는 곳에 dll파일들을 위치시키셔야합니다.

- 화면에 "ython"만 보일 경우
  
파이선 설치하실때 with path 옵션을 체크하지 않았거나 기타 이유로 파이선을 찾지못하는 경우입니다.

- ModuleNotFoundError: No module named 'soundcard'
  
realtrans 압축파일의 install.bat을 실행하지 않을 경우 발생합니다.

- runTransWin.py: error: argument -d/--device: expected one argument
  
기존에 RealTrans를 사용하다가 새로운 버젼을 덮어씌울때 config.json파일이 예전버젼이라서 생기는 호환성 오류입니다. 설정에서 환경설정을 저장한 후에 다시 시작해보세요.

- Fetching 4 files: 25%에서 멈춰있는 경우

모델을 다운로드 받을 경우 완료되기까지 시간이 걸립니다. small 모델은 10분 medium 모델은 30분, large 모델은 1시간 이상이 걸릴 수 있습니다.

- File "C:\python\Lib\site-packages\faster_whisper\transcribe.py", line 419, in transcribe
    language = max(
               ^^^^
ValueError: max() iterable argument is empty

 언어를 ALL로 설정한 경우에 음성이 아닌 것에서 언어를 감지못하는 경우가 있는데, 이 경우 faster-whisper에 에러가 발생한다. faster-whisper의 소스를 아래와같이 변경하면 발생하지 않는다.

python/Lib/site-packages/faster_whisper/transcribe.py 파일의 419번째 라인부터 변경하자.

기존)

                    language = max(
                        detected_language_info,
                        key=lambda lang: len(detected_language_info[lang]),
                    )
                    language_probability = max(detected_language_info[language])

변경) 

                    if( not detected_language_info ):
                        language = "en"
                        language_probability = 1
                    else:
                        language = max(
                            detected_language_info,
                            key=lambda lang: len(detected_language_info[lang]),
                        )
                        language_probability = max(detected_language_info[language])
