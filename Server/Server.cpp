#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream> 
#include <cstdio> 
#include <cstring> 
#include <winsock2.h> 

#pragma comment(lib, "WS2_32.lib")

using namespace std;

DWORD WINAPI serverReceive(LPVOID lpParam) {
	char buffer[1024] = { 0 };
	SOCKET client = (SOCKET)lpParam;

	// Загадываем число
	srand(GetTickCount());
	int secretNumber = rand() % 100 + 1;

	// Начало игры
	std::string startMessage = "Let's play a game! I'm thinking of a number between 1 and 100. You have 7 attempts to guess it.";
	int result = send(client, startMessage.c_str(), startMessage.length(), 0);
	if (result == SOCKET_ERROR) {
		cout << "send function failed with error " << WSAGetLastError() << endl;
		return -1;
	}

	int attempts = 0;
	while (attempts < 7) {
		// Получаем данные от клиента
		result = recv(client, buffer, sizeof(buffer), 0);
		if (result == SOCKET_ERROR) {
			cout << "recv function failed with error " << WSAGetLastError() << endl;
			return -1;
		}

		// Проверяем попытку
		int guess = atoi(buffer);
		if (guess == secretNumber) {
			// Отправляем сообщение о победе
			std::string winMessage = "Congratulations, you guessed the number!";
			result = send(client, winMessage.c_str(), winMessage.length(), 0);
			if (result == SOCKET_ERROR) {
				cout << "send function failed with error " << WSAGetLastError() << endl;
				return -1;
			}
			// Завершаем игру
			break;
		}
		else {
			// Сообщаем клиенту, было ли число больше или меньше загаданного
			std::string message = (guess < secretNumber) ? "Your guess was too low." : "Your guess was too high.";
			result = send(client, message.c_str(), message.length(), 0);
			if (result == SOCKET_ERROR) {
				cout << "send function failed with error " << WSAGetLastError() << endl;
				return -1;
			}
			attempts++;
		}

		memset(buffer, 0, sizeof(buffer));
	}

	// Если клиент не угадал число за 7 попыток, отправляем сообщение об этом
	if (attempts == 7) {
		std::string loseMessage = "Sorry, you didn't guess the number within 7 attempts.";
		result = send(client, loseMessage.c_str(), loseMessage.length(), 0);
		if (result == SOCKET_ERROR) {
			cout << "send function failed with error " << WSAGetLastError() << endl;
			return -1;
		}
	}

	// Завершение игры
	std::string endMessage = "Thank you for playing! Enter \"yes\" to start a new game, or any other message to quit.";
	result = send(client, endMessage.c_str(), endMessage.length(), 0);
	if (result == SOCKET_ERROR) {
		cout << "send function failed with error " << WSAGetLastError() << endl;
		return -1;
	}

	return 1;
}

DWORD WINAPI serverSend(LPVOID lpParam) {
	char buffer[1024] = { 0 };
	SOCKET client = *(SOCKET*)lpParam;

	while (true) {
		// Получаем ответ от клиента
		if (recv(client, buffer, sizeof(buffer), 0) == SOCKET_ERROR) {
			cout << "recv function failed with error " << WSAGetLastError() << endl;
			return -1;
		}

		// Если клиент хочет начать новую игру, отправляем сообщение об этом на сервер
		std::string response = buffer;
		if (response == "yes\n") {
			std::string startMessage = "Starting a new game...";
			send(client, startMessage.c_str(), startMessage.length(), 0);
		}
		else {
			// Закрываем соединение с клиентом
			std::string quitMessage = "Thank you for playing!";
			send(client, quitMessage.c_str(), quitMessage.length(), 0);
			break;
		}

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
		HANDLE t2 = CreateThread(NULL, 0, serverSend, &client, 0, &tid);
		if (t2 == NULL) {
			cout << "Thread Creation Error: " << WSAGetLastError() << endl;
		}

		WaitForSingleObject(t1, INFINITE);
		WaitForSingleObject(t2, INFINITE);

		closesocket(client);
		if (closesocket(server) == SOCKET_ERROR) {
			cout << "Close socket failed with error: " << WSAGetLastError() << endl;
			return -1;
		}
		WSACleanup();
	}
}