#ifndef __SHELL_BUILDIN_CMD_H
#define __SHELL_BUILDIN_CMD_H
#include "stdint.h"
void buildin_ls(uint32_t argc, char **argv);
char *buildin_cd(uint32_t argc, char **argv);
int32_t buildin_mkdir(uint32_t argc, char **argv);
int32_t buildin_rmdir(uint32_t argc, char **argv);
int32_t buildin_rm(uint32_t argc, char **argv);
void make_clear_abs_path(char *path, char *wash_buf);
void buildin_pwd(uint32_t argc, char **argv);
void buildin_ps(uint32_t argc, char **argv);
void buildin_clear(uint32_t argc, char **argv);
void try_write(char *name, int sector, uint32_t file_size);
#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP))
struct item
{
    char name[16];
    long start;
    long length;
};
void read_img_files(int start_sector);
void uprog_init();
#endif