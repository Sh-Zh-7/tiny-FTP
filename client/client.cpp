#include "stdafx.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#define _DLL_EXPORTS
#include "client.h"

using namespace std;

const string CRLF = "\r\n";
const unsigned int kBufferSize = 1024;

#define PrintMessage                            \
  do {                                          \
    std::cout << ReceiveMessage() << std::endl; \
  } while (0)

#define SendControlMessage(command)          \
  do {                                       \
    const string message = command + CRLF;   \
    this->control_socket_.SendData(message); \
    PrintMessage;                            \
  } while (0)

#define CloseDataSocket         \
  do {                          \
    this->data_socket_.Close(); \
    PrintMessage;               \
  } while (0)

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
  int length;
  stringstream dir_info;
  char receive_buffer[kBufferSize] = {0};
  while ((length = this->data_socket_.ReceiveData(receive_buffer,
                                                  kBufferSize)) != 0) {
    dir_info << string(receive_buffer, length);
  }
  // ���صĸ�ʽ������cmd dirָ�Windowsϵͳ��
  string line;
  stringstream line_resovler;
  vector<string> file_property(4);
  vector<string> files_in_dir;
  // TODO: DirTypes(�Ҳ�֪�����Զ���ĸ�ʽC#�ܷ�ʹ��)
  while (!dir_info.eof()) {
    getline(dir_info, line);  // ��ȡÿһ��
    if (!dir_info.eof()) {    // �������һ�հ���
      line_resovler << line;
      line_resovler >> file_property[0] >> file_property[1] >>
          file_property[2] >> file_property[3];  // ÿ���ÿո�ָ��ĸ���Ϣ
      files_in_dir.push_back(file_property[3]);  // ���һ��Ϊ�ļ�/�ļ�����
    }
  }
  CloseDataSocket;
  return files_in_dir;
}

unsigned int Client::GetFileSize(const std::string& filename) {
	SendControlMessage("SIZE " + filename);
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
  // ͨ�����������������
  int length = 0;
  char receive_buffer[kBufferSize] = {0};
  while ((length = this->data_socket_.ReceiveData(receive_buffer,
                                                  kBufferSize)) != 0) {
    for (int i = 0; i < length; ++i) {
      file << receive_buffer[i];
    }
  }
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
    this->data_socket_.SendData(string(send_buffer));
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
