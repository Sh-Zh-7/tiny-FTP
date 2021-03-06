#pragma once

#include <iostream>
#include <string>

using std::cout;
using std::endl;
using std::string;

// Normal settings
const string CRLF = "\r\n";
const unsigned int kBufferSize = 1024;

// Inline functions
#define CloseDataSocket         \
  do {                          \
    this->data_socket_.Close(); \
    PrintMessage();               \
  } while (0)

#define AssertFileExisted(file)                   \
  do {                                            \
    if (!file) {                                  \
      this->logger->critical("CANNOT FIND FILE!"); \
      exit(1);                                    \
    }                                             \
  } while (0)

#define DownloadFileByBuffer(file)                                    \
  do {                                                                \
    int length = 0;                                                   \
    char receive_buffer[kBufferSize] = {0};                           \
    while ((length = this->data_socket_.Receive(receive_buffer,       \
                                                kBufferSize)) != 0) { \
      file.write(receive_buffer, length);                             \
			this->logger->info("Buffer receive: " + length);								\
    }                                                                 \
  } while (0)

#define UploadFileByBuffer(file)                           \
  do {                                                     \
    char send_buffer[kBufferSize] = {0};                   \
    while (!file.eof()) {                                  \
      file.read(send_buffer, kBufferSize);                 \
      this->data_socket_.Send(send_buffer, file.gcount()); \
    }                                                      \
  } while (0)