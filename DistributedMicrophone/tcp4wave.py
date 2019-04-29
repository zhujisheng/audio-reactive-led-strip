#! /usr/bin/python3

import sys
import time
import getopt
import socket

MicIP = ''
MicPort = 3344
PlayAudio = False
WaveFilename = None

def usage():
    print("Usage: %s <MicIP> [-p <MicPort>] [-f <file name>] [--play] [-h]"%(sys.argv[0]))
    sys.exit(2)

def SocketInit():
    global sock, Connected
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5)
    Connected = False

try:
    MicIP = sys.argv[1]
    if MicIP.startswith('-'):
        usage()
    opts,args = getopt.getopt(sys.argv[2:],'hp:f:',['play'])
except (getopt.GetoptError, IndexError):
    usage()

for opt_name,opt_value in opts:
    if opt_name=='-p':
        MicPort = int(opt_value)
    if opt_name=='-f':
        WaveFilename = opt_value
    if opt_name=='--play':
        PlayAudio = True
    if opt_name=='-h':
        usage()

if WaveFilename:
    import wave
    wavfile = wave.open(WaveFilename, 'wb')
    wavfile.setparams((1, 2, 16000, 0, 'NONE', 'NONE'))

if PlayAudio:
    import pyaudio
    p = pyaudio.PyAudio()
    stream = p.open(
        format =  pyaudio.paInt16,
        channels = 1,
        rate = 16000,
        output = True
        )

SocketInit()
while True:
    try:
        if not Connected:
            sock.connect((MicIP, MicPort))
            Connected = True;
            print('Connecting to {}:{}'.format(MicIP, MicPort))

        recvData = sock.recv(1024)
        if WaveFilename:
            wavfile.writeframes(recvData)
        if PlayAudio:
            stream.write(recvData)

        if len(recvData)==0:
            sock.close()
            SocketInit()
            print('Receive none from {}:{}, Disconnect it.'.format(MicIP, MicPort))
            continue

    except socket.timeout:
        print('Timeout. Reconnecting ...')
        sock.close()
        SocketInit()
        continue
    except (socket.error, OSError):
        print('Connection failed. Reconnecting after 5s ...')
        sock.close()
        time.sleep(5)
        SocketInit()
        continue
    except KeyboardInterrupt:
        sock.close()
        if WaveFilename:
            wavfile.close()
        if PlayAudio:
            stream.stop_stream()
            stream.close()
            p.terminate()
        break;
