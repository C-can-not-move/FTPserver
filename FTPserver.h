#pragma once
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#include <stdbool.h>
#include <windows.h>
#define SPORT 8888// �������˿ں�
//������
enum MSGTAG
{
	MSG_FILENAME = 1, //�ļ�����ʶ
	MSG_FIlESIZE = 2, //�ļ���С
	MSG_READY_READ = 3, //׼������
	MSG_SENDFILE = 4, //����
	MSG_SUCCESSED = 5, //�������

	MSG_OPENFILE_FAILD = 6, //�ļ�������
};
#pragma pack(1)//���ýṹ���ֽڶ���
#define PACKET_SIZE (1024-sizeof(int)*3)
struct MsgHeader  //��װ��Ϣͷ
{
	enum MSGTAG msgID; //��ǰ�����Ϣ 4byte
	union MyUnion
	{
		struct
		{
			int fileSize;       //�ļ���С4byte
			char fileName[256]; //�ļ���s 256byte
		}fileInfo;
		struct
		{
			int nStart; //���ı��
			int nsize;  //�������ݴ�С
			char buf[PACKET_SIZE];
		}packet;
	};

};
#pragma pack()  //�ָ�Ĭ�϶���

//��ʼ��socket�� 
bool initSocket();
//�ر�socket��
bool closeSocket();
//�����ͻ�������
void listenToClient();
//������Ϣ
bool processMsg(SOCKET);
//��ȡ�ļ�������ļ���С
bool readFile(SOCKET);
//�����ļ�
bool sendFile(SOCKET, struct MsgHeader*);