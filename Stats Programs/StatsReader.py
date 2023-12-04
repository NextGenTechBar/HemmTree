import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import csv
import time

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


    plt.draw()
    #time.sleep(10) #regenerate plot every x mins
    plt.pause(60)
    fig.clear()
    print("Updating chart...")







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
