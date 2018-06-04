# -*- coding: UTF-8 -*-
import os
import re
import os, sys

g_dict = {}

READ_ELF = "EOF_EOF_EOF=eof_eof_eof\n"


# 正则匹配，返回匹配的 组
def re_match(pattern, str):
    ret_obj = re.search(pattern, str)
    if ret_obj != None:
        return ret_obj.groups()
    else:
        return ()


# 打开 path 路径的文件，对于每行，使用 consumer 方法处理，如果 consumer 返回 False 则不继续处理后面的行
def read_file_foreach_line(path, consumer):
    with open(path, mode="r", encoding="utf-8") as file:
        while (True):
            _line = file.readline()
            if (not _line):
                break
            if (not consumer(_line)):
                break


# 找到文件中的 #include 的行
def pars_file_includes(file_path):
    _list = []
    global have_inc

    def _each_lien(x):
        groups = re_match("\s?#include\s+([\s\S]+)", x)
        if (len(groups) > 0):
            xgroup = re_match("[\s]?[\"<]+?(\S+)[\">]+?[\s]?", groups[0])
            if (len(xgroup) > 0):
                _list.append(xgroup[0])
                return True
        return x == "\n"

    read_file_foreach_line(file_path, _each_lien)
    return _list


# 遍历 rootDir 目录下的所有文件，对于每个文件使用 func 方法
def traversal(rootDir, func):
    for lists in os.listdir(rootDir):
        path = os.path.join(rootDir, lists)
        if os.path.isdir(path):
            traversal(path, func)
        else:
            func(path)


def read_by_eof(path):
    global c
    c = ""

    def g(line):
        global c
        c = c + line
        return not (line == READ_ELF)

    read_file_foreach_line(path, g)
    if not c.endswith(READ_ELF):
        print("not " + READ_ELF)
        exit(0)
    return c


def get_objs(path):
    global find_obj_start
    global find_obj_end
    global str
    str = ""
    find_obj_start = False
    find_obj_end = False

    def xc(line):
        global find_obj_start
        global find_obj_end
        global str
        if (line.startswith("OBJS=")):
            str = str + line
            find_obj_start = True
            return True
        if (line.startswith("OBJS_END=objs_end")):
            str = str + line
            find_obj_end = True
            return True
        if (find_obj_start and not find_obj_end):
            str = str + line
            return True
        if (not find_obj_start):
            return True
        return False

    read_file_foreach_line(path, xc)
    return str


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


gArgs = argMap()




# with open("/Users/yzr/repos/YangOS/core/m1", mode="w", encoding="utf-8") as file:
#     file.write(ct+str)


def file_arg(key):
    val = gArgs.get(key)
    if (val == None):
        print(key + " 参数不存在")
        exit(0)
    if (not os.path.exists(val)):
        print(val + " 路径不存在");
        exit(0)
    # print(key+"="+val)
    return val


scan_dir = file_arg("scandir")

if (not os.path.isdir(scan_dir)):
    print(scan_dir + " 不是目录")
    exit(0)

src_makefile = file_arg("u_src_mk")
dst_makefile = file_arg("u_dst_mk")

g_src_mk=file_arg("g_src_mk")
g_dst_mk=file_arg("g_dst_mk")

if_print=gArgs.get("info")

mode=gArgs.get("mode")
if(mode==None):
    print("no mode")
    exit(0)
elif (mode=="hs"):
    print("hs mode")
elif (mode=="objs"):
    print("objs mode")
elif (mode=="all"):
    print("all mode")
else:
    print("error mode "+mode)
    exit(0)


path_prefix_len = len(scan_dir)
g_h_dicts = {}


# 将 .h 文件进行映射 文件名:相对路径
# 映射结果放到 g_h_dicts 中
def find_hs(path):
    if (path.endswith(".h")):
        last_seg = path.rfind("/")
        if (last_seg >= 0):
            k = path[last_seg + 1:len(path)]
            g_h_dicts[k] = path[path_prefix_len:len(path)]


# 所有.h 文件映射相对路径
traversal(scan_dir, find_hs)


# 返回所有 .h 文件列表的 相对路径列表
def depends(include_h_list):
    name = ""
    for h_f in include_h_list:
        name = name + " " + g_h_dicts[h_f]
    return name


global lines
lines = ""


def deal_c(path):
    global lines
    _dict = {}
    if (path.endswith(".c")):
        _lsit = pars_file_includes(path)
        _dict[path] = _lsit
    for (k, v) in _dict.items():
        incs = depends(v);
        name = path[path_prefix_len:len(path)]
        lines = lines + name + "_hs=" + incs + "\n"

if(mode=="hs" or mode=="all"):

    traversal(scan_dir, deal_c)

    elf_pre_lines = read_by_eof(src_makefile)

    if (not elf_pre_lines.endswith(READ_ELF)):
        print("file not end with EOF_EOF_EOF=eof_eof_eof")
        exit(1)

    with open(dst_makefile, mode="w", encoding="utf-8") as file:
        out=elf_pre_lines + lines
        file.write(out)
        if(if_print==True):
            print(out)

if(mode=="objs" or mode=="all"):

    str = get_objs(src_makefile)

    ct = read_by_eof(g_src_mk)

    with open(g_dst_mk, mode="w", encoding="utf-8") as file:
        out=ct + str
        file.write(out)
        if (if_print == True):
            print(out)




