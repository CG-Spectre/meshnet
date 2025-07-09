from mesh_module import MeshModule
import time
from browser import Browser
import threading

def secondary_thread(b):
    time.sleep(5)
    #b.load("https://google.com")

if(__name__ == "__main__"):
    mm = MeshModule()
    while(not mm.connected):
        print("Waiting for connection...")
        time.sleep(500)
    print("Connected.")
    b = Browser()
    t = threading.Thread(target=secondary_thread, args=(b,))
    t.start()
    b.start(mm)
    #time.sleep(2)
    #b.load("https://google.com")
    #b.load("https://example.com")

