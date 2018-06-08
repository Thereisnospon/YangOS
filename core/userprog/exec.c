#include "exec.h"
#include "thread.h"
#include "stdio-kernel.h"
#include "fs.h"
#include "string.h"
#include "global.h"
#include "memory.h"

extern void intr_exit(void);
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* 32位elf头 */
struct Elf32_Ehdr
{
    unsigned char e_ident[16];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
};

/* 程序头表Program header.就是段描述头 */
struct Elf32_Phdr
{
    Elf32_Word p_type; // 见下面的enum segment_type
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
};

/* 段类型 */
enum segment_type
{
    PT_NULL,    // 忽略
    PT_LOAD,    // 可加载程序段
    PT_DYNAMIC, // 动态加载信息
    PT_INTERP,  // 动态加载器名称
    PT_NOTE,    // 一些辅助信息
    PT_SHLIB,   // 保留
    PT_PHDR     // 程序头表
};

/* 将文件描述符fd指向的文件中,偏移为offset,大小为filesz的段加载到虚拟地址为vaddr的内存 */
static bool segment_load(int32_t fd, uint32_t offset, uint32_t filesz, uint32_t vaddr)
{
   

    uint32_t vaddr_first_page = vaddr & 0xfffff000;               // vaddr地址所在的页框
    uint32_t size_in_first_page = PG_SIZE - (vaddr & 0x00000fff); // 加载到内存后,文件在第一个页框中占用的字节大小
    uint32_t occupy_pages = 0;
    /* 若一个页框容不下该段 */
    if (filesz > size_in_first_page)
    {
        uint32_t left_size = filesz - size_in_first_page;
        occupy_pages = DIV_ROUND_UP(left_size, PG_SIZE) + 1; // 1是指vaddr_first_page
    }
    else
    {
        occupy_pages = 1;
    }

    /* 为进程分配内存 */
    uint32_t page_idx = 0;
    uint32_t vaddr_page = vaddr_first_page;
    while (page_idx < occupy_pages)
    {
        uint32_t *pde = pde_ptr(vaddr_page);
        uint32_t *pte = pte_ptr(vaddr_page);
      
        /* 如果pde不存在,或者pte不存在就分配内存.
       * pde的判断要在pte之前,否则pde若不存在会导致
       * 判断pte时缺页异常 */
        if (!(*pde & 0x00000001) || !(*pte & 0x00000001))
        {
            if (get_a_page(PF_USER, vaddr_page) == NULL)
            {
                return false;
            }
        } // 如果原进程的页表已经分配了,利用现有的物理页,直接覆盖进程体
        else{
        }
        vaddr_page += PG_SIZE;
        page_idx++;
    }
    sys_lseek(fd, offset, SEEK_SET);
    sys_read(fd, (void *)vaddr, filesz);
    return true;
}

/* 从文件系统上加载用户程序pathname,成功则返回程序的起始地址,否则返回-1 */
static int32_t load(const char *pathname)
{
    int32_t ret = -1;
    struct Elf32_Ehdr elf_header;
    struct Elf32_Phdr prog_header;
    memset(&elf_header, 0, sizeof(struct Elf32_Ehdr));
    
    int32_t fd = sys_open(pathname, O_RDONLY);
   
    if (fd == -1)
    {
        return -1;
    }


  
    if (sys_read(fd, &elf_header, sizeof(struct Elf32_Ehdr)) != sizeof(struct Elf32_Ehdr))
    {
        ret = -1;
        goto done;
    }
   

    /* 校验elf头 */
    if (memcmp(elf_header.e_ident, "\177ELF\1\1\1", 7) || elf_header.e_type != 2 || elf_header.e_machine != 3 || elf_header.e_version != 1 || elf_header.e_phnum > 1024 || elf_header.e_phentsize != sizeof(struct Elf32_Phdr))
    {
        ret = -1;
        goto done;
    }

    Elf32_Off prog_header_offset = elf_header.e_phoff;
    Elf32_Half prog_header_size = elf_header.e_phentsize;

   
    /* 遍历所有程序头 */
    uint32_t prog_idx = 0;
    while (prog_idx < elf_header.e_phnum)
    {
        memset(&prog_header, 0, prog_header_size);

        /* 将文件的指针定位到程序头 */
        sys_lseek(fd, prog_header_offset, SEEK_SET);
        /* 只获取程序头 */
        if (sys_read(fd, &prog_header, prog_header_size) != prog_header_size)
        {
            ret = -1;
            goto done;
        }
        /* 如果是可加载段就调用segment_load加载到内存 */
        if (PT_LOAD == prog_header.p_type)
        {
             if (!segment_load(fd, prog_header.p_offset, prog_header.p_filesz, prog_header.p_vaddr))
            {
                ret = -1;
                goto done;
            }
        }

        /* 更新下一个程序头的偏移 */
        prog_header_offset += elf_header.e_phentsize;
        prog_idx++;
    }
    ret = elf_header.e_entry;
done:
    sys_close(fd);
    return ret;
}

/*


关于堆内存起始设置在 0x8048000 位置的问题。

由于加载程序起始位置也是 0x8048000, 而之前程序的分配的内存信息在 0x8048000附近。
加载的程序把之前的 内存分配元信息覆盖，会产生问题。
所以，后面必须要改掉堆内存分配的问题。

bug 复现

getCwd 忘记释放一个 512 字节的内存块

概念：
内存仓库: arena 
arena 占据一个自然页框 
arena 页的开始部分是本仓库分配的元信息。元信息后面跟着 若干个 同样大小的 内存块。（16，32，64，512，1024
每次分配内存块时，先查找是否有与它相近大小的 空闲内存块，如果有，就分配出去（修改元信息），没有就新建一个内存仓库
每次释放内存块时，修改内存仓库的元信息，如果当前内存仓库的所有内存块都被释放，就释放整个内存仓库，并释放整个页框，解除页和物理地址的映射。

先看正常情况 :

step1: 申请一个 512 ，1024 单位的内存块

 0x8048000 ->  arena1[ cnt 7 / size 512  （之前没有512单位的内存块，在 0x8048000 新建一个内存仓库 （有7个空闲
 0x8049000 ->  arena2[ cnt 3 / size 1024  (没有1024大小的内存块，新建一个内存仓库
    arena1 -- 
    arena2 -- 
    arena1 += 0x8048000 释放 (刚创建的 arena1 所有内存块释放，整个内存仓库释放，页也释放
    arena2 ++ 0x8049000 释放 (arena2 同样释放

step2: 需要分配两页，（这里不是分配内存单元。） 需要的虚拟地址为 0x8048000,0x8049000

getApage -> 0x8048000 之前被释放了，所以分配到了 0x8048000
getApage -> 0x8049000 之前被释放了,紧接着0x8048000 分配了 0x8049000

step3: 申请一个 512 ，1024 单位的内存块
 0x804A000 ->  arena1[ cnt 7 / size 512 (紧接着0x8049000在 0x804A000 新建一
 0x804B000 ->  arena2[ cnt 3 / size 1024 (紧接着0x804A000在  0x804B000 新建一个内存仓库
    arena1 -- 
    arena2 -- 

step4: 读入文件到内存 0x8048000 ~ 0x8049044 (大小0x1044)
读入 0x1044 -> 0x8048000 ~ 0x8049044

    arena1 +=  0x804A000 (free) 
    arena2 ++  0x804B000 (free)

每次都把内存仓库最后一个内存块释放，每次创建新的内存仓库获取内存块.
最后读入文件到内存的时候，0x8048000 ~ 0x8049044  并没覆盖到之前的分配元信息，0x804A000~


bug 情况。

step1: 申请一个 512 ，1024 单位的内存块

 0x8048000 ->  arena1[ cnt 6 / size 512 （之前有个512单位的内存单元没有释放，所以用原来的仓库分配
 0x8049000 ->  arena2[ cnt 3 / size 1024 （新建
    arena1 -- 
    arena2 --
    arena1 ++  还有内存单元，不释放
    arena2 ++  释放内存仓库

step2: 需要分配两页，（这里不是分配内存单元。） 需要的虚拟地址为 0x8048000,0x8049000
getApage -> 0x8048000 已经存在了。决定用现有的 （代码逻辑。有现有的页，不申请新的。。。   
getApage -> 0x8049000 

step3: 申请一个 512 ，1024 单位的内存块
0x8048000 ->  arena1[ cnt 6 / size 512 （现有的
0x804A000 ->  arena2[ cnt 3 / size 1024 (创建
    arena1 -- 
    arena2 --

step4: 读入文件到内存 0x8048000 ~ 0x8049044 (大小0x1044)（破坏了 内存仓库元信息)
读入 读入 0x1044 -> 0x8048000 ~ 0x8049044 

free(addr2 （元信息在0x804A000) 成功
free(addr1  (元信息在0x8048000，已经被破坏，)失败。 

由于不是每次创建新的内存仓库，导致新的内存仓库在 被用户程序覆盖的内存范围之内

*/
int32_t sys_execv(const char *path, const char *argv[])
{
  
   
    uint32_t argc = 0;
    while (argv[argc])
    {
        argc++;
    }
  
    int32_t entry_point = load(path);
    if (entry_point == -1)
    { // 若加载失败则返回-1
        return -1;
    }
  
    struct task_struct *cur = running_thread();
    /* 修改进程名 */
    memcpy(cur->name, path, TASK_NAME_LEN);
    cur->name[TASK_NAME_LEN - 1] = 0;

    struct intr_stack *intr_0_stack = (struct intr_stack *)((uint32_t)cur + PG_SIZE - sizeof(struct intr_stack));
    /* 参数传递给用户进程 */
    intr_0_stack->ebx = (int32_t)argv;
    intr_0_stack->ecx = argc;
    intr_0_stack->eip = (void *)entry_point;
    /* 使新用户进程的栈地址为最高用户空间地址 */
    intr_0_stack->esp = (void *)0xc0000000;

    /* exec不同于fork,为使新进程更快被执行,直接从中断返回 */
    asm volatile("movl %0, %%esp; jmp intr_exit"
                 :
                 : "g"(intr_0_stack)
                 : "memory");
    return 0;
}
//****************************************************************************************************************
//*      exec 流程
//****************************************************************************************************************
// ;          --           --
// ;          | 0 | ss_old  |  <-----  发送特权级切换 压入的旧栈段 ss   (cpu压入， 执行 sys_exc 前的进程的 栈段)
// ;          --           --
// ;          |   esp_old   |  <-----  发送特权级切换 压入的旧栈指针 esp (cpu压入) 修改为新用户进程栈地址 0xc0000000)
// ;          --           --
// ;          |  EFLAGS     |  <----- 标识位寄存器 (cpu压入))
// ;          --           --
// ;          |  0 | cs_old |  <----- (cpu压入) 之前进程的cs，3 特权级)
// ;          --           --
// ;          |   eip_old   |  <----- (cpu压入 修改为了 entry_point
// ;          --           --
// ;          |  error_code |  step4: add esp,4 跳过 error_code
// ;          --           --
// ;          |    ds       | pop ds
// ;          --           --
// ;          |    es       | pop es
// ;          --           --
// ;          |    fs       | pop fs
// ;          --           --
// ;          |    gs       | pop gs step3： pop gs,fs,es,ds 等
// ;          --           -- ←←←←←←←←←←↑ <------ step2: popad 时，esp 重新指向 gs
// ;          |    eax      |           ↑
// ;          --           --           ↑
// ;          |    ecx      |           ↑
// ;          --           --           ↑
// ;          |    edx      |           ↑
// ;          --           --           ↑
// ;          |    ebx      |           ↑
// ;          --           --           ↑
// ;          |    esp      | →→→→→→→→→→↑ 压入 pushad 时，esp 应该指向的是 gs 的位置
// ;          --           --
// ;          |    ebp      |
// ;          --           --
// ;          |    esi      |
// ;          --           --
// ;          |    edi      |
// ;          --           --
// ;          |  intr_no    | <------- step1 : add esp,4 跳过中断向量号 // jmp intr_exit 时， esp 指向了这里
// ;          --           --
// ;          |  ret_addr   | 
// ;          --           --
// ;          |             |
// ;          --           --