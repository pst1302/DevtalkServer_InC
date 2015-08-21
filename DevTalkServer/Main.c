#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<WinSock2.h>
#include<process.h>
#include<windows.h>

#define BUF_SIZE 100

void CompressSockets(SOCKET hSockArr[], int idx, int total);
void CompressEvents(WSAEVENT hEventArr[], int idx, int total);
void ErrorHandling(char* message);

int main(int argc, char* argv[]) 
{
	
	// 소켓 관련 변수
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;

	// 비동기 Notification 관련 변수
	SOCKET hSockArr[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT hEventArr[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT newEvent;
	WSANETWORKEVENTS netEvents;

	// 일반 변수
	int numOfClntSock = 0;
	int strLen, i;
	int posInfo, startIdx;
	int clntAdrLen;
	char msg[BUF_SIZE];

	if (argc != 2) {
		printf("Usage : %s <port>", argv[0]);
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ErrorHandling("WSAStartup error!");
	}

	hServSock = socket(PF_INET, SOCK_STREAM, 0);
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

	// Event 생성
	newEvent = WSACreateEvent();
	if (WSAEventSelect(hServSock, newEvent, FD_ACCEPT) == SOCKET_ERROR)
		ErrorHandling("WSAEventSelect error");

	hSockArr[numOfClntSock] = hServSock;
	hEventArr[numOfClntSock] = newEvent;
	numOfClntSock++;

	while (1) {
		posInfo = WSAWaitForMultipleEvents(numOfClntSock, hEventArr, FALSE, WSA_INFINITE, FALSE);
		startIdx = posInfo - WSA_WAIT_EVENT_0;

		for (i = startIdx; i < numOfClntSock; i++) {

			// 이벤트 설정
			int sigEventIdx = WSAWaitForMultipleEvents(1, &hEventArr[i], TRUE, 0, FALSE);

			if ((sigEventIdx == WSA_WAIT_FAILED || sigEventIdx == WSA_WAIT_TIMEOUT))
				continue;
			else {

				sigEventIdx = i;

				WSAEnumNetworkEvents(hSockArr[sigEventIdx], hEventArr[sigEventIdx], &netEvents);

				if (netEvents.lNetworkEvents & FD_ACCEPT) {
					if (netEvents.iErrorCode[FD_ACCEPT_BIT] != 0){
						puts("Accept Error");
						break;
					}

					clntAdrLen = sizeof(clntAdr);
					hClntSock = accept(hSockArr[sigEventIdx], (SOCKADDR*)&clntAdr, &clntAdrLen);
					newEvent = WSACreateEvent();
					WSAEventSelect(hClntSock, newEvent, FD_READ | FD_CLOSE);

					hEventArr[numOfClntSock] = newEvent;
					hSockArr[numOfClntSock] = hClntSock;

					numOfClntSock++;

					puts("connected new client....");
				}

				if (netEvents.lNetworkEvents & FD_READ) {
					if (netEvents.iErrorCode[FD_READ_BIT] != 0)	{
						puts("Read Error");
						break;
					}
					strLen = recv(hSockArr[sigEventIdx], msg, sizeof(msg), 0);
					send(hSockArr[sigEventIdx], msg, strLen, 0);
				}

				if (netEvents.lNetworkEvents & FD_CLOSE) {
					if (netEvents.iErrorCode[FD_CLOSE_BIT] != 0) {
						puts("Close Error");
						break;
					}
					WSACloseEvent(hEventArr[sigEventIdx]);
					closesocket(hSockArr[sigEventIdx]);

					numOfClntSock--;
					CompressSockets(hSockArr, sigEventIdx, numOfClntSock);
					CompressEvents(hEventArr, sigEventIdx, numOfClntSock);
				}
			}
		}
	}

	WSACleanup();
	return 0;


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

void CompressSockets(SOCKET hSockArr[], int idx, int total) {
	
	int i;
	for (i = idx; i < total; i++)
		hSockArr[i] = hSockArr[i + 1];
}


void CompressEvents(WSAEVENT hEventArr[], int idx, int total) {

	int i;
	for (i = idx; i < total; i++)
		hEventArr[i] = hEventArr[i + 1];
}
