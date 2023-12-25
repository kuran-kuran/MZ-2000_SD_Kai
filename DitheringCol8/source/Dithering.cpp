#include <iterator>
#include "Dithering.hpp"
#include "Png.hpp"

Dithering::Dithering(void)
:sourceBuffer()
,width(0)
,height(0)
,plane(RED | GREEN | BLUE)
,maskTable{ 0x00000000, 0x00FF0000, 0x000000FF, 0x0000FF00 }
{
}

Dithering::~Dithering(void)
{
}

void Dithering::SetPlane(unsigned int plane)
{
	this->plane = plane;
}

void Dithering::SetPixelBuffer(void* buffer, int width, int height)
{
	size_t bufferSize = static_cast<size_t>(width) * height;
	this->sourceBuffer.clear();
	std::copy(reinterpret_cast<unsigned int*>(buffer), reinterpret_cast<unsigned int*>(buffer) + bufferSize, std::back_inserter(this->sourceBuffer));
	this->width = width;
	this->height = height;
}

void Dithering::GetBuffer(std::vector<unsigned int>& buffer)
{
	buffer.clear();
	std::copy(this->ditheringBuffer.begin(), this->ditheringBuffer.end(), std::back_inserter(buffer));
}

void Dithering::Dithering2x2(void)
{
	// 白黒画像作成
	std::vector<unsigned char> grayBuffer;
	for (size_t i = 0; i < this->sourceBuffer.size(); ++i)
	{
		double red = static_cast<double>(this->sourceBuffer[i] & 255);
		double blue = static_cast<double>((this->sourceBuffer[i] >> 8) & 255);
		double green = static_cast<double>((this->sourceBuffer[i] >> 16) & 255);
		double gray = blue * 0.11 + red * 0.3 + green * 0.59;
		if (gray > 255.0)
		{
			gray = 0.0;
		}
		grayBuffer.push_back(static_cast<unsigned char>(gray));
	}
	// 白黒画像拡張
	std::vector<unsigned char> dilateBuffer;
	std::copy(grayBuffer.begin(), grayBuffer.end(), std::back_inserter(dilateBuffer));
	for (size_t y = 1; y < height - 1; ++y)
	{
		for (size_t x = 1; x < width - 1; ++x)
		{
			unsigned char cmp[5];
			cmp[0] = grayBuffer[y * width + x];
			cmp[1] = grayBuffer[(y - 1) * width + x];
			cmp[2] = grayBuffer[(y + 1) * width + x];
			cmp[3] = grayBuffer[y * width + x - 1];
			cmp[4] = grayBuffer[y * width + x + 1];
			unsigned char color = 0;
			for (int i = 0; i < 5; ++i)
			{
				if (color < cmp[i])
				{
					color = cmp[i];
				}
			}
			dilateBuffer[y * width + x] = color;
		}
	}
	// 線画作成
	std::vector<unsigned char> lineBuffer(width * height, 0);
	for (size_t i = 0; i < width * height; ++i)
	{
		lineBuffer[i] = abs(grayBuffer[i] - dilateBuffer[i]) > 64 ? 0 : 255;
	}
	// ディザ画像作成
	static const unsigned char dithering2x2Table[5][4] =
	{
		{0, 0, 0, 0},
		{255, 0, 0, 0},
		{255, 0, 0, 255},
		{255, 255, 0, 255},
		{255, 255, 255, 255}
	};
	this->ditheringBuffer.clear();
	this->ditheringBuffer.resize(width * height * sizeof(unsigned int), 0);
	for (size_t y = 0; y < height; y += 2)
	{
		if (y + 1 >= height)
		{
			break;
		}
		for (size_t x = 0; x < width; x += 2)
		{
			if (x + 1 >= width)
			{
				break;
			}
			Color color[4];
			color[0].pixelData = this->sourceBuffer[(y + 1)* width + x];
			color[1].pixelData = this->sourceBuffer[y * width + x + 1];
			color[2].pixelData = this->sourceBuffer[(y + 1) * width + x + 1];
			color[3].pixelData = this->sourceBuffer[y * width + x];
			unsigned int blueSum = color[0].pixel32.blue + color[1].pixel32.blue + color[2].pixel32.blue + color[3].pixel32.blue;
			unsigned int redSum = color[0].pixel32.red + color[1].pixel32.red + color[2].pixel32.red + color[3].pixel32.red;
			unsigned int greenSum = color[0].pixel32.green + color[1].pixel32.green + color[2].pixel32.green + color[3].pixel32.green;
			unsigned int blueIndex = (blueSum / 205);
			unsigned int redIndex = (redSum / 205);
			unsigned int greenIndex = (greenSum / 205);
			Color pixelColor[4] = {0, 0, 0, 0};
			for (int i = 0; i < 4; ++ i)
			{
				pixelColor[i].pixel32.blue = (this->plane & BLUE) ? dithering2x2Table[blueIndex][i] : 0;
				pixelColor[i].pixel32.red = (this->plane & RED) ? dithering2x2Table[redIndex][i] : 0;
				pixelColor[i].pixel32.green = (this->plane & GREEN) ? dithering2x2Table[greenIndex][i] : 0;
				pixelColor[i].pixel32.alpha = 255;
			}
			ditheringBuffer[y * width + x] = pixelColor[0].pixelData;
			ditheringBuffer[y * width + x + 1] = pixelColor[1].pixelData;
			ditheringBuffer[(y + 1) * width + x] = pixelColor[2].pixelData;
			ditheringBuffer[(y + 1) * width + x + 1] = pixelColor[3].pixelData;
		}
	}
	// 線画重ね合わせ
	for (size_t y = 0; y < height; ++ y)
	{
		for (size_t x = 0; x < width; ++ x)
		{
			if (lineBuffer[y * width + x] == 0)
			{
				Color black;
				black.pixelData = 0;
				black.pixel32.alpha = 255;
				ditheringBuffer[y * width + x] = black.pixelData;
			}
#if false
			else
			{
				Color white;
				white.pixelData = 0xFFFFFFFF;
				white.pixel32.alpha = 255;
				ditheringBuffer[y * width + x] = white.pixelData;
			}
#endif
		}
	}
}
