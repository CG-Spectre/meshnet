import serial
import time
import threading

def listenFunc(mm):
    while(True):
        line = mm.module.readline().decode('utf-8', errors='ignore')
        print(line.strip())


class MeshModule:
    connected = False
    def __init__(self, port="COM4", baud=115200, id=None):
        self.module = serial.Serial(port, baud, timeout=1)
        time.sleep(2)
        self.connected = True
        if(id):
            self.module.write(id.encode('utf-8'))
        self.listenThread = threading.Thread(target=listenFunc, args=(self,))
        self.listenThread.start()
    def mmGetRequest(self, url):
        if(url.startswith("mnp://")):
            url = url.split("://", 1)[1].rstrip("/")
            payload = f"GET {url}\n"
            self.module.write(payload.encode('utf-8'))
    def getRequest(self, url):
        if(not self.connected):
            return False
        relay = f"GET {url.replace('https://', '')}"
        if(relay.endswith("/")):
            relay = relay[:-1]
        self.module.write(relay.encode('utf-8'))
        response = ""
        line = ""
        end = False
        start = False
        while(not end):
            line = self.module.readline().decode('utf-8')
            if line.startswith("!!&&END&&!!"):
                end = True
                break
            if line.startswith("!!&&START&&!!"):
                start = True
                continue
            if not start:
                continue
            response += line
        return response

