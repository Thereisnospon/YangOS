# -*- coding: UTF-8 -*-
import os
import sys
#
# -a -s  --name  go 这种形式的参数，返回一个 Map
# {'a': True, 's': True, 'name': 'go'}
#
def argMap():
    arglen = len(sys.argv)
    retMap = {}
    for i in range(1, arglen):
        name = str(sys.argv[i])
        if (name.startswith("-") and not (name.startswith("--"))):
            if (len(name) <= 1):
                raise RuntimeError("arg error -arg len <=1")
            retMap[name[1:]] = True
        elif (name.startswith("--")):
            if (len(name) <= 2):
                raise RuntimeError("arg error -arg len <=2")
            j = i + 1
            if (j >= arglen):
                raise RuntimeError("arg error --arg no next")
            elif sys.argv[j].startswith("-"):
                raise RuntimeError("arg error --arg next start with -")
            else:
                retMap[name[2:]] = sys.argv[j]
        else:
            pass
    return retMap

et=argMap()
print(et)
print(et["src"])
print(et["x"])
print(et.get("y"))