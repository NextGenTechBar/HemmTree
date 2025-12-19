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
    if("DISCONNECT" in lastSeen[i]):
        online= "~" #character to indicate it has been online at some point, but isn't now.
    print(online+people[i] + " -- " + lastSeen[i])


additionalMacs=[]
#now check if there's any logged MACs that aren't in the list of mac addresses
with open('Connection Log.csv', newline='') as csvfile:
    spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
    for row in spamreader:
        #print(', '.join(row))
        rowValue=(', '.join(row))
        mac=cell(rowValue,4)
        if(not(mac in macs) and not(mac in additionalMacs)):
            additionalMacs.append(mac)

lastSeenAdditional = ["Never"] * len(additionalMacs)

for i in range(len(additionalMacs)):
    with open('Connection Log.csv', newline='') as csvfile:
        spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
        for row in spamreader:
            #print(', '.join(row))
            rowValue=(', '.join(row))
            if(additionalMacs[i] in rowValue):
                lastSeenAdditional[i]=rowValue
if(len(additionalMacs)>0):
    print("")
    print("Additional MACs in log not in known MAC list: ")
    print("WARNING: possible bug -- first value in this list may actually be in the known mac list")
    for i in range(len(additionalMacs)):
        online = ""
        if(("BOOT" in lastSeenAdditional[i]) or ("RECONNECT" in lastSeenAdditional[i]) or ("PING" in lastSeenAdditional[i])):
            online= "**"
        print(online+lastSeenAdditional[i])
           
x = input("Press enter to exit")
