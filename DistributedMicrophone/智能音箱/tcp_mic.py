import time
import socket
from speech_recognition import AudioSource

class TcpAudio(AudioSource):
    def __init__(self, MicIP, MicPort):
        self._MicIP = MicIP
        self._MicPort = MicPort
        self.SAMPLE_WIDTH = 2
        self.SAMPLE_RATE = 16000
        self.CHUNK = 1024
        self.stream = None

    def __enter__(self):
        self.stream = TcpAudio.AudioTcpStream(self._MicIP, self._MicPort, self.SAMPLE_WIDTH)
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.stream.close()
        self.stream = None

    class AudioTcpStream(object):
        def __init__(self, MicIP, MicPort, SampleWidth):
            self._MicIP = MicIP
            self._MicPort = MicPort
            self._SampleWidth = SampleWidth
            self._socket = None
            self._connected = False
            self.SocketInit()

        def SocketInit(self):
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._socket.settimeout(3)
            self._connected = False

        def read(self, size):
            try:
                if not self._connected:
                    self._socket.connect((self._MicIP,self._MicPort))
                    self._connected = True

                toread = size*self._SampleWidth
                buffer = bytearray(toread)
                view = memoryview(buffer)
                while toread:
                    nbytes = self._socket.recv_into(view, toread)
                    if nbytes==0:
                        self._socket.close()
                        self.SocketInit()
                        #buffer = b'\x00'*size
                        break
                    view = view[nbytes:]
                    toread -= nbytes
            except (socket.timeout, socket.error, OSError):
                self._socket.close()
                time.sleep(5)
                self.SocketInit()
                buffer = b'\x00'*size

            return buffer

        def close(self):
            self._socket.close()