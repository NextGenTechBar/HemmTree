#Feed this program the HemmTreeMacAddresses.csv list and the output of running ConnectionLog.py for awhile.
#Output: the full list of MACs in the Mac Addresses file, with ** next to the ones online, and the last message heard from them.
#If you have not been running ConnectionLog.py for awhile, you can send "WHOSTHERE" to the MQTT channel "GUHemmTree" while running the python program to get a record of who's online right now

import csv

macs = []
people = []

##functions##
def cell(text: str, num: int):
    if(num==1):
        return(text[0:find_nth(text,",",num)])
    else:
        return(text[find_nth(text,",",num-1)+1:find_nth(text,",",num)])

def find_nth(haystack: str, needle: str, n: int) -> int:
    start = haystack.find(needle)
    while start >= 0 and n > 1:
        start = haystack.find(needle, start+len(needle))
        n -= 1
    return start
##/functions##

#populate list of macs and people
with open('HemmTreeMacAddresses.csv', newline='') as csvfile:
    spamreader = csv.reader(csvfile, delimiter=',', quotechar='|')
    for row in spamreader:
        #print(', '.join(row))
        rowValue=(', '.join(row))
        if(not (cell(rowValue,1)=="MAC")): #ignore header row
            mac=cell(rowValue,1)
            name=cell(rowValue,2)
            macs.append(mac)
            people.append(name)

lastSeen = ["Never"] * len(people)


with open('Connection Log.csv', newline='') as csvfile:
    spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
    for row in spamreader:
        #print(', '.join(row))
        rowValue=(', '.join(row))
        for i in range(len(people)):
            if(macs[i] in rowValue):
                lastSeen[i]=rowValue

for i in range(len(people)):
    online = ""
    if(("BOOT" in lastSeen[i]) or ("RECONNECT" in lastSeen[i]) or ("PING" in lastSeen[i])):
        online= "**"
    print(online+people[i] + " -- " + lastSeen[i])
