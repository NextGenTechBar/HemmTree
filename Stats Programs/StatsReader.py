import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import csv
import time
import os.path as ospath

#####functions
def cell(text: str, num: int):
    if(num==1):
        return(text[0:find_nth(text,",",num)])
    else:
        if(find_nth(text,",",num)==-1): #last column
            return(text[find_nth(text,",",num-1)+1:])
        else:
            return(text[find_nth(text,",",num-1)+1:find_nth(text,",",num)])

def find_nth(haystack: str, needle: str, n: int) -> int:
    start = haystack.find(needle)
    while start >= 0 and n > 1:
        start = haystack.find(needle, start+len(needle))
        n -= 1
    return start

def countOnlineClients():
    macs=[]
    if(not (ospath.isfile('Connection Log.csv'))):
            print('Please put \'Connection Log.csv\' in this directory to count number of online clients')
            return("Unknown")
    with open('Connection Log.csv', newline='') as csvfile:
        spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
        for row in spamreader:
            #print(', '.join(row))
            rowValue=(', '.join(row))
            mac=cell(rowValue,4)
            if(not(mac in macs)):
                macs.append(mac)
    macs.pop(0)

    onlineStatus=[0]*len(macs)

    online_count=0
    with open('Connection Log.csv', newline='') as csvfile:
        spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
        for row in spamreader:
            #print(', '.join(row))
            rowValue=(', '.join(row))
            for i in range(len(macs)):
                if (macs[i] in rowValue):
                    if  ('BOOT' in rowValue) or ('RECONNECT' in rowValue) or ('PING' in rowValue):
                        onlineStatus[i]=1
                    else:
                        onlineStatus[i]=0
                
    for i in range(len(onlineStatus)):
        online_count+=onlineStatus[i]
    return(online_count)
#####

####this part is just an inefficient way to filter out duplicate commands
def countCommands():
    uniqueCommands=0
    lastCommand=""
    with open('HemmTreeStats.csv', newline='') as csvfile:
        spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
        for row in spamreader:
            rowValue=(', '.join(row))
            command=cell(rowValue,3)
            if(command!=lastCommand):
                uniqueCommands+=1
            lastCommand=command
    return(uniqueCommands)
####


fig = plt.figure()
while(True):
    data = pd.read_csv('HemmTreeStats.csv')


    user_counts = data.iloc[:,3].value_counts()

    uniqueUsers=len(user_counts)
    numCommands=len(data)
    uniqueCommands=str(countCommands())
    onlineClients=str(countOnlineClients())

    ax=fig.add_subplot(111)

    bin_edges = [0,10.1,20.1,30.1,40.1,50.1,60]
    ax.hist(np.clip(user_counts,bin_edges[0],bin_edges[-1]), #clip makes all values above the max bin edge to fall within the last bin as an overflow
    bins=bin_edges,edgecolor='black')
    plt.xlabel('Number of Commands Sent')
    plt.ylabel('Number of Users')
    plt.title('Number of commands sent per user')
    plt.xticks(range(0,51,5))

    ax.text(0.5,1.15,"Christmas Tree Stats",fontsize=14,ha='center',va='top',weight='bold',color='green',transform=ax.transAxes)
    ax.text(55,-7,">50",fontsize=10,ha='center')
    userNumText=str(uniqueUsers)+" Unique Users"
    ax.text(.7,.87,userNumText,fontsize=12,ha='center',transform=ax.transAxes)
    commandsText=str(uniqueCommands)+" Commands Sent"
    ax.text(.7,.8,commandsText,fontsize=12,ha='center',transform=ax.transAxes)
    clientText=onlineClients+" Clients online"
    ax.text(.7,.74,clientText,fontsize=10,ha='center',transform=ax.transAxes)

    plt.draw()
    plt.pause(60) #time interval to regenerate plot
    fig.clear()
    print("Updating chart...")


