#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void sig_user(int signo) {
    int status;
    switch (signo)
    {
    case SIGUSR1:
        printf("收到了 SIGUER1 信號，進程 ID = %d!, 開始休息 10 秒\n", getpid());
        sleep(10);
        printf("收到了 SIGUER1 信號，進程 ID = %d!, 休息 10 秒結束!\n", getpid());
        break;
    case SIGUSR2:
      printf("收到了 SIGUSR2 信號，進程 ID = %d!, 開始休息 10 秒\n", getpid());
        sleep(10);
        printf("收到了 SIGUSR2 信號，進程 ID = %d!, 休息 10 秒結束!\n", getpid());
        break;
    case SIGCHLD:
        printf("收到了 SIGCHLD 信號，進程 ID = %d!\n", getpid());
        pid_t pid = waitpid(-1,&status,WNOHANG);

        if (pid==0)
            return;
        if  (pid==-1)
            return;

        return;
        break;
    }
}


int main(int argc, char* const *argv) {
    printf("進程開始執行\n");
    // pid_t pid;
    //處理簡單進程
    if (signal(SIGUSR1, sig_user)== SIG_ERR){
        printf("無法捕捉 SIGUSR1 信號!\n");
        exit(1);
    }
     if (signal(SIGUSR2, sig_user)== SIG_ERR){
        printf("無法捕捉 SIGUSR2 信號!\n");
        exit(1);
    }

    //處理子進程
    if (signal(SIGCHLD, sig_user) == SIG_ERR){
        printf("無法捕捉 SIGCHLD 信號!\n");
        exit(1);
    }

    while (1)
    {
        sleep(1);
        printf("休息 1 秒, 進程 ID = %d\n",getpid());
    }

    printf("進程退出\n");
    return 0;
}