#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<WinSock2.h>
#include<process.h>
#include<windows.h>

// Define
#define BUF_SIZE 100

// Socket ���� ������ �� �� �κ� ���� �ٽ� ��迭 �ϴ� �Լ�
void CompressSockets(SOCKET hSockArr[], int idx, int total);
void CompressEvents(SOCKET hEventArr[], int idx, int total);

void ErrorHandling(char* msg);

int main(int argc, char* argv[]) 
{
	// Socket���� ����
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;

	// Notification ���� ����
	SOCKET hSockArr[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT hEventArr[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT newEvent;
	WSANETWORKEVENTS netEvents;

	// �Ϲ� ����
	int numOfClntSock = 0;
	int strLen, i;
	int posInfo, startIdx;
	int clntAdrLen;
	char msg[BUF_SIZE];

	// main argc ��ȿ�� �˻�
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		getchar();
		exit(1);
	}

	// WSAStartup 
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

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

	fputs("���� ���� ������ ��....\n", stdout);

	// Listening
	if (listen(hServSock, 5) == SOCKET_ERROR) {
		ErrorHandling("listen() error!");
	}
	
	// �������� FD_ACCEPT ���, �̺�Ʈ ����
	newEvent = WSACreateEvent();
	if (WSAEventSelect(hServSock, newEvent, FD_ACCEPT) == SOCKET_ERROR)
		ErrorHandling("WSAEventSelect error!");

	// ���� ������ 0�� �ε���
	hSockArr[numOfClntSock] = hServSock;
	hEventArr[numOfClntSock] = newEvent;
	numOfClntSock++;

	while (1) {
		
		// Event�� Ȱ��ȭ�� �� ù��° Index �о��
		posInfo = WSAWaitForMultipleEvents(numOfClntSock, hEventArr, FALSE, WSA_INFINITE, FALSE);
		startIdx = posInfo - WSA_WAIT_EVENT_0;


		for (i = startIdx; i < numOfClntSock; i++) {
			
			// 1���� �̺�Ʈ�� Ȱ��ȭ �Ǿ����� ����
			int sigEventIdx = WSAWaitForMultipleEvents(1, hEventArr, TRUE, 0, FALSE);

			// Ÿ�Ӿƿ� �ǰų� Wait ���� ������ ���� �̺�Ʈ ����� �Ѿ
			if ((sigEventIdx == WSA_WAIT_FAILED) || (sigEventIdx == WSA_WAIT_TIMEOUT))
				continue;
			// �̺�Ʈ�� Ȱ��ȭ �Ǿ������� ���� ����
			else {
				sigEventIdx = i;
				
				// ���� ���ϰ� �̺�Ʈ�� netEvent ����
				WSAEnumNetworkEvents(hSockArr[sigEventIdx], hEventArr[sigEventIdx], &netEvents);

				// Event�� Ÿ�Կ� �´� ���� ����
				if (netEvents.lNetworkEvents & FD_ACCEPT) {		// ���� ��û��

					if (netEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {		// ���� ó��
						puts("Accept Error");
						break;
					}

					// ���� ����
					clntAdrLen = sizeof(clntAdr);
					hClntSock = accept(hSockArr[sigEventIdx], (SOCKADDR*)&clntAdr, &clntAdrLen);

					// �̺�Ʈ ������ Ŭ���̾�Ʈ ���Ͽ� �̺�Ʈ ����
					newEvent = WSACreateEvent();
					WSAEventSelect(hClntSock, newEvent, FD_READ | FD_CLOSE);

					// �̺�Ʈ/Socket �迭�� ����
					hEventArr[numOfClntSock] = newEvent;
					hSockArr[numOfClntSock] = hClntSock;
					numOfClntSock++;

					puts("connected new Client...");
				}
				
				// ���� ����Ÿ�� ������ �˸��� �̺�Ʈ�� ��
				if (netEvents.lNetworkEvents & FD_READ) {
					
					// ���� ó��
					if (netEvents.iErrorCode[FD_READ_BIT] != 0)
					{
						puts("Read Error");
						break;
					}
					
					// ���� ������ ����
					strLen = recv(hSockArr[sigEventIdx], msg, sizeof(msg), 0);

					printf("Client : %d , Message : %s\n", sigEventIdx, msg);
					printf("Echo complete\n");

					send(hSockArr[sigEventIdx], msg, strLen, 0);
				}

				// ���� ��û��
				if (netEvents.lNetworkEvents & FD_CLOSE) {
					
					// ���� ó��
					if (netEvents.iErrorCode[FD_CLOSE_BIT] != 0)
					{
						puts("Close Error");
						break;
					}

					printf("client : %d. close", sigEventIdx);

					// �̺�Ʈ/socket close
					WSACloseEvent(hEventArr[sigEventIdx]);
					closesocket(hSockArr[sigEventIdx]);
					numOfClntSock--;

					// �迭 ���ġ
					CompressEvents(hEventArr, sigEventIdx, numOfClntSock);
					CompressSockets(hSockArr, sigEventIdx, numOfClntSock);
				}
			}
		}
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

// Socket ��迭
void CompressSockets(SOCKET hSockArr[], int idx, int total) {
	
	int i;
	for (i = idx; i < total; i++)
		hSockArr[i] = hSockArr[i + 1];
}

// Event ��迭
void CompressEvents(SOCKET hEventArr[], int idx, int total) {
	
	int i;
	for (i = idx; i < total; i++)
		hEventArr[i] = hEventArr[i + 1];
}
