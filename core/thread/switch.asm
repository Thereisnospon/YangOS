
[bits 32]
section .text
global switch_to
switch_to:
    ;栈中此处是返回地址
    push esi 
    push edx 
    push ebx 
    push ebp 

    mov eax,[esp+20];得到栈中的参数 cur,cur=[esp+20]
    mov [eax],esp ;保存栈顶指针 esp ， task_struct 的 self_kstack 字段.
                  ; self_kstack 在 task_struct 偏移为 0
                  ; 所以在 thread 开头处存放 4 字节就可以了

    ; 以上保存当前线程环境,以下恢复下一个线程环境

    mov eax,[esp+24] ;得到栈中参数 next,next=[esp+24]
    mov esp,[eax] ; pcb 第一个成员是 self_kstack
                  ; 它用来记录 0 级栈指针，被换上cpu时，用来恢复0级栈
                  ; 0 级栈保存了进程和线程所有信息，包括 3 级栈
    pop ebp 
    pop ebx 
    pop edx 
    pop esi 

    ret

; // ;       <------------------->  pcb 低端
; // ;    ↓←←←←←| self_stack  | step5: 把 pcb 的 self_stack 指向 当前 esp
; // ;    ↓   <--+-------------+-->
; // ;    ↓     |             |
; // ;    ↓     |             |
; // ;    ↓     |             |
; // ;    ↓    -+-------------+-->
; // ;    ↓
; // ;    ↓
; // ;    ↓→→→→→--           --    <-------------------------- 现在的 esp 地址
; // ;          |    ebp      |  <--- step4: push ebp
; // ;          --           -- 
; // ;          |    ebx      |  <--- step3: push ebx
; // ;          --           --
; // ;          |    edi      |  <--- step2: push edi
; // ;          --           --
; // ;          |    esi      |  <---- step1 : push esi
; // ;          --           --  <----  某个时候 执行到 switch_to 此时的 esp
; // ;          |    eip      |  <----  switch_to 之前压入的返回地址。 可能是 kernel_thread 也可能是 switch_to 返回地址 
; // ;          --           --   


; thread_stack 仅仅是数据的结构，只是按照它的格式 依次保存数据。 这个数据结构保存在哪里不固定（不一定在pcb中）
; 将内存映像压栈后，以后需要知道内存映像被压到了哪里（不一定在pcb，取决于当前 esp)
; // ;       <-------------------> pcb 低端
; // ;    ↓←←←←←| self_stack  |  恢复 step1: <-------- 拿到了 slef_stack,然后根据它索引到了
; // ;    ↓   <--+-------------+-->
; // ;    ↓     |             |
; // ;    ↓     |             |
; // ;    ↓     |             |
; // ;    ↓→→→→-+-------------+--> 恢复 step2: <------- esp 被加载到了这里
; // ;          |thread_stack |
; // ;          --           --
; // ;          |    ebp      | 恢复 step3: pop ebp    <-----esp
; // ;          --           --
; // ;          |    ebx      | 恢复 step4: pop ebx    <-----esp
; // ;          --           --
; // ;          |    edi      | 恢复 step5: pop edi    <-----esp
; // ;          --           --
; // ;          |    esi      | 恢复 step6: pop esi    <-----esp
; // ;          --           --
; // ;          |             | 恢复 step7: ret, 返回到 eip
; // ;          |             |  线程第一次执行时，eip 指向待调用的函数 kernel_thread   
; // ;          |             |     因此，开始调用 function 函数
; // ;          |    eip      |  其他时候，eip 指向 switch_to 的返回地址
; // ;          |             |     继续执行 scudule, 时间中断处理函数，然后退出中断，继续执行切换好的任务
; // ;          |             |
; // ;          --           --
; // ;          |   unused    |
; // ;          --           --
; // ;          |   function  |
; // ;          --           --
; // ;          |   func_arg  |
; // ;        <--+-------------+-->
; // ;          |             | 初始情况下，此栈在线程自己的内核栈中固定，在 pcb 页顶端，每次进入中断时并不一样
; // ;          | intr_stack  | 如果进入中断不涉及 特权级变换，位置在 esp 之下，否则在 tss 中获取
; // ;          |             |
; // ;       <--+-------------+--> pcb 顶端


; struct task_struct           
; {
;     uint32_t *self_kstack;    
;         struct thread_stack  
;         {
;             uint32_t ebp;    
;             uint32_t ebx;    
;             uint32_t edi;    
;             uint32_t esi;     
;             void (*eip)(thread_func *func, void *func_arg);  
;             void(*unused_retaddr);
;             thread_func *function; 
;             void *func_arg;        
;         };
;     enum task_status status;
;     char name[16];
;     uint8_t priority;             
;     uint8_t ticks;                
;     uint32_t elasped_ticks;        
;     struct list_elem general_tag;  
;     struct list_elem all_list_tag; 
;     uint32_t *pgdir;              
;     uint32_t stack_magic;
; };