#pragma once

#include <string>

class ZJH_encrypto {
public:
	bool encrypto(const std::string& path, int len);
};

class ZJH_encrypto1 {
public:
	bool encrypto(const std::string& path, int len, const char* password = 0);
};

class ZJH_encrypto2 {
public:
	bool encrypto(const std::string& path, int len);
};

class ZJH_decrypto {
public:
	bool decrypto(const std::string& path, const char* password = 0);
	bool decrypto_to(const std::string& path, const std::string& output_path, const char* password = 0);
};

bool ZJH_GetPackageInfo(const std::string& path, bool* hasPassword);
