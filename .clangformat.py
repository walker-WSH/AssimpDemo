#!/usr/bin/env python
# -*- coding: utf-8 -*-

import re
import os
import os.path
import sys,re  
import codecs

customIgnoreList = []
realIgnoreList = []

# 判断在不在白名单
def IsInIgnoreList(fullPath):
    for temp in customIgnoreList:
        if (temp in fullPath):
            return True
    
    return False;
    

currentDir, filename = os.path.split(os.path.abspath(__file__))
print('Current Dir:' + currentDir) # 末尾不包含“\”


# 读取并保存白名单
print('\n\n================================= read ignore =====================================')
f = codecs.open(".ignoreClangFormat.txt", 'r',encoding= u'utf-8', errors='ignore')  
lines = f.readlines()
for line in lines:
    line = line.replace("\r", "") #移除txt中的特殊符号
    line = line.replace("\n", "") #移除txt中的特殊符号
    fullPath = currentDir + '\\' + line
    customIgnoreList.append(fullPath)
    print(fullPath)
f.close()


# 遍历当前目录及子目录下的所有文件 并判断在不在白名单
print('\n\n================================= format files =====================================')
formatDir = currentDir + '\\src';
for root, dirs, files in os.walk(formatDir):
    for name in files:
        if (name.endswith(".h") or name.endswith(".hpp") or name.endswith(".hxx") or name.endswith(".c") or name.endswith(".cpp") or name.endswith(".cc") or name.endswith(".cxx")):
            fullPath = root + '\\' + name
            ignoreFile = IsInIgnoreList(fullPath)
            if ignoreFile == True:
                realIgnoreList.append(fullPath)
            else:
                print('Format:' + fullPath)
                os.system("clang-format -i %s -style=File" %(fullPath))
                
                
# 显示忽略了的文件
print('\n\n================================= ignore files =====================================')
for fullPath in realIgnoreList:
    print('Ignore File: ' + fullPath)
