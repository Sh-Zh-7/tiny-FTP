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

bool Client::Rename(const std::string& old_name, const std::string& new_name) {
	SendControlMessage("RNFR " + old_name);
	if (this->control_socket_.GetStatus() != 350) return false;
	SendControlMessage("RNTO " + new_name);
  return this->control_socket_.GetStatus() == 250;
}

bool Client::RemoveFile(const std::string& filename) {
	SendControlMessage("DELE " + filename);
	return this->control_socket_.GetStatus() == 250;
}

bool Client::RemoveDir(const std::string& dirname) {
	return true; }

bool Client::MakeDir(const std::string& dirname) {
	SendControlMessage("MKD " + dirname);
	return this->control_socket_.GetStatus() == 257;
}

vector<string> Client::GetDirList(const std::string& target_dir) {
  EnterPassiveMode();
  SendControlMessage("LIST " + target_dir);
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

bool Client::ChangeWorkingDir(const std::string& dirname) { 
	SendControlMessage("CWD " + dirname);
	if (GetWorkingDir() != "") {
		return control_socket_.GetStatus() == 250;
	} else {
		return false;
	}
}

std::string Client::GetWorkingDir() {
	const string get_dir_message = "PWD" + CRLF;
	this->control_socket_.Send(get_dir_message);
	const string dir_info = this->control_socket_.GetResponse();
	// ˫�����м�����ݾ���WD
	auto first_quot = dir_info.find_first_of("\"");
	auto last_quot = dir_info.find_last_of("\"");
	if (last_quot <= first_quot) {
		cout << "û���ҵ�Working Dirctory" << endl << endl;
		exit(1);
	}
	// �ж�ʧ�ܵ����
	return this->control_socket_.GetStatus()
            ? dir_info.substr(first_quot + 1, last_quot - first_quot - 1) : "";
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
  SendControlMessage("TYPE I");	// �����ƴ���
  SendControlMessage("RETR " + filename);
  // �����ļ�
  auto file = ofstream(filename, ios::out | ios::binary);
	AssertFileExisted(file);
	// �����ֽ��������ļ�
  int length = 0;
  char receive_buffer[kBufferSize] = {0};
  while ((length = this->data_socket_.Receive(receive_buffer, kBufferSize)) != 0) {
		file.write(receive_buffer, length);
		cout << "Buffer receive: " << length << endl;
  }
  file.close();
  CloseDataSocket;
}

void Client::UploadFile(const string& filename) {
  EnterPassiveMode();  // �Ƚ��뱻��ģʽ
  SendControlMessage("TYPE I");	// �����ƴ���
  SendControlMessage("STOR " + filename);
  // ���ļ�
  auto file = ifstream(filename, ios::in | ios::binary);
	AssertFileExisted(file);
  // ͨ����ȡ�ļ����ݲ�����������ϴ�
  char send_buffer[kBufferSize] = {0};
  while (!file.eof()) {
    file.read(send_buffer, kBufferSize);
    this->data_socket_.Send(send_buffer, file.gcount());
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
