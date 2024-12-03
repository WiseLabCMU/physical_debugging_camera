#pragma once

#include <string>

// Socket streaming
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

class NetworkConnection {
public:
  NetworkConnection(const std::string& ip_address, int port);

  ~NetworkConnection();

  bool connect_server();

  bool accept_client();

  int get_server_socket() const;

  int get_client_socket() const;

private:
  std::string ip_address_;
  int port_;

  int socket_;
  int client_socket_;
  struct sockaddr_in server_addr_;
  struct sockaddr_in client_addr_;
};

ssize_t send_all(int socket, const void* buffer, size_t length);

ssize_t receive_all(int socket, char* buffer, size_t length);
