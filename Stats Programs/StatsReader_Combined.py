import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import csv
from datetime import datetime, timedelta
import os.path as ospath

##### Functions #####
def cell(text: str, num: int):
    if num == 1:
        return text[0:find_nth(text, ",", num)]
    else:
        if find_nth(text, ",", num) == -1:  # last column
            return text[find_nth(text, ",", num - 1) + 1:]
        else:
            return text[find_nth(text, ",", num - 1) + 1:find_nth(text, ",", num)]

def find_nth(haystack: str, needle: str, n: int) -> int:
    start = haystack.find(needle)
    while start >= 0 and n > 1:
        start = haystack.find(needle, start + len(needle))
        n -= 1
    return start

def countOnlineClients():
    macs = []
    if not (ospath.isfile('Connection Log.csv')):
        print('Please put \'Connection Log.csv\' in this directory to count the number of online clients')
        return "Unknown"
    with open('Connection Log.csv', newline='') as csvfile:
        spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
        for row in spamreader:
            rowValue = (', '.join(row))
            mac = cell(rowValue, 4)
            if not (mac in macs):
                macs.append(mac)
    macs.pop(0)

    onlineStatus = [0] * len(macs)

    online_count = 0
    with open('Connection Log.csv', newline='') as csvfile:
        spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
        for row in spamreader:
            rowValue = (', '.join(row))
            for i in range(len(macs)):
                if macs[i] in rowValue:
                    if 'BOOT' in rowValue or 'RECONNECT' in rowValue or 'PING' in rowValue:
                        onlineStatus[i] = 1
                    else:
                        onlineStatus[i] = 0

    for i in range(len(onlineStatus)):
        online_count += onlineStatus[i]
    return online_count

def countUniqueInteractions(data):
    data['datetime'] = pd.to_datetime(data.iloc[:, 0] + ' ' + data.iloc[:, 1])
    data = data.sort_values(by=[3, 'datetime'])

    unique_interactions = 0
    current_user = None
    current_interaction_start = None

    for index, row in data.iterrows():
        if current_user is None or current_user != row[3] or \
                row['datetime'] > current_interaction_start + timedelta(minutes=10):
            # Start a new interaction
            current_user = row[3]
            current_interaction_start = row['datetime']
            unique_interactions += 1

    return unique_interactions

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


# PROGRAM 1
fig, axs = plt.subplots(1, 2, figsize=(12, 6))
fig.subplots_adjust(top=0.82)
fig.set_facecolor('#f6f0e3')

# Subplot 1: Histogram for Number of Commands Sent
ax1 = axs[0]

while True:
    data = pd.read_csv('HemmTreeStats.csv',header=None)

    user_counts = data.iloc[:, 3].value_counts()

    uniqueUsers = len(user_counts)
    numCommands = len(data)
    uniqueCommands = str(countCommands())
    onlineClients = str(countOnlineClients())

    bin_edges = [0, 10.1, 20.1, 30.1, 40.1, 50.1, 60]

    ax1.hist(
        np.clip(user_counts, bin_edges[0], bin_edges[-1]),
        bins=bin_edges,
        edgecolor='black'
    )
    ax1.set_xlabel('Number of Commands Sent')
    ax1.set_ylabel('Number of Users')
    ax1.set_title('Number of commands sent per user')
    ax1.set_xticks(range(0, 51, 5))

    ax1.text(
        1.1, 1.24,
        "Christmas Tree Stats", fontsize=20, ha='center', va='top',
        weight='bold', color='green', transform=ax1.transAxes
    )
    ax1.text(55, -7, ">50", fontsize=10, ha='center')
    userNumText = str(uniqueUsers) + " Unique Users"
    ax1.text(1.1, 1.13, userNumText, fontsize=12, ha='center', transform=ax1.transAxes,color='blue')
    commandsText = str(uniqueCommands) + " Total Commands Sent"
    ax1.text(.6, .85, commandsText, fontsize=12, ha='center', transform=ax1.transAxes,color='blue')
    clientText = onlineClients + " Clients online"
    ax1.text(1.1, 1.07, clientText, fontsize=10, ha='center', transform=ax1.transAxes,color='blue')

    plt.draw()
    #plt.pause(5)
    #ax1.clear()
    print("Updating chart...")

# PROGRAM 2
    ax2 = axs[1]

#while True:
    #data = pd.read_csv('HemmTreeStats.csv', header=None)

    unique_interactions = countUniqueInteractions(data)

    interactions_count = {}

    current_user = None
    current_interaction_start = None

    for index, row in data.iterrows():
        if current_user is None or current_user != row[3] or \
                row['datetime'] > current_interaction_start + timedelta(minutes=10):
            current_user = row[3]
            current_interaction_start = row['datetime']
            interactions_count[current_user] = interactions_count.get(current_user, 0) + 1

    uniqueUsers = len(data.iloc[:, 3].unique())
    numCommands = len(data)
    onlineClients = str(countOnlineClients())

    bin_edges = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21]

    user_counts = list(interactions_count.values())
    user_counts = np.clip(user_counts, 0, 20)

    ax2.hist(user_counts, bins=bin_edges, edgecolor='black', align='left')

    ax2.set_xlabel('Number of Interactions')
    ax2.set_ylabel('Number of Users')
    ax2.set_title('Number of interactions per user')

    ax2.text(.94, -0.055, ">20", fontsize=10, ha='center', transform=ax2.transAxes)
    interactionsText = str(unique_interactions) + " Total Interactions"
    ax2.text(.6, .85, interactionsText, fontsize=12, ha='center', transform=ax2.transAxes,color='blue')
    ax2.set_xticks(range(1, 20, 1))

    plt.draw()
    plt.pause(60)
    if not plt.fignum_exists(fig.number): #if we got here cause user closed the program
        plt.close()
        break
    ax2.clear()
    ax1.clear()
    print("Updating charts...")
