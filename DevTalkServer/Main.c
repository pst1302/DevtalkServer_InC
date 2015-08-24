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
	// �ý��� ����
	SYSTEM_INFO sysInfo;
	// Buffer ����
	LPPER_IO_DATA ioInfo;
	// SOCKET ���� (Clnt)
	LPPER_HANDLE_DATA handleInfo;

	// ���� ����
	SOCKET hServSock;
	// ���� �ּ� & ��Ʈ����
	SOCKADDR_IN servAdr;
	// �Ϲ� ����
	int recvBytes, i, flags = 0;
	
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ErrorHandling("WSAStartup error");
	}

	// CP ����
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	
	// �ý��� ���� �о Processor������ŭ ������ ����
	GetSystemInfo(&sysInfo);
	for (i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
		_beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);
	}

	// ���� ���� ����
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

		// Ŭ���̾�Ʈ Accept�� handleInfo����ü�� Ŭ���̾�Ʈ �������� �Ҵ�
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &addrLen);
		handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		handleInfo->hClntSock = hClntSock;
		memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);

		// CP�� ���� ����
		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);

		// ���� ������ ���� ����ְ� Recv ���
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

// ������ ���� �Լ�
DWORD WINAPI EchoThreadMain(LPVOID pComPort) {
	
	// CP
	HANDLE hComPort = (HANDLE)pComPort;
	// clnt Socket
	SOCKET sock;
	// ��/���ŵ� Bytes
	DWORD bytesTrans;
	// Clnt ���� ����
	LPPER_HANDLE_DATA handleInfo;
	// BUF ����
	LPPER_IO_DATA ioInfo;
	DWORD flags = 0;

	// �� ������� �ݺ��ϸ鼭 ���� �޼����� Echo��
	while (1) {
		
		// IO �Ϸ�� ������ Ȯ��
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		
		// �Ϸ�� ���� ����
		sock = handleInfo->hClntSock;
		
		// READ ��û�϶�
		if (ioInfo->rwMode == READ) {
			puts("message received!");
			
			// EOF �޾��� ��
			if (bytesTrans == 0) {
				closesocket(sock);
				free(handleInfo); free(ioInfo);
				continue;
			}

			// ���� �޼��� ����
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = bytesTrans;
			ioInfo->rwMode = WRITE;
			WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

			// ioInfo �޸� �ʱ�ȭ �� ���� ���
			ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = BUF_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
		}
		// WRITE ��û�� ��
		else {
			puts("message sent!");
			free(ioInfo);
		}
	}

}