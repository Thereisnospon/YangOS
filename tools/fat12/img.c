/*  img.c
    操作 FAT12 文件系统的镜像文件
    四彩
    2015-12-08
*/


#include <stdio.h>
#include <string.h>
#include "FAT12.h"


// 显示提示信息
void PrintUsage(char *strFileName);

int main(int argc, char *argv[])
{
    int i;

    // 显示提示信息
    if((argc == 1) || ((argc == 2) && (!stricmp(argv[1], "-h"))))
    {
        PrintUsage(argv[0]);
        return 0;
    }

    // 创建新的空白镜像文件
    if((argc == 3) && (!stricmp(argv[1], "-n")))
    {
        if(CreateNewImg(argv[2]))
        {
            printf("Error in creating a new img file : %s !\n", argv[2]);
            return 1;
        }
        return 0;
    }

    // 复制引导扇区
    if((argc == 4) && (!stricmp(argv[1], "-c")))
    {
        if(CopyBootSector2Img(argv[2], argv[3]))
        {
            printf("Error in copying Boot Sector from %s to %s ...\n", argv[3], argv[2]);
            return 1;
        }
        return 0;
    }

    // 增加文件
    if(((argc >= 4) && (!stricmp(argv[1], "-a"))))
    {
        for(i = 3; i < argc; i++)
        {
            if(AddFile2Img(argv[2], argv[i], 0))
            {
                printf("Error in adding file %s to %s ...\n", argv[i], argv[2]);
                return 1;
            }
        }
        return 0;
    }

    // 删除文件
    if(((argc >= 4) && (!stricmp(argv[1], "-d"))))
    {
        for(i = 3; i < argc; i++)
            DeleteFileFromImg(argv[2], argv[i]);
        return 0;
    }

    printf("Can't recognize this command.\n");
    return 1;
}

// 显示提示信息
void PrintUsage(char *strFileName)
{
    printf("Usage: %s <-a | -d | -c | -n | -h> vImg [File1 [File2 ...]]\n", strFileName);
    printf("  -a Add files to vImg\n");
    printf("  -d Delete files from vImg\n");
    printf("  -c Copy Boot Sector to vImg\n");
    printf("  -n Create a new vImg\n");
    printf("  -h Show this usage\n");
}