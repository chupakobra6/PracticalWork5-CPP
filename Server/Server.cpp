#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream> 
#include <cstdio> 
#include <cstring> 
#include <winsock2.h>

#pragma comment(lib, "WS2_32.lib")

using namespace std;

void playGuessNumber(SOCKET client) {
	srand((unsigned)time(NULL)); // инициализируем генератор случайных чисел
	int secretNumber = rand() % 100 + 1; // загадываем число от 1 до 100
	int attemptsLeft = 7; // количество попыток

	char buffer[1024] = { 0 };
	sprintf_s(buffer, "I'm thinking of a number between 1 and 100. You have %d attempts to guess it.\n", attemptsLeft);
	send(client, buffer, strlen(buffer), 0);

	while (attemptsLeft > 0) {
		memset(buffer, 0, sizeof(buffer));
		if (recv(client, buffer, sizeof(buffer), 0) == SOCKET_ERROR) {
			cout << "recv function failed with error: " << WSAGetLastError() << endl;
			return;
		}

		int guess = atoi(buffer);
		if (guess == secretNumber) {
			sprintf_s(buffer, "You guessed the number! Congratulations!\n");
			send(client, buffer, strlen(buffer), 0);
			break;
		}
		else if (guess > secretNumber) {
			sprintf_s(buffer, "Your guess is too high. %d attempts left.\n", attemptsLeft - 1);
			send(client, buffer, strlen(buffer), 0);
		}
		else {
			sprintf_s(buffer, "Your guess is too low. %d attempts left.\n", attemptsLeft - 1);
			send(client, buffer, strlen(buffer), 0);
		}

		attemptsLeft--;
		if (attemptsLeft == 0) {
			sprintf_s(buffer, "You didn't guess the number. The number was %d.\n", secretNumber);
			send(client, buffer, strlen(buffer), 0);
		}
	}
	sprintf_s(buffer, "Do you want to play again? (yes or no)\n");
	send(client, buffer, strlen(buffer), 0);

	memset(buffer, 0, sizeof(buffer));
	if (recv(client, buffer, sizeof(buffer), 0) == SOCKET_ERROR) {
		cout << "recv function failed with error: " << WSAGetLastError() << endl;
		return;
	}

	if (strcmp(buffer, "yes\n") == 0) {
		playGuessNumber(client);
	}
	else {
		sprintf_s(buffer, "Thank you for playing!\n");
		send(client, buffer, strlen(buffer), 0);
	}
}

DWORD WINAPI serverReceive(LPVOID lpParam) {
	char buffer[1024] = { 0 };
	SOCKET client = *(SOCKET*)lpParam;
	while (true) {
		if (recv(client, buffer, sizeof(buffer), 0) == SOCKET_ERROR) {
			cout << "recv function failed with error: " << WSAGetLastError() << endl;
			return -1;
		}
		if (strcmp(buffer, "exit\n") == 0) {
			cout << "Client Disconnected." << endl;
			break;
		}
		playGuessNumber(client);
		memset(buffer, 0, sizeof(buffer));
	}
	return 1;
}

int main() {
	WSADATA WSAData;
	SOCKET server, client;
	SOCKADDR_IN serverAddr, clientAddr;
	WSAStartup(MAKEWORD(2, 0), &WSAData);

	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server == INVALID_SOCKET) {
		cout << "Socket creation failed with error:" << WSAGetLastError() << endl;
		return -1;
	}
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(5555);
	if (bind(server, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		cout << "Bind function failed with error: " << WSAGetLastError() << endl;
		return -1;
	}

	if (listen(server, 0) == SOCKET_ERROR) {
		cout << "Listen function failed with error:" << WSAGetLastError() << endl;
		return -1;
	}
	cout << "Listening for incoming connections...." << endl;

	char buffer[1024];
	int clientAddrSize = sizeof(clientAddr);
	if ((client = accept(server, (SOCKADDR*)&clientAddr, &clientAddrSize)) != INVALID_SOCKET) {
		cout << "Client connected!" << endl;

		DWORD tid;
		HANDLE t1 = CreateThread(NULL, 0, serverReceive, &client, 0, &tid);
		if (t1 == NULL) {
			cout << "Thread Creation Error: " << WSAGetLastError() << endl;
		}

		WaitForSingleObject(t1, INFINITE);

		closesocket(client);
		if (closesocket(server) == SOCKET_ERROR) {
			cout << "Close socket failed with error: " << WSAGetLastError() << endl;
			return -1;
		}
		WSACleanup();
	}
}