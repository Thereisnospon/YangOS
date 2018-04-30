#include <stdio.h>
int go()
{
    return 1230;
}
int main()
{
    int a = 10, b = 100;
    //将保存输入操作数的寄存器中的数 拷贝到 eax
    //将 eax 的内容拷贝到 输出操作数对应的寄存器
    //然后将 输出操作数对应寄存器内容 更新到 输出操作数的变量
    asm(" movl %1,%%eax;  \
          movl %%eax,%0;" //
        : "=r"(b)         // %0 :-> 第一个输出操作数  ‘=’ 号表示 只写 （输出操作数特定约束）
        : "r"(a)          // 第一个输入操作数。 2 - 1 =1  ：-> %1
        : "%eax");        //用于列出指令中涉及到的且没出现在output operands字段及input operands字段的那些寄存器这些寄存器可能会被内联汇编命令改写。
                          //因此，执行内联汇编的过程中，这些寄存器就不会被gcc分配给其它进程或命令使用。
    printf("%d %d\n", a, b);

    /*
        寄存器约束
        当操作数被指定为这类约束时，表明汇编指令执行时，操作数被将存储在指定的通用寄存器（General Purpose Registers, GPR）中
        asm ("movl %%eax, %0\n" : "=r"(out_val));
        该指令的作用是将%eax的值返回给%0所引用的C语言变量out_val，
        根据"=r"约束可知具体的操作流程为：先将%eax值复制给任一GPR，
        最终由该寄存器将值写入%0所代表的变量中。
        "r"约束指明gcc可以先将%eax值存入任一可用的寄存器，然后由该寄存器负责更新内存变量。

        通常还可以明确指定作为“中转”的寄存器，约束参数与寄存器的对应关系为：
            a : %eax, %ax, %al
            b : %ebx, %bx, %bl
            c : %ecx, %cx, %cl
            d : %edx, %dx, %dl
            S : %esi, %si
            D : %edi, %di
    */
    int w = 0;
    int p = go();
    asm("movl %%eax, %0;" // %0 :-> 第一个输出操作数 
        : "=r"(w));     //由 任意通用寄存器 (r) 将数据负责把内容更新到 输出操作数 w
    printf("%d\n", w);

    /*
        内存操作数约束
        当我们不想通过寄存器中转，而是直接操作内存时，可以用"m"来约束。例如：
        asm volatile ( "lock; decl %0" : "=m" (counter) : "m" (counter));
        该指令实现原子减一操作，输入、输出操作数均直接来自内存（也正因如此，才能保证操作的原子性）。    
    */
    int counter=1;
    asm volatile("lock; incl %0"
                 : "=m"(counter)
                 : "m"(counter));
    printf("%d\n",counter);

    /*
        关联约束
        在有些情况下，如果命令的输入、输出均为同一个变量
        则可以在内联汇编中指定以matching constraint方式分配寄存器
        此时，input operand和output operand共用同一个“中转”寄存器。例如 ：

        asm("decl %0": "=a"(var): "0"(var));

        该指令对变量var执行incl操作
        由于输入、输出均为同一变量，因此可用"0"来指定都用%eax作为中转寄存器。
        注意"0"约束修饰的是input operands。
    */
    int var=100;
    asm("decl %0"
        : "=a"(var)
        : "0"(var));
    printf("%d\n",var);

    int in_a=1,in_b=2,out_sum;
    asm("addl %%ebx,%%eax;":"=a"(out_sum):"a"(in_a),"b"(in_b));
    printf("out_sum:%d\n",out_sum);
    return 0;
}

