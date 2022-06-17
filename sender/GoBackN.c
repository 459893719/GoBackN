#include "GoBackN.h"

#define MAX_SEQ 3


/**********物理层连接需要定义的变量声明***********/
int sockfd;// 创建socket
struct sockaddr_in addr;// 设置地址与端口

/***************互斥锁的声明********************/
pthread_mutex_t mutex1;      //实现链路层中全局变量的互斥访问
pthread_mutex_t time_mutex;  //实现全局定时器的互斥访问
sem_t buffer_sem;            //进行流量控制


/****************定时器的定义*******************/
time_t timers[MAX_SEQ+2];

/***************全局变量声明********************/
seq_nr next_frame_to_send;
seq_nr ack_expected;
seq_nr frame_expected;
frame r;
packet buffer[MAX_SEQ + 1];
seq_nr nbuffered;

int msgid; //与网络层进行通信的消息队列ID

/*
    from_network_layer：
    函数功能：到网络层的接口，从网络层中接收数据包
    接收到的数据包将会储存在p中
*/
void from_network_layer(packet p){
    Msg rcv;
    //printf("消息队列id: %d\n", msgid);
    while(1){
        int res = msgrcv(msgid,&rcv,sizeof(rcv)-sizeof(rcv.type),1, 0);
        //printf("%d\n", res);
	    if(res == MAX_PKT){
	        printf("链路层: 收到网络层的数据包: %s\n\
--------------------\n",rcv.data);
            strcpy(p, rcv.data);   
            break;
        }
    } 
	         
}
/*
    to_network_layer：
    函数功能：到网络层的接口，向网络层发送数据包

    发送的内容为p
*/
void to_network_layer(packet p){
    printf("链路层: 向网络层发送数据包: %s\n\
--------------------\n",p);
}

/*
    between：
    函数功能：判断某个帧序号b是否在滑动窗口(a,c)中
*/
static int between(seq_nr a, seq_nr b, seq_nr c) {
    if (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
        return 1;
    else
        return 0;
}

/*
    inc：
    函数功能：将序号seq进行自加操作
*/
static void inc(unsigned int* seq) {
    *seq = (*seq + 1) % (MAX_SEQ + 1);
}


/*
    send_data：
    函数功能：将一个帧发往物理层，在参数中给出帧中每个字段的内容
*/
static void send_data(frame_kind kind, seq_nr frame_nr, seq_nr frame_expected, packet buffer[]) {
    frame s;
    s.kind = kind;
	if(kind == data){
		strcpy(s.info, buffer[frame_nr]);
	}
	else{
		strcpy(s.info, "OK");
	}
    s.seq = frame_nr;
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
	printf("链路层: 向物理层发送数据:\n\tinfo: %s\n\tseq: %d\n\tack: %d\n\
--------------------\n", s.info, s.seq, s.ack);
    //向物理层发送数据
	Sendto(s);

    if(s.kind == data)
        start_timer(frame_nr);
    stop_ack_timer();
}

/*
    start_timer(), stop_timer()：
    函数功能：对指定的定时器进行启动和关闭操作，在参数中给出定时器的索引
*/
void start_timer(seq_nr frame_nr) {
	if (pthread_mutex_lock(&time_mutex) != 0) {
        	fprintf(stdout, "lock error!\n");
	}
	timers[frame_nr] = time(NULL) + FRAME_TIMER;
	//释放锁
	pthread_mutex_unlock(&time_mutex);
}

void stop_timer(seq_nr frame_nr) {
	if (pthread_mutex_lock(&time_mutex) != 0) {
        	fprintf(stdout, "lock error!\n");
	}

	timers[frame_nr] = 0; //将该定时器的内容清零
	//释放锁
	pthread_mutex_unlock(&time_mutex);
}


/*
    start_ack_timer(), stop_ack_timer()：
    函数功能：对ACK定时器进行启动和关闭操作
*/
void start_ack_timer(){
    if (pthread_mutex_lock(&time_mutex) != 0) {
        	fprintf(stdout, "lock error!\n");
	}
	if(timers[MAX_SEQ+1] == 0)    //只有当ack定时器没有开启时,才能进行重新设置
	    timers[MAX_SEQ+1] = time(NULL) + ACK_TIMER;
	//释放锁
	pthread_mutex_unlock(&time_mutex);
}

void stop_ack_timer(){
    if (pthread_mutex_lock(&time_mutex) != 0) {
        	fprintf(stdout, "lock error!\n");
	}
	timers[MAX_SEQ+1] = 0;
	//释放锁
	pthread_mutex_unlock(&time_mutex);
}

/*
    retransmission()：
    函数功能：将发送窗口中的所有帧进行重传
*/
void retransmission(){
	printf("/*******超时,进行重传*******/\n");
    //获取锁，进行数据传输操作
    if (pthread_mutex_lock(&mutex1) != 0) {
        fprintf(stdout, "lock error!\n");
    }
    //将发送窗口中的所有帧进行重传
    next_frame_to_send = ack_expected;
    for (int i = 1; i <= nbuffered; i++){
        send_data(data, next_frame_to_send, frame_expected, buffer);
    	inc(&next_frame_to_send);
    }
    //释放锁
    pthread_mutex_unlock(&mutex1);
}

/*
    sendAckFrame()：
    函数功能：不进行重传，直接发送单独的ACK帧
*/
void sendAckFrame(){
    printf("/*******超时,不捎带确认*******/\n");
    if (pthread_mutex_lock(&mutex1) != 0) {    //获取锁，进行数据传输操作
        fprintf(stdout, "lock error!\n");
    }
    
    send_data(ack, next_frame_to_send, frame_expected, buffer);//发送一个单独的ack帧
    
    pthread_mutex_unlock(&mutex1);//释放锁
}


/*
    timerThread()：
    函数功能：定时器管理线程，每次通过轮询查看是否出现超时的情况
*/
void* timerThread(void* args){
	time_t now;
	int timeout, acktimeout;
	while(true){
		if (pthread_mutex_lock(&time_mutex) != 0) {  //获取时间锁
			fprintf(stdout, "lock error!\n");
		}
		now = time(NULL);   //获取当前时间

		/*对所有的定时器进行轮询,如果有超时的定时器, 则轮完之后进行超时处理*/
		//检查每个帧的定时器
		timeout = 0;
		for(int i=0;i<=MAX_SEQ;i++){
			if(timers[i] == now){
				timeout = 1;
				timers[i] = 0;   //将触发后的定时器清除
			}
		}
		//检查ack定时器
        acktimeout = 0;
        if(timers[MAX_SEQ+1] == now){
            acktimeout = 1;
            timers[MAX_SEQ+1] = 0;  //将触发后的定时器清除
        }
		pthread_mutex_unlock(&time_mutex);
		
		/*进行超时处理*/
		if(timeout)
			retransmission();
		if(acktimeout)
		    sendAckFrame();
	}
}

/*
    recvFromNetwork()：
    函数功能：链路层的发送线程，从网络层接收数据, 将数据发送到物理层
*/
void *recvFromNetwork(void* args) {
    while (true) {
        //当窗口满的时候，不能进行接收，等待信号量
        sem_wait(&buffer_sem);
        //从网络层接收数据
	    from_network_layer(buffer[next_frame_to_send]);

        //获取锁，进行数据传输操作
        if (pthread_mutex_lock(&mutex1) != 0) {
            fprintf(stdout, "lock error!\n");
        }

        nbuffered += 1; //信号量机制，在这里信号量-1

        send_data(data, next_frame_to_send, frame_expected, buffer);
        inc(&next_frame_to_send);
        //释放锁
        pthread_mutex_unlock(&mutex1);
    }

}

/*
    recvFromPhysical()：
    函数功能：链路层的接收线程，从物理层接收数据,发送到网络层
*/
void *recvFromPhysical(void* args) {
    while (true) {

        //从物理层中接收帧，如果没有接收到则在此阻塞
	    RecvFrame(&r, sockfd, addr);

        //获取锁，进行数据传输操作
        if (pthread_mutex_lock(&mutex1) != 0) {
            fprintf(stdout, "lock error!\n");
        }
        //判断数据帧是data类型还是ack类型
        if (r.kind != err) {
            if (r.kind == data) {   //如果包含捎带确认的数据，则进行接收
                if (r.seq == frame_expected) {
                    //收到的帧应该是有序的
                    to_network_layer(r.info);
                    inc(&frame_expected);
                    
                    //启动ack计时器
			        start_ack_timer();

                }
            }

            while (between(ack_expected, r.ack, next_frame_to_send)) {
                //累计确认
                nbuffered = nbuffered - 1;   
                sem_post(&buffer_sem);//在这里信号量-1
                stop_timer(ack_expected);
                inc(&ack_expected);
            }
        }

        //释放锁
        pthread_mutex_unlock(&mutex1);
    }
}


/*
    goBackN()：
    函数功能：链路层主程序，负责管理线程的创建和变量的初始化
*/
void goBackN(void) {
    pthread_t pl_thread, nl_thread, timer_thread;
    seq_nr i;
    //对全局变量进行初始化
    ack_expected = 0;
    next_frame_to_send = 0;
    frame_expected = 0;
    nbuffered = 0;
    // 初始化互斥锁
    if (pthread_mutex_init(&mutex1, NULL) != 0) {
        // 互斥锁初始化失败
        return;
    }
    int ret=sem_init(&buffer_sem, 0, MAX_SEQ);
    if (ret == -1) 
	{
        printf("sem_init failed \n");
        return;
    }
    //创建线程
    pthread_create(&pl_thread, NULL, recvFromPhysical, NULL);
    pthread_create(&nl_thread, NULL, recvFromNetwork, NULL);
    pthread_create(&timer_thread, NULL, timerThread, NULL);
    pthread_join(pl_thread, NULL);
    pthread_join(nl_thread, NULL);
    pthread_join(timer_thread, NULL);

}

int main(){
    msgid = msgget(KEY,IPC_CREAT|0777);
    if(msgid<0)
    {   
        perror("msgget error!");
        exit(-1);
    }  
    
    
	pl_connect(&sockfd, &addr);
	goBackN();
	return 0;
}






