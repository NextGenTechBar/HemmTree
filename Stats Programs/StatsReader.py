#This program will parse your stats file and only count commands that are unique from the previous command (eg: red,green,green,green,red gets counted as 3 instead of 5)
#In the future, it would be nice for the output to be another csv that discards the unwanted values

#Additionally, it tells the number of unique users and outputs a csv-formatted print with a row for each user and columns for total commands and "useful" commands which is without duplicates.

import csv

def cell(text: str, num: int):
    return(text[find_nth(text,",",num-1)+1:find_nth(text,",",num)])

def find_nth(haystack: str, needle: str, n: int) -> int:
    start = haystack.find(needle)
    while start >= 0 and n > 1:
        start = haystack.find(needle, start+len(needle))
        n -= 1
    return start

lastCommand=""
commandCtr=0;

with open('HemmTreeStats.csv', newline='') as csvfile:
    spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
    for row in spamreader:
        rowValue=(', '.join(row))
        command=cell(rowValue,3)
        if(command!=lastCommand):
            commandCtr+=1
        lastCommand=command

print("Non-duplicate commands: "+str(commandCtr))


userIDs=[]
#populate list with unique users
with open('HemmTreeStats.csv', newline='') as csvfile:
    spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
    for row in spamreader:
        rowValue=(', '.join(row))
        user=cell(rowValue,4)
        if(not(user in userIDs)):
            userIDs.append(user)

print("Unique Users: "+str(len(userIDs)))

commandsPerUser = [0]*len(userIDs)
commandsPerUserRemovedDuplicates = [0]*len(userIDs)

for i in range(len(userIDs)):
    with open('HemmTreeStats.csv', newline='') as csvfile:
        spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
        for row in spamreader:
            rowValue=(', '.join(row))
            user=cell(rowValue,4)
            command=cell(rowValue,3)
            if(user==userIDs[i]):
                commandsPerUser[i]+=1
                if(command!=lastCommand):
                    commandsPerUserRemovedDuplicates[i]+=1
            lastCommand=command

for i in range(len(userIDs)):
    print(str(userIDs[i])+","+str(commandsPerUser[i])+","+str(commandsPerUserRemovedDuplicates[i]))
