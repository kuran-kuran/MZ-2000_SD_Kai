#include <iostream>
#include "FileData.hpp"
#include "Format.hpp"
#include "Json.hpp"
#include "Png.hpp"
#include "MzImage.hpp"

static const char* const NAME = "MZ画像エンコードプログラム";
static const char* const VERSION = "1.0.0";
static const char* const FILENAME = "MmzEccode";
static const unsigned int OPTION_HELP = 0x00000001;
static const unsigned int OPTION_80B = 0x00000002;

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
			path2 = path1;
			path3 = path2;
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
		MzImage mzImage;
		if (isBeforeValid == true)
		{
			mzImage.SetBeforeImage(&beforePng);
		}
		mzImage.SetImage(&sourcePng);
		std::vector<std::vector<unsigned char>> mmzImageList = mzImage.GetEncodeData();
		int a = 0;
	}
	catch (std::string message)
	{
		std::cout << "Error: " << message << std::endl;
	}
}
