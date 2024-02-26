#include <iostream>
#include <helpers/olc_net.h>

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
Описывает работу сервера при получении сообщений от клиента
*/
class CustomServer : public olc::net::server_interface<CustomMsgTypes>
{
public:
	CustomServer(uint16_t nPort) : olc::net::server_interface<CustomMsgTypes>(nPort){}

protected:

	/*
	Описание: когда клиент подключается к серверу, он создает сообщение, помечая
	заголовок заголовком "ServerAccept". Затем отправляет это сообщение клиенту.
	Далее клиент обработает это сообщение в соответствии с инструкциями.
	*/
	virtual bool OnClientConnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client)
	{
		olc::net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::ServerAccept;
		client->Send(msg);
		return true;
	}

	// Вызывается, когда кажется, что клиент отключился
	virtual void OnClientDisconnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client)
	{
		std::cout << "Disconnect client [" << client->GetID() << "]\n";
	}

	// Вызывается при поступлении сообщения
	virtual void OnMessage(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client, olc::net::message<CustomMsgTypes>& msg)
	{

		/*
		Вызывается при поступлении сообщения При получении сообщения от клиента проверяется его заголовок.
		Далее выполняются следующие действия:
		1. Сервер отображает соответствующую информацию о действиях клиента.
		2. Генерируется сообщение требуемого формата, которое отправляется всем клиентам сети.
		((смотрите каждый случай отдельно)s
		*/
		switch (msg.header.id)
		{
			case CustomMsgTypes::ServerPing:
			{
				std::cout << "[" << client->GetID() << "]: checking ping\n";
				/*
				Просто отправьте сообщение обратно.
				Он уже содержит необходимую информацию для клиента,
				поскольку в него встроена временная метка отправки
				*/
				client->Send(msg);
			}
			break;

			case CustomMsgTypes::SayHello:
			{
				 std::cout << "[" << client->GetID() << "]: send <Hello!> to everyone\n";

				olc::net::message<CustomMsgTypes> msg;
				msg.header.id = CustomMsgTypes::SayHello;
				msg << client->GetID();
				MessageAllClients(msg, client);

			}
			break;
		}
	}
};

int main()
{
	CustomServer server(6000);
	
	server.Start();

	while (1)
	{
		server.Update(-1, true);
	}

	return 0;
}
