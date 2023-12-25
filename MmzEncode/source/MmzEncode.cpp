#include <iostream>
#include <shlwapi.h>
#include "FileData.hpp"
#include "Format.hpp"
#include "Json.hpp"
#include "Png.hpp"
#include "MzImage.hpp"

#pragma comment(lib, "Shlwapi.lib")

extern "C"
{
	void encode(unsigned char* in_buffer, size_t in_buffer_size, unsigned char* out_buffer, size_t* out_buffer_size);
};

static const char* const NAME = "MZ画像エンコードプログラム";
static const char* const VERSION = "1.0.0";
static const char* const FILENAME = "MmzEccode";
static const unsigned int OPTION_HELP = 0x00000001;
static const unsigned int OPTION_80B  = 0x00000002;
static const unsigned int OPTION_ADD  = 0x00000004;

int main(int argc, char* argv[])
{
	try
	{
		unsigned int option = 0;
		std::string path1;
		std::string path2;
		std::string path3;
		int width = 640;
		int height = 200;
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
						else if (_strnicmp(&argv[i][1], "?", 1) == 0)
						{
							option |= OPTION_HELP;
						}
						else if (_strnicmp(&argv[i][1], "80B", 3) == 0)
						{
							option |= OPTION_80B;
							width = 320;
						}
						else if (_strnicmp(&argv[i][1], "ADD", 3) == 0)
						{
							option |= OPTION_ADD;
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
			std::cout << "Usage : " << NAME << " [option] <input png1 filename> [input png2 filename] <output mzt filename>" << std::endl;
			std::cout << std::endl;
			std::cout << "Option : /HELP            Display help message." << std::endl;
			std::cout << std::endl;
			return -1;
		}
		if (path1.empty() == true)
		{
			std::cout << "Invalid png filename." << std::endl;
			return -1;
		}
		if (path2.empty() == true)
		{
			std::cout << "Invalid mzt filename." << std::endl;
			return -1;
		}
		if (path3.empty() == true)
		{
			// ファイル名が2つしか指定されていない場合はpath2にpath1、path3にpath2をいれる
			path3 = path2;
			path2 = path1;
			path1.clear();
		}
		bool isBeforeValid = !path1.empty();
		FileData fileData;
		Png beforePng;
		if (isBeforeValid == true)
		{
			// PNG1ファイル読み込み
			if (fileData.Load(path1) == true)
			{
				void* buffer = fileData.GetBuffer();
				size_t bufferSize = fileData.GetBufferSize();
				// PNG1作成
				beforePng.Load(buffer, bufferSize);
				unsigned int* pixelBuffer = reinterpret_cast<unsigned int*>(beforePng.GetBuffer());
				int sourceWidth = beforePng.GetWidth();
				int sourceHeight = beforePng.GetHeight();
				if ((sourceWidth != width) || (sourceHeight != height))
				{
					std::cout << "Invalid png1 size." << std::endl;
					beforePng.Release();
					return -1;
				}
			}
			else
			{
				std::cout << "Cannot load png1." << std::endl;
				return -1;
			}
		}
		Png sourcePng;
		// PNG2ファイル読み込み
		if (fileData.Load(path2) == true)
		{
			void* buffer = fileData.GetBuffer();
			size_t bufferSize = fileData.GetBufferSize();
			// PNG2作成
			sourcePng.Load(buffer, bufferSize);
			unsigned int* pixelBuffer = reinterpret_cast<unsigned int*>(sourcePng.GetBuffer());
			int sourceWidth = sourcePng.GetWidth();
			int sourceHeight = sourcePng.GetHeight();
			if ((sourceWidth != width) || (sourceHeight != height))
			{
				std::cout << "Invalid png2 size." << std::endl;
				sourcePng.Release();
				return -1;
			}
		}
		else
		{
			std::cout << "Cannot load png2." << std::endl;
			return -1;
		}
		if (!(option & OPTION_ADD))
		{
			_unlink(path3.c_str());
		}
		MzImage mzImage;
		if (option & OPTION_80B)
		{
			mzImage.SetMode(MzImage::MODE_80B);
		}
		if (isBeforeValid == true)
		{
			mzImage.SetBeforeImage(&beforePng);
		}
		mzImage.SetImage(&sourcePng);
		std::vector<std::vector<unsigned char>> mmzImageList = mzImage.GetEncodeData();
		if(mmzImageList.size() == 1)
		{
			if(mmzImageList[0].size() == 1)
			{
				// 違いが無いので追加しない
				std::cout << "AddFile: " << path2 << ", AddSize: " << 0 << " byte" << std::endl;
				return 0;
			}
		}
		unsigned char signature = 'A';
		size_t addSize = 0;
		for(std::vector<unsigned char> mmzImage: mmzImageList)
		{
			size_t lzeBufferSize = 8192;
			std::vector<unsigned char> lzeBuffer(lzeBufferSize, 0);
			// Debug
#if false
			{
				FileData file;
				file.SetBuffer(&mmzImage[0], mmzImage.size());
				std::string debugFilename = path3;
				debugFilename.push_back(signature);
				debugFilename += ".bin";
				file.Save(debugFilename);
			}
#endif
			encode(&mmzImage[0], mmzImage.size(), &lzeBuffer[0], &lzeBufferSize);
			// Debug
#if false
			{
				FileData file;
				file.SetBuffer(&lzeBuffer[0], lzeBufferSize);
				std::string debugFilename = path3;
				debugFilename.push_back(signature);
				debugFilename += ".lze";
				file.Save(debugFilename);
			}
#endif
			std::vector<unsigned char> MztHeader(128, 0);
			MztHeader[0x0000] = 1; // Binary
			for(int i = 0; i < 17; ++ i)
			{
				MztHeader[0x0001 + i] = 0x0D;
			}
			std::string filename = PathFindFileNameA(path2.c_str());
			for(int i = 0; i < 16; ++ i)
			{
				if(filename[i] == 0)
				{
					MztHeader[0x0001 + i] = signature;
					break;
				}
				if(filename[i] == '.')
				{
					MztHeader[0x0001 + i] = signature;
					break;
				}
				MztHeader[0x0001 + i] = filename[i];
			}
			// Size
			MztHeader[0x0012] = static_cast<unsigned char>(lzeBufferSize & 255);
			MztHeader[0x0013] = static_cast<unsigned char>(lzeBufferSize / 256);
			// LoadAddress
			MztHeader[0x0014] = 0x00;
			MztHeader[0x0015] = 0xC0;
			// ExecuteAddress
			MztHeader[0x0016] = 0xB1;
			MztHeader[0x0017] = 0;
			// Save
			FileData mztFile;
			mztFile.SetBuffer(&MztHeader[0], 128);
			mztFile.SaveAdd(path3);
			addSize += 128;
			mztFile.SetBuffer(&lzeBuffer[0], lzeBufferSize);
			mztFile.SaveAdd(path3);
			addSize += lzeBufferSize;
#if false
			{
				FileData file;
				std::string debugFilename = path3;
				debugFilename.push_back(signature);
				debugFilename += ".mzt";
				file.SetBuffer(&MztHeader[0], 128);
				file.Save(debugFilename);
				file.SetBuffer(&lzeBuffer[0], lzeBufferSize);
				file.SaveAdd(debugFilename);
			}
#endif
			++ signature;
		}
		std::cout << "AddFile: " << path2 << ", AddSize: " << addSize << " bytes" << std::endl;
	}
	catch (std::string message)
	{
		std::cout << "Error: " << message << std::endl;
	}
}
