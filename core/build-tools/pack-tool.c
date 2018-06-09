#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP))
struct item
{
    char name[16];
    long start;
    long length;
};

int file_size(char *file_name)
{
    struct stat statbuf;
    stat(file_name, &statbuf);
    int size = statbuf.st_size;
    return size;
}

char *item_path[64];
struct item item_array[64];
char ZERO_A[512] = {0};

int write_index(FILE *f, struct item array[], int len, int write_size)
{
    int sz = sizeof(struct item);
    int re = 0;
    fwrite(array, sz, len, f);
    if (sz * len > write_size)
    {
        return 1;
    }
    else
    {
        re = write_size - sz * len;
    }
    while (re > 0)
    {
        if (re >= 512)
        {
            fwrite(ZERO_A, 1, 512, f);
            re -= 512;
        }
        else
        {
            fwrite(ZERO_A, 1, 512 - re, f);
            re = 0;
        }
    }
    return 0;
}

int write_f(FILE *write_file, char *path, struct item item)
{
    FILE *read_file = fopen(path, "r");
    if (read_file == NULL)
    {
        return 1;
    }
    int read_file_length = file_size(path);
    if (read_file_length <= 0)
    {
        return 2;
    }
    int sectors = DIV_ROUND_UP(read_file_length, 512);
    char buf[512];
    int len = 0;
    int sum = 0;
    while ((len = fread(buf, 1, 512, read_file)) > 0)
    {
        if (fwrite(buf, 1, len, write_file) != len)
        {
            return 3;
        }
        sum += len;
    }
    int re = sectors * 512 - item.length;
    if (fwrite(buf, 1, re, write_file) != re)
    {
        return 4;
    }
    fclose(read_file);
    return item.length - sum;
}

char *path_name(char *str)
{
    char *p = str;
    while (p != NULL && *p != '\0')
        p++;
    while (p != NULL && *p != '/' && p != str)
        p--;
    if (p == NULL)
        return NULL;
    return *p == '/' ? ++p : (p == str ? p : NULL);
}

int get_f(char *path, struct item *item)
{
    struct stat statbuf;
    stat(path, &statbuf);
    int size = statbuf.st_size;
    char *name = path_name(path);
    if (size > 0 && name != NULL)
    {
        strcpy(item->name, name);
        item->length = size;
        return 1;
    }
    else
    {
        item->length = -1;
        return 0;
    }
}

void create(char *paths[], int len, char *write_to)
{
    // char *file_path = "/Users/yzr/repos/YangOS/core/build/boot.bin";
    // char *paths[10];
    // paths[0] = "/Users/yzr/repos/YangOS/core/build/boot.bin";

    int i = 0, j = 1;

    struct item *meta = &item_array[0];
    item_path[0] = "thereisnospon";
    strcpy(meta->name, "thereisnospon");
    meta->start = 0;
    meta->length = 2048;

    struct item *pre = meta;
    struct item *cur = NULL;

    for (i = 0, j = 1; i < len && j < 64 - 1; i++)
    {
        cur = &item_array[j];
        if (get_f(paths[i], cur))
        {
            cur->start = pre->start + DIV_ROUND_UP(pre->length, 512);
            item_path[j] = paths[i];
            //printf("path:%s name:%s, start:%d length:%d\n", paths[i], cur->name, cur->start, cur->length);
            pre = cur;
            j++;
        }
    }

    FILE *out = fopen(write_to, "w");
    write_index(out, item_array, 64, 2048);

    for (i = 1; i < j; i++)
    {
        struct item *p = item_array + i;
        printf("path %s ,name :%s start:%d length:%d\n", item_path[i], p->name, p->start, p->length);
        if (write_f(out, item_path[i], *p) != 0)
        {
            printf("not write %s\n", item_path[i]);
            exit(1);
        }
    }

    fclose(out);
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "size") == 0)
    {
        int sz = file_size(argv[2]);
        if (sz > 0)
        {
            printf("%d", sz);
        }
        else
        {
            printf("file error %s\n", argv[2]);
            exit(1);
        }
        return 0;
    }
    if (argc < 4)
    {
        printf("too few arg %d\n", argc);
        exit(1);
    }

    int i = 0;

    while (i < argc)
    {
        printf("%d arg %s\n", i, argv[i]);
        i++;
    }

    if (strcmp(argv[1], "create") == 0)
    {
        char *create_path = argv[2];
        char *create_name = path_name(create_path);
        int plen = argc - 2;
        char **paths = argv + 3;
        if (create_name == NULL || strcmp("prog-pack.img", create_name) != 0)
        {
            printf("path name %s not prog-pack.img\n", argv[2]);
            exit(1);
        }
        create(paths, plen, create_path);
    }
    // else if (strcmp(argv[1], "append") == 0)
    // {
    //     char *create_path = argv[2];
    //     char *create_name = path_name(create_path);
    //     char *append_path=argv[3];

    //     if (create_name == NULL || strcmp("prog-pack.bin", create_name) != 0)
    //     {
    //         printf("path name %s not prog-pack.bin\n", argv[2]);
    //         exit(1);
    //     }
    //     read_add(create_path,append_path);
    // }
    else
    {
        printf("not support mode arg %s\n", argv[1]);
        exit(1);
    }
    return 0;
}

// void read_add(char *file_path, char *f)
// {
//     FILE *file = fopen(file_path, "r");

//     if (file == NULL)
//     {
//         printf("no such file%s\n", file_path);
//         return;
//     }

//     FILE *in = fopen(f, "r");
//     if (in == NULL)
//     {
//         printf("no such file %s\n", f);
//         return;
//     }

//     int zsize = file_size(file_path);
//     if (zsize <= 0)
//     {
//         printf("%s size error\n", file_path);
//         return;
//     }
//     int rsize = file_size(f);
//     if (rsize <= 0)
//     {
//         printf("%s size error\n", f);
//     }
//     struct item add_item;
//     if (!get_f(f, &add_item))
//     {
//         printf("path name error %s\n", f);
//     }

//     struct item array[64];
//     struct item *meta = array;
//     printf("rsize %d zsize %d\n", rsize, zsize);
//     printf("1\n");
//     int len = 0;
//     if ((len = fread(array, sizeof(struct item), 1, file)) != 1)
//     {
//         printf("no meta data %d\n", len);
//     }

//     if (strcmp(meta->name, "thereisnospon") != 0 || meta->start < 0 || meta->length > 2048 || meta->length < 0)
//     {
//         printf("error meta \n");
//         return;
//     }
//     printf("2\n");
//     int sz = meta->length - sizeof(struct item);

//     if ((len = fread(array + 1, 1, sz, file) != sz))
//     {
//         printf("read error\n");
//         return;
//     }
//     int last = -1, i = 0;
//     for (i = 1; i < 64; i++)
//     {
//         struct item it = array[i];
//         if (it.start <= 0 || it.length <= 0)
//         {
//             break;
//         }
//         last = i;
//     }
//     printf("3\n");
//     if (last <= 0 || last > 63)
//     {
//         printf("error\n");
//         return;
//     }
//     char buf[512];
//     char name[512];
//     strcat(name, file_path);
//     strcat(name, "_tmp");
//     printf("4\n");
//     add_item.start = array[last].start + DIV_ROUND_UP(array[last].length, 512);
//     array[last + 1] = add_item;
//     printf("45dc\n");
//     FILE *wf = fopen(name, "w");

//     if ((len = fwrite(array, sizeof(struct item), 64, wf)) != 64)
//     {
//         printf("write error 1\n");
//     }
//     int sum = 0;
//     printf("5\n");

//     while ((len = fread(buf, 1, 512, file)) > 0)
//     {
//         printf("5 1\n");
//         if (fwrite(buf, 1, len, wf) != len)
//         {

//             printf("write error 2\n");
//         }
//         printf("5 3\n");
//         sum += len;
//     }
//     printf("6\n");
//     if (sum != zsize - meta->length)
//     {
//         printf("write sum %d zsize %d meta-length %d error 3\n", sum, zsize, meta->length);
//     }
//     sum = 0;
//     while ((len = fread(buf, 1, 512, in)) > 0)
//     {
//         if (fwrite(buf, 1, len, wf) != len)
//         {
//             printf("write error 2\n");
//         }
//         sum += len;
//     }
//     printf("7\n");
//     if (sum != rsize)
//     {
//         printf("write sum %d rsize %d\n", sum, rsize);
//     }
//     remove(file_path);
//     rename(name, file_path);
//     fclose(wf);
//     fclose(file);
// }