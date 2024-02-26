#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace olc
{
	namespace net
	{
		template<typename T>
		class connection : public std::enable_shared_from_this<connection<T>>
		{
		public:
			/*
			Соединение "принадлежит" либо серверу, либо клиенту, и его
			поведение в обоих случаях немного отличается.
			*/
			enum class owner
			{
				server,
				client
			};

			/*
			Конструктор: Укажите владельца, подключитесь к контексту, передайте сокет
			Укажите ссылку на очередь входящих сообщений
			*/
			connection(owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& qIn)
				: m_asioContext(asioContext), m_socket(std::move(socket)), m_qMessagesIn(qIn)
			{
				m_nOwnerType = parent;
			}

			virtual ~connection()
			{}
			/*
			This ID is used system wide - its how clients will understand other clients
			exist across the whole system.
			*/
			uint32_t GetID() const
			{
				return id;
			}

			void ConnectToClient(uint32_t uid = 0)
			{
				if (m_nOwnerType == owner::server)
				{
					if (m_socket.is_open())
					{
						id = uid;
						ReadHeader();
					}
				}
			}

			void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
			{
				/// Только клиенты могут подключаться к серверам
				if (m_nOwnerType == owner::client)
				{
					/// Asio-запрос на подключение к конечной точке
					asio::async_connect(m_socket, endpoints,
						[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
						{
							if (!ec)
							{
								ReadHeader();
							}
						});
				}
			}


			void Disconnect()
			{
				if (IsConnected())
					asio::post(m_asioContext, [this]() { m_socket.close(); });
			}

			bool IsConnected() const
			{
				return m_socket.is_open();
			}
			
			/*
			Асинхронная отправка сообщения, соединения являются взаимно-однозначными, поэтому нет необходимости указывать
			целью для клиента является сервер, и наоборот
			*/
			void Send(const message<T>& msg)
			{
				asio::post(m_asioContext,
					[this, msg]()
					{
						/*
						Если в очереди есть сообщение, то мы должны
						предположить, что оно находится в процессе асинхронной записи.
						В любом случае добавьте сообщение в очередь для вывода. Если сообщений нет
						были доступны для записи, затем запустите процесс записи
						сообщения в начале очереди.
						*/
						bool bWritingMessage = !m_qMessagesOut.empty();
						m_qMessagesOut.push_back(msg);
						if (!bWritingMessage)
						{
							WriteHeader();
						}
					});
			}



		private:
			/// ASYNC  - основной контекст для записи заголовка сообщения
			void WriteHeader()
			{
				/*
				Если вызывается эта функция, мы знаем, что в очереди исходящих сообщений должно быть
				хотя бы одно сообщение для отправки. Итак, выделите буфер передачи для хранения
				сообщения и выполните команду work - asio, отправьте эти байты
				*/
				asio::async_write(m_socket, asio::buffer(&m_qMessagesOut.front().header, sizeof(message_header<T>)),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							if (m_qMessagesOut.front().body.size() > 0)
							{
								WriteBody();
							}
							else
							{
								// ...it didnt, so we are done with this message. Remove it from 
								// the outgoing message queue
								m_qMessagesOut.pop_front();

								// If the queue is not empty, there are more messages to send, so
								// make this happen by issuing the task to send the next header.
								if (!m_qMessagesOut.empty())
								{
									WriteHeader();
								}
							}
						}
						else
						{
							/*
							Asio не удалось написать сообщение, мы могли бы проанализировать причину, но
							пока просто предположим, что соединение прервалось, закрыв
							сокет. Когда будущая попытка записи в этот клиент завершится неудачей из-за
							к закрытой розетке, она будет приведена в порядок.
							*/
							std::cout << "[" << id << "] Write Header Fail.\n";
							m_socket.close();
						}
					});
			}

			/// ASYNC  - основной контекст для записи тела сообщения
			void WriteBody()
			{
				/*
				Если вызывается эта функция, только что был отправлен заголовок, и этот заголовок
				указал, что для этого сообщения существует тело. Заполните буфер передачи
				основными данными и отправьте их!
				*/
				asio::async_write(m_socket, asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							/*
							Отправка прошла успешно, так что мы закончили с сообщением
							и удалите его из очереди
							*/
							m_qMessagesOut.pop_front();
							/*
							Если в очереди все еще есть сообщения, то выполните задание на
							отправку заголовка следующего сообщения.
							*/
							if (!m_qMessagesOut.empty())
							{
								WriteHeader();
							}
						}
						else
						{
							// Ошибка отправки, смотрите эквивалент функции WriteHeader() для описания :P
							std::cout << "[" << id << "] Write Body Fail.\n";
							m_socket.close();
						}
					});
			}

			/// ASYNC - Основной контекст, готовый к чтению заголовка сообщения         
			void ReadHeader()
			{
				/*
				Если эта функция вызывается, мы ожидаем, что asio будет ждать, пока она не получит
				достаточно байт, чтобы сформировать заголовок сообщения. Мы знаем, что заголовки являются фиксированными
				размер, поэтому выделите буфер передачи, достаточно большой для его хранения. Фактически,
				мы создадим сообщение во "временном" объекте message так, как с ним
				удобно работать.
				*/
				asio::async_read(m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<T>)),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							if (m_msgTemporaryIn.header.size > 0)
							{
								m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
								ReadBody();
							}
							else
							{
								AddToIncomingMessageQueue();
							}
						}
						else
						{
							/// Чтение с клиента пошло неправильно, скорее всего, произошло отключение. 
							/// Закрываем сокет и позволяем системе привести его в порядок позже.
							std::cout << "[" << id << "] Read Header Fail.\n";
							m_socket.close();
						}
					});
			}

			/// ASYNC - Основной контекст, готовый к чтению текста сообщения
			void ReadBody()
			{
				/*
				Если эта функция вызвана, заголовок уже прочитан, и в этом запросе 
				заголовка мы читаем тело, пространство для этого тела уже 
				выделено во временном объекте сообщения, поэтому просто дождитесь поступления байтов...
				*/
				asio::async_read(m_socket, asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							// ...and they have! The message is now complete, so add
							// the whole message to incoming queue
							AddToIncomingMessageQueue();
						}
						else
						{
							// Как указано выше!
							std::cout << "[" << id << "] Read Body Fail.\n";
							m_socket.close();
						}
					});
			}

			/// Как только полное сообщение будет получено, мы добавим его во входящую очередь
			void AddToIncomingMessageQueue()
			{
				/*
				Поместите его в очередь, преобразовав в "собственное сообщение", 
				инициализировав с помощью общего указателя из этого объекта подключения
				*/
				if (m_nOwnerType == owner::server)
					m_qMessagesIn.push_back({ this->shared_from_this(), m_msgTemporaryIn });
				else
					m_qMessagesIn.push_back({ nullptr, m_msgTemporaryIn });
				/*
				Теперь мы должны настроить контекст asio для получения следующего сообщения. 
				Он будет просто ждать поступления байтов, и процесс построения сообщения повторится.
				*/
				ReadHeader();
			}

		protected:
			/// Каждое соединение имеет уникальный разъем для подключения к удаленному
			asio::ip::tcp::socket m_socket;

			/// Этот контекст является общим для всего экземпляра asio
			asio::io_context& m_asioContext;

			/// В этой очереди хранятся все сообщения, которые должны быть отправлены на удаленную сторону этого соединения
			tsqueue<message<T>> m_qMessagesOut;

			/// Это ссылается на входящую очередь родительского объекта
			tsqueue<owned_message<T>>& m_qMessagesIn;

			/// Входящие сообщения создаются асинхронно, поэтому мы будем хранить здесь частично собранное сообщение до тех пор, пока оно не будет готово
			message<T> m_msgTemporaryIn;

			owner m_nOwnerType = owner::server;

			uint32_t id = 1;

		};
	}
}
