# -*- coding: UTF-8 -*-
import docker
import os
import sys
from docker.types import Mount
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

gArgsMap = argMap()

# gArgMountSrcDir = "/Users/yzr/repos/docker/testdir/go"
# gArgMountTargetDir = "/opt/mydir"
# gArgContainerName = "gxc"

DEFATULT_TAR_DIR = "/opt/mydir"

gArgMountSrcDir = gArgsMap.get("src")
gArgMountTargetDir = gArgsMap.get("dst")
gArgContainerName = gArgsMap.get("name")


test=gArgsMap.get("test")

print(gArgMountSrcDir, gArgMountTargetDir, gArgContainerName)

if (gArgMountSrcDir == None):
    raise RuntimeError("no src dir")
else:
    abpath = os.path.abspath(gArgMountSrcDir)
    if (os.path.exists(abpath)):
        gArgMountSrcDir = abpath
        print("exist path %s" % abpath)
    else:
        raise RuntimeError("not exist path:%s ,abspath:%s" % (gArgMountSrcDir, abpath))
if (gArgMountTargetDir == None):
    print("tart dir is None")
    gArgMountTargetDir = DEFATULT_TAR_DIR
    print("use default target dir %s" % DEFATULT_TAR_DIR)
if (gArgContainerName == None):
    RuntimeError("container name is None")

if(test==True):
    sys.exit(0)

gFormatContainerName = ("/%s" % (gArgContainerName))

client = docker.from_env()

# 查询 container
gAllContainers = client.containers.list(all=True)


# 创建 container 的 mount 路径
def createMount(targetDir, srcDir):
    lRetMount = Mount(target=targetDir, source=srcDir, type="bind")
    lRetMounts = [lRetMount]
    return lRetMounts


# 查找 container
def getContainerByName(name, containerList):
    for container in containerList:
        if (container.attrs.get("Name") == name):
            return container
    return None


# 答应一个 container 信息
def printContainer(container):
    if (container != None):
        lName = container.attrs["Name"]
        lState = container.attrs["State"]
        lMounts = container.attrs["HostConfig"]["Mounts"]
        print("containerName:", lName)
        print("state:", lState)
        print("lMounts:", lMounts)
        print("allAttr:", container.attrs)


# 启动一个 container
def createContainer():
    mMounts = createMount(gArgMountTargetDir, gArgMountSrcDir)
    container = client.containers.run("ubuntu:work-os",
                                      "/bin/bash",
                                      name=gArgContainerName,
                                      detach=True,
                                      mounts=mMounts,
                                      stdin_open=True)
    print("create container %s" % gArgContainerName)
    printContainer(container)
    # 貌似刚创建的时候的状态就是 create
    # if (container.attrs["State"]["Running"]):
    #     print("create container %s success" % gArgContainerName)
    #     printContainer(container)
    # else:
    #     print("create container %s error" % gArgContainerName)
    #     printContainer(container)
    #     sys.exit(1)


# 重新从 api 获取 container 状态，并获取指定name 的 container
def reFindContainerByName(name):
    lContainers = client.containers.list(all=True)
    return getContainerByName(name, lContainers)


# 从新创建一个 contaienr
def recreateContainer(container):
    # 先删除一个 container
    container.remove(force=True)
    createContainer()


# 重启一个 contaienr
def restartContainer(container):
    print("try restart cntainer... %s" % gArgContainerName)
    # 启动
    container.start()
    # 启动后，重新查询状态
    lRefindContainer = reFindContainerByName(gFormatContainerName)
    # 获得了重新启动的状态
    if (lRefindContainer != None):
        print("refind container ... %s" % gArgContainerName)
        # 如果重启后的状态是 Running,那重启成功
        if (lRefindContainer.attrs["State"]["Running"]):
            print("restart container %s success" % gArgContainerName)
            printContainer(lRefindContainer)
        else:
            # 如果重启后的状态仍然不是 Running, 那么尝试重建
            print("restart container %s error" % gArgContainerName)
            recreateContainer(container)
    else:
        # 查询重启状态失败，重建 container
        print("not refind container... %s" % gArgContainerName)
        recreateContainer(container)


# 尝试加载 container
def tryContainer(containers):
    # 根据 format 形式的 container name 查找需要的 container
    container = getContainerByName(gFormatContainerName, containers)
    # 如果查找 container 不为 None,说明创建过 container
    if (container != None):
        # 如果已创建的 container 是 Running 的，那么打印相关信息。
        if (container.attrs["State"]["Running"]):
            print("container is running")
            printContainer(container)
        else:
            # 如果已经创建了 container 但是没有，running ,需要重新启动它
            print("container is not running")
            restartContainer(container)
            # recreateContainer(container) test 用
    else:
        # 如果根本没有启动 container, 那么启动
        createContainer()


tryContainer(gAllContainers)
