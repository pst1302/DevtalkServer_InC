#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<WinSock2.h>

#define BUF_SIZE 1024

void ErrorHandling(char* message);

int main(int argc, char* argv[]) 
{

	// Socket ���� ������
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;

	// select �Լ� ���� ���� 
	TIMEVAL timeout;
	fd_set reads, cpyReads;
	
	// �Ϲ� ����
	int adrSz;
	int strLen, fdNum, i;
	char buf[BUF_SIZE];

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
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	fputs("���� ���� ���ε� ��....\n", stdout);

	// ���� ���ε�
	if (bind(hServSock, (SOCKADDR*) &servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
		ErrorHandling("bind() error");
	}

	// ������ ip �ּ� �ҷ�����
	char strAddrBuf[50];

	int size = sizeof(strAddrBuf);

	WSAAddressToString((SOCKADDR*)&servAdr, sizeof(servAdr), NULL, strAddrBuf, &size);

	fputs("���� ���� ����....\n", stdout);
	fputs("���� ip�ּ� : ", stdout);
	printf("%d", INADDR_ANY);
	fputs(" ��Ʈ : ", stdout);
	fputs(argv[1], stdout);
	fputs("\n", stdout);

	fputs("���� ���� ������ ��....\n", stdout);

	// Listening
	if (listen(hServSock, 5) == SOCKET_ERROR) {
		ErrorHandling("listen() error!");
	}
	
	FD_ZERO(&reads);
	FD_SET(hServSock, &reads);

	while (1) {
		cpyReads = reads;

		timeout.tv_sec = 5;
		timeout.tv_usec = 5000;

		if ((fdNum = select(0, &cpyReads, 0, 0, &timeout)) == SOCKET_ERROR)
			break;

		if (fdNum == 0)
			continue;

		for (i = 0; i < reads.fd_count; i++) 
		{
			// Connection Request
			if (FD_ISSET(reads.fd_array[i], &cpyReads)){
				if (reads.fd_array[i] == hServSock) {
					adrSz = sizeof(clntAdr);
					hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &adrSz);
					FD_SET(hClntSock, &reads);
					printf("connected client: %d \n", hClntSock);
				}
				else
				{
					strLen = recv(reads.fd_array[i], buf, BUF_SIZE - 1, 0);
					// �޼��� ���� ��
					if (strLen == 0) {
						FD_CLR(reads.fd_array[i], &reads);
						closesocket(cpyReads.fd_array[i]);
						printf("close client: %d \n", cpyReads.fd_array[i]);
					}
					else {
						send(reads.fd_array[i], buf, strLen, 0);	// echo
					}
				}
			}
		}
	}


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