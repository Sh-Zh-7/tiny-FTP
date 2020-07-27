#pragma once

#include <string>
#include "ftp_socket.h"

#ifdef _DLL_EXPORTS
#define DLL_API _declspec(dllexport)
#else
#define DLL_API _declspec(dllimport)
#endif

// ����COM��˼�룬ʹ�ýӿڵ���������
class IClient {
 public:
	// ����������������������ͷ��������Դ
	virtual ~IClient() {}

	virtual const std::string ReceiveMessage()=0;
  virtual void Login(const std::string& username, const std::string& password)=0;
	virtual void DownloadFile(const std::string& filename)=0;
  // virtual void UploadFile(const std::string& filename)=0;
};

class Client: public IClient {
 private:
	const std::string ip_address_;
  FTPSocket control_socket_;
  FTPSocket data_socket_;

	void EnterPassiveMode();
  unsigned int ResolveDataSocketPort(const std::string&);

 public:
  Client(const std::string& ip_address, unsigned int port);
  ~Client();

  void Login(const std::string&, const std::string&);
  const std::string ReceiveMessage();
  void DownloadFile(const std::string& filename);
  // void UploadFile(const std::string& filename);
};

extern "C" DLL_API IClient* GetClient(const std::string ip_address);


