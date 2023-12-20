#include <iterator>
#include "MzImage.hpp"

MzImage::MzImage(void)
:beforePng(NULL)
,png(NULL)
,samePlaneFlag()
,maskTable{0x00000000, 0x00FF0000, 0x000000FF, 0x0000FF00}
{
}

MzImage::~MzImage(void)
{
}

void MzImage::SetBeforeImage(Png* beforePng)
{
	this->beforePng = beforePng;
}

void MzImage::SetImage(Png* png)
{
	this->png = png;
	for (int y = 0; y < HEIGHT; ++ y)
	{
		for (int x = 0; x < WIDTH; ++ x)
		{
			this->samePlaneFlag[y][x] = GetPlaneFlag(x * 16, y * 8);
		}
	}
}

// コマンド
// 00        : 1個スキップ
void MzImage::Command00(void)
{
}

// 01-07, xx : プレーン, 個数
void MzImage::Command01_07(int planeFlag, std::vector<unsigned char>& encodeBuffer)
{
	encodeBuffer.push_back(planeFlag);
}

// 0Ah       : 中断
void MzImage::CommandA0(void)
{
}

// 0Bh       : 終了
void MzImage::CommandB0(void)
{
}

// C000-FFFF : GVRAM Position
void MzImage::CommandC0(std::vector<unsigned char>& encodeBuffer)
{
	unsigned short vramAddress = 0xC000 + (y * 640 + x * 2);
	std::copy(reinterpret_cast<unsigned char*>(&vramAddress), reinterpret_cast<unsigned char*>(&vramAddress) + sizeof(vramAddress), std::back_inserter(encodeBuffer));
}

std::vector<unsigned char> MzImage::GetEncodeData(void)
{
	std::vector<unsigned char> encodeBuffer;
	std::vector<unsigned char> tempImageBuffer;
	for (int y = 0; y < HEIGHT; ++ y)
	{
		// 0: 最初の画像検索、見つかったらアドレス保存
		int phase = 0;
		int beforePlaneFlag = 0;
		bool imageLatch = false;
		int imageCount = 0;
		for (int x = 0; x < WIDTH; ++ x)
		{
			int planeFlag = this->samePlaneFlag[y][x];
			if(planeFlag == 0)
			{
				continue;
			}
			switch(phase)
			{
			case 0:
				{
					if(planeFlag == 0)
					{
						break;
					}
					// 描画アドレス
					CommandC0(encodeBuffer);
					// プレーン
					Command01_07(planeFlag, encodeBuffer);
					imageCount = 0;
					phase = 1;
					break;
				}
			case 1:
				// 個数取得
				{
					if(planeFlag == beforePlaneFlag)
					{
						++ imageCount;
						break;
					}
					// 変った
				}
			}
			if(planeFlag != 0)
			{
				int x16 = x * 16;
				int y8 = y * 8;
				for(int i = 1; i <= 3; ++ i)
				{
					std::vector<unsigned char> image16x8 = GetMzImage16x8(x16, y8, planeFlag);
					std::copy(image16x8.begin(), image16x8.end(), std::back_inserter(tempImageBuffer));
				}
			}
			beforePlaneFlag = planeFlag;
		}
	}
	return encodeBuffer;
}

std::vector<unsigned char> MzImage::GetMzImage16x8(int x, int y, int plane) const
{
	std::vector<unsigned char> image16x8Buffer;
	unsigned char* imageBuffer = this->png->GetBuffer();
	for(int yy = 0; yy < 8; ++ yy)
	{
		unsigned short data = 0;
		for(int xx = 0; xx < 16; ++ xx)
		{
			int index = ((y + yy) * IMAGE_WIDTH + (x + xx)) * 4;
			unsigned int mask = this->maskTable[plane];
			unsigned int pixel = *reinterpret_cast<unsigned int*>(&imageBuffer[index]);
			unsigned short orData = 0;
			if((pixel & mask) != 0)
			{
				orData = 0x8000;
			}
			data >>= 1;
			data |= orData;
		}
		std::copy(reinterpret_cast<unsigned char*>(&data), reinterpret_cast<unsigned char*>(&data) + sizeof(data), std::back_inserter(image16x8Buffer));
	}
	return image16x8Buffer;
}

int MzImage::GetSamePlaneFlag(int x, int y) const
{
	return this->samePlaneFlag[y][x];
}

// Result:
// 0: 全部同じ
// 0b001: 青プレーンが違う 
// 0b010: 赤プレーンが違う 
// 0b100: 緑プレーンが違う 
int MzImage::GetPlaneFlag(int x, int y) const
{
	int plane = 7;
	// 青が一緒か
	if (IsSame(x, y, 1) == true)
	{
		plane -= 1;
	}
	// 赤が一緒か
	if (IsSame(x, y, 2) == true)
	{
		plane -= 2;
	}
	// 赤が一緒か
	if (IsSame(x, y, 3) == true)
	{
		plane -= 4;
	}
	return plane;
}

bool MzImage::IsSame(int x, int y, int plane) const
{
	if (this->beforePng == NULL)
	{
		return false;
	}
	unsigned char* beforeImageBuffer = this->beforePng->GetBuffer();
	unsigned char* imageBuffer = this->png->GetBuffer();
	for (int yy = 0; yy < 8; ++ yy)
	{
		for (int xx = 0; xx < 16; ++ xx)
		{
			int index = ((y + yy) * IMAGE_WIDTH + (x + xx)) * 4;
			unsigned int mask = this->maskTable[plane];
			unsigned int beforePixel = *reinterpret_cast<unsigned int*>(&beforeImageBuffer[index]);
			unsigned int pixel = *reinterpret_cast<unsigned int*>(&imageBuffer[index]);
			if ((beforePixel & mask) != (pixel & mask))
			{
				return false;
			}
		}
	}
	return true;
}
