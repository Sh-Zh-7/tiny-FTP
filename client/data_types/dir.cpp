#include "dir.h"

void Dir::Push(const PathInfo& pi) {
	this->all_info.push_back(pi); }

vector<string> Dir::GetFilesName() {
	vector<string> all_files_name;	// �����ļ���
	for (auto path: this->all_info) {
		all_files_name.push_back(path.name_);
	}
	return all_files_name;
}
