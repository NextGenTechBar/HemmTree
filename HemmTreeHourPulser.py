from datetime import datetime, timedelta
import time
import paho.mqtt.publish as publish

print("will now pulse the time at the top of each hour...")

while True:
    dt=datetime.now() + timedelta(hours=1)
    dt=dt.replace(minute=0)
    while datetime.now()<dt:
        time.sleep(1)
    hour24=int(str(datetime.now().strftime("%H")))
    pulseNum=int(str(datetime.now().strftime("%H")))%12
    if(hour24==12): #if it's noon, pulse 12 times, not 0. if it's midnight, don't pulse at all cause we're turning off the tree simultaniously
        pulseNum=12
    pulseCmd="PULSE"+str(int(str(datetime.now().strftime("%H")))%12)
    publish.single("GUHemmTree",pulseCmd, hostname="broker.mqttdashboard.com")
    print("Pulsing "+str(pulseNum)+":00")