/*  FAT12.h
    操作 FAT12 文件系统的镜像文件的库函数申明
    四彩
    2015-12-08
*/


#ifndef _FAT12_H
#define _FAT12_H


#ifdef WINDOWS
    #define SEPARATOR   '\\'                 // Windows 系统下的分隔符
#else
    #define SEPARATOR   '/'                 // linux 系统下的分隔符
    #define stricmp     strcasecmp          // 比较字符串（不区分大小写）
#endif



//========================================================================================
/*  功能：
        创建一个新的空白镜像文件
    参数表：
        strImgName = 镜像文件名
    返回值：
        0 = 成功
        1 = 未能创建文件
*/
int CreateNewImg(char *strImgName);


/*  功能：
        复制引导扇区到镜像
    参数表：
        strImgName  = 镜像文件名
        strFileName = 引导扇区文件名
    返回值：
        0 = 成功
        1 = 失败
*/
int CopyBootSector2Img(char *strImgName, char *strFileName);


// 文件属性如下：
#define DIRATTR_READ_ONLY       0x01        //只读
#define DIRATTR_HIDDEN          0x02        //隐藏
#define DIRATTR_SYSTEM          0x04        //系统
//#define DIRATTR_VOLUME_ID       0x08        //卷标
//#define DIRATTR_DIRECTORY       0x10        //目录
#define DIRATTR_ARCHIVE         0x20        //文档
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
int AddFile2Img(char *strImgName, char *strPathName, unsigned char ucFileAttr);


/*  功能：
        从镜像中删除文件
    参数表：
        strImgName  = 镜像文件名
        strFileName = 目标文件名
    返回值：
        无
*/
void DeleteFileFromImg(char *strImgName, char *strFileName);
//****************************************************************************************


#endif
