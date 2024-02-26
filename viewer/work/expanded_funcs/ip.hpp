#include <asio.hpp>
#include <iostream>


// Создание объекта tcp::resolver который разрешает имя хоста и возвращает IP - адреса, 
// связанные с этим именем.
// Затем он перебирает полученные IP - адреса и выводит первый IPv4 адрес, найденный в локальной сети.
// Так мы получаем наш ip.

// TODO: 
// Создать состояние объекта CustomMsgTypes::ClientIp
// Добавить логику в заголовочные вспомогательные файлы в headers/helpers - 
//		- учесть клиент и сервер
// Описать принятие сообщений типа CustomMsgTypes::ClientIp в server.cpp / client.cpp
// Библиотека asio уже подключена в net_common.h -
//					- устранить дополнительные включение <asio.hpp>

void get_ip() {
	using asio::ip::tcp;
	asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(asio::ip::host_name(), "");
	tcp::resolver::iterator it = resolver.resolve(query);

	while (it != tcp::resolver::iterator())
	{
		tcp::endpoint ep = *it++;
		if (ep.address().is_v4())
		{
			std::cout << "IP : " << ep.address().to_string();
			break;
		}
	}
}