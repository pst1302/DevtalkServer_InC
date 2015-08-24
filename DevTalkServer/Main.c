#include<stdio.h>
#include<stdlib.h>
#include<WinSock2.h>
#include<Windows.h>
#include<process.h>

#define BUF_SIZE 1024
#define READ 3
#define WRITE 5

// socket infomation
typedef struct {
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

// buffer infomation
typedef struct {
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	int rwMode;
} PER_IO_DATA, *LPPER_IO_DATA;

DWORD WINAPI EchoThreadMain(LPVOID pComPort);
void ErrorHandling(char* msg);

int main(int argc, char* argv[]) {

	WSADATA wsaData;

	// IOCP 관련 변수
	HANDLE hComPort;
	SYSTEM_INFO sysInfo;
	LPPER_IO_DATA ioInfo;
	LPPER_HANDLE_DATA handleInfo;

	SOCKET hServSock;
	SOCKADDR_IN servAdr;
	int recvBytes, i, flags = 0;
	
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup error");

	// CP 생성 -> CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0); 매개변수를 이렇게 지정해서 CP 생성한다.
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// System 정보를 읽고 프로세서의 수만큼 Thread를 생성한다. 4개의 Thread 생성
	GetSystemInfo(&sysInfo);
	for (i = 0; i < sysInfo.dwNumberOfProcessors; i++)
		_beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);

	// 서버 소켓 생성
	hServSock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	// bind , listen
	bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr));
	listen(hServSock, 5);


	while (1) {

		// Client Socket information
		SOCKET hClntSock;
		SOCKADDR_IN clntAdr;

		// accept
		int addrLen = sizeof(clntAdr);
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &addrLen);

		// handleInfo에 클라이언트 소켓, 소켓 정보 연결
		handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		handleInfo->hClntSock = hClntSock;
		memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);
		
		// CP와 Client Socket 연결
		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);

		// overlapped, wsaBuf관련 rwMode를 READ관련 
		ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUF_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->rwMode = READ;

		// Recv 
		WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
	}

	getchar();
	return 0;
	
}

void ErrorHandling(char* msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

// Thread에서 처리하는 함수
DWORD WINAPI EchoThreadMain(LPVOID pComPort) {

	HANDLE hComPort = (HANDLE)pComPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	DWORD flags = 0;

	while (1) {

		// GetQueuedCompletionStatus(CP오브젝트, 송/수신되는 Byte수, 클라이언트 정보, 버퍼 Infomation, 타임아웃);
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		sock = handleInfo->hClntSock;


		// READ 처리
		if (ioInfo->rwMode == READ) {

			puts("message received!");

			// EOF -> Socket 종료
			if (bytesTrans == 0) {
				closesocket(sock);
				free(handleInfo); free(ioInfo);
				continue;
			}

			// Echo
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = bytesTrans;
			ioInfo->rwMode = WRITE;
			WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

			// 전송 후 ioInfo 다시 setting하고 READ모드로 전환 후 다시 대기
			ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = BUF_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			ioInfo->rwMode = READ;
			WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
		}
		// WRITE 처리
		else {
			puts("message sent!");
			free(ioInfo);
		} 
	}

	return 0;
}