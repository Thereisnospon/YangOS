


```
nop 
mov ax,$
mov ax,$$
```


反汇编

00000000  90                nop
00000001  B80100            mov ax,0x1
00000004  B80000            mov ax,0x0

```
org 0xc900
nop 
mov ax,$
mov ax,$$
```

反汇编

```
0000C900  90                nop
0000C901  B801C9            mov ax,0xc901
0000C904  B800C9            mov ax,0xc900
```

