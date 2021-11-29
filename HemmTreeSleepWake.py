from datetime import datetime, timedelta
import time
import paho.mqtt.publish as publish
import random

print("Turning the tree off at midnight and on at 7am...")

while True:
    dt=datetime.now() + timedelta(hours=1)
    dt=dt.replace(minute=0)
    while datetime.now()<dt:
        time.sleep(1)
    currHour=int(str(datetime.now().strftime("%H")))
    if(currHour==0):
        print("Turning Tree off...")
        publish.single("GUHemmTree","COLOR000000000", hostname="broker.mqttdashboard.com")
        publish.single("GUHemmTree","SLEEP", hostname="broker.mqttdashboard.com")
    if(currHour==7):
        print("Turning Tree on...")
        publish.single("GUHemmTree","AWAKE", hostname="broker.mqttdashboard.com")
        red=random.randint(0,255)
        green=random.randint(0,255)
        blue=random.randint(0,255)
        sendCmd="COLOR"+str(red).zfill(3)+str(green).zfill(3)+str(blue).zfill(3)
        publish.single("GUHemmTree",sendCmd, hostname="broker.mqttdashboard.com")
