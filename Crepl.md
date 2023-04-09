# Crepl

## 实现功能

- 解析c语言表达式（int类型），给出答案
- 解析c语言函数（以int开头）并记录，可以供以后的表达式调用

## 实现方式

1. 一个基本的大框架，即一个while循环用于不断接收输入
2. 讲所有的输入编译入共享库，这个过程又分两种情况
   - 对于函数，直接存入.c文件，再编译入共享库
   - 对于表达式，用一个函数的框架来存入表达式，将表达式直接放在return部分即可，然后将这个函数写入.c文件再编译进共享库

3. 用dlopen函数打开共享库和用dlsym函数调用表达式函数，即可获得结果并输出

## 具体细节

### 写入.c文件

需要用到string.h中的部分库函数

还需要c语言文件读写的知识

都比较简单,不再赘述

在本程序中,所以写就的.c文件都放在tem目录中

### 编译.c文件为共享库

只需要一个指令

```shell
gcc -shared -fpic -w -o 库名.so firename.c 
```

### 在c程序中调用gcc

#### execvp

使用的是execvp函数,用此函数调用shell脚本来执行gcc命令,就可以得到共享库

```c
#include <unistd.h>

int execvp(const char* file, char* argv[]);

//file即调用文件的路径
//argv是一个字符串数组代表传入给file的参数,需要以NULL结尾,比如定义一个argv[] = {"./a.sh","./test.c",NULL};则"./test.c"即shell脚本的第一个参数
```

#### fork

该函数会直接用file来接管当前进程,所以如果该函数运行成功的话,不会向后运行,所以我们需要使用fork函数来复制一个子进程,在子进程中调用execvp函数,这样程序就不会停止

```c
#include <unistd.h>
pid_t fork(void);
//该函数的作用是创建一个新的进程，称为子进程。这个新的进程将会和父进程拥有相同的程序代码。在子进程中，fork() 函数返回值为 0，而在父进程中，返回值为新进程的进程 ID（PID）。如果出现错误，则返回值为 -1。
```

#### wait

在子进程中调用execvp函数同时我们需要在主进程中调用wait函数来保证子进程成功完成,否则可能会导致动态链接时出错,并且使用wait函数也可以处理一下输入的函数或者表达式语法错误,保证程序正确运行.

```c
#include <sys/wait.h>
pid_t wait(int* status);
```

可以用wait.h中的相关宏判断status

| WIFEXITED(status)   | 子进程正常退出返回true否则false      |
| ------------------- | ------------------------------------ |
| WEXITSTATUS(status) | 当正常退出时，返回exit(code)中的code |

#### 例子

以下是本程序使用的一个例子,用于具体阐述我对这几个函数的理解

```c
#include <unistd.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>
pid_t fid;    //用于创建子进程
char * a = ".//a.sh";    //用于编译共享库
char * arg[] = {NULL};
int status2;
//以上为定义和声明

				fid = fork();    //fork函数创建子进程
                if(fid < 0)    //子进程创建失败
                {
                    printf("FunctionInputError\n");
                    continue;
                }
                else if(fid == 0)    //子进程中调用gcc
                {
                    execvp(a, arg);
                    exit(1);    //如果execvp函数调用成功的话,不会走到这一步,则子进程正常exit为0,对应下面父进程中的if判断
                }
                else if(fid > 0)    //父进程
                {
                    wait(&status2);    //如果子进程没有完成,会阻塞在这里
                    if (WIFEXITED(status2) && WEXITSTATUS(status2) == 0)     //用两个宏来判断子进程退出的状态
                    {
                        //printf("Child process exec() success!\n");
                    }
                    else 
                    {
                        printf("函数定义错误\n");
                    }
                }
```

#### 具体补充

fork调用gcc进行编译时,如果.c文件有语法错误,也就是子进程中execvp执行失败,则我们需要在父进程判断得出子进程失败时不仅要输出错误,还要再进行execvp调用将错误的.c文件删除,即再进行一个fork,在本程序中也使用过这个.

### 链接动态库

在链接动态库时使用两个函数dlopen和dlsym,前者用于链接动态库,后者用于使用动态库中的函数.

放上提供的网址[dlopen](https://man7.org/linux/man-pages/man3/dlopen.3.html)   [dlsym](https://man7.org/linux/man-pages/man3/dlsym.3.html)

需要注意的有三点

1. 由于函数和改写的表达式函数都是单独的.c文件,有可能表达式函数中使用了其他文件中的函数,这时想法可能是会编译失败,实际上只会警告,不会错误,也就是可以生成共享库.为了让gcc编译更好看,使用了-w选项来忽略警告
2. 在使用dlsym引用共享库的函数时,如果引用函数中同时引用了其他函数,会自动的加载入内存,不需要再调用dlsym,这就说明我只需要dlsym函数引用我改写的表达式函数就可以了,只要共享库中有表达式使用的函数,就自动计算.所以写入的函数只需要编入共享库就行
3. 使用dlopen函数时,由于本程序的共享库是在不断地丰富和改变的,所以一定要在改while循环中用dlclose断开该共享库的链接,下次再dlopen打开,这样才能保证链接始终是最新的共享库.

## 编写程序过程中遇到的问题

1. fork函数打开的子进程和父进程同时运行,本来未使用wait函数,会导致链接动态库时子进程还没编译好动态库,这是一个问题
2. 经常忽视指针的置空问题,忘了将用完的指针设为NULL,这可能是导致我写程序前期它经常崩溃的原因
3. 忽视了dlclose函数,和fclose函数,由于这些文件都不断地在变化,所以需要记得把它关闭,这很重要...尤其是dlclose函数,曾经程序始终只能输出第一个表达式,后来才知道是dlclose的问题
4. 不足之处在于关于链接和库的知识很丰富,本次并没有细致的学习,csapp和程序员的自我修养还需要再学习学习

## 我是怎么测试的??

1. 也就是使用单步调试等方式,保证程序的细节没有问题
2. 对于execvp,fork,dlopen,dlsym等不好测试的函数,我在编写程序时根据不同的情况设置了不同的输出,来判断当前程序是哪种情况,有什么问题
3. 对于程序的正确性,我是手动输入了一些表达式和函数,并没有写个脚本来测试,这个技能也应该是我所需要学习的