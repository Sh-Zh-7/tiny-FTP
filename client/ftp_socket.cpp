#include "stdafx.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <iostream>
#include "ftp_socket.h"

#pragma comment(lib, "ws2_32.lib")

FTPSocket::FTPSocket(const std::string ip_address, unsigned int port) {
  // 加载套接字
  WORD sock_version = MAKEWORD(2, 2);
  WSADATA data;
  if (WSAStartup(sock_version, &data) != 0) {
    std::cout << "Web Server API启动失败！" << std::endl;
    exit(-1);
  };
  // 创建socket
  if ((this->socket_ = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
    std::cout << "创建socket失败！" << std::endl;
    exit(-1);
  }
  // 绑定IP和端口
  memset(&this->socaddr_, 0, sizeof(this->socaddr_));
  this->socaddr_.sin_family = AF_INET;
  this->socaddr_.sin_port = htons(port);
  this->socaddr_.sin_addr.S_un.S_addr = inet_addr(ip_address.c_str());
  if (connect(this->socket_, (sockaddr*)&this->socaddr_,
              sizeof(this->socaddr_)) < 0) {
    std::cout << "绑定失败！" << std::endl;
    exit(-1);
  }
}

FTPSocket::~FTPSocket() {}

void FTPSocket::send_data(const std::string send_data) {
  const char* send_data_c = send_data.c_str();
  send(this->socket_, send_data_c, strlen(send_data_c), 0);
}

int FTPSocket::receive_data(char* receive_buffer, unsigned int buffer_size) {
  return recv(this->socket_, receive_buffer, buffer_size, 0);
}

void FTPSocket::close() { closesocket(this->socket_); }
