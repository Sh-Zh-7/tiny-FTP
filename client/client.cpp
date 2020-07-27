#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#define _DLL_EXPORTS
#include "client.h"

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::ofstream;
using std::ifstream;

const string CRLF = "\r\n";
const unsigned int kBufferSize = 1024;


#define PrintMessage \
	do {	\
		std::cout << ReceiveMessage() << std::endl;	\
	} while(0)


Client::Client(const string& ip_address, unsigned int port=21)
    : ip_address_(ip_address), control_socket_(FTPSocket(ip_address, port)) { PrintMessage; }


Client::~Client() {
  // ������������QUIT���ر�socket
  const string close_message = "QUIT" + CRLF;
  this->control_socket_.SendData(close_message);
  PrintMessage;
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
  PrintMessage;
  // �ٴ�������
  const string password_message = "PASS " + password + CRLF;
  this->control_socket_.SendData(password_message);
  PrintMessage;
}


void Client::DownloadFile(const string& filename) {
	EnterPassiveMode();	// �Ƚ��뱻��ģʽ
	const string download_message = "RETR " + filename + CRLF;
	this->control_socket_.SendData(download_message);
  PrintMessage;
	// �����ļ�
	auto file = ofstream(filename, std::ios::out || std::ios::binary);
	if (!file) {
		cout << "δ�ܴ��ļ�!" << endl;
		exit(1);
	}
	// ͨ�����������������
	int length = 0;
	char receive_buffer[kBufferSize] = {0};
	while ((length = this->data_socket_.ReceiveData(receive_buffer, kBufferSize)) != 0) {
		for (int i = 0; i < length; ++i) {
			file << receive_buffer[i];
		}
	}
	file.close();
	this->data_socket_.Close();
	PrintMessage;
}


void Client::UploadFile(const std::string& filename) {
	EnterPassiveMode();	// �Ƚ��뱻��ģʽ
	const string upload_message = "STOR " + filename + CRLF;
	this->control_socket_.SendData(upload_message);
	PrintMessage;
	// ���ļ�
	auto file = ifstream(filename, std::ios::in || std::ios::binary);
	if (!file) {
		cout << "δ���ҵ��ļ���" << endl;
		exit(1);
	}
	// ͨ����ȡ�ļ����ݲ�����������ϴ�
	char send_buffer[kBufferSize] = {0};
	while (!file.eof()) {
		file.read(send_buffer, kBufferSize);
		this->data_socket_.SendData(std::string(send_buffer));
	}
	file.close();
	this->data_socket_.Close();
	PrintMessage;
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
  const string target_socket =
      data_socket_info.substr(left_parentheses + 1, socket_length);
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
