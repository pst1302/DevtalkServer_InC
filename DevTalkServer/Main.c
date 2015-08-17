#include<stdio.h>
#include<stdlib.h>
#include<WinSock2.h>

void ErrorHandling(char* message);

int main(int argc, char* argv[]) 
{

	// Socket 관련 변수들
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAddr, clntAddr;

	// 일반 변수
	int szClntAddr;
	char message[] = "Hello World!";

	// main argc 유효성 검사
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		getchar();
		exit(1);
	}

	// WSA 시작
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("Error Occured In WSAStartUp Call....\n");
	}

	// 서버 소켓 생성
	hServSock = socket(PF_INET, SOCK_STREAM, 0);
	if (hServSock == INVALID_SOCKET) {
		ErrorHandling("socket() error");
	}

	// 서버 소켓 설정
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(atoi(argv[1]));

	// 소켓 바인드
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

	// 자동 종료 방지
	printf("if you want to shutdown the program, press any key..");
	getchar();
	return 0;
}

void ErrorHandling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}