#include <stdio.h>
#include <stdlib.h>
#include "FTPserver.h"

//全局变量
char g_recvBuf[1024];//消息缓存
char* g_sendingFileBuf;     //待发送的文件缓存
int g_sendingFileSize;      //待发送的文件大小
char g_sendingFileName[256]; //待发送的文件名字

int main() {
	initSocket();
	listenToClient();
	closeSocket();
	return 0;
}

//初始化socket库 
bool initSocket()
{
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		printf("WSAStartup faild:%d\n", WSAGetLastError());
		return false;
	}
	printf("初始化socket库成功！\n");
	return true;
}
//关闭socket库
bool closeSocket()
{
	if (0 != WSACleanup())//错误码
	{
		printf("WSAStartup faild:%d\n", WSAGetLastError());
		return false;
	}
	printf("关闭socket库成功！\n");
	return true;
}
//监听客户端
void listenToClient()
{
	//创建server socket套接字 地址、端口号
	SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//ip协议(ipv4)，套接字类型(流式)，协议
	if (INVALID_SOCKET == serfd)
	{
		printf("socket faild:%d\n", WSAGetLastError());
		return;
	}
	//给socket绑定ip地址和端口号
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;//必须和创建socket时一样
	serAddr.sin_port = htons(SPORT);//htons把本地字节序转化为网络字节序
	serAddr.sin_addr.S_un.S_addr = ADDR_ANY;//监听本地所有网卡
	if (0 != bind(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)))
	{
		printf("bind faild:%d\n", WSAGetLastError());
		return;
	}
	//监听客户端链接
	if (0 != listen(serfd, 10))
	{
		printf("listen faild:%d\n", WSAGetLastError());
		return;
	}
	//有客户端连接，那么就要接受链接
	struct sockaddr_in cliAddr;
	int len = sizeof(cliAddr);
	SOCKET clifd = accept(serfd, (struct sockaddr*)&serAddr, &len);
	if (INVALID_SOCKET == clifd)
	{
		printf("accept faild:%d\n", WSAGetLastError());
		return;
	}
	//开始处理消息
	int i = 1;
	while (processMsg(clifd))
	{
		printf("第%d次\n", i);
		i++;
	}
}
//处理消息
bool processMsg(SOCKET clifd)
{
	//成功接收消息返回字节数，失败返回0
	int nRes = recv(clifd, g_recvBuf, 1024, 0);
	if (nRes <= 0)
	{
		printf("客户端下线。。。%d\n", WSAGetLastError());
	}
	//获取接收的消息
	struct MsgHeader* pmsg = (struct MsgHeader*)g_recvBuf;//强转
	switch (pmsg->msgID)
	{
	case MSG_FILENAME:
		printf("收到MSG_FILENAME准备执行readFile函数\n");
		strcpy(g_sendingFileName, pmsg->fileInfo.fileName);
		readFile(clifd);
		printf("readFile函数执行完毕！\n");
		break;
	case MSG_SENDFILE:
		printf("收到MSG_SENDFILE准备执行sendFile函数\n");
		sendFile(clifd, pmsg);
		printf("sendFile函数执行完毕\n");
		break;
	case MSG_SUCCESSED:
		printf("成功！\n");
		break;
	default:
		printf("未知消息！！！！！\n");
		break;
	}
	return true;
}

/*
1、客户端请求下载文件->把文件名发送给服务器
2、服务器接受客户端发送的文件名 ->根据文件名找到文件
   把文件大小发送给客户端
3、客户端接收文件大小 ->准备开始接受，开辟内存，返回确定消息
4、服务器接受开始发送指令 ->开始发送数据
5、客户端接收数据 ->返回接受完成讯息
6、关闭连接
*/

bool readFile(SOCKET clifd)//获得请求的信息头中的文件
{
	FILE* pread = fopen(g_sendingFileName, "rb");
	if (pread == NULL)
	{
		printf("找不到[%s]文件...\n", g_sendingFileName);
		struct MsgHeader msg = { .msgID = MSG_OPENFILE_FAILD };
		if (SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			printf("sent faild:%d\n", WSAGetLastError());
		}
		return false;
	}

	//获取文件大小
	fseek(pread, 0, SEEK_END);//文件内指针移到最后
	g_sendingFileSize = ftell(pread);
	fseek(pread, 0, SEEK_SET);

	//把文件大小和文件名发给客户端
	struct MsgHeader msg;
	msg.msgID = MSG_FIlESIZE;
	msg.fileInfo.fileSize = g_sendingFileSize;

	char tfname[200] = { 0 };
	char text[100];
	_splitpath(g_sendingFileName, NULL, NULL, tfname, text);
	strcat(tfname, text);
	strcpy(msg.fileInfo.fileName, tfname);
	send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0);
	printf("在readFile函数中发出MSG_FIlESIZE\n");
	//读取文件内容
	g_sendingFileBuf = calloc(g_sendingFileSize + 1, sizeof(char));
	if (g_sendingFileBuf == NULL)//内存申请失败
	{
		printf("内存不足!\n");
		return false;
	}
	fread(g_sendingFileBuf, sizeof(char), g_sendingFileSize, pread);
	g_sendingFileBuf[g_sendingFileSize] = '\0';
	fclose(pread);
	return true;
}
bool sendFile(SOCKET clifd)
{
	//告诉客户端准备接收文件
	struct MsgHeader sendFileMsg;
	sendFileMsg.msgID = MSG_READY_READ;
	//如果文件长度大于包的大小PACKET_SIZE——分块
	printf("%d %d\n", g_sendingFileSize, PACKET_SIZE);
	for (size_t i = 0;i< g_sendingFileSize;i+= PACKET_SIZE)
	{
		printf("%d  %d\n", i, g_sendingFileSize);
		sendFileMsg.packet.nStart = i;
		//包的大小大于文件的大小
		if ((i+PACKET_SIZE+1)>g_sendingFileSize)
		{
			sendFileMsg.packet.nsize = g_sendingFileSize - i;
		}
		else
		{
			sendFileMsg.packet.nsize = PACKET_SIZE;
		}

		memcpy(sendFileMsg.packet.buf, g_sendingFileBuf + sendFileMsg.packet.nStart, sendFileMsg.packet.nsize);

		if (SOCKET_ERROR == send(clifd, (char*)&sendFileMsg, sizeof(struct MsgHeader), 0))
		{
			printf("文件发送失败！");
		}
		printf("在sendFile函数的for循环中发出MSG_READY_READ，i=%d\n",i);
	}
	return true;
}