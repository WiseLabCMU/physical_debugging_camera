#include "network_connection.hpp"

#include <string>
#include <cstring>
#include <vector>
#include <iostream>
#include <thread>

// Socket streaming
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

NetworkConnection::NetworkConnection(const std::string& ip_address, int port) {
  this->ip_address_ = ip_address;
  this->port_ = port;

  this->socket_ = socket(AF_INET, SOCK_STREAM, 0);
  if (this->socket_ < 0) {
    perror("Socket creation failed");
    exit(1);
  }

  if (this->ip_address_ == "") 
  {
    this->client_addr_.sin_family = AF_INET;
    this->client_addr_.sin_addr.s_addr = INADDR_ANY;
    this->client_addr_.sin_port = htons(this->port_);

    accept_client();
  }
  else
  {
    this->server_addr_.sin_family = AF_INET;
    this->server_addr_.sin_port = htons(this->port_);
    inet_pton(AF_INET, this->ip_address_.c_str(), &this->server_addr_.sin_addr);

    connect_server();
  }
}

NetworkConnection::~NetworkConnection() {
  if (this->socket_ != -1)
  {
    close(this->socket_);
  }
}

bool NetworkConnection::connect_server() {
  if (this->socket_ < 0)
  {
    return false;
  }

  while (true) {
    if (connect(this->socket_, (struct sockaddr*)&this->server_addr_, sizeof(this->server_addr_)) == 0) 
    {
      perror("Connection to server failed");
      return 1;
    }
    else 
    {
      perror("Connection to server failed. Retrying in 1 seconds...");
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  }

  std::cout << "Connected to server at " << this->ip_address_ << ":" << this->port_ << std::endl;
  return true;
}

bool NetworkConnection::accept_client() {
  if (bind(this->socket_, (struct sockaddr*)&this->client_addr_, sizeof(this->client_addr_)) < 0) 
  {
    perror("Bind failed");
    return 1;
  }

  listen(this->socket_, 1);

  std::cout << "Waiting for a connection...\n";
  this->client_socket_ = accept(this->socket_, NULL, NULL);
  if (this->client_socket_ < 0) 
  {
    perror("Failed to accept connection");
    close(this->socket_);
    return false;
  }

  std::cout << "Client connected!\n";
  return true;
}

int NetworkConnection::get_server_socket() const {
  return this->socket_;
}

int NetworkConnection::get_client_socket() const {
  return this->client_socket_;
}

ssize_t send_all(int socket, const void* buffer, size_t length) {
  size_t bytes_sent = 0;
  while (bytes_sent < length) {
    ssize_t result = send(socket, (const char*)buffer + bytes_sent, length - bytes_sent, 0);
    if (result == -1) {
      fprintf(stderr, "Error sending data: %s\n", strerror(errno));
      return result;
    }
    bytes_sent += result;
  }
  return bytes_sent;
}

ssize_t receive_all(int socket, char* buffer, size_t length) {
  size_t bytes_received = 0;
  while (bytes_received < length)
  {
    ssize_t result = recv(socket, buffer + bytes_received, length - bytes_received, 0);
    if (result == -1)
    {
      fprintf(stderr, "Error receiving data: %s\n", strerror(errno));
      return result;
    }
    bytes_received += result;
  }
  return bytes_received;
}
