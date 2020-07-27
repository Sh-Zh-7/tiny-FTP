#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#define _DLL_EXPORTS
#include "client.h"
#include "utils.h"
#include "../socket/data_socket.h"
#include "../socket/control_socket.h"
#include "../data_types/dir.h"
#include "../data_types/path_info.h"

using namespace std;

Client::Client(const string& ip_address, unsigned int port = 21)
    : ip_address_(ip_address), control_socket_(ControlSocket(ip_address, port)) {
  PrintMessage;
}

Client::~Client() {
  // ������������QUIT���ر�socket
  SendControlMessage("QUIT");
  this->control_socket_.Close();
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
  this->control_socket_.Send(file_size_message);
  stringstream file_size_info;
	file_size_info << this->control_socket_.GetResponse();
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

void Client::EnterPassiveMode() {
  // �������������뱻��ģʽ��ָ��
  const string passive_mode_message = "PASV" + CRLF;
  this->control_socket_.Send(passive_mode_message);
  // ���ܷ�������Ϣ
  const string data_socket_info = this->control_socket_.GetResponse();
  cout << data_socket_info << endl;
  // ����˿ںŲ����������׽���
	this->data_socket_ = this->control_socket_.GetDataSocket(this->ip_address_, data_socket_info);
}

extern "C" DLL_API IClient* GetClient(const string ip_address) {
  return new Client(ip_address);
}
