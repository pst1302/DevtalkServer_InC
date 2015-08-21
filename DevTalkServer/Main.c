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

// 멀티 쓰레딩 관련 함수
int clntCnt = 0;
SOCKET clntSock[MAX_CLNT];
HANDLE hMutex;

int main(int argc, char* argv[]) 
{

	// Socket 관련 변수들
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;

	// Thread 관련 변수
	HANDLE hThread;

	// 일반 변수
	int numOfClntSock = 0;
	int strLen, i;
	int posInfo, startIdx;
	int clntAdrLen;
	char msg[BUF_SIZE];


	// main argc 유효성 검사
	if (argc != 2) {
		printf("Usage : %s <port>", argv[0]);
		exit(1);
	}

	// WSA 시작
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("Error Occured In WSAStartUp Call....\n");
	}

	// 뮤텍스 생성
	hMutex = CreateMutex(NULL, FALSE, NULL);

	// 서버 소켓 생성0
	hServSock = socket(PF_INET, SOCK_STREAM, 0);
	if (hServSock == INVALID_SOCKET) {
		ErrorHandling("socket() error");
	}
	

	// 서버 소켓 설정
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	fputs("소켓 바인딩중....\n", stdout);
	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
		ErrorHandling("bind error!");
	}

	fputs("소켓 리스닝중....\n", stdout);
	if (listen(hServSock, 5) == SOCKET_ERROR) {
		ErrorHandling("listen error");
	}
	
	// 서버 응답 대기
	while (1) {
		posInfo = WSAWaitForMultipleEvents(numOfClntSock, hEventArr, FALSE, WSA_INFINITE, FALSE);
		startIdx = posInfo - WSA_WAIT_EVENT_0;

		clntAdrSz = sizeof(clntAdr);
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrSz);

		// 임계 영역
		WaitForSingleObject(hMutex, INFINITE);
		clntSock[clntCnt++] = hClntSock;
		ReleaseMutex(hMutex);

		hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClnt, (void*)&hClntSock, 0, NULL);
		printf("Connect client IP: %s \n", inet_ntoa(clntAdr.sin_addr));
	}

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

unsigned WINAPI HandleClnt(void* arg) {
	
	SOCKET hClntSock = *((SOCKET*)arg);
	int strLen = 0, i;
	char msg[BUF_SIZE];

	while ((strLen = recv(hClntSock, msg, sizeof(msg), 0)) != 0)
		SendMsg(msg, strLen);

	// 임계 영역
	WaitForSingleObject(hMutex, INFINITE);

	for (i = 0; i < clntCnt; i++) {
		if (hClntSock == clntSock[i])
		{
			while (i++ < clntCnt - 1)
				clntSock[i] = clntSock[i + 1];
			break;
		}
	}
					WSACloseEvent(hEventArr[sigEventIdx]);
					closesocket(hSockArr[sigEventIdx]);

	clntCnt--;

	ReleaseMutex(hMutex);
	

	// 자동 종료 방지
	printf("if you want to shutdown the program, press any key..");
	getchar();
	return 0;
}

void SendMsg(char* msg, int len) {

	int i;
	
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i < clntCnt; i++)
		send(clntSock[i], msg, len, 0);

	ReleaseMutex(hMutex);

	int i;
	for (i = idx; i < total; i++)
		hEventArr[i] = hEventArr[i + 1];
}
