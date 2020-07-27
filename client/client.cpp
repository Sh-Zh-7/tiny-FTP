#include "stdafx.h"

#include <iostream>
#include <string>
#define _DLL_EXPORTS
#include "client.h"

using std::cout;
using std::endl;
using std::string;

const string CRLF = "\r\n";
const unsigned int kBufferSize = 1024;


Client::Client(const string& ip_address, unsigned int port = 21)
    : ip_address_(ip_address), control_socket_(FTPSocket(ip_address, port)) {
  cout << ReceiveMessage() << endl;
}

Client::~Client() {
	// ������������QUIT���ر�socket
	const string close_message = "QUIT" + CRLF;
	this->control_socket_.SendData(close_message);
	cout << ReceiveMessage() << endl;
	this->control_socket_.Close();
}


const string Client::ReceiveMessage() {
  int length;
  char buffer[kBufferSize] = {0};

  if ((length = this->control_socket_.ReceiveData(buffer, kBufferSize)) == -1) {
    cout << "û�н��յ�����" << endl;
  }
  return buffer;
}


void Client::Login(const string& username, const string& password) {
  // �ȴ����û���
  const string username_message = "USER " + username + CRLF;
  this->control_socket_.SendData(username_message);
  cout << ReceiveMessage() << endl;
  // �ٴ�������
  const string password_message = "PASS " + password + CRLF;
  this->control_socket_.SendData(password_message);
  cout << ReceiveMessage() << endl;
}


void Client::EnterPassiveMode() {
	// �������������뱻��ģʽ��ָ��
  const string passive_mode_message = "PASV" + CRLF;
  this->control_socket_.SendData(passive_mode_message);
	// ���ܷ�������Ϣ
  const string data_socket_info = ReceiveMessage();
	cout << data_socket_info << endl;
  // ����˿ںŲ����������׽���
	unsigned int target_port = ResolveDataSocketPort(data_socket_info);
	cout << target_port << endl;
	this->data_socket_ = FTPSocket(this->ip_address_, target_port);
}


unsigned int Client::ResolveDataSocketPort(const string& data_socket_info) {
  // �ҵ�()�е�����
  auto left_parentheses = data_socket_info.find("(");
  auto right_parentheses = data_socket_info.rfind(")");
  if (right_parentheses <= left_parentheses) {
    cout << "δ����ȷ�������ص�����" << endl;
    exit(-1);
  }
	auto socket_length = right_parentheses - left_parentheses - 1;
  const string target_socket = data_socket_info.substr(left_parentheses + 1, socket_length);
  int socket_info[6];
	// ����sscanfֱ����ȡ��ʽ���ַ����е���������
  sscanf_s(target_socket.c_str(), "%d,%d,%d,%d,%d,%d", &socket_info[0],
           &socket_info[1], &socket_info[2], &socket_info[3], &socket_info[4],
           &socket_info[5]);
	// �¶˿ںż��㹫ʽ�������ڶ����� X 256 + ���һ����
  unsigned int target_port = socket_info[4] * 256 + socket_info[5];

  return target_port;
}


extern "C" DLL_API IClient* GetClient(const string ip_address) {
  return new Client(ip_address);
}
