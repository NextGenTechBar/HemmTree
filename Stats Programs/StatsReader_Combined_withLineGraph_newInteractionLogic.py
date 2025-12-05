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
    # Use copy to prevent modifying the main dataframe
    data_local = data.copy() 
    
    # 1. Prepare data and sort (sorting ONLY by time is crucial for the new logic)
    data_local['datetime'] = pd.to_datetime(data_local.iloc[:, 0] + ' ' + data_local.iloc[:, 1])
    data_local = data_local.sort_values(by=['datetime']) 

    unique_interactions = 0
    # Dictionary to track the start time of the *current* interaction for each user
    user_interaction_start_times = {}

    for index, row in data_local.iterrows():
        user_id = row['UserID']
        current_time = row['datetime']
        
        # Get the start time of this user's currently active interaction, or None if new
        last_start_time = user_interaction_start_times.get(user_id)

        # Check for new interaction condition (New user OR 10-minute gap)
        if last_start_time is None or \
                current_time > last_start_time + timedelta(minutes=10):
            
            # Found a new, separate interaction
            unique_interactions += 1
            
            # Update the start time for this user (resets the 10-minute clock)
            user_interaction_start_times[user_id] = current_time

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

def plotInteractionsPerDay(data, ax):
    # Prepare data for daily interaction counting
    data_for_daily = data.copy()
    data_for_daily['datetime'] = pd.to_datetime(data_for_daily.iloc[:, 0] + ' ' + data_for_daily.iloc[:, 1])
    # IMPORTANT: Sort ONLY by datetime for the new logic
    data_for_daily = data_for_daily.sort_values(by=['datetime']) 

    daily_interactions = {}
    user_interaction_start_times = {} # New tracking dictionary

    for index, row in data_for_daily.iterrows():
        user_id = row['UserID']
        current_time = row['datetime']
        last_start_time = user_interaction_start_times.get(user_id)
        
        # Check for new interaction (10-minute window, PER USER)
        if last_start_time is None or \
                current_time > last_start_time + timedelta(minutes=10):
            
            # Count the interaction for the corresponding day
            date_str = row['datetime'].date().strftime('%Y-%m-%d')
            daily_interactions[date_str] = daily_interactions.get(date_str, 0) + 1
            
            # Update the interaction start time for this user
            user_interaction_start_times[user_id] = current_time

    # Convert dictionary to Series and sort by date
    daily_interactions_series = pd.Series(daily_interactions).sort_index()
    
    # Plotting
    dates = daily_interactions_series.index
    counts = daily_interactions_series.values
    
    ax.plot(dates, counts, marker='o', linestyle='-', color='red')
    
    # Set Y-axis to start at 0
    ax.set_ylim(bottom=0)
    
    # X-axis label removed
    ax.set_ylabel('Total Interactions')
    ax.set_title('Total Number of Interactions Per Day')
    ax.tick_params(axis='x', rotation=45)
    ax.grid(True, linestyle='--', alpha=0.6)
    
    # Control X-axis ticks to avoid crowding
    if len(dates) > 0:
        if len(dates) > 14:
            # If more than 14 days, sample ticks every 3rd day
            step = 3
            tick_indices = np.arange(0, len(dates), step)
            ax.set_xticks(np.array(dates)[tick_indices])
        else:
            # If 14 days or less, show all dates
            ax.set_xticks(dates)
        

# PROGRAM START

# --- FIGURE AND AXES SETUP ---
# Create a figure
fig = plt.figure(figsize=(12, 12)) 
fig.subplots_adjust(top=0.92, hspace=0.6) 
fig.set_facecolor('#f6f0e3')

# Define a 2-row by 2-column GridSpec with custom height ratios (Top:Bottom = 2:1)
gs = fig.add_gridspec(2, 2, height_ratios=[2, 1]) 

# Map the axes:
ax1 = fig.add_subplot(gs[0, 0]) 
ax2 = fig.add_subplot(gs[0, 1]) 
ax3 = fig.add_subplot(gs[1, :]) 
# -----------------------------


while True:
    data = pd.read_csv('HemmTreeStats.csv',header=None)
    data.columns = ['Date', 'Time', 'Command', 'UserID'] # Give columns names for clarity

    # --- CALCULATE GLOBAL STATS ---
    user_counts = data.iloc[:, 3].value_counts()
    uniqueUsers = len(user_counts)
    uniqueCommands = str(countCommands())
    onlineClients = str(countOnlineClients())
    
    # Data prep for interaction count and subsequent plotting
    data['datetime'] = pd.to_datetime(data.iloc[:, 0] + ' ' + data.iloc[:, 1])
    # REVISED: Sort ONLY by time for new interaction logic
    data = data.sort_values(by=['datetime']) 
    unique_interactions_total = countUniqueInteractions(data)

    # --- FIGURE HEADER TEXT (Centralized & Stacked) ---
    
    # 1. Main Title
    fig.text(0.52, 0.97, "Christmas Tree Stats", fontsize=20, ha='center', va='top',
             weight='bold', color='green', transform=fig.transFigure)
             
    # 2. Unique Users (First stat line, centered)
    userNumText = f"{uniqueUsers} Unique Users"
    fig.text(0.52, 0.93, userNumText, fontsize=12, ha='center', va='top',
             transform=fig.transFigure, color='blue')

    # 3. Clients Online (Second stat line, centered, providing vertical gap)
    clientText = f"{onlineClients} Clients online"
    fig.text(0.52, 0.90, clientText, fontsize=10, ha='center', va='top',
             transform=fig.transFigure, color='blue')
    # ------------------------------------------

    # --- SUBPLOT 1: Histogram for Number of Commands Sent ---
    ax1.clear()
    
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

    # Total Commands Sent (Placed inside the plot area)
    commandsText = str(uniqueCommands) + " Total Commands Sent"
    ax1.text(0.75, 0.95, commandsText, fontsize=12, ha='right', va='top', transform=ax1.transAxes, color='blue')
    
    # --- SUBPLOT 2: Histogram for Number of Interactions per user ---
    ax2.clear()

    # Calculate per-user interaction counts (REVISED LOGIC)
    interactions_count = {}
    user_interaction_start_times = {} 

    # data is already sorted by datetime from the global prep step above
    
    for index, row in data.iterrows():
        user_id = row['UserID']
        current_time = row['datetime']
        last_start_time = user_interaction_start_times.get(user_id)
        
        if last_start_time is None or \
                current_time > last_start_time + timedelta(minutes=10):
            
            # Found a new, separate interaction
            
            # Increment the interaction count for this user
            interactions_count[user_id] = interactions_count.get(user_id, 0) + 1
            
            # Update the start time for this user
            user_interaction_start_times[user_id] = current_time


    bin_edges_interactions = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21]

    user_counts_interactions = list(interactions_count.values())
    user_counts_interactions = np.clip(user_counts_interactions, 0, 20)

    ax2.hist(user_counts_interactions, bins=bin_edges_interactions, edgecolor='black', align='left') 

    ax2.set_xlabel('Number of Interactions')
    ax2.set_ylabel('Number of Users')
    ax2.set_title('Number of interactions per user')

    ax2.text(.94, -0.055, ">20", fontsize=10, ha='center', transform=ax2.transAxes)
    
    # Total Interactions (Placed inside the plot area)
    interactionsText = str(unique_interactions_total) + " Total Interactions"
    ax2.text(0.55, 0.95, interactionsText, fontsize=12, ha='left', va='top', transform=ax2.transAxes, color='blue')
    ax2.set_xticks(range(1, 21, 2))

    # --- SUBPLOT 3: Line Graph for Total Interactions Per Day ---
    ax3.clear()
    plotInteractionsPerDay(data, ax3)

    # --- FIX: Increase top margin by setting rect=[0, 0, 1, 0.90] ---
    plt.tight_layout(rect=[0, 0, 1, 0.90]) 

    plt.draw()
    plt.pause(60)
    
    if not plt.fignum_exists(fig.number):
        plt.close()
        break
    
    print("Updating charts...")
