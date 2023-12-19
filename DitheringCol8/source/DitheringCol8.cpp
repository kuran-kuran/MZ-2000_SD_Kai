#include <iostream>
#include "FileData.hpp"
#include "Format.hpp"
#include "Json.hpp"
#include "Png.hpp"
#include "Dithering.hpp"

static const char* const NAME = "ディザリング減色プログラム";
static const char* const VERSION = "1.0.0";
static const char* const FILENAME = "DitheringCol8";
static const unsigned int OPTION_HELP = 0x00000001;

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
			std::cout << std::endl;
			return -1;
		}
		if (path1.empty() == true)
		{
			std::cout << "Invalid png filename." << std::endl;
			return -1;
		}

		// PNGファイル読み込み
		FileData fileData;
		fileData.Load(path1);
		void* buffer = fileData.GetBuffer();
		size_t bufferSize = fileData.GetBufferSize();

		// PNG作成
		Png png;
		png.Load(buffer, bufferSize);
		unsigned int* pixelBuffer = reinterpret_cast<unsigned int*>(png.GetBuffer());
		int width = png.GetWidth();
		int height = png.GetHeight();

		// ディザリング実行
		Dithering dithering;
		dithering.SetPixelBuffer(pixelBuffer, width, height);
		dithering.Dithering2x2();
		std::vector<unsigned int> ditheringBuffer;
		dithering.GetBuffer(ditheringBuffer);

		// ディザリング画像作成
		Png ditheringPng;
		ditheringPng.Create(width, height);
		ditheringPng.SetPixelBuffer(&ditheringBuffer[0], width * height * sizeof(unsigned int));
//		ditheringPng.SetPixelBuffer(pixelBuffer, width * height * sizeof(unsigned int)); //@@
		std::vector<unsigned char> saveBuffer;
		ditheringPng.Save(saveBuffer);

		// ディザリング画像保存
		fileData.SetBuffer(&saveBuffer[0], saveBuffer.size());
		fileData.Save(path2);
	}
	catch (std::string message)
	{
		std::cout << "Error: " << message << std::endl;
	}
}
