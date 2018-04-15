import os


META_NAME="wtest"

allStatus = '''
Up 1 day (paused)
Restarting (123) 1 day ago
Up 1 day
Removal in Progress
Dead
Created
Exited (123) 1 day ago
(empty string)
'''

OK_STAT = 0
NEED_CREATE = 1
NEED_REMOVE_CREATE = 2
NEED_START = 3
def status2(stat_str):
    if stat_str.startswith("Up"):
        print("Up")
        return OK_STAT
    elif stat_str.startswith("Restarting"):
        print("Restarting")
        return NEED_REMOVE_CREATE
    elif stat_str.startswith("Removal"):
        print("Removal")
        return NEED_REMOVE_CREATE
    elif stat_str.startswith("Dead"):
        print("Dead")
        return NEED_REMOVE_CREATE
    elif stat_str.startswith("Created"):
        print("Created")
        return NEED_REMOVE_CREATE
    elif stat_str.startswith("Exited"):
        print("Exited")
        return NEED_START
    elif stat_str.startswith(" "):
        print("empty")
        return NEED_CREATE
    elif len(stat_str)==0:
        print("empty")
        return NEED_CREATE
    else:
        print("other")
        return NEED_REMOVE_CREATE

COMMAND_DOCKER_STAT="docker container ls -a --format \"{{.Status}}\" -f \"name=%s\"" % (META_NAME)
COMMAND_DOCKER_RM=" docker container rm -f %s" %(META_NAME)






stat = os.popen(COMMAND_DOCKER_STAT).read()

status2(stat)

print(stat)


