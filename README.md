# ___ ___ "Centaurus"

Тестовое задание на позицию стажера.

Задача:
```
Клиент-серверное приложение для мониторинга рабочей активности

Примеры приложений: https://democompany.teramind.co/#/report/Focus+Dashboard

Клиент (Windows) - c/c++
Автоматически запускается при входе пользователя в систему и работает в фоновом режиме. 
Взаимодействует с сервером по любому протоколу

Сервер - desktop или веб-интерфейс - любой язык
Список всех подключенных клиентов - домен/компьютер/ip/пользователь
Показать время последней активности клиента
Возможность получения скриншота с рабочего стола клиента
```
ОТСЧЁТ:

Реализовано:
1. Клиент и сервер в консоли. Используется asio :( - подключается через CmakeLists.txt и vcpkg.
2. Клиент может послать команду Ping для получения ping'а.
3. Сервер видит все подключения. Если сервер закрывается, все клиенты закрываются автоматически.
4. Реализованы функции скриншота и получения ip. Но они не подключены к клиенту и серверу. Папка expanded_funcs.

Не реализовано:
1. Подключение к серверу с разных устройств из разных сетей.
2. Вывод в область администрирования времени подключения клиента.
3. Получение скриншота по требованию со стороны сервера и отправка изображения на сервер.

Баги:
1. Если один клиент отправляет пинг, его же отправляют и другие клиенты - оишбка при отправке пакетов.
2. Если клиент запущен после запуска сервера, то клиент не сможет подключиться(нужно перезапускать клиент).

Что следует улучшить:
1. Перенести проект на winsocket.
2. Добавить новые типы пакетов, способные передавать другие типы данных.
3. Добавить возможность создавать новые типы пакетов более динамично, чем сейчас, не переписывая весь проект.
4. Реализовать вывод данных не в консоль, а в окно windows( !Перед этим желательно перейти на winsocket).

Доп. комментарии:
1. При запуске клиента у него срабатаывает конструктор, в этом месте можно создавать временную метку, которую потом передавать тело сообщения.
В данной версии проекта реализована передача временных точек, но их нужно создавать немного в других местах. Пакет на такую передачу уже подготовлен.
2. При запуске клиента до запуска сервера запускать цикл, который будет пробовать подключиться к серверу. 
