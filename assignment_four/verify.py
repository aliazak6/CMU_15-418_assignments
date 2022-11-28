import sys, string, os


def getDistance(cityID, reference=False):
    if reference:
        output = os.popen("./wsp-ref city/dist" + str(cityID)).read().split("\n")
    else:
        output = os.popen("./wsp -p 1 city/dist" + str(cityID)).read().split("\n")
    for outputLine in output:
        outputWords = outputLine.split(" ")
        if(outputWords[0] == "Cost:"):
            return (True, int(outputWords[1]))
    return (False, -1)

def verifyCity(cityID):

    found, cost = getDistance(cityID)
    _, costRef = getDistance(cityID, reference=True)
    if found:
        if(cost == costRef):
            print("City" + str(cityID) + " passed! " + str(costRef) + " " + str(cost))
            return True
        else:
            print("City" + str(cityID) + " failed! Requires distance " + str(costRef) + ", got " + str(cost) + ".")
            return False

    print("City" + str(cityID) + " failed! Could not find distance. Check to see if you are returning Cost:")
    return False


def main():

    print("")
    print("Testing city distance...")
    print("-----------------------------------------------")
    numFailed = 0

    for cityID in range(4,11):
        passed = verifyCity(cityID)
        if not passed:
            numFailed += 1
    print("-----------------------------------------------")
    if(numFailed == 0):
        print("All tests passed!")
    else:
        print("%d test(s) failed." % (numFailed))
    print("")

if __name__ == '__main__':
    main()