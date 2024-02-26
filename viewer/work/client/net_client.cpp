#include <iostream>
#include <helpers/olc_net.h>
#include <fstream>
#include <string>
#include <cstdlib> 
/*
Набор возможных состояний объекта.
Каждое состояние описывает маркеры заголовков сообщения, отправленные и полученные с сервера.
*/


enum class CustomMsgTypes : uint32_t
{
	ServerAccept, ///< Сервер одобрил подключение клиента
	ServerDeny, ///< Сервер отклонил клиентское соединение
	ServerMessage,

	ServerPing, ///< Запрашивает пинг сервера
	SayHello, ///< Отправляет соответствующую команду на сервер
};


/*
Предоставляет интерфейс для работы клиента с сервером.
*/
class CustomClient : public olc::net::client_interface<CustomMsgTypes>
{
	public:

		/*
		На сервер отправляется сообщение с отметкой времени
		*/
		void PingServer()
		{
			
			/// Создание сообщения специального формата
			olc::net::message<CustomMsgTypes> msg;
			
			/// Маркировка заголовка
			msg.header.id = CustomMsgTypes::ServerPing;

			/// Создание временной точки
			std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

			/// Сохранение данных в сообщении
			msg << timeNow;

			/// Отправка сообщения
			Send(msg);
		}

		/*
		На сервер отправляется пустое сообщение с маркером SayHello
		*/
		void SayHello()
		{
			olc::net::message<CustomMsgTypes> msg;
			msg.header.id = CustomMsgTypes::SayHello;
			Send(msg);
		}
};

int main()
{
	//ShellExecute(nullptr, "open", NULL, NULL, nullptr, SW_HIDE);
	/// Вывод приветствия
	std::cout << "--------------------\nWelcome to AViewer\n\nConnecting to server...\n\n";
	
	/// Создание объекта класса
	CustomClient c;

	/// Подключение к серверу
	c.Connect("127.0.0.1", 6000);

	/// Подключение к серверным массивам, в которых будут храниться состояния кнопок
	bool key[3]{ false };
	bool old_key[3]{ false };

	/// bQuit - Флаг для выхода из цикла прослушивания сервера
	bool bQuit = false;
	while (!bQuit)
	{	
		key[0] = GetAsyncKeyState('1') & 0x8000;
		key[1] = GetAsyncKeyState('2') & 0x8000;
		key[2] = GetAsyncKeyState('0') & 0x8000;

		/*if (GetForegroundWindow() == GetConsoleWindow())
		{
			key[0] = GetAsyncKeyState('1') & 0x8000;
			key[1] = GetAsyncKeyState('2') & 0x8000;
			key[2] = GetAsyncKeyState('0') & 0x8000;
		}*/
		{
			/// Проверка клавиш на клавиатуре для вызова соответствующих функций
			if (key[0] && !old_key[0]) c.PingServer();
			if (key[1] && !old_key[1]) c.SayHello();
			if (key[2] && !old_key[2]) bQuit = true;
		}

		for (int i = 0; i < 2; i++) old_key[i] = key[i];

		/// Проверка подключения
		if (c.IsConnected())
		{
			if (!c.Incoming().empty())
			{
				auto msg = c.Incoming().pop_front().msg;

				/*
				После того, как сообщение принято, его заголовок повторяется.
				В зависимости от значения заголовка клиенту выводятся соответствующие сообщения
				*/
				switch (msg.header.id){
					case CustomMsgTypes::ServerAccept:
					{
						std::cout << "Server approve connection!\n"; 
						std::cout << "\t [ 1 ] - Ping-Pong\n";
						std::cout << "\t [ 2 ] - Say Hello!\n";
						std::cout << "\t [ 0 ] - Disconnect\n";
					}
					break;

					case CustomMsgTypes::ServerPing:
					{
						/// Сообщение получено с указанием момента создания его отправки.
						/// Затем создается временная точка настоящего времени
						/// Результирующая разница выводится клиенту в виде значения ping
						std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
						std::chrono::system_clock::time_point timeThen;
						msg >> timeThen;
						std::cout << "ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
					}
					break;

					case CustomMsgTypes::SayHello:
					{
						uint32_t clientID;
						msg >> clientID;
						std::cout << "---[" << clientID << "]: send <Hello> to everyone\n";
					}
					break;
				}
			}
		}
		/// Отключитесь от сервера
		else
		{
			std::cout << "Server has down... \n";
			bQuit = true;
		}

	}

	return 0;
}
