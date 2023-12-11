#include <iostream>
#include <Windows.h>
#include "FileData.hpp"
#include "Format.hpp"
#include "Json.hpp"

#pragma comment(lib, "urlmon.lib")

static const char* const NAME = "パッチ当てプログラム";
static const char* const VERSION = "1.0.0";
static const char* const FILENAME = "Patch";
static const unsigned int OPTION_HELP  = 0x00000001;
static const unsigned int OPTION_FORCE = 0x00000002;

std::vector<std::string> split(std::string str, char delimiter)
{
	size_t first = 0;
	size_t last = str.find_first_of(delimiter);
	std::vector<std::string> result;
	if (last == std::string::npos)
	{
		result.push_back(str);
		return result;
	}
	while (first < str.size())
	{
		std::string subStr(str, first, last - first);
		result.push_back(subStr);
		first = last + 1;
		last = str.find_first_of(delimiter, first);
		if (last == std::string::npos)
		{
			last = str.size();
		}
	}
	return result;
}

int main(int argc, char* argv[])
{
	try
	{
		unsigned int option = 0;
		std::string path1;
		std::string path2;
		std::string path3;
		int i;
		// オプション取得
		if (path1.empty() == true)
		{
			if (argc > 1)
			{
				for (i = 1; i < argc; i++)
				{
					if ((argv[i][0] == '/') || (argv[i][0] == '-'))
					{
						if (_strnicmp(&argv[i][1], "HELP", 4) == 0)
						{
							option |= OPTION_HELP;
						}
						else if (_strnicmp(&argv[i][1], "FORCE", 5) == 0)
						{
							option |= OPTION_FORCE;
						}
						else if (_strnicmp(&argv[i][1], "?", 1) == 0)
						{
							option |= OPTION_HELP;
						}
					}
					else
					{
						if (path1.empty())
						{
							path1 = argv[i];
						}
						else if (path2.empty())
						{
							path2 = argv[i];
						}
						else if (path3.empty())
						{
							path3 = argv[i];
						}
					}
				}
			}
			else
			{
				option |= OPTION_HELP;
			}
		}
		// タイトル表示
		std::cout << NAME << " version." << VERSION << "\n";
		// オプション表示
		if (option & OPTION_HELP)
		{
			std::cout << "Usage : " << NAME << " [option] [input json] [input mzt filename] [output mzt filename]" << std::endl;
			std::cout << std::endl;
			std::cout << "Option : /HELP            Display help message." << std::endl;
			std::cout << "         /FORCE           Force execution." << std::endl;
			std::cout << std::endl;
			return -1;
		}
		if (path1.empty() == true)
		{
			std::cout << "Invalid mzt filename." << std::endl;
			return -1;
		}
		if (path2.empty())
		{
			std::cout << "Invalid json filename." << std::endl;
			return -1;
		}
		if (path3.empty())
		{
			std::cout << "Invalid output filename." << std::endl;
			return -1;
		}
		// Create Folder
		CreateDirectoryA("APP_BASIC_LISP_SD", NULL);
		CreateDirectoryA("APP_TS-1000_TS-2000_SD", NULL);
		CreateDirectoryA("EXT-ROM", NULL);
		// Load json
		FileData jsonFile;
		jsonFile.Load(path1);
		std::string jsonText = reinterpret_cast<char*>(jsonFile.GetBuffer());
		Json json(jsonText);
		std::string titleText = json["Title"];
		std::string originalText = json["Original"];
		std::string createText = json["Create"];
		std::string support = json["Support"];
		std::string patchText = json["Patch"];
		std::string patchFileText = json["PatchFile"];
		std::string saveFileText = json["SaveFile"];
		// 表題
		std::cout << titleText << std::endl;
		std::cout << "「" << originalText << "」から" << support << "対応バージョン「" << createText << "」を作成します。" << std::endl;
		// Load mzt
		FileData mztFile;
		mztFile.Load(path2);
		unsigned char* mztFileBuffer = reinterpret_cast<unsigned char*>(mztFile.GetBuffer());
		std::vector<unsigned char> mztBuffer;
		std::copy(mztFileBuffer, mztFileBuffer + mztFile.GetBufferSize(), std::back_inserter(mztBuffer));
		// Patch
		if (patchText.size() > 0)
		{
			Json patchJson(patchText);
			for (size_t i = 0; i < patchJson.Count(); ++ i)
			{
//				std::cout << patchJson[i] << std::endl;
				Json patchData(patchJson[i]);
				size_t address = strtol(patchData["Address"].c_str(), NULL, 16);
				std::vector<std::string> after = split(patchData["After"], ',');
				std::vector<std::vector<std::string>> before;
				Json beforeJson(patchData["Before"]);
				for (size_t j = 0; j < beforeJson.Count(); ++ j)
				{
					std::vector<std::string> beforeData = split(beforeJson[j], ',');
					if ((!(option & OPTION_FORCE)) && (beforeData.size() != after.size()))
					{
						throw Format("Abort: jsonデータのBeforeとAfterのバイト数が違っています。 (%s : %s)", patchData["Before"].c_str(), patchData["After"].c_str());
					}
					before.push_back(beforeData);
				}
				// Patchする
				for (size_t j = 0; j < after.size(); ++ j)
				{
					unsigned char patchData = static_cast<unsigned char>(strtol(after[j].c_str(), NULL, 16));
					// beforeと比較する
					bool find = false;
					for (size_t k = 0; k < before.size(); ++ k)
					{
						unsigned char beforeData = static_cast<unsigned char>(strtol(before[k][j].c_str(), NULL, 16));
						if (mztBuffer[address + j] == beforeData)
						{
							find = true;
							break;
						}
					}
					if ((!(option & OPTION_FORCE)) && (find == false))
					{
						throw Format("Abort: 元データの%Xh(%u)バイト目が想定と違っています。", address + j, address + j);
					}
					mztBuffer[address + j] = patchData;
				}
			}
		}
		// PatchFile
		if (patchFileText.size() > 0)
		{
			Json patchFileJson(patchFileText);
			for (size_t i = 0; i < patchFileJson.Count(); ++ i)
			{
//				std::cout << patchFileJson[i] << std::endl;
				Json patchFileData(patchFileJson[i]);
				// ダウンロード
				std::string dwnld_URL = "https://github.com/yanataka60/MZ-2000_SD/raw/main/APP_BASIC_LISP_SD/APP_BASIC_LISP_SD_MZ-1Z001.bin";
				URLDownloadToFileA(NULL, dwnld_URL.c_str(), patchFileData["File"].c_str(), 0, NULL);
				std::cout << patchFileData["File"] << "をダウンロードしました。" << std::endl;
				// ファイルパッチ
				FileData patchFile;
				bool result = patchFile.Load(patchFileData["File"]);
				if(result == false)
				{
					throw Format("Abort: %sがみつかりません。", patchFileData["File"].c_str());
				}
				size_t address = strtol(patchFileData["Address"].c_str(), NULL, 16);
				size_t fileSize = patchFile.GetBufferSize();
				size_t bufferSize = address + fileSize;
				mztBuffer.resize(bufferSize, 0);
				memcpy(&mztBuffer[address], patchFile.GetBuffer(), fileSize);
			}
		}
		// Save
		if (saveFileText.size() > 0)
		{
			Json saveFileJson(saveFileText);
			size_t saveStart = strtol(saveFileJson["Start"].c_str(), NULL, 16);
			size_t saveSize;
			if (_strnicmp(saveFileJson["Size"].c_str(), "ALL", 3) == 0)
			{
				saveSize = mztBuffer.size();
			}
			else
			{
				saveSize = strtol(saveFileJson["Size"].c_str(), NULL, 16);
			}
			unsigned short mztDataSize = static_cast<unsigned short>(saveSize - 0x80);
			mztBuffer[0x0012] = mztDataSize % 256;
			mztBuffer[0x0013] = mztDataSize / 256;
			size_t bufferSize = saveSize + saveStart;
			if(mztBuffer.size() < bufferSize)
			{
				mztBuffer.resize(bufferSize, 0);
			}
			FileData saveFile;
			saveFile.SetBuffer(&mztBuffer[saveStart], saveSize);
			saveFile.Save(path3);
			std::cout << path3 << "を作成しました。" << std::endl;
		}
	}
	catch (std::string message)
	{
		std::cout << "Error: " << message << std::endl;
	}
}
