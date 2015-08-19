#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<WinSock2.h>
#include<process.h>
#include<windows.h>

#define BUF_SIZE 1024
#define MAX_CLNT 256

unsigned WINAPI HandleClnt(void* arg);
void SendMsg(char* msg, int len);
void ErrorHandling(char* message);

// ��Ƽ ������ ���� �Լ�
int clntCnt = 0;
SOCKET clntSock[MAX_CLNT];
HANDLE hMutex;

int main(int argc, char* argv[]) 
{

	// Socket ���� ������
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;

	// Thread ���� ����
	HANDLE hThread;

	// �Ϲ� ����
	int clntAdrSz;


	// main argc ��ȿ�� �˻�
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		getchar();
		exit(1);
	}

	// WSA ����
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("Error Occured In WSAStartUp Call....\n");
	}

	// ���ؽ� ����
	hMutex = CreateMutex(NULL, FALSE, NULL);

	// ���� ���� ����0
	hServSock = socket(PF_INET, SOCK_STREAM, 0);
	if (hServSock == INVALID_SOCKET) {
		ErrorHandling("socket() error");
	}
	

	// ���� ���� ����
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	fputs("���� ���� ���ε� ��....\n", stdout);

	// ���� ���ε�
	if (bind(hServSock, (SOCKADDR*) &servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
		ErrorHandling("bind() error");
	}

	fputs("���� ���� ������ ��....\n", stdout);

	// Listening
	if (listen(hServSock, 5) == SOCKET_ERROR) {
		ErrorHandling("listen() error!");
	}
	
	// ���� ���� ���
	while (1) {

		clntAdrSz = sizeof(clntAdr);
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrSz);

		// �Ӱ� ����
		WaitForSingleObject(hMutex, INFINITE);
		clntSock[clntCnt++] = hClntSock;
		ReleaseMutex(hMutex);

		hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClnt, (void*)&hClntSock, 0, NULL);
		printf("Connect client IP: %s \n", inet_ntoa(clntAdr.sin_addr));
	}

	closesocket(hServSock);
	WSACleanup();


	// �ڵ� ���� ����
	printf("if you want to shutdown the program, press any key..");
	getchar();
	return 0;
}

void ErrorHandling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

unsigned WINAPI HandleClnt(void* arg) {
	
	SOCKET hClntSock = *((SOCKET*)arg);
	int strLen = 0, i;
	char msg[BUF_SIZE];

	while ((strLen = recv(hClntSock, msg, sizeof(msg), 0)) != 0)
		SendMsg(msg, strLen);

	// �Ӱ� ����
	WaitForSingleObject(hMutex, INFINITE);

	for (i = 0; i < clntCnt; i++) {
		if (hClntSock == clntSock[i])
		{
			while (i++ < clntCnt - 1)
				clntSock[i] = clntSock[i + 1];
			break;
		}
	}

	clntCnt--;

	ReleaseMutex(hMutex);
	
	closesocket(hClntSock);

	return 0;
}

void SendMsg(char* msg, int len) {

	int i;
	
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i < clntCnt; i++)
		send(clntSock[i], msg, len, 0);

	ReleaseMutex(hMutex);

}
