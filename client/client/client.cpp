#include "../stdafx.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#define _DLL_EXPORTS
#include "../socket/data_socket.h"
#include "client.h"
#include "utils.h"
#include "../data_types/dir.h"
#include "../data_types/path_info.h"

using namespace std;

Client::Client(const string& ip_address, unsigned int port = 21)
    : ip_address_(ip_address), control_socket_(FTPSocket(ip_address, port)) {
  PrintMessage;
}

Client::~Client() {
  // ������������QUIT���ر�socket
  SendControlMessage("QUIT");
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
  // �ȴ����û������ٴ����루�������Ⱥ�˳��
  SendControlMessage("USER " + username);
  SendControlMessage("PASS " + password);
}

vector<string> Client::GetDirList() {
  EnterPassiveMode();
  SendControlMessage("LIST");
  // ���ܷ�������data_socket���ص��������
  stringstream dir_info;
	dir_info << this->data_socket_.GetResponse();
  // ���صĸ�ʽ������cmd dirָ�Windowsϵͳ��
  Dir dir;
  string line;
  stringstream line_resovler;
  while (!dir_info.eof()) {
    getline(dir_info, line);  // ��ȡÿһ��
    if (!dir_info.eof()) {    // �������һ�հ���
      line_resovler << line;
			PathInfo pi(line_resovler);
      dir.Push(pi);
    }
  }
  CloseDataSocket;
  return dir.GetFilesName();
}

unsigned int Client::GetFileSize(const std::string& filename) {
  const string file_size_message = "SIZE " + filename + CRLF;
  this->control_socket_.SendData(file_size_message);
  stringstream file_size_info;
	file_size_info << ReceiveMessage();
	int numbers[2];
	file_size_info >> numbers[0] >> numbers[1];
	return numbers[1];
}

void Client::DownloadFile(const string& filename) {
  EnterPassiveMode();  // �Ƚ��뱻��ģʽ
  SendControlMessage("RETR " + filename);
  // �����ļ�
  auto file = ofstream(filename, ios::out || ios::binary);
  if (!file) {
    cout << "δ�ܴ��ļ�!" << endl;
    exit(1);
  }
	file << this->data_socket_.GetResponse();
  file.close();
  CloseDataSocket;
}

void Client::UploadFile(const string& filename) {
  EnterPassiveMode();  // �Ƚ��뱻��ģʽ
  SendControlMessage("STOR " + filename);
  // ���ļ�
  auto file = ifstream(filename, ios::in || ios::binary);
  if (!file) {
    cout << "δ���ҵ��ļ���" << endl;
    exit(1);
  }
  // ͨ����ȡ�ļ����ݲ�����������ϴ�
  char send_buffer[kBufferSize] = {0};
  while (!file.eof()) {
    file.read(send_buffer, kBufferSize);
    this->data_socket_.Send(string(send_buffer));
  }
  file.close();
  CloseDataSocket;
}

//////////////////////////////////////////PrivateMethod/////////////////////////////////////////////

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
  this->data_socket_ = DataSocket(this->ip_address_, target_port);
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