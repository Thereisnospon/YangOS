# -*- coding: UTF-8 -*-
import os
import re
import os, sys

g_dict = {}


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
            if(not _line):
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
        global  c
        c=c+line
        return not(line=="EOF_EOF_EOF=eof_eof_eof\n")
    read_file_foreach_line(path,g)
    return c

if (len(sys.argv) < 2):
    print("没有提供参数 工作路径，src makefile 文件， dst makefile 文件")
    exit(1)
if (len(sys.argv) < 3):
    print("没有提供参数 src makefile 文件， dst makefile 文件")
    exit(1)
if (len(sys.argv) < 4):
    print("没有提供参数 dst makefile 文件")
    exit(1)

scan_dir = sys.argv[1]
src_makefile= sys.argv[2]
dst_makefile=sys.argv[3]

if (not os.path.exists(scan_dir)):
    print(scan_dir + " 路径不存在")
    exit(2)

if (not os.path.exists(src_makefile)):
    print(src_makefile + " 路径不存在")
    exit(2)

if (not os.path.exists(dst_makefile)):
    print(dst_makefile + " 路径不存在")
    exit(2)

if (not os.path.isdir(scan_dir)):
    print(scan_dir + " 不是文件夹")
    exit(3)

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
lines=""

def deal_c(path):
    global  lines
    _dict = {}
    if (path.endswith(".c")):
        _lsit = pars_file_includes(path)
        _dict[path] = _lsit
    for (k, v) in _dict.items():
        incs = depends(v);
        name = path[path_prefix_len:len(path)]
        lines=lines+name + "_hs=" + incs +"\n"

traversal(scan_dir, deal_c)

elf_pre_lines = read_by_eof(src_makefile)

if (not elf_pre_lines.endswith("EOF_EOF_EOF=eof_eof_eof\n")):
    print("file not end with EOF_EOF_EOF=eof_eof_eof")
    exit(1)

with open(dst_makefile, mode="w", encoding="utf-8") as file:
    file.write(elf_pre_lines+lines)

