#pragma once

#include <string>

// 进度阶段枚举：
// 用于把“总进度百分比”拆分为可解释的处理阶段，便于界面显示“当前阶段 + ETA”。
enum ZJH_PROGRESS_STAGE
{
	ZJH_PROGRESS_STAGE_PREPARE = 0,
	ZJH_PROGRESS_STAGE_READ_INPUT = 1,
	ZJH_PROGRESS_STAGE_BUILD_TREE = 2,
	ZJH_PROGRESS_STAGE_OPTIMIZE_CODE = 3,
	ZJH_PROGRESS_STAGE_PACK_OUTPUT = 4,
	ZJH_PROGRESS_STAGE_ENCRYPT_PAYLOAD = 5,
	ZJH_PROGRESS_STAGE_WRITE_FILE = 6,
	ZJH_PROGRESS_STAGE_DONE = 7
};

// 压缩进度回调：
// percent   : 总进度（0~100）
// stage     : 当前阶段，取值见 ZJH_PROGRESS_STAGE
// elapsedMs : 从压缩开始到当前的已耗时（毫秒）
// etaMs     : 预计剩余时间（毫秒）
// userData  : 上层透传上下文指针（通常传 this）
typedef void (*ZJH_ProgressCallback)(int percent,
	int stage,
	unsigned long elapsedMs,
	unsigned long etaMs,
	void* userData);

class ZJH_encrypto {
public:
	bool encrypto(const std::string& path,
		int len,
		ZJH_ProgressCallback progressCallback = 0,
		void* userData = 0);
};

class ZJH_encrypto1 {
public:
	bool encrypto(const std::string& path,
		int len,
		const char* password = 0,
		ZJH_ProgressCallback progressCallback = 0,
		void* userData = 0);
};

class ZJH_encrypto2 {
public:
	bool encrypto(const std::string& path,
		int len,
		ZJH_ProgressCallback progressCallback = 0,
		void* userData = 0);
};

class ZJH_decrypto {
public:
	bool decrypto(const std::string& path, const char* password = 0);
	bool decrypto_to(const std::string& path, const std::string& output_path, const char* password = 0);
};

bool ZJH_GetPackageInfo(const std::string& path, bool* hasPassword);
