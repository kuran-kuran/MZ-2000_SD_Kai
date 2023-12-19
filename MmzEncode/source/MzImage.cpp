#include "MzImage.hpp"

MzImage::MzImage(void)
:beforePng(NULL)
,png(NULL)
,samePlaneFlag()
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
			samePlaneFlag[y][x] = GetPlaneFlag(x * 16, y * 8);
		}
	}
}

std::vector<unsigned char> MzImage::GetMzImage16x8(int plane) const
{
	std::vector<unsigned char> imageBuffer;
	return imageBuffer;
}

// Result:
// 0: 全部同じ
// 0b001: 青プレーンが違う 
// 0b010: 赤プレーンが違う 
// 0b100: 緑プレーンが違う 
int MzImage::GetPlaneFlag(int x, int y)
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

bool MzImage::IsSame(int x, int y, int plane)
{
	if (this->beforePng == NULL)
	{
		return false;
	}
	unsigned int maskTable[] =
	{
		0x00000000, 0x000000FF, 0x0000FF00, 0x00FF0000
	};
	unsigned char* beforeImageBuffer = this->beforePng->GetBuffer();
	unsigned char* imageBuffer = this->png->GetBuffer();
	for (int yy = 0; yy < 8; ++ yy)
	{
		for (int xx = 0; xx < 16; ++ xx)
		{
			int index = ((y + yy) * IMAGE_WIDTH + (x + xx)) * 4;
			unsigned int mask = maskTable[plane];
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
