#include<sys/select.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include <sys/msg.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>

#define KEY 450  //*****
#define MAX_PKT 1024

#define FRAME_TIMER 5
#define ACK_TIMER 2

//********
typedef struct{
    long type;
    char data[MAX_PKT];
}Msg;
typedef enum{data, ack, err} frame_kind;
typedef unsigned int seq_nr;
typedef int mutex;

typedef char packet[MAX_PKT];

typedef struct{
	frame_kind kind; 
	int seq;
	int ack;
	packet info;
}frame;

void start_timer(seq_nr frame_nr);
void stop_timer(seq_nr frame_nr);
void start_ack_timer();
void stop_ack_timer();

void retransmission();
void sendAckFrame();


void Sendto(frame s);
void RecvFrame(frame* r,int sockfd, struct sockaddr_in addr);
void pl_connect(int *sockfd, struct sockaddr_in *addr);
