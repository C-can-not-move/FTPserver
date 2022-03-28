#pragma once
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#include <stdbool.h>
#include <windows.h>
#define SPORT 8888// 服务器端口号
//定义标记
enum MSGTAG
{
	MSG_FILENAME = 1, //文件名标识
	MSG_FIlESIZE = 2, //文件大小
	MSG_READY_READ = 3, //准备接收
	MSG_SENDFILE = 4, //发送
	MSG_SUCCESSED = 5, //传输完成

	MSG_OPENFILE_FAILD = 6, //文件不存在
};
#pragma pack(1)//设置结构体字节对齐
#define PACKET_SIZE (1024-sizeof(int)*3)
struct MsgHeader  //封装消息头
{
	enum MSGTAG msgID; //当前标记消息 4byte
	union MyUnion
	{
		struct
		{
			int fileSize;       //文件大小4byte
			char fileName[256]; //文件名s 256byte
		}fileInfo;
		struct
		{
			int nStart; //包的编号
			int nsize;  //包的数据大小
			char buf[PACKET_SIZE];
		}packet;
	};

};
#pragma pack()  //恢复默认对齐

//初始化socket库 
bool initSocket();
//关闭socket库
bool closeSocket();
//监听客户端链接
void listenToClient();
//处理消息
bool processMsg(SOCKET);
//读取文件，获得文件大小
bool readFile(SOCKET);
//发送文件
bool sendFile(SOCKET, struct MsgHeader*);