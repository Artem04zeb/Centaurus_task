#pragma once
#include "net_common.h"

namespace olc
{
	namespace net
	{
		template <typename T>
		class client_interface
		{
		public:
			client_interface()
			{}

			virtual ~client_interface()
			{
				// Если клиент уничтожен, то попытаться отключиться от сервера
				Disconnect();
			}

			// Поджключение к серверу с помощью имя хоста/ip-address and port
			bool Connect(const std::string& host, const uint16_t port)
			{
				try
				{
					// Преобразование имя_хоста/ip-адреса в физический адрес
					asio::ip::tcp::resolver resolver(m_context);
					asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

					// Создание соединения
					m_connection = std::make_unique<connection<T>>(connection<T>::owner::client, m_context, asio::ip::tcp::socket(m_context), m_qMessagesIn);

					// Указание объекту подключения, чтобы он покдлючился к серверу
					m_connection->ConnectToServer(endpoints);

					// Запуск контекста и потока
					thrContext = std::thread([this]() { m_context.run(); });
				}
				catch (std::exception& e)
				{
					std::cerr << "Client Exception: " << e.what() << "\n";
					return false;
				}
				return true;
			}

			// Отключение от сервера
			void Disconnect()
			{
				// Если соединение существует - разорвать его
				if (IsConnected()) m_connection->Disconnect();

				// При любом раскладе прерываем контекст и поток		
				m_context.stop();
				if (thrContext.joinable()) thrContext.join();

				// Уничтожаем объект соединения
				m_connection.release();
			}

			/// Проверка, действительно ли клиент подключен к серверу
			bool IsConnected()
			{
				if (m_connection)
					return m_connection->IsConnected();
				else
					return false;
			}

			/// Отправить сообщение на сервер
			void Send(const message<T>& msg)
			{
				if (IsConnected())
					m_connection->Send(msg);
			}

			/// Извлекать очередь сообщений с сервера
			tsqueue<owned_message<T>>& Incoming()
			{
				return m_qMessagesIn;
			}

		protected:
			/*
			Контекст Asio обрабатывает передачу данных, 
			но для выполнения своей работы ему нужен собственный поток 
			*/			
			asio::io_context m_context;
			std::thread thrContext;
			/// У клиента есть единственный экземпляр объекта "connection", который обрабатывает передачу данных
			std::unique_ptr<connection<T>> m_connection;

		private:
			/// Это потокобезопасная очередь входящих сообщений с сервера
			tsqueue<owned_message<T>> m_qMessagesIn;
		};
	}
}
