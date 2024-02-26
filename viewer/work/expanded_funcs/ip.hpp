#include <asio.hpp>
#include <iostream>


// �������� ������� tcp::resolver ������� ��������� ��� ����� � ���������� IP - ������, 
// ��������� � ���� ������.
// ����� �� ���������� ���������� IP - ������ � ������� ������ IPv4 �����, ��������� � ��������� ����.
// ��� �� �������� ��� ip.

// TODO: 
// ������� ��������� ������� CustomMsgTypes::ClientIp
// �������� ������ � ������������ ��������������� ����� � headers/helpers - 
//		- ������ ������ � ������
// ������� �������� ��������� ���� CustomMsgTypes::ClientIp � server.cpp / client.cpp
// ���������� asio ��� ���������� � net_common.h -
//					- ��������� �������������� ��������� <asio.hpp>

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