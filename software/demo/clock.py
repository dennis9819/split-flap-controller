#!/usr/bin/env python
from websockets.sync.client import connect
import json
import datetime
import time
endpoint = "ws://127.0.0.1:8087/manage/"

def printStr(endpoint, x,y, string):
    with connect(endpoint) as websocket:
        cmd = {
            'command': 'dm_print',
            'full': False,
            'x': x,
            'y': y,
            'string': string
        }
        websocket.send(json.dumps(cmd))
        websocket.close()

def printDate():
    x = datetime.datetime.now()
    printStr(endpoint,0,0,x.strftime("%d.%m.%y"))

def printTime():
    x = datetime.datetime.now()
    printStr(endpoint,0,0,x.strftime("%H:%M:%S"))

    


while True:
    printStr(endpoint,0,0," Es ist ")
    time.sleep(8)
    printDate()
    time.sleep(8)
    for x in range(30):
        printTime()
        time.sleep(1)
    printStr(endpoint,0,0," Frohes ")
    time.sleep(5)
    printStr(endpoint,0,0," Neues! ")
    time.sleep(8)