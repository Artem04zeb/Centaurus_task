#pragma once
#include "net_common.h"

namespace olc
{
	namespace net
	{
		/// Message Header отправляется в начале каждого сообщения. 
		/// Шаблон позволяет нам использовать "enum class", чтобы убедиться
		/// что сообщения валидны во время компиляции.
		template <typename T>
		struct message_header{
			T id{};
			uint32_t size = 0;
		};

		/// Message Body содержит header и std::vector, содержащий 'сырые' байты иформации.
		/// Благодаря этому сообщения могут быть различной длинны, 
		/// но если меняется длинна сообщения, то размер в заголовке должен быть обновлен
		template <typename T>
		struct message
		{
			/// Header
			message_header<T> header{};
			/// Body vector
			std::vector<uint8_t> body;

			/// Возвращает размер всего пакета
			size_t size() const { return body.size(); }

			// Переопределение для совместимости с std::cout - создание понятного описания сообщения
			friend std::ostream& operator << (std::ostream& os, const message<T>& msg)
			{
				os << "ID:" << int(msg.header.id) << " Size:" << msg.header.size;
				return os;
			}

			/// Перегрузки оператора. Нужно чтобы удобнее добавлять и удалять сообщения из body

			/// Pushes any POD-like data into the message buffer
			template<typename DataType>
			friend message<T>& operator << (message<T>& msg, const DataType& data)
			{
				/// Проверяет входные данные
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

				/// Кеширует текущий размер вектора, т.к. это будет
				/// точкой, куда мы вставим данные
				size_t i = msg.body.size();

				/// Изменяет размер веткора согласно размеру поданой информации
				msg.body.resize(msg.body.size() + sizeof(DataType));

				/// Копирует инфомарцию в новый вектор
				std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

				/// Пересчитывает рамзер пакета
				msg.header.size = msg.size();

				return msg;
			}

			/// Извлекает данные из буфера сообщений
			template<typename DataType>
			friend message<T>& operator >> (message<T>& msg, DataType& data)
			{
				/// Проверяет входные данные
				/// static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pulled from vector");

				// Кеширует местоположение в конец вектора, где хранится извлеченная инфа
				size_t i = msg.body.size() - sizeof(DataType);

				/// Копирует данные из вектора в пользовательскую переменную
				std::memcpy(&data, msg.body.data() + i, sizeof(DataType));

				/// Уменьшает вектор, чтобы удалить прочитанные байты
				msg.body.resize(i);

				/// Пересчитывает рамзер пакета
				msg.header.size = msg.size();

				return msg;
			}
		};


		/// Такое сообщение идентично обычному сообщению, но различия проявляются 
		/// в разных видах подключения.
		/// На сервере владельцем был бы клиент, отправивший сообщение, 
		/// на клиенте владельцем был бы сервер.

		// --------
		template <typename T>
		class connection;

		template <typename T>
		struct owned_message
		{
			std::shared_ptr<connection<T>> remote = nullptr;
			message<T> msg;

			friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg)
			{
				os << msg.msg;
				return os;
			}
		};

	}
}
