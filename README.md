# GoBackN
simulation of GoBackN protocol in computer network

### 目录简介
工程文件中包含两个文件夹：sender和receiver，每一个文件夹中都有代码文件和编译生成的可执行文件。
两个文件夹中的代码完全一样，区别仅在于UDP协议中绑定的端口不同。
为了方便在一台虚拟机中进行测试，因此分了发送方和接收方，运行时同时运行发送方和接收方的文件，即可实现两个协议实体之间的数据传输。

### 可执行文件说明
send：网络层程序
gobackn：链路层程序，在其中调用了物理层的接口，实现协议实体之间的传输

### 源代码编译方法
网络层程序：在相应目录下输入命令 g++ network_layer.c -o send
链路层程序：在相应目录下输入命令 g++ GoBackN.c physical_layer.c -o gobackn -pthread

### 程序运行方法
同时开启四个终端，分别运行发送方的send和gobackn，以及接收方的send和gobackn。
在send中输入网络层想要发送的数据，之后可以查看gobackn程序的输出来跟踪链路层的运行情况。
运行的时候需要注意，帧定时器的超时时间为5秒，ack定时器的超时时间为2秒
