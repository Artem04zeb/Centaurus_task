#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_connection.h"

namespace olc
{
	namespace net
	{
		template<typename T>
		class server_interface
		{
		public:
			/// Создаем сервер, готовый к прослушиванию через указанный порт
			server_interface(uint16_t port)
				: m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
			{

			}

			virtual ~server_interface()
			{
				Stop();
			}

			// Запускаем сервер!
			bool Start()
			{
				try
				{
					/*
					Передача задачи контексту asio - это важно, так как это добавит в контекст "work" и 
					предотвратит его немедленный выход. 
					Поскольку это сервер, мы хотим, чтобы он был готов к работе с клиентами, 
					пытающимися подключиться.
					*/
					WaitForClientConnection();

					/// Запуск контекста asio в его собственном потоке
					m_threadContext = std::thread([this]() { m_asioContext.run(); });
				}
				catch (std::exception& e)
				{
					std::cerr << "[SERVER] Exception: " << e.what() << "\n";
					return false;
				}

				std::cout << "[SERVER] Started!\n";
				return true;
			}

			/// Стоп сервер!
			void Stop()
			{
				/// Запрос закрытие контекста
				m_asioContext.stop();

				/// Приведим в порядок контекстный поток
				if (m_threadContext.joinable()) m_threadContext.join();

				std::cout << "[SERVER] Stopped!\n";
			}

			/// ASYNC - запрос asio дождаться подключения
			void WaitForClientConnection()
			{
				/*
				Основной контекст с инструкцией дождаться подключения сокета. 
				Это и есть назначение объекта "acceptor". 
				Это обеспечит уникальный сокет для каждой попытки входящего подключения
				*/
				m_asioAcceptor.async_accept(
					[this](std::error_code ec, asio::ip::tcp::socket socket)
					{
						/// Вызывается входящим запросом на подключение
						if (!ec)
						{
							/// Отобразить некоторую полезную информацию
							std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";

							/// Создание нового подключение для работы с этим клиентом
							std::shared_ptr<connection<T>> newconn =
								std::make_shared<connection<T>>(connection<T>::owner::server,
									m_asioContext, std::move(socket), m_qMessagesIn);



							/// Дайте серверу пользователя возможность запретить подключение
							if (OnClientConnect(newconn))
							{
								/// Подключение разрешено, поэтому добавляйте в контейнер новые подключения
								m_deqConnections.push_back(std::move(newconn));
								/*
								И очень важно! Отправьте задание контексту asio соединения, 
								чтобы он ждал поступления байтов
								*/
								m_deqConnections.back()->ConnectToClient(nIDCounter++);

								std::cout << "[" << m_deqConnections.back()->GetID() << "] Connection Approved\n";
							}
							else
							{
								std::cout << "[-----] Connection Denied\n";
								/*
								Соединение выйдет за пределы области видимости без каких-либо отложенных задач, 
								поэтому будет автоматически уничтожено из-за чуда интеллектуальных указателей
								*/
							}
						}
						else
						{
							/// Во время приема произошла ошибка
							std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
						}
						/*
						Загрузите контекст asio дополнительной работой - снова 
						просто дождаться другого подключения
						*/
						WaitForClientConnection();
					});
			}

			/// Отправка сообщение конкретному клиенту
			void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg)
			{
				/// Проверьте, является ли клиент законным...
				if (client && client->IsConnected())
				{
					/// ...и отправьте сообщение через соединение
					client->Send(msg);
				}
				else
				{
					/*
					Если мы не можем связаться с клиентом, то мы также можем 
					удалить клиента - сообщите серверу, возможно, 
					он каким-то образом отслеживает его
					*/
					OnClientDisconnect(client);

					client.reset();

					/// Затем физически извлеките его из контейнера
					m_deqConnections.erase(
					std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
				}
			}

			/// Отправить сообщение всем клиентам
			void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr)
			{
				bool bInvalidClientExists = false;

				/// Выполнить итерацию по всем клиентам в контейнере
				for (auto& client : m_deqConnections)
				{
					/// Проверьте, подключен ли клиент...
					if (client && client->IsConnected())
					{
						if (client != pIgnoreClient)
							client->Send(msg);
					}
					else
					{
						/// С клиентом связаться не удалось, поэтому предположим, что он отключился.
						OnClientDisconnect(client);
						client.reset();

						/// Установите этот флаг, чтобы затем удалить мертвые клиенты из контейнера
						bInvalidClientExists = true;
					}
				}
				/*
				Удаляйте мертвых клиентов за один раз - таким образом, мы 
				не делаем контейнер недействительным при повторном просмотре.
				*/
				if (bInvalidClientExists)
					m_deqConnections.erase(
						std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());
			}

			/// Принудительно заставлять сервер отвечать на входящие сообщения
			void Update(size_t nMaxMessages = -1, bool bWait = false)
			{
				if (bWait) m_qMessagesIn.wait();

				/// Обработайте столько сообщений, сколько сможете, до указанного значения
				size_t nMessageCount = 0;
				while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty())
				{
					auto msg = m_qMessagesIn.pop_front();

					/// Передать сообщение обработчику сообщений
					OnMessage(msg.remote, msg.msg);

					nMessageCount++;
				}
			}

		protected:
			/*
			Этот серверный класс должен переопределять эти функции 
			для реализации настраиваемой функциональности
			*/

			/// Вызывается при подключении клиента, вы можете наложить вето на подключение, вернув значение false
			virtual bool OnClientConnect(std::shared_ptr<connection<T>> client)
			{
				return false;
			}

			/// Вызывается, когда кажется, что клиент отключился
			virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client)
			{ }

			/// Вызывается при поступлении сообщения
			virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T>& msg) { }

			/// Потокобезопасная очередь для входящих пакетов сообщений
			tsqueue<owned_message<T>> m_qMessagesIn;

			/// Контейнер активных проверенных подключений
			std::deque<std::shared_ptr<connection<T>>> m_deqConnections;

			/// Порядок объявления важен - это также порядок инициализации
			asio::io_context m_asioContext;
			std::thread m_threadContext;

			/// Эти вещи нуждаются в контексте asio
			asio::ip::tcp::acceptor m_asioAcceptor; 

			/// Клиенты будут идентифицированы в "более широкой системе" с помощью идентификатора
			uint32_t nIDCounter = 1;
		};
	}
}
