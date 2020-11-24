#include "tcpserver.h"

//Конструктор принимает:
// port - порт на котором будем запускать сервер
// handler - callback-функция запускаямая при подключении клиента
//          объект которого и передают первым аргументом в callback
//          (пример лямбда-функции: [](TcpServer::Client){...do something...})
TcpServer::TcpServer(const uint16_t port, handler_function_t handler)
    : port(port), handler(handler) {}

//Деструктор останавливает сервер если он был запущен
//и вычищает заданную версию WinSocket
TcpServer::~TcpServer() {
  if (_status == status::up)
    stop();
}

//Задаёт callback-функцию запускаямую при подключении клиента
void TcpServer::setHandler(TcpServer::handler_function_t handler) {
  this->handler = handler;
}

// Getter/Setter порта
uint16_t TcpServer::getPort() const { return port; }
uint16_t TcpServer::setPort(const uint16_t port) {
  this->port = port;
  restart(); //Перезапустить если сервер был запущен
  return port;
}

//Перезапуск сервера
TcpServer::status TcpServer::restart() {
  if (_status == status::up)
    stop();
  return start();
}

// Вход в поток обработки соединений
void TcpServer::joinLoop() { handler_thread.join(); }

//Загружает в буфер данные от клиента и возвращает их размер
int TcpServer::Client::loadData() {
  return recv(socket, buffer, buffer_size, 0);
}
//Возвращает указатель на буфер с данными от клиента
char *TcpServer::Client::getData() { return buffer; }
//Отправляет данные клиенту
bool TcpServer::Client::sendData(const char *buffer, const size_t size) const {
  if (send(socket, buffer, size, 0) < 0)
    return false;
  return true;
}

//Запуск сервера
TcpServer::status TcpServer::start() {
  struct sockaddr_in server;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
  server.sin_family = AF_INET;
  serv_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (serv_socket == -1)
    return _status = status::err_socket_init;
  if (bind(serv_socket, (struct sockaddr *)&server, sizeof(server)) < 0)
    return _status = status::err_socket_bind;
  if (listen(serv_socket, 3) < 0)
    return _status = status::err_socket_listening;

  _status = status::up;
  handler_thread = std::thread([this] { handlingLoop(); });
  return _status;
}

//Остановка сервера
void TcpServer::stop() {
  _status = status::close;
  close(serv_socket);
  joinLoop();
  for (std::thread &cl_thr : client_handler_threads)
    cl_thr.join();
  client_handler_threads.clear();
  client_handling_end.clear();
}

// Функиця обработки соединений (по аналогии с реализацией для Windows)
void TcpServer::handlingLoop() {
  while (_status == status::up) {
    int client_socket;
    struct sockaddr_in client_addr;
    int addrlen = sizeof(struct sockaddr_in);
    if ((client_socket = accept(serv_socket, (struct sockaddr *)&client_addr,
                                (socklen_t *)&addrlen)) >= 0 &&
        _status == status::up)
      client_handler_threads.push_back(
          std::thread([this, &client_socket, &client_addr] {
            handler(Client(client_socket, client_addr));
            client_handling_end.push_back(std::this_thread::get_id());
          }));

    if (!client_handling_end.empty())
      for (std::list<std::thread::id>::iterator id_it =
               client_handling_end.begin();
           !client_handling_end.empty(); id_it = client_handling_end.begin())
        for (std::list<std::thread>::iterator thr_it =
                 client_handler_threads.begin();
             thr_it != client_handler_threads.end(); ++thr_it)
          if (thr_it->get_id() == *id_it) {
            thr_it->join();
            client_handler_threads.erase(thr_it);
            client_handling_end.erase(id_it);
            break;
          }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

// Конструктор клиента по сокету и адресу
TcpServer::Client::Client(int socket, struct sockaddr_in address)
    : socket(socket), address(address) {}
// Конструктор копирования
TcpServer::Client::Client(const TcpServer::Client &other)
    : socket(other.socket), address(other.address) {}

TcpServer::Client::~Client() {
  shutdown(socket, 0); //Обрыв соединения сокета
  close(socket);       //Закрытие сокета
}

// Геттеры хоста и порта
uint32_t TcpServer::Client::getHost() const { return address.sin_addr.s_addr; }
uint16_t TcpServer::Client::getPort() const { return address.sin_port; }
