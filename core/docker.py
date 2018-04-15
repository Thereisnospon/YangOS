import os

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
str = os.popen("docker container ls -a --format \"{{.Status}}\" -f \"name=wtest\"").read()
if str.startswith("Up"):
    print("Up")
elif str.startswith("Restarting"):
    print("Restarting")
elif str.startswith("Removal"):
    print("Removal")
elif str.startswith("Dead"):
    print("Dead")
elif str.startswith("Created"):
    print("Created")
elif str.startswith("Exited"):
	print("Exited")
elif str.startswith(" "):
    print("empty")
else:
    print("other")