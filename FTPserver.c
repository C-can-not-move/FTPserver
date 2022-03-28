#include <stdio.h>
#include <stdlib.h>
#include "FTPserver.h"

//ȫ�ֱ���
char g_recvBuf[1024];//��Ϣ����
char* g_sendingFileBuf;     //�����͵��ļ�����
int g_sendingFileSize;      //�����͵��ļ���С
char g_sendingFileName[256]; //�����͵��ļ�����

int main() {
	initSocket();
	listenToClient();
	closeSocket();
	return 0;
}

//��ʼ��socket�� 
bool initSocket()
{
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		printf("WSAStartup faild:%d\n", WSAGetLastError());
		return false;
	}
	printf("��ʼ��socket��ɹ���\n");
	return true;
}
//�ر�socket��
bool closeSocket()
{
	if (0 != WSACleanup())//������
	{
		printf("WSAStartup faild:%d\n", WSAGetLastError());
		return false;
	}
	printf("�ر�socket��ɹ���\n");
	return true;
}
//�����ͻ���
void listenToClient()
{
	//����server socket�׽��� ��ַ���˿ں�
	SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//ipЭ��(ipv4)���׽�������(��ʽ)��Э��
	if (INVALID_SOCKET == serfd)
	{
		printf("socket faild:%d\n", WSAGetLastError());
		return;
	}
	//��socket��ip��ַ�Ͷ˿ں�
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;//����ʹ���socketʱһ��
	serAddr.sin_port = htons(SPORT);//htons�ѱ����ֽ���ת��Ϊ�����ֽ���
	serAddr.sin_addr.S_un.S_addr = ADDR_ANY;//����������������
	if (0 != bind(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)))
	{
		printf("bind faild:%d\n", WSAGetLastError());
		return;
	}
	//�����ͻ�������
	if (0 != listen(serfd, 10))
	{
		printf("listen faild:%d\n", WSAGetLastError());
		return;
	}
	//�пͻ������ӣ���ô��Ҫ��������
	struct sockaddr_in cliAddr;
	int len = sizeof(cliAddr);
	SOCKET clifd = accept(serfd, (struct sockaddr*)&serAddr, &len);
	if (INVALID_SOCKET == clifd)
	{
		printf("accept faild:%d\n", WSAGetLastError());
		return;
	}
	//��ʼ������Ϣ
	int i = 1;
	while (processMsg(clifd))
	{
		printf("��%d��\n", i);
		i++;
	}
}
//������Ϣ
bool processMsg(SOCKET clifd)
{
	//�ɹ�������Ϣ�����ֽ�����ʧ�ܷ���0
	int nRes = recv(clifd, g_recvBuf, 1024, 0);
	if (nRes <= 0)
	{
		printf("�ͻ������ߡ�����%d\n", WSAGetLastError());
	}
	//��ȡ���յ���Ϣ
	struct MsgHeader* pmsg = (struct MsgHeader*)g_recvBuf;//ǿת
	switch (pmsg->msgID)
	{
	case MSG_FILENAME:
		printf("�յ�MSG_FILENAME׼��ִ��readFile����\n");
		strcpy(g_sendingFileName, pmsg->fileInfo.fileName);
		readFile(clifd);
		printf("readFile����ִ����ϣ�\n");
		break;
	case MSG_SENDFILE:
		printf("�յ�MSG_SENDFILE׼��ִ��sendFile����\n");
		sendFile(clifd, pmsg);
		printf("sendFile����ִ�����\n");
		break;
	case MSG_SUCCESSED:
		printf("�ɹ���\n");
		break;
	default:
		printf("δ֪��Ϣ����������\n");
		break;
	}
	return true;
}

/*
1���ͻ������������ļ�->���ļ������͸�������
2�����������ܿͻ��˷��͵��ļ��� ->�����ļ����ҵ��ļ�
   ���ļ���С���͸��ͻ���
3���ͻ��˽����ļ���С ->׼����ʼ���ܣ������ڴ棬����ȷ����Ϣ
4�����������ܿ�ʼ����ָ�� ->��ʼ��������
5���ͻ��˽������� ->���ؽ������ѶϢ
6���ر�����
*/

bool readFile(SOCKET clifd)//����������Ϣͷ�е��ļ�
{
	FILE* pread = fopen(g_sendingFileName, "rb");
	if (pread == NULL)
	{
		printf("�Ҳ���[%s]�ļ�...\n", g_sendingFileName);
		struct MsgHeader msg = { .msgID = MSG_OPENFILE_FAILD };
		if (SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			printf("sent faild:%d\n", WSAGetLastError());
		}
		return false;
	}

	//��ȡ�ļ���С
	fseek(pread, 0, SEEK_END);//�ļ���ָ���Ƶ����
	g_sendingFileSize = ftell(pread);
	fseek(pread, 0, SEEK_SET);

	//���ļ���С���ļ��������ͻ���
	struct MsgHeader msg;
	msg.msgID = MSG_FIlESIZE;
	msg.fileInfo.fileSize = g_sendingFileSize;

	char tfname[200] = { 0 };
	char text[100];
	_splitpath(g_sendingFileName, NULL, NULL, tfname, text);
	strcat(tfname, text);
	strcpy(msg.fileInfo.fileName, tfname);
	send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0);
	printf("��readFile�����з���MSG_FIlESIZE\n");
	//��ȡ�ļ�����
	g_sendingFileBuf = calloc(g_sendingFileSize + 1, sizeof(char));
	if (g_sendingFileBuf == NULL)//�ڴ�����ʧ��
	{
		printf("�ڴ治��!\n");
		return false;
	}
	fread(g_sendingFileBuf, sizeof(char), g_sendingFileSize, pread);
	g_sendingFileBuf[g_sendingFileSize] = '\0';
	fclose(pread);
	return true;
}
bool sendFile(SOCKET clifd)
{
	//���߿ͻ���׼�������ļ�
	struct MsgHeader sendFileMsg;
	sendFileMsg.msgID = MSG_READY_READ;
	//����ļ����ȴ��ڰ��Ĵ�СPACKET_SIZE�����ֿ�
	printf("%d %d\n", g_sendingFileSize, PACKET_SIZE);
	for (size_t i = 0;i< g_sendingFileSize;i+= PACKET_SIZE)
	{
		printf("%d  %d\n", i, g_sendingFileSize);
		sendFileMsg.packet.nStart = i;
		//���Ĵ�С�����ļ��Ĵ�С
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
			printf("�ļ�����ʧ�ܣ�");
		}
		printf("��sendFile������forѭ���з���MSG_READY_READ��i=%d\n",i);
	}
	return true;
}