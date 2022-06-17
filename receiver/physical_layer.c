#include "GoBackN.h"


/***********************物理层********************/
void pl_connect(int *sockfd, struct sockaddr_in *addr){

	*sockfd = socket(AF_INET, SOCK_DGRAM, 0);
      if(-1==*sockfd){
        return;
        puts("Failed to create socket");
      }
      socklen_t          addr_len=sizeof(*addr);
      memset(addr, 0, sizeof(*addr));
      addr->sin_family = AF_INET;
      addr->sin_port   = htons(8080);
      addr->sin_addr.s_addr = htonl(INADDR_ANY);

      // Bind 端口，用来接受之前设定的地址与端口发来的信息,作为接受一方必须bind端口，并且端口号与发送方一致
      if (bind(*sockfd, (struct sockaddr*)addr, addr_len) == -1){
        printf("Failed to bind socket on port\n");
        close(*sockfd);
        return ;
      }
}


void Sendto(frame s){
  //15%的概率出现传输错误
    srand((unsigned)time(NULL));
    int iserror = rand() % 100 + 1; //产生1~100的随机数
    if(iserror <= 15){
        s.kind = err;
    }
  
    char buf[sizeof(s)];
    sprintf(buf,"%d-%d-%d-%.*s",s.kind,s.seq,s.ack,(int)sizeof(s.info),s.info);
    // buf contains frame's bytes

    // UDP send
    int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(-1==sockfd){
        return;
        puts("Failed to create socket");
    }
      // 设置地址与端口
    struct sockaddr_in addr;
    socklen_t          addr_len=sizeof(addr);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;       
    addr.sin_port   = htons(8081);    
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    sendto(sockfd, buf, sizeof(buf), 0, (sockaddr*)&addr, addr_len);
    //printf("Sended %s\n", buf);
    close(sockfd);
}


void RecvFrame(frame* r,int sockfd, struct sockaddr_in addr){
  char buf[sizeof(frame)];
  memset(buf,0,sizeof(frame));
  struct sockaddr_in src;
  socklen_t src_len = sizeof(src);
  memset(&src, 0, sizeof(src));
  int sz = recvfrom(sockfd, buf, sizeof(frame), 0, (sockaddr*)&src, &src_len);
  if (sz > 0){
    buf[sz] = 0;
    char* kindstr = strtok(buf,"-");
    char* seqstr = strtok(NULL,"-");
    char* ackstr = strtok(NULL,"-");
    char* infodata = strtok(NULL,"-");
    r->kind=(frame_kind)atoi(kindstr);
    r->seq=atoi(seqstr);
    r->ack=atoi(ackstr);
    strcpy((char*)r->info,infodata);
    
    printf("物理层: ");
    if(r->kind == data)
        printf("接收到数据帧:\n  kind=%s\n  seq=%s\n  ack=%s\n  info=%s\n", kindstr,seqstr,ackstr,infodata);
    else if(r->kind == ack)
        printf("收到ACK帧:  ack=%s\n",ackstr);
    else
        printf("收到错帧\n");
    printf("--------------------\n");
  }
  return;
}
