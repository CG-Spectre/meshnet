import serial
import time
import sys
import threading
import webview

def input_thread():
    while True:
        query = input()
        module.write(f"{query.strip()}$".encode('utf-8'))

id = sys.argv[1]
port = sys.argv[2]
print(id, port)

module = serial.Serial(port, 115200, timeout=1)
time.sleep(2)
print("Connected")
module.write(f"ID {id}$".encode('utf-8'))
print("Changed ID")
threading.Thread(target=input_thread, args=()).start()

window = webview.create_window("Test", html="test")

def background():

    while(True):
        #if msvcrt.kbhit():
         #   print(msvcrt.getch().decode())
        line = module.readline().decode('utf-8', errors='ignore').strip()
        if(line.startswith("HANDLE")):
            line = line.split("HANDLE ", 1)[1]
            if(line.startswith("GET")):
                with open("index.html") as f:
                    module.write(f"{f.read()}$".encode('utf-8'))
                    module.flush()
        if(line.startswith("RESPR")):
            line = line[5:]
            window.load_html(line)
        elif(line.strip() != ""):
            print(line.strip())

def start_background():
    threading.Thread(target=background).start()

webview.start(start_background)