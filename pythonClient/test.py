import serial
import time
import sys
import threading

def input_thread():
    while True:
        query = input()
        module.write(query.strip().encode('utf-8'))

id = sys.argv[1]
port = sys.argv[2]
print(id, port)

module = serial.Serial(port, 115200, timeout=1)
time.sleep(2)
print("Connected")
module.write(f"ID {id}".encode('utf-8'))
print("Changed ID")
threading.Thread(target=input_thread, args=()).start()
while(True):
    #if msvcrt.kbhit():
     #   print(msvcrt.getch().decode())
    line = module.readline().decode('utf-8', errors='ignore').strip()
    if(line.startswith("HANDLE")):
        line = line.split("HANDLE ", 1)[1]
        if(line.startswith("GET")):
            module.write("Nigger".encode('utf-8'))
            module.flush()
    elif(line.strip() != ""):
        print(line.strip())