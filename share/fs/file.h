#pragma once

#include <string>
#include <vector>
#include "path_info.h"

// ��װWindows API�õ��ķ��ִ����Ե�File��
namespace FileSystem {
	namespace File {
		bool IsDirectory(const string& path);

		bool IsFile(const string& path);

		vector<string> GetPathInfoInDir(const string& dir_name);

		void CreateFolder(const string& folder_name);
	}  // namespace File
} // namespace FileSystem

