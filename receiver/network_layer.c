#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include<stdlib.h>


#define MAX_PKT 1024
#define KEY 450

typedef struct{
    long type;
    char data[MAX_PKT];
}Msg;


//网络层调用, 输入包的内容

int main()
{
    //创建消息队列
    int msgid = msgget(KEY,IPC_CREAT|0777);
    if(msgid<0)
    {   
        perror("msgget error!");
        exit(-1);
    }   
    //printf("%d\n", msgid);

    Msg m;
    
    while(true){
        printf("输入网络层想要发送的包的内容: ");
        scanf("%s", m.data);
        m.type = 1;
        msgsnd(msgid,&m,sizeof(m)-sizeof(m.type),0);
    }
    return 0;
}
