#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<WinSock2.h>
#include<process.h>
#include<windows.h>

// Define
#define BUF_SIZE 100

// Socket 연결 끊어질 때 그 부분 빼고 다시 재배열 하는 함수
void CompressSockets(SOCKET hSockArr[], int idx, int total);
void CompressEvents(SOCKET hEventArr[], int idx, int total);

void ErrorHandling(char* msg);

int main(int argc, char* argv[]) 
{
	// Socket관련 변수
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;

	// Notification 관련 변수
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

	// main argc 유효성 검사
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		getchar();
		exit(1);
	}

	// WSAStartup 
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	// 서버 소켓 생성
	hServSock = socket(PF_INET, SOCK_STREAM, 0);
	if (hServSock == INVALID_SOCKET) {
		ErrorHandling("socket() error");
	}
	
	// 서버 소켓 설정
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	fputs("서버 소켓 바인딩 중....\n", stdout);

	// 소켓 바인드
	if (bind(hServSock, (SOCKADDR*) &servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
		ErrorHandling("bind() error");
	}

	fputs("서버 소켓 리스닝 중....\n", stdout);

	// Listening
	if (listen(hServSock, 5) == SOCKET_ERROR) {
		ErrorHandling("listen() error!");
	}
	
	// 서버소켓 FD_ACCEPT 대기, 이벤트 연결
	newEvent = WSACreateEvent();
	if (WSAEventSelect(hServSock, newEvent, FD_ACCEPT) == SOCKET_ERROR)
		ErrorHandling("WSAEventSelect error!");

	// 서버 소켓은 0번 인덱스
	hSockArr[numOfClntSock] = hServSock;
	hEventArr[numOfClntSock] = newEvent;
	numOfClntSock++;

	while (1) {
		
		// Event가 활성화된 맨 첫번째 Index 읽어옴
		posInfo = WSAWaitForMultipleEvents(numOfClntSock, hEventArr, FALSE, WSA_INFINITE, FALSE);
		startIdx = posInfo - WSA_WAIT_EVENT_0;


		for (i = startIdx; i < numOfClntSock; i++) {
			
			// 1개씩 이벤트가 활성화 되었는지 조사
			int sigEventIdx = WSAWaitForMultipleEvents(1, hEventArr, TRUE, 0, FALSE);

			// 타임아웃 되거나 Wait 실패 했을때 다음 이벤트 조사로 넘어감
			if ((sigEventIdx == WSA_WAIT_FAILED) || (sigEventIdx == WSA_WAIT_TIMEOUT))
				continue;
			// 이벤트가 활성화 되어있으면 소켓 읽음
			else {
				sigEventIdx = i;
				
				// 현재 소켓과 이벤트로 netEvent 설정
				WSAEnumNetworkEvents(hSockArr[sigEventIdx], hEventArr[sigEventIdx], &netEvents);

				// Event의 타입에 맞는 로직 수행
				if (netEvents.lNetworkEvents & FD_ACCEPT) {		// 연결 요청시

					if (netEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {		// 에러 처리
						puts("Accept Error");
						break;
					}

					// 소켓 연결
					clntAdrLen = sizeof(clntAdr);
					hClntSock = accept(hSockArr[sigEventIdx], (SOCKADDR*)&clntAdr, &clntAdrLen);

					// 이벤트 생성후 클라이언트 소켓에 이벤트 연결
					newEvent = WSACreateEvent();
					WSAEventSelect(hClntSock, newEvent, FD_READ | FD_CLOSE);

					// 이벤트/Socket 배열에 저장
					hEventArr[numOfClntSock] = newEvent;
					hSockArr[numOfClntSock] = hClntSock;
					numOfClntSock++;

					puts("connected new Client...");
				}
				
				// 읽을 데이타가 있음을 알리는 이벤트일 떄
				if (netEvents.lNetworkEvents & FD_READ) {
					
					// 에러 처리
					if (netEvents.iErrorCode[FD_READ_BIT] != 0)
					{
						puts("Read Error");
						break;
					}
					
					// 읽은 다음에 에코
					strLen = recv(hSockArr[sigEventIdx], msg, sizeof(msg), 0);

					printf("Client : %d , Message : %s\n", sigEventIdx, msg);
					printf("Echo complete\n");

					send(hSockArr[sigEventIdx], msg, strLen, 0);
				}

				// 종료 요청시
				if (netEvents.lNetworkEvents & FD_CLOSE) {
					
					// 에러 처리
					if (netEvents.iErrorCode[FD_CLOSE_BIT] != 0)
					{
						puts("Close Error");
						break;
					}

					printf("client : %d. close", sigEventIdx);

					// 이벤트/socket close
					WSACloseEvent(hEventArr[sigEventIdx]);
					closesocket(hSockArr[sigEventIdx]);
					numOfClntSock--;

					// 배열 재배치
					CompressEvents(hEventArr, sigEventIdx, numOfClntSock);
					CompressSockets(hSockArr, sigEventIdx, numOfClntSock);
				}
			}
		}
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

// Socket 재배열
void CompressSockets(SOCKET hSockArr[], int idx, int total) {
	
	int i;
	for (i = idx; i < total; i++)
		hSockArr[i] = hSockArr[i + 1];
}

// Event 재배열
void CompressEvents(SOCKET hEventArr[], int idx, int total) {
	
	int i;
	for (i = idx; i < total; i++)
		hEventArr[i] = hEventArr[i + 1];
}
