#include "stdafx.h"

#include <string>
#include <iostream>
#define _DLL_EXPORTS
#include "client.h"

const unsigned int kBufferSize = 1024;

Client::Client(const std::string ip_address, unsigned int port=21) {
  this->control_socket_ = FTPSocket(ip_address, port);
  std::cout << ReceiveMessage() << std::endl;
}

Client::~Client() {}

const std::string Client::ReceiveMessage() {
  int length;
	char buffer[kBufferSize] = {0};

	if ((length = this->control_socket_.ReceiveData(buffer, kBufferSize)) == -1) {
		std::cout << "û�н��յ�����" << std::endl;
	}
	return buffer;
}

extern "C" DLL_API IClient* GetClient(const std::string ip_address) {
	return new Client(ip_address);
}
