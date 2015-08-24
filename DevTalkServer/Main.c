#include<stdio.h>
#include<stdlib.h>
#include<WinSock2.h>
#include<Windows.h>
#include<process.h>

#define BUF_SIZE 100
#define READ 3
#define WRITE 5

// socket info
typedef struct {
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

// buffer info
typedef struct {
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	int rwMode;
}PER_IO_DATA, *LPPER_IO_DATA;

DWORD WINAPI EchoThreadMain(LPVOID pComPort);
void ErrorHandling(char* msg);

int main(int argc, char* argv[]) {

	// WSADATA
	WSADATA wsaData;
	// CP 
	HANDLE hComPort;
	// 시스템 정보
	SYSTEM_INFO sysInfo;
	// Buffer 정보
	LPPER_IO_DATA ioInfo;
	// SOCKET 정보 (Clnt)
	LPPER_HANDLE_DATA handleInfo;

	// 서버 소켓
	SOCKET hServSock;
	// 서버 주소 & 포트정보
	SOCKADDR_IN servAdr;
	// 일반 변수
	int recvBytes, i, flags = 0;
	
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ErrorHandling("WSAStartup error");
	}

	// CP 생성
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	
	// 시스템 정보 읽어서 Processor갯수만큼 스레드 생성
	GetSystemInfo(&sysInfo);
	for (i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
		_beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);
	}

	// 서버 소켓 생성
	hServSock = WSASocketW(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(argv[1]);

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("bind error");

	if (listen(hServSock, 5) == SOCKET_ERROR)
		ErrorHandling("listen error");

	while (1){
		SOCKET hClntSock;
		SOCKADDR_IN clntAdr;
		int addrLen = sizeof(clntAdr);

		// 클라이언트 Accept후 handleInfo구조체에 클라이언트 소켓정보 할당
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &addrLen);
		handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		handleInfo->hClntSock = hClntSock;
		memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);

		// CP와 소켓 연결
		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);

		// 버퍼 정보에 정보 집어넣고 Recv 대기
		ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUF_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->rwMode = READ;
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

// 쓰레드 메인 함수
DWORD WINAPI EchoThreadMain(LPVOID pComPort) {
	
	// CP
	HANDLE hComPort = (HANDLE)pComPort;
	// clnt Socket
	SOCKET sock;
	// 송/수신된 Bytes
	DWORD bytesTrans;
	// Clnt 소켓 정보
	LPPER_HANDLE_DATA handleInfo;
	// BUF 정보
	LPPER_IO_DATA ioInfo;
	DWORD flags = 0;

	// 각 쓰레드는 반복하면서 받은 메세지를 Echo함
	while (1) {
		
		// IO 완료된 소켓의 확인
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		
		// 완료된 소켓 읽음
		sock = handleInfo->hClntSock;
		
		// READ 요청일때
		if (ioInfo->rwMode == READ) {
			puts("message received!");
			
			// EOF 받았을 때
			if (bytesTrans == 0) {
				closesocket(sock);
				free(handleInfo); free(ioInfo);
				continue;
			}

			// 받은 메세지 전송
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = bytesTrans;
			ioInfo->rwMode = WRITE;
			WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

			// ioInfo 메모리 초기화 및 수신 대기
			ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = BUF_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
		}
		// WRITE 요청일 때
		else {
			puts("message sent!");
			free(ioInfo);
		}
	}

}