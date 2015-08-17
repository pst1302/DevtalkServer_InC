#include<stdio.h>
#include<stdlib.h>
#include<WinSock2.h>

void ErrorHandling(char* message);

int main(int argc, char* argv[]) 
{

	// Socket ���� ������
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAddr, clntAddr;

	// �Ϲ� ����
	int szClntAddr;
	char message[] = "Hello World!";

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

	// ���� ���� ����
	hServSock = socket(PF_INET, SOCK_STREAM, 0);
	if (hServSock == INVALID_SOCKET) {
		ErrorHandling("socket() error");
	}

	// ���� ���� ����
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(atoi(argv[1]));

	// ���� ���ε�
	if (bind(hServSock, (SOCKADDR*) &servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
		ErrorHandling("bind() error");
	}

	// Listening
	if (listen(hServSock, 5) == SOCKET_ERROR) {
		ErrorHandling("listen() error!");
	}

	// Client accept
	szClntAddr = sizeof(clntAddr);
	hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &szClntAddr);
	if (hClntSock == INVALID_SOCKET)
		ErrorHandling("accept() error");

	send(hClntSock, message, sizeof(message), 0);
	closesocket(hClntSock);
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