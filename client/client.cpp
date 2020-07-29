#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#define _DLL_EXPORTS
#include "client.h"
#include "utils.h"
#include "../share/fs/file.h"
#include "../share/fs/path_info.h"
#include "../share/socket/data_socket.h"
#include "../share/socket/control_socket.h"

using namespace ClientSpace;

ClientSpace::Client::Client(const string& ip_address, unsigned int port = 21)
    : ip_address_(ip_address), control_socket_(ControlSocket(ip_address, port)) {
  PrintMessage();
}

ClientSpace::Client::~Client() {
  // ������������QUIT���ر�socket
  SendControlMessage("QUIT");
	if (this->control_socket_.GetStatus() != 221) {
		cout << "δ�ܹر����ӣ���ʼǿ�ƹر�!" << endl << endl;
		this->control_socket_.Close();
		exit(1);
	}
  this->control_socket_.Close();
}

void Client::Help() { SendControlMessage("HELP"); }

bool Client::Login(const string& username, const string& password) {
  SendControlMessage("USER " + username);
  SendControlMessage("PASS " + password);
	return this->control_socket_.GetStatus() == 230;
}

bool Client::Rename(const string& old_name, const string& new_name) {
	SendControlMessage("RNFR " + old_name);
	if (this->control_socket_.GetStatus() != 350) return false;
	SendControlMessage("RNTO " + new_name);
  return this->control_socket_.GetStatus() == 250;
}

bool Client::RemoveFile(const string& filename) {
	SendControlMessage("DELE " + filename);
	return this->control_socket_.GetStatus() == 250;
}

bool Client::RemoveDir(const string& dirname) {
	return true; }

bool Client::MakeDir(const string& dirname) {
	SendControlMessage("MKD " + dirname);
	auto status = this->control_socket_.GetStatus();
	if (status != 257 && status != 226) {
		PrintMessage();	// Receive server's error message
		return false;
	} else {
		return true;
	}
}

vector<PathInfo> Client::GetDirList(const string& target_dir) {
  EnterPassiveMode();
  SendControlMessage("LIST " + target_dir);
  // ���ܷ�������data_socket���ص��������
  stringstream dir_info;
	dir_info << this->data_socket_.GetResponse();
  // ���صĸ�ʽ������cmd dirָ�Windowsϵͳ��
  vector<PathInfo> ftp_path_info;
  string line;
  stringstream line_resovler;
  while (!dir_info.eof()) {
    getline(dir_info, line);  // ��ȡÿһ��
    if (!dir_info.eof()) {    // �������һ�հ���
      line_resovler << line;
			PathInfo pi(line_resovler);
      ftp_path_info.push_back(pi);
    }
  }
  CloseDataSocket;
  return ftp_path_info;
}

void Client::UploadDir(const string& dirname) {
	if (MakeDir(dirname)) {
    auto path_list = File::GetPathInfoInDir(dirname);
    for (auto path : path_list) {
      if (File::IsDirectory(path)) {
        UploadDir(path);
      } else {
        UploadFile(path);
      }
      cout << "Upload " << path << " finished." << endl;
    }
	}
}

void Client::DownloadDir(const string& dirname) {
	File::CreateFolder(dirname);
	auto dir_content = GetDirList(dirname);
	for (PathInfo pi : dir_content) {
		const string abs_path = dirname + "\\" + pi.name_;
    cout << abs_path << endl;
		if (pi.is_dir_) { 
			DownloadDir(abs_path);
		} else {
			DownloadFile(abs_path);
		}
		cout << "Download " << pi.name_ << " finished." << endl;
	}
}

bool Client::ChangeWorkingDir(const string& dirname) { 
	SendControlMessage("CWD " + dirname);
	return GetWorkingDir() == ""? false : control_socket_.GetStatus() == 250;
}

string Client::GetWorkingDir() {
	const string get_dir_message = "PWD" + CRLF;
	this->control_socket_.Send(get_dir_message);
	const string dir_info = this->control_socket_.GetResponse();
	// ˫�����м�����ݾ���WD
	auto first_quot = dir_info.find_first_of("\"");
	auto last_quot = dir_info.find_last_of("\"");
	if (last_quot <= first_quot) {
		cout << "û���ҵ�Working Dirctory" << endl << endl;
		return "";
	}
	// �ж�ʧ�ܵ����
	return this->control_socket_.GetStatus()
            ? dir_info.substr(first_quot + 1, last_quot - first_quot - 1) : "";
}

int Client::GetFileSize(const string& filename) {
  stringstream file_size_info;
	file_size_info << SendControlMessage("SIZE " + filename);
	unsigned int status = this->control_socket_.GetStatus();
	if (status == 550 || status == 451) { 
		PrintMessage();
		return -1;
	}
	// ����stream�������Զ������ո�
	int numbers[2];
	file_size_info >> numbers[0] >> numbers[1];
	return numbers[1];
}

void Client::DownloadFile(const string& filename) {
  EnterPassiveMode();  // �Ƚ��뱻��ģʽ
  SendControlMessage("TYPE I");	// �����ƴ���

	if (!experimental::filesystem::exists(filename)) {
    // �����ϴ�
		SendControlMessage("RETR " + filename);
    auto file = ofstream(filename, ios::out | ios::binary);
    AssertFileExisted(file);
    DownloadFileByBuffer(file);
    file.close();
	} else {
		// �ϵ�����
		DownloadFileWithCheckPoint(filename);
	}
  CloseDataSocket;
}

void Client::DownloadFileWithCheckPoint(const string& filename) {
  auto file = ofstream(filename, ios::out | ios::binary | ios::app);
  AssertFileExisted(file);
  // �鿴�ļ���С˳�㽫�ļ�ָ���Ƶ�ĩβ
  file.seekp(0, ios::end);
  auto local_file_size = file.tellp();
  auto server_file_size = GetFileSize(filename);
  // �����ϵ��ش�
  if (local_file_size != server_file_size) {
    SendControlMessage("REST " + to_string(local_file_size));
    SendControlMessage("RETR " + filename);
    DownloadFileByBuffer(file);
    file.close();
  }
}

void Client::UploadFileWithCheckPoint(const string& filename, int server_file_size) {
  auto file = ifstream(filename, ios::out | ios::binary);
  AssertFileExisted(file);
  file.seekg(0, ios::end);
  auto local_file_size = file.tellg();
  if (local_file_size > server_file_size) {
    SendControlMessage("REST " + to_string(server_file_size));
    SendControlMessage("STOR " + filename);
    file.seekg(local_file_size);  // �ƶ�ָ��
    UploadFileByBuffer(file);
    file.close();
  }
}

void Client::UploadFile(const string& filename) {
  EnterPassiveMode();  // �Ƚ��뱻��ģʽ
  SendControlMessage("TYPE I");	// �����ƴ���

	int server_file_size = GetFileSize(filename);
	if (server_file_size == -1) {
		// �����ϴ�
		SendControlMessage("STOR " + filename);
		auto file = ifstream(filename, ios::in | ios::binary);
		AssertFileExisted(file);
		UploadFileByBuffer(file);
		file.close();
	} else {
		// �ϵ�����
		UploadFileWithCheckPoint(filename, server_file_size);
	}
	CloseDataSocket;  
}

void Client::EnterPassiveMode() {
    const string data_socket_info = SendControlMessage("PASV");
    // ���ݷ��������ص���Ϣ���������׽���
		this->data_socket_ = this->control_socket_.GetDataSocket(this->ip_address_,
                                                             data_socket_info);
}

const string Client::PrintMessage() {
  auto response = this->control_socket_.GetResponse();
  cout << response << endl;
	return response;
}

const string Client::SendControlMessage(const string& command) {
  const string message = command + CRLF;
  this->control_socket_.Send(message);
	const string ret = PrintMessage();
	return ret;
}

extern "C" DLL_API IClient* GetClient(const std::string ip_address) {
  return new Client(ip_address);
}
