import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import csv
from datetime import datetime, timedelta
import os.path as ospath

def testUniqueInteractions(data):
    data['datetime'] = pd.to_datetime(data.iloc[:, 0] + ' ' + data.iloc[:, 1])
    data = data.sort_values(by=[3, 'datetime'])

    current_user = None
    current_interaction_start = None
    interactions_count = {}

    for index, row in data.iterrows():
        if current_user is None or current_user != row[3] or \
                row['datetime'] > current_interaction_start + timedelta(minutes=10):
            # Start a new interaction
            if current_user is not None:
                interactions_count[current_user] = interactions_count.get(current_user, 0) + 1
                print(f"User {current_user}: Interaction started at {current_interaction_start}, "
                      f"Number of different interactions: {interactions_count[current_user]}")

            current_user = row[3]
            current_interaction_start = row['datetime']

    # Print the details for the last interaction
    if current_user is not None:
        interactions_count[current_user] = interactions_count.get(current_user, 0) + 1
        print(f"User {current_user}: Interaction started at {current_interaction_start}, "
              f"Number of different interactions: {interactions_count[current_user]}")

    # Print the total number of interactions for each user
    for user, count in interactions_count.items():
        print(f"User {user}: Total number of interactions: {count}")





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


##### Main Loop #####
fig = plt.figure()
while True:
    data = pd.read_csv('HemmTreeStats.csv', header=None)
    #data = pd.read_csv('HemmTreeStats_test.csv', header=None)

    unique_interactions = countUniqueInteractions(data)
    
    # Add this part to count interactions in the main loop
    #interactions_count = {}  # Initialize or clear the interactions_count dictionary
    #for index, row in data.iterrows():
    #    interactions_count[row[3]] = interactions_count.get(row[3], 0) + 1

    #replacing above code, this part considers different interactions depending on the time
    interactions_count = {}  # Initialize or clear the interactions_count dictionary

    current_user = None
    current_interaction_start = None

    for index, row in data.iterrows():
        if current_user is None or current_user != row[3] or \
                row['datetime'] > current_interaction_start + timedelta(minutes=10):
            # Start a new interaction
            current_user = row[3]
            current_interaction_start = row['datetime']
            interactions_count[current_user] = interactions_count.get(current_user, 0) + 1
    

    uniqueUsers = len(data.iloc[:, 3].unique())
    numCommands = len(data)
    onlineClients = str(countOnlineClients())

    ax = fig.add_subplot(111)

    bin_edges = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21]  # Adjust as needed
 
    user_counts = list(interactions_count.values())
    print("number of interactions for users over 20 interactions")
    for i in range(len(user_counts)):
        if(user_counts[i]>20):
            print(user_counts[i])
    user_counts = np.clip(user_counts,0,20) #set anything above the last paramater equal to the last paramater
    #print(user_counts)
    #bin_edges = np.arange(min(user_counts) - 0.5, max(user_counts) + 1.5, 1)
    plt.hist(user_counts, bins=bin_edges, edgecolor='black', align='left')


    plt.xlabel('Number of Interactions')
    plt.ylabel('Number of Users')

    ax.text(0.5, 1.15, "Christmas Tree Stats", fontsize=14, ha='center', va='top', weight='bold', color='green',
            transform=ax.transAxes)
    ax.text(.94, -.055, ">20", fontsize=10, ha='center',transform=ax.transAxes)
    userNumText = str(uniqueUsers) + " Unique Users"
    ax.text(.7, .87, userNumText, fontsize=12, ha='center', transform=ax.transAxes)
    interactionsText = str(unique_interactions) + " Interactions"
    ax.text(.7, .8, interactionsText, fontsize=12, ha='center', transform=ax.transAxes)
    clientText = onlineClients + " Clients online"
    ax.text(.7, .74, clientText, fontsize=10, ha='center', transform=ax.transAxes)
    plt.xticks(range(1,20,1))

    plt.draw()
    #testUniqueInteractions(data)
    plt.pause(60)  # time interval to regenerate the plot
    fig.clear()
    print("Updating chart...")

