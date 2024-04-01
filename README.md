# RealTrans ( English )

![](thumbnail.png?raw=true)

![](setting.png?raw=true)

![](summary.png?raw=true)

This is a program for real-time voice translation. Recognizes and translates the voice heard from the current sound output device. Create sentences from speech using the Whisper model, and translate the sentences using the NHNDQ/nllb-finetuned-en2ko model. It also has a function to summarize through API based on spoken sentences. The translation part also provides the ability to translate using the DeepL API.

- Support API : OpenAI API ( Summary ), DeepL ( real time translation )
- Support OS : Windows 10, 11 ~

### Install

Install Cuda Toolkit : https://developer.nvidia.com/cuda-downloads ( Window Version )

Install Python ( Require 3.10.x ) : https://www.python.org/downloads/ ( *** Be sure to select run Admin and add PATH. *** )

- Create a console window by running cmd in the window.
- In the console window, cd C:\installation folder
- Run python (when executed, exit by entering exit())

run batch file ( install python package )
> install.bat

- install.bat must be completed without error. When an error occurs, you must respond to the error message.
- Run python runTransWin.py. If the Translation processing with cuda message appears, it is normal.
- If you play something like YouTube and make audio come out, the message will be translated into English and Korean in the console window you run.
- If you have succeeded up to this point, run RealTrans.exe and it will run normally.

Final Windows program execution
> realtrans.exe

  - If execution is successful, you only need to run realtrans.exe from next time.

## Usage

run realtrans.exe ( windows program )

When you run the realtrans.exe file, the program will run.
After execution, it receives the sound heard from the current sound output device as input, creates an English sentence, and translates it into Korean.

  - Click the settings gear in the top left corner to select the voice language you are inputting and select the language you want to translate to.
  - When translating, the PC has the function of translating from its own computer to AI, and the API sends voice-recognized text through the API KEY of an external cloud service and receives and displays the translated text.
  - The summary sends the contents of the text window to OpenAI using the API KEY, receives the summarized results, and displays them.
    
( cf. At first, downloading the model may take a long time. )

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

Not tested in environments without Nvidia GPUs.

Nvidia GPU tested 3080.

Recommended graphics card: Nvidia 2080 or higher

	    Nvidia 1660 super setting ( change Whisper Model from large to small. runTransWin.py 390 line)

총 VRAM Usage : 5.4GB ( Require VRAM : minumum 6GB  )

Fast Whisper Large Model : 3GB VRAM

NHNDQ/nllb-finetuned-en2ko Model : 2.4GB VRAM

- If your VRAM is small, you can reduce usage by using Whisper small, base, and tiny models. (In this case, source modification is required)
  
   Small models may have lower recognition quality.

* When the program is executed, the running Python program is terminated. (To terminate remaining processes in case of abnormal termination)
* When used with games, the program may terminate due to insufficient VRAM or duplicate use of Cuda devices, or the system may become paralyzed, making control difficult.
   Please never use it with games.

## Terms of Use

The use of the source for the following purposes is prohibited.

* Any act that violates the MIT License and the laws of your country. 

## Disclaimer

I am not liable for any direct, indirect, consequential, incidental, or special damages arising out of or in any way connected with the use/misuse or inability to use this software.

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/realtrans)

--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# RealTrans ( 한국어 )

실시간 음성 번역을 위한 프로그램입니다. 현재의 소리 출력 장치에서 들리는 음성을 인식하고 번역을 합니다. Whisper 모델을 사용하여 음성에서 문장을 만들고, NHNDQ/nllb-finetuned-en2ko 모델을 사용하여 문장을 번역합니다. 음성 문장을 기반으로 API를 통해 요약하는 기능도 가지고 있습니다. 번역하는 부분은 DeepL API를 사용하여 번역을 할 수 있는 기능도 제공합니다.

- 지원 API : OpenAI API ( 요약 ), DeepL ( 실시간 번역 )
- 지원 OS : Windows 10, 11 ~

### 설치

Cuda Toolkit 설치 : https://developer.nvidia.com/cuda-downloads ( 윈도우 버젼을 설치하시면 됩니다. )

파이선 설치 ( https://wikidocs.net/8 - *** run Admin과 PATH 추가를 꼭 선택하세요. *** )

 - window에서 cmd를 실행하여 console창을 만듬
 - 콘솔창에서 cd C:\설치폴더
 - python 실행 ( 실행이 되면 exit()를 입력하여 빠져나옴 )

파이선 패키지 설치
>install.bat

 - install.bat이 오류 없이 완료가 되어야함. 오류 발생시 해당 오류 메시지에 대한 대처를 해야됩니다.
 - python runTransWin.py를 실행해봄 Translation processing with cuda 메시지가 나오면 정상
 - 유튜브 같은 것을 플레이해서 음성이 나오게 하면 실행한 콘솔창에서 영한번역이 되어서 메시지가 출력됩니다.
 - 여기까지 성공하셨으면 RealTrans.exe를 실행하면 정상적으로 수행되게됩니다.

최종 윈도우 프로그램 실행
>realtrans.exe 실행

 - 실행이 정상적으로 되면 다음부터는 realtrans.exe만 실행하면 됩니다. 

## 사용법
realtrans.exe 파일을 실행하면 프로그램이 실행됩니다.
실행후에는 현재의 소리 출력 장치에서 들리는 소리를 입력으로 받아, 영어 문장을 만들어 한국어로 번역을 합니다.

 - 왼쪽 제일 위에 톱니바퀴의 설정을 눌러서 입력하는 음성 언어를 선택하시고 번역할 언어를 선택하시면 됩니다.
 - 번역시에 PC는 자체 컴퓨터에서 AI로 번역하는 기능이고, API는 외부 클라우드 서비스의 API KEY를 통해 음성인식한 텍스트를 보내고 번역된 텍스트를 받아서 표시합니다.
 - 요약은 OpenAI에 API KEY로 텍스트 창의 내용을 보내서 요약한 결과를 받아서 표시합니다.

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

Nvidia GPU가 없는 경우 환경에서 테스트 되지 않았습니다.

Nvidia GPU는 3080을 사용하여 테스트 되었습니다. 

권장 그래픽카드 : Nvidia 2080 이상

                 Nvidia 1660 super일 경우 설정 ( Whisper Model을 large -> small로 변경해주세요. runTransWin.py 390 line )

총 VRAM 사용량 : 5.4GB ( 최소 6GB 이상의 VRAM이 필요합니다. )

openai Whisper Large Model : 3GB VRAM

NHNDQ/nllb-finetuned-en2ko Model : 2.4GB VRAM

- VRAM이 작은 경우에는  Whisper small, base, tiny 모델을 사용하여 사용량을 줄일 수 있습니다. ( 이 경우는 소스 수정 필요 )
  
  small 모델은 인식 품질이 떨어질 수 있습니다.

* 프로그램 실행시에 실행중인 Python 프로그램이 종료됩니다. ( 비정상 종료시에 남아있는 프로세스를 종료시키기 위하여 )
* 게임등과 같이 사용할 경우에 VRAM이 부족하거나 Cuda 장치 중복 사용으로 프로그램이 종료되거나 시스템이 마비상태가 되어 제어가 어려운 상황에 놓일 수 있습니다. 
  절대 게임과 같이 사용하지 말아주세요.

## 이용 약관

다음과 같은 목적으로 소스를 사용하는 것을 금지합니다.

* MIT License와 해당 국가의 법률에 위배되는 행위

## 부연 설명

이 소프트웨어의 사용으로 발생하는 직접적, 간접적, 결과적, 우발적 또는 특별한 손해에 대해 책임을 지지 않습니다.

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/realtrans)

