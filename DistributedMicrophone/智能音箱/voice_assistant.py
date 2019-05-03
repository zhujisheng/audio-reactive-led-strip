#!/usr/bin/env python3

import speech_recognition as sr
from ha_cli import ha_cli
from ais_cli import tuling123
import logging
import threading
logging.basicConfig(format='[%(levelname)s] %(asctime)s %(message)s', level=logging.INFO)

def RemoteMicCB(stage=1):
    if stage==0:
        ha.call_service('light', 'turn_off', data={"entity_id": "light.miclight"})
    elif stage==1:
        ha.call_service('light', 'turn_on', data={"entity_id": "light.miclight", "effect":"Strobe"})
    elif stage==2:
        ha.call_service('light', 'turn_on', data={"entity_id": "light.miclight", "brightness": 255, "effect":"None"})

REMOTE_MIC_CONFIG = [('192.168.3.17',3344, RemoteMicCB)]
LOCAL_MIC_CONFIG = True

HA_URL = 'http://localhost:8123'
HA_TOKEN = 'eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpYXQiOjE1NTA1ODY4MDIsImlzcyI6IjRlODRjNjJiYTI3NzQxNzA5ZjgyYWMzYWYzMjgzNzJmIiwiZXhwIjoxODY1OTQ2ODAyfQ.YK5X47ucA5Ug2NlRNa0VE7Z6bZruwthIdPzFAVT7eYA'
TTS_SERVICE_NAME = 'baidu_say'
MEDIA_PLAYER_NAME = 'all'

TULING_USER_ID = '407535'
TULING_API_KEY = '0bee51f9bca64295a5c7f34b2eeb81ed'

SNOWBOY_LOCATION = '/home/pi/voice_assistant/sb/'
SNOWBOY_MODELS = ['/home/pi/voice_assistant/sb/models/snowboy.umdl']
SNOWBOY_CONFIG = (SNOWBOY_LOCATION, SNOWBOY_MODELS)
import sys
sys.path.append(SNOWBOY_LOCATION)
import snowboydecoder
sys.path.pop()

def LocalMicProcess():
    with sr.Microphone(sample_rate=16000) as source:
        while True:
            try:
                logging.info("(LocalMicrophone)开始监听……")
                audio = r.listen(source,
                                 phrase_time_limit=6,
                                 snowboy_configuration=SNOWBOY_CONFIG,
                                 hot_word_callback=snowboydecoder.play_audio_file
                                 )
                logging.info("(LocalMicrophone)开始识别……")
                snowboydecoder.play_audio_file(fname=SNOWBOY_LOCATION+'resources/dong.wav')

                recognized_text = r.recognize_google_cn(audio, language='zh-CN')
                # 你也可以选择使用微软的语音识别服务
                #recognized_text = r.recognize_azure(audio, language='zh-CN', key='7a393a3b7954490dab750a490b264f27', location='westus')
                #import re
                #recognized_text = re.sub(r'[^\w\s]', '', recognized_text)
            except sr.UnknownValueError:
                recognized_text = ''
            except Exception as e:
                logging.warning("(LocalMicrophone)识别错误：{0}".format(e))
                continue
            logging.info("(LocalMicrophone)识别结果：" + recognized_text)

            try:
                speech = ha.process(recognized_text)
                if speech == "Sorry, I didn't understand that":
                    speech = tuling.command(recognized_text)
                    ha.note(message=recognized_text)
                ha.speak(speech, tts=TTS_SERVICE_NAME, media_player=MEDIA_PLAYER_NAME)
                logging.info("(LocalMicrophone)语音返回结果：" + speech)
            except Exception as e:
                logging.error("(LocalMicrophone)与HomeAssistant通讯失败：{0}".format(e))
                continue

def RemoteMicProcess(MicIP, MicPort, StageCallback):
    with TcpAudio(MicIP, MicPort) as source:
        logging.info("({0})加载远程麦克风...".format(MicIP))
        while True:
            try:
                StageCallback(0)
                logging.info("({0})开始监听……".format(MicIP))
                audio = r.listen(source,
                                 phrase_time_limit=6,
                                 snowboy_configuration=SNOWBOY_CONFIG,
                                 hot_word_callback=StageCallback,
                                 min_time_after_hot_word=3
                                )
                StageCallback(2)
                logging.info("({0})开始识别……".format(MicIP))

                recognized_text = r.recognize_google_cn(audio, language='zh-CN')
            except sr.UnknownValueError:
                recognized_text = ''
            except Exception as e:
                logging.warning("({0})识别错误：{1}".format(MicIP,e))
                continue
            logging.info("({0})识别结果：".format(MicIP) + recognized_text)

            try:
                speech = ha.process(recognized_text)
                if speech == "Sorry, I didn't understand that":
                    speech = tuling.command(recognized_text)
                    ha.note(message=recognized_text)

                ha.speak(speech, tts=TTS_SERVICE_NAME, media_player=MEDIA_PLAYER_NAME)
                logging.info("({0})语音返回结果：".format(MicIP) + speech)
            except Exception as e:
                logging.error("({0})与HomeAssistant通讯失败：{1}".format(MicIP,e))
                continue

r = sr.Recognizer()
ha = ha_cli(token=HA_TOKEN, base_url=HA_URL)
tuling = tuling123(user_id=TULING_USER_ID, api_key=TULING_API_KEY)

if REMOTE_MIC_CONFIG:
    from tcp_mic import TcpAudio
    for config in REMOTE_MIC_CONFIG:
        handler = threading.Thread(target=RemoteMicProcess, args=config)
        handler.start()

if LOCAL_MIC_CONFIG:
    LocalMicProcess()
