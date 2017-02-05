/*  FAT12.c
    操作 FAT12 文件系统的镜像文件的实现部分
    四彩
    2015-12-06
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "FAT12.h"


//========================================================================================
// FAT12 文件系统 3.5 英寸软盘常量
#define TOTALSECTORS                2880    // 总扇区数
#define BYTESPERSECTOR              512     // 每扇区字节数

#define FIRSTOFFAT                  1       // FAT 表的起始逻辑扇区号
#define FIRSTOFROOTDIR              19      // 根目录的起始逻辑扇区号
#define FIRSTOFDATA                 33      // 数据区的起始逻辑扇区号
//****************************************************************************************


//========================================================================================
// 目录表项结构
typedef struct tag_DirectoryItem
{
    char            DIR_Name[11];           // 文件名， 8 + 3（注意：没有字符串结尾符 0）
    char            DIR_Attr;               // 文件属性
    char            DIR_Rsvd[10];           // 保留
    unsigned short  DIR_WrtTime;            // 2 位，最后修改时间
    unsigned short  DIR_WrtDate;            // 2 位，最后修改日期
    unsigned short  DIR_FstClus;            // 2 位，此条目对应的开始簇号（即扇区号）
    unsigned int    DIR_FileSize;           // 4 位，文件大小
} T_DIRECTORYITEM;
//****************************************************************************************


unsigned short ReadFatEntryValue(FILE *fpImg, unsigned short iEntry);
void WriteFatEntryValue(FILE *fpImg, unsigned short iEntry, unsigned short nValue);
void ConvertDirName2FileName(char *strDirName, char *strFileName);
void ConvertFileName2DirName(char *strFileName, char *strDirName);
unsigned short FindEmptyEntryInRootDirectory(FILE *fpImg);
unsigned short FindEmptyEntryInFat(FILE *fpImg, unsigned short iFirstEntry);
unsigned short FindFileInRootDirectory(FILE *fpImg, char *strFileName);
unsigned short GetRestSectors(FILE *fpImg);
void DeleteFile(FILE *fpImg, unsigned short iRootDirEntry);
void AddFile(FILE *fpImg, FILE *fpFile, char *strDirName, unsigned char ucFileAttr);

/*  功能：
        创建一个新的空白镜像文件
    参数表：
        strImgName = 镜像文件名
    返回值：
        0 = 成功
        1 = 未能创建文件
*/
int CreateNewImg(char *strImgName)
{
    // 下面这段神一样的数据就是 BootSector.sys 的 16 进制数而已
    unsigned char BootSector[BYTESPERSECTOR] =
    {
        0xEB,0x39,0x90,0x4E,0x41,0x53,0x4D,0x2B,0x47,0x43,0x43,0x00,0x02,0x01,0x01,0x00,
        0x02,0xE0,0x00,0x40,0x0B,0xF0,0x09,0x00,0x12,0x00,0x02,0x00,0x00,0x00,0x00,0x00,
        0x40,0x0B,0x00,0x00,0x00,0x00,0x29,0x00,0x00,0x00,0x00,0x54,0x65,0x73,0x74,0x58,
        0x5F,0x76,0x30,0x2E,0x30,0x31,0x46,0x41,0x54,0x31,0x32,0x8C,0xC8,0x8E,0xD8,0x8E,
        0xD0,0xB8,0x00,0x7C,0x89,0xC5,0x89,0xC4,0xBE,0x7F,0x7D,0xE8,0x1C,0x00,0x83,0xF8,
        0x00,0x75,0x08,0xBE,0x75,0x7D,0xE8,0x06,0x01,0xEB,0xFE,0x68,0x00,0x90,0x07,0xBB,
        0x00,0x10,0xE8,0x50,0x00,0xEA,0x00,0x10,0x00,0x90,0x55,0x89,0xE5,0x83,0xEC,0x04,
        0x06,0x51,0x53,0x68,0xE0,0x07,0x07,0xC7,0x46,0xFE,0x13,0x00,0xC7,0x46,0xFC,0x0E,
        0x00,0x8B,0x46,0xFE,0x31,0xDB,0xE8,0x97,0x00,0xB9,0x10,0x00,0xB8,0x0B,0x00,0xE8,
        0xB1,0x00,0x83,0xF8,0x00,0x74,0x13,0x83,0xC3,0x20,0xE2,0xF0,0xFF,0x4E,0xFC,0x74,
        0x05,0xFF,0x46,0xFE,0xEB,0xDB,0x31,0xC0,0xEB,0x04,0x26,0x8B,0x47,0x1A,0x5B,0x59,
        0x07,0x89,0xEC,0x5D,0xC3,0x55,0x89,0xE5,0x53,0x50,0x53,0x50,0x83,0xC0,0x1F,0xE8,
        0x5E,0x00,0x58,0xE8,0x12,0x00,0x5B,0x3D,0xF8,0x0F,0x73,0x06,0x81,0xC3,0x00,0x02,
        0xEB,0xE8,0x58,0x5B,0x89,0xEC,0x5D,0xC3,0x55,0x89,0xE5,0x06,0x52,0x51,0x53,0x31,
        0xD2,0xBB,0x03,0x00,0xF7,0xE3,0xBB,0x02,0x00,0xF7,0xF3,0x89,0xD1,0x31,0xD2,0xBB,
        0x00,0x02,0xF7,0xF3,0x83,0xC0,0x01,0x52,0x51,0x50,0x68,0xE0,0x07,0x07,0x31,0xDB,
        0xE8,0x1D,0x00,0x58,0x40,0xBB,0x00,0x02,0xE8,0x15,0x00,0x59,0x5B,0x26,0x8B,0x07,
        0xE3,0x03,0xC1,0xE8,0x04,0x25,0xFF,0x0F,0x5B,0x59,0x5A,0x07,0x89,0xEC,0x5D,0xC3,
        0x55,0x89,0xE5,0x52,0x51,0xB2,0x12,0xF6,0xF2,0x88,0xC5,0x88,0xC6,0x88,0xE1,0xD0,
        0xED,0xFE,0xC1,0x80,0xE6,0x01,0xB8,0x01,0x02,0x30,0xD2,0xCD,0x13,0x59,0x5A,0x89,
        0xEC,0x5D,0xC3,0x55,0x89,0xE5,0x56,0x51,0x53,0x89,0xC1,0xE3,0x09,0xAC,0x26,0x3A,
        0x07,0x75,0x03,0x43,0xE2,0xF7,0x89,0xC8,0x5B,0x59,0x5E,0x89,0xEC,0x5D,0xC3,0x55,
        0x89,0xE5,0x56,0x50,0xB4,0x0E,0xAC,0x3C,0x00,0x74,0x04,0xCD,0x10,0xEB,0xF7,0x58,
        0x5E,0x89,0xEC,0x5D,0xC3,0x4E,0x6F,0x74,0x20,0x66,0x6F,0x75,0x6E,0x64,0x20,0x4C,
        0x4F,0x41,0x44,0x45,0x52,0x20,0x20,0x53,0x59,0x53,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x55,0xAA,
    };
    // FAT 表第一个字节代表存储介质（0xF0 = 软盘），第二、三字节代表 FAT 文件分配表标识符
    char FATMSG[3] = {0xF0, 0xFF, 0xFF};
    FILE *fpImg;
    unsigned long int i;

    // 新建镜像文件
    fpImg = fopen(strImgName, "wb");
    if(fpImg == NULL)
    {
        //printf("Cann't create %s .\n", strImgName);
        return 1;
    }

    // 写引导扇区
    fwrite(BootSector, BYTESPERSECTOR, 1, fpImg);

    // 写 FAT1 表信息
    fwrite(FATMSG, 3, 1, fpImg);            // 表头
    for(i = BYTESPERSECTOR + 3; i < 10 * BYTESPERSECTOR; i++)           // 剩下填 0
        fputc(0, fpImg);
    // 写 FAT2 表信息
    fwrite(FATMSG, 3, 1, fpImg);            // 表头
    for(i = 10 * BYTESPERSECTOR + 3; i < 2880 * BYTESPERSECTOR; i++)    // 后面全部填 0
        fputc(0, fpImg);

    fclose(fpImg);
    return 0;
}

/*  功能：
        复制引导扇区到镜像
    参数表：
        strImgName  = 镜像文件名
        strFileName = 引导扇区文件名
    返回值：
        0 = 成功
        1 = 失败
*/
int CopyBootSector2Img(char *strImgName, char *strFileName)
{
    FILE *fpFile, *fpImg;
    unsigned char ucBuffer[BYTESPERSECTOR], ucTemp[2];

    // 打开引导扇区文件
    fpFile = fopen(strFileName, "rb");
    if(fpFile == NULL)
    {
        //printf("%s does not exist.\n", strFileName);
        return 1;
    }

    // 检查源文件长度，不能低于 512 字节
    fseek(fpFile, 0, SEEK_END);
    if(ftell(fpFile) < BYTESPERSECTOR)
    {
        //printf("%s is less than %d bytes in length.\n", strFileName, BYTESPERSECTOR);
        fclose(fpFile);
        return 1;
    }

    // 检查源文件510、511处 2 个字节，必须是 0xAA55
    fseek(fpFile, 510, SEEK_SET);
    fread(ucTemp, 2, 1, fpFile);
    if(ucTemp[0] != 0x55 || ucTemp[1] != 0xAA)
    {
        //printf("The last two bytes of %s are not 0xAA55！\n", strFileName);
        fclose(fpFile);
        return 1;
    }

    // 打开镜像文件
    fpImg = fopen(strImgName, "rb+");
    if(fpImg == NULL)
    {
        //printf("%s does not exist.\n", strImgName);
        fclose(fpFile);
        return 1;
    }

    // 读源文件数据到内存
    fseek(fpFile, 0, SEEK_SET);
    fread(ucBuffer, BYTESPERSECTOR, 1, fpFile);

    // 写内存数据到镜像文件
    fseek(fpImg, 0, SEEK_SET);
    fwrite(ucBuffer, BYTESPERSECTOR, 1, fpImg);

    fclose(fpFile);
    fclose(fpImg);
    return 0;
}

/*  功能：
        向镜像增加文件
    参数表：
        strImgName  = 镜像文件名
        strPathName = 源文件全路径
        ucFileAttr  = 文件属性
    返回值：
        0 = 成功
        1 = 失败
*/
int AddFile2Img(char *strImgName, char *strPathName, unsigned char ucFileAttr)
{
    FILE *fpFile, *fpImg;
    unsigned short iPos, iRootDirItem, size = sizeof(T_DIRECTORYITEM);
    T_DIRECTORYITEM tDirItem;
    char strDirName[12], *strFileName;

    // 打开两个文件
    fpFile = fopen(strPathName, "rb");
    if(fpFile == NULL)
    {
        //printf("Cann't open %s .\n", strPathName);
        return 1;
    }
    fpImg = fopen(strImgName, "rb+");
    if(fpImg == NULL)
    {
        //printf("Cann't open %s .\n", strImgName);
        fclose(fpFile);
        return 1;
    }

    // 从源文件路径中取出文件名
    strFileName = strrchr(strPathName, SEPARATOR);
    strFileName = ((NULL == strFileName) ? strPathName : (strFileName + 1));

    // 若已存在同名文件，取得其长度
    iRootDirItem = FindFileInRootDirectory(fpImg, strFileName);
    if(iRootDirItem < 0xFF8)
    {
        fseek(fpImg, FIRSTOFROOTDIR * BYTESPERSECTOR + iRootDirItem * size, SEEK_SET);
        fread(&tDirItem, size, 1, fpImg);
    }
    else
        tDirItem.DIR_FileSize = 0;

    // 判断剩余空间（包括已存在同名文件占用的部分）是否足够
    fseek(fpFile, 0, SEEK_END);
    if((tDirItem.DIR_FileSize + GetRestSectors(fpImg) * BYTESPERSECTOR) < ftell(fpFile))
    {
        //printf("Not enough space.\n");
        fclose(fpFile);
        fclose(fpImg);
        return 1;
    }
    // 确认根目录区是否有空表项
    if(FindEmptyEntryInRootDirectory(fpImg) > 0xFF8)
    {
        //printf("Not empty item in Root Directory.\n");
        fclose(fpFile);
        fclose(fpImg);
        return 1;
    }

    // 删掉已存在的同名文件
    if(iRootDirItem < 0xFF8)
        DeleteFile(fpImg, iRootDirItem);

    // 转换文件名格式
    ConvertFileName2DirName(strFileName, strDirName);

    // 向镜像中增加文件
    AddFile(fpImg, fpFile, strDirName, ucFileAttr);

    fclose(fpFile);
    fclose(fpImg);
    return 0;
}

/*  功能：
        从镜像中删除文件
    参数表：
        strImgName  = 镜像文件名
        strFileName = 目标文件名
    返回值：
        无
*/
void DeleteFileFromImg(char *strImgName, char *strFileName)
{
    FILE *fpImg;
    unsigned short iEntry;

    fpImg = fopen(strImgName, "rb+");
    if(fpImg != NULL)
    {
        iEntry = FindFileInRootDirectory(fpImg, strFileName);
        if(iEntry < 0xFF8)
            DeleteFile(fpImg, iEntry);
        fclose(fpImg);
    }
}

/*  功能：
        读取 FAT 表中某表项序号的值
    参数表：
        fpImg  = 镜像文件句柄
        iEntry = FAT 表项序号
    返回值：
        FAT 表项的值
*/
unsigned short ReadFatEntryValue(FILE *fpImg, unsigned short iEntry)
{
    unsigned short nTemp;

    // 读出两个字节
    fseek(fpImg, FIRSTOFFAT * BYTESPERSECTOR + iEntry * 3 / 2, SEEK_SET);
    fread(&nTemp, 2, 1, fpImg);

    // 偶数项取低 12 位，奇数项取高 12 位
    return (iEntry % 2 == 0 ? nTemp & 0xFFF : nTemp >> 4);
}

/*  功能：
        写入 FAT 表中某表项序号的值
    参数表：
        fpImg  = 镜像文件句柄
        iEntry = FAT 表项序号
        nValue = FAT 表项的值
    返回值：
        无
*/
void WriteFatEntryValue(FILE *fpImg, unsigned short iEntry, unsigned short nValue)
{
    unsigned short nTemp;

    // 读出原位置两个字节
    fseek(fpImg, FIRSTOFFAT * BYTESPERSECTOR + iEntry * 3 / 2, SEEK_SET);
    fread(&nTemp, 2, 1, fpImg);

    // 偶数项放在低 12 位，奇数项放在高 12 位
    if(iEntry % 2 == 0)
        nValue |= (nTemp & 0b1111000000000000);
    else
        nValue = ((nValue << 4) | (nTemp & 0b1111));

    // 回写 FAT1
    fseek(fpImg, FIRSTOFFAT * BYTESPERSECTOR + iEntry * 3 / 2, SEEK_SET);
    fwrite(&nValue, 2, 1, fpImg);
    // 回写 FAT2
    fseek(fpImg, (FIRSTOFFAT + FIRSTOFROOTDIR) * BYTESPERSECTOR / 2 +
                                                            iEntry * 3 / 2, SEEK_SET);
    fwrite(&nValue, 2, 1, fpImg);
}

/*  功能：将文件名从 8 + 3 大写格式转换为普通字符串
    参数表：
        strDirName  = 待处理的文件名字符串，8 + 3 + 0 大写，长 12 字符
        strFileName = 处理后的文件名字符串，8 + '.' + 3 + 0，长 13 字符
    返回值：
        无
*/
void ConvertDirName2FileName(char *strDirName, char *strFileName)
{
    int i, j = 7, k = 10;

    // 前面 8 字节为文件名
    while(strDirName[j] == ' ')           // 忽略掉尾部空格
        j--;
    for(i = 0; i <= j; i++)
        strFileName[i] = strDirName[i];

    // 后面 3 字节为扩展名
    while(strDirName[k] == ' ')
        k--;
    if(k > 7)                               // k < 8 则没有扩展名
    {
        strFileName[++j] = '.';
        for(i = 8; i <= k; i++)
            strFileName[++j] = strDirName[i];
    }

    // 字符串结尾符
    strFileName[++j] = 0;
    // 将所有字符全部转换为小写字符
    for(i = 0; i < j; i++)
        strFileName[i] = tolower(strFileName[i]);
}

/*  功能：将文件名从普通字符串转换为 8 + 3 大写格式
    参数表：
        strFileName = 待处理的文件名字符串，8 + '.' + 3 + 0，长 13 字符
        strDirName  = 处理后的文件名字符串，8 + 3 + 0 大写，长 12 字符
    返回值：
        无
*/
void ConvertFileName2DirName(char *strFileName, char *strDirName)
{
    int i, j = -1;
    char *pExtension;

    // 取最后一个 '.' 位置，输入字串最多 12 个字符：8 + '.' + 3
    pExtension = strrchr(strFileName, '.');

    // 没有找到 '.' 表示该文件没有扩展名
    if(NULL == pExtension)
    {
        // 文件名最长 8 字节
        for(i = 0; i < 8; i++)
        {
            if(strFileName[i] == 0)
                break;
            strDirName[i] = strFileName[i];
        }
        // 后面全部补空格
        while(i < 11)
            strDirName[i++] = ' ';
    }
    else
    {
        // '.' 之前为文件名，最长 8 字节
        j = pExtension - strFileName;
        for(i = 0; i < j && i < 8; i++)
            strDirName[i] = strFileName[i];
        // 不够 8 字节的补空格
        while(i < 8)
            strDirName[i++] = ' ';

        j++;                                // j + 1 跳过 '.'

        // '.' 之后的为扩展名，最多 3 字节
        for(i = 8; i < 11; i++, j++)
        {
            if(strFileName[j] == 0)
                break;
            strDirName[i] = strFileName[j];
        }
        // 不够 3 字节的补空格
        while(i < 11)
            strDirName[i++] = ' ';
    }

    // 将所有字符全部转换为大写字符
    for(i = 0; i < 11; i++)
        strDirName[i] = toupper(strDirName[i]);
    // 字符串结尾符
    strDirName[11] = 0;
}

/*  功能：
        在根目录区中寻找空值表项
    参数表：
        fpImg  = 镜像文件句柄
    返回值：
        第一个空值表项序号，0xFFF = 没有
*/
unsigned short FindEmptyEntryInRootDirectory(FILE *fpImg)
{
    T_DIRECTORYITEM tDirItem;
    unsigned short i, max, size = sizeof(T_DIRECTORYITEM);
    unsigned char ch;

    max = (FIRSTOFDATA - FIRSTOFROOTDIR) * BYTESPERSECTOR / size;
    for(i = 0; i < max; i++)
    {
        fseek(fpImg, FIRSTOFROOTDIR * BYTESPERSECTOR + i * size, SEEK_SET);
        fread(&tDirItem, size, 1, fpImg);

        // 文件名第一个字母 = 0 表示空，= 0xE5 表示已删除的文件
        ch = tDirItem.DIR_Name[0];
        if((ch == 0) || (ch == 0xE5))
            return i;
    }

    return 0xFFF;
}

/*  功能：
        在 FAT 表中寻找空值表项（即寻找空扇区）
    参数表：
        fpImg       = 镜像文件句柄
        iFirstEntry = 起始表项序号
    返回值：
        iFirstEntry 之后（包括 iStartNum）的第一个空值表项序号，0xFFF = 没有
*/
unsigned short FindEmptyEntryInFat(FILE *fpImg, unsigned short iFirstEntry)
{
    unsigned short i, max;

    // 从第 iFirstEntry 号开始取值判断到最后（ / 3 = / 2 * 8 / 12）
    max = (FIRSTOFROOTDIR - FIRSTOFFAT) * BYTESPERSECTOR / 3;
    for(i = iFirstEntry; i < max; i++)
    {
        if(ReadFatEntryValue(fpImg, i) == 0)
            return i;
    }

    return 0xFFF;
}

/*  功能：
        统计剩余空间（数据区的空白扇区数）
    参数表：
        fpImg = 镜像文件句柄
    返回值：
        数据区的空白扇区数
*/
unsigned short GetRestSectors(FILE *fpImg)
{
    unsigned short iFirstEntry = 2, iNext, nNum = 0;

    while((iNext = FindEmptyEntryInFat(fpImg, iFirstEntry)) > 0 && (iNext < 0xFFF))
    {
        iFirstEntry = iNext + 1;
        nNum++;
    }
    return nNum;
}

/*  功能：
        在根目录区中寻找文件
    参数表：
        fpImg       = 镜像文件句柄
        strFileName = 文件名
    返回值：
        该文件的表项序号，0xFFF = 未找到
*/
unsigned short FindFileInRootDirectory(FILE *fpImg, char *strFileName)
{
    T_DIRECTORYITEM tDirItem;
    char FileItemName[12], DirItemName[12];
    unsigned short i, max, size = sizeof(T_DIRECTORYITEM);

    ConvertFileName2DirName(strFileName, FileItemName);

    DirItemName[11] = 0;
    max = (FIRSTOFDATA - FIRSTOFROOTDIR) * BYTESPERSECTOR / size;
    for(i = 0; i < max; i++)
    {
        fseek(fpImg, FIRSTOFROOTDIR * BYTESPERSECTOR + i * size, SEEK_SET);
        fread(&tDirItem, size, 1, fpImg);

        // 根据文件名判断（FAT12 目录结构中文件名没有以 0 结尾，要自己控制复制长度）
        strncpy(DirItemName, tDirItem.DIR_Name, 11);
        if(!strcmp(DirItemName, FileItemName))
            return i;
    }

    return 0xFFF;
}

/*  功能：
        从镜像中删除文件
    参数表：
        fpImg  = 镜像文件句柄
        iEntry = 目标文件在根目录区的表项号
    返回值：
*/
void DeleteFile(FILE *fpImg, unsigned short iRootDirEntry)
{
    unsigned short nFatValue, iFatNext, size = sizeof(T_DIRECTORYITEM);
    T_DIRECTORYITEM tDirItem;
//    char i, ch = 0;

    // 读出根目录表项
    fseek(fpImg, FIRSTOFROOTDIR * BYTESPERSECTOR + iRootDirEntry * size, SEEK_SET);
    fread(&tDirItem, size, 1, fpImg);

    // 依次清空 FAT 表链
    iFatNext = tDirItem.DIR_FstClus;
    do
    {
        // 这里可添加数据区扇区清 0 的代码
        // 。。。。。。
        nFatValue = ReadFatEntryValue(fpImg, iFatNext);
        WriteFatEntryValue(fpImg, iFatNext, 0);
        iFatNext = nFatValue;
    } while(iFatNext < 0xFF8);

    // 删除根目录区表项
    fseek(fpImg, FIRSTOFROOTDIR * BYTESPERSECTOR + iRootDirEntry * size, SEEK_SET);
    fputc(0xE5, fpImg);                     // 第一个字符改删除标识：0xE5
//    for(i = 0; i < size; i++)               // 全部清 0
//        fputc(ch, fpImg);
}

/*  功能：
        向镜像增加文件
    参数表：
        fpImg      = 镜像文件句柄
        fpFile     = 源文件句柄
        strDirName = 格式化后的文件名
        ucFileAttr = 文件属性
    返回值：
        无
*/
void AddFile(FILE *fpImg, FILE *fpFile, char *strDirName, unsigned char ucFileAttr)
{
    unsigned char ucData[BYTESPERSECTOR];
    unsigned long nFileLen;
    unsigned short arrySectorList[TOTALSECTORS], i, size = sizeof(T_DIRECTORYITEM);
    unsigned short nNeedSectors, iLast, nRemainderBytes, iRootDirItem;
    T_DIRECTORYITEM tDirItem;

    // 取得源文件长度，并计算所需扇区数
    fseek(fpFile, 0, SEEK_END);
    nFileLen = ftell(fpFile);
    nRemainderBytes = nFileLen % BYTESPERSECTOR;
    nNeedSectors = nFileLen / BYTESPERSECTOR + (nRemainderBytes > 0 ? 1 : 0);
    iLast = nNeedSectors - 1;

    // 寻找 FAT 空表项，同时建立 FAT 表链
    arrySectorList[0] = FindEmptyEntryInFat(fpImg, 2);
    for(i = 1; i < nNeedSectors; i++)
    {
        arrySectorList[i] = FindEmptyEntryInFat(fpImg, arrySectorList[i - 1] + 1);
        WriteFatEntryValue(fpImg, arrySectorList[i - 1], arrySectorList[i]);
    }
    WriteFatEntryValue(fpImg, arrySectorList[iLast], 0xFFF);

    // 根目录区表项登记
    iRootDirItem = FindEmptyEntryInRootDirectory(fpImg);
    strncpy(tDirItem.DIR_Name, strDirName, 11);
    tDirItem.DIR_Attr = ucFileAttr;
//  tDirItem.DIR_WrtTime = ;
//  tDirItem.DIR_WrtDate = ;
    tDirItem.DIR_FstClus = arrySectorList[0];
    tDirItem.DIR_FileSize = nFileLen;
    fseek(fpImg, FIRSTOFROOTDIR * BYTESPERSECTOR + iRootDirItem * size, SEEK_SET);
    fwrite(&tDirItem, size, 1, fpImg);

    // 拷贝数据到相应扇区
    fseek(fpFile, 0, SEEK_SET);
    for(i = 0; i < iLast; i++)              // 最后一个扇区单独处理（防止不满 512 字节）
    {
        fread(ucData, BYTESPERSECTOR, 1, fpFile);
        fseek(fpImg, (arrySectorList[i] + FIRSTOFDATA - 2) * BYTESPERSECTOR, SEEK_SET);
        fwrite(ucData, BYTESPERSECTOR, 1, fpImg);
    }
    if(nRemainderBytes > 0)                 // 最后一个扇区不满
    {
        fread(ucData, nRemainderBytes, 1, fpFile);
        for(i = nRemainderBytes; i < BYTESPERSECTOR; i++)
            ucData[i] = 0;
    }
    else                                    // 最后一个扇区也是满的
        fread(ucData, BYTESPERSECTOR, 1, fpFile);
    fseek(fpImg, (arrySectorList[iLast] + FIRSTOFDATA - 2) * BYTESPERSECTOR, SEEK_SET);
    fwrite(ucData, BYTESPERSECTOR, 1, fpImg);
}