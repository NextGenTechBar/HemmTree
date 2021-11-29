import paho.mqtt.subscribe as subscribe
from datetime import date
from datetime import datetime

filename='HemmTreeStats.csv'
print("Will now log all incoming commands to "+filename)

def on_message_print(client, userdata, message):
    msg=message.payload.decode("utf-8")
    print(str(date.today())+" "+str(datetime.now().strftime("%H:%M"))+" - "+str(msg))
    with open(filename,"a") as myfile:
        myfile.write("\n")
        myfile.write(str(date.today()))
        myfile.write(",")
        myfile.write(str(datetime.now().strftime("%H:%M")))
        myfile.write(",")
        myfile.write(msg)
        
subscribe.callback(on_message_print, "GUHemmTreeStats", hostname="broker.mqttdashboard.com")
