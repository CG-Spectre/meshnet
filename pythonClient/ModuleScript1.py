import sys
import time

from mesh_module import MeshModule

if(__name__ == "__main__"):
    id = sys.argv[1]
    port = sys.argv[2]
    mm = MeshModule(baud=115200, port=port, id=id)
    while not mm.connected:
        continue
    print("Connected.")
    time.sleep(1)
    mm.mmGetRequest("mnp://1")