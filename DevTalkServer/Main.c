#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<WinSock2.h>
#include<process.h>
#include<windows.h>

void ErrorHandling(char* msg);

int main(int argc, char* argv[]) {

	// ���� ���� ����
	WSADATA wsaData;
	SOCKET hSocket;
	SOCKADDR_IN sendAdr;

	// �޼��� ���� ����
	char msg[] = "Network is Computer!";
	int sendBytes = 0;

	// Overlapped ���� ����
	WSABUF dataBuf;
	WSAEVENT evObj;
	WSAOVERLAPPED overlapped;

	if (argc != 3) {
		printf("Usage : %s <ip> <port>\n", argv[0);
		getchar();
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ErrorHandling("WSAStartup error!");
	}

	// WSA���� ����, overlapped ����
	hSocket = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	memset(&sendAdr, 0, sizeof(sendAdr));
	sendAdr.sin_family = AF_INET;
	sendAdr.sin_addr.s_addr = inet_addr(argv[1]);
	sendAdr.sin_port = htos(atoi(argv[2]));

	// ���� ����
	if (connect(hSocket, &sendAdr, sizeof(sendAdr))) {
		ErrorHandling("connect error");
	}

	// Event ���� , �޼��� ����
	evObj = WSACreateEvent();
	memset(&overlapped, 0, sizeof(overlapped));
	dataBuf.len = strlen(msg) - 1;
	dataBuf.buf = msg;

	// �޼��� ����
	if (WSASend(hSocket, &dataBuf, 1, &sendBytes, 0, &overlapped, NULL) == SOCKET_ERROR) {
		// �ٷ� ���޵Ǵ� ���� �ƴѰ�� WSA_IO_PENDING setting �ǰ� ��׶��忡�� ��� ����Ÿ ����
		if (WSAGetLastError() == WSA_IO_PENDING) {
			puts("Background data send");
			WSAWaitForMultipleEvents(1, &evObj, TRUE, WSA_INFINITE, FALSE);
			WSAGetOverlappedResult(hSocket, &overlapped, &sendBytes, FALSE, NULL);
		}
		else {
			ErrorHandling("WSASend error!");
		}

		printf("Send data size: %d \n", sendBytes);
		WSACloseEvent(evObj);
		closesocket(hSocket);
		WSACleanup();
		return 0;
	}
}

void ErrorHandling(char* msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

