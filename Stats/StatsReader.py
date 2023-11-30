#This program reads in a stats file and counts only the non-repeating values. For example, (red,green,green,green,red) would count as 3 instead of 5
#in the future, it would be good to output a second stats csv that only contains the non-repeated values

#it also prints a the number of unique users, and a csv-formatted list with a row for each user, and a column for total commands and "useful" commands (non-repeating ones)

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

print("UserID,total commands,useful commands")
for i in range(len(userIDs)):
    print(str(userIDs[i])+","+str(commandsPerUser[i])+","+str(commandsPerUserRemovedDuplicates[i]))
