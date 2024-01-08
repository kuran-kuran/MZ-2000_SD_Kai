#include <iterator>
#include "MzImage.hpp"

MzImage::MzImage(void)
:mode(MODE_2000)
,beforePng(NULL)
,png(NULL)
,samePlaneFlag()
,maskTable{0x00000000, 0x00FF0000, 0x000000FF, 0x0000FF00}
,maskShift{0, 16, 0, 8}
{
	this->vramBase = this->mode == MODE_2000 ? 0xC000 : 0x6000;
	this->width = this->mode == MODE_2000 ? IMAGE_WIDTH_2000 : IMAGE_WIDTH_80B;
	this->tileWidth = this->mode == MODE_2000 ? 16 : 8;
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
			this->samePlaneFlag[y][x] = GetPlaneFlag(x * this->tileWidth, y * 8);
		}
	}
}

void MzImage::SetMode(int mode)
{
	this->mode = mode;
	this->vramBase = this->mode == MODE_2000 ? 0xC000 : 0x6000;
	this->width = this->mode == MODE_2000 ? IMAGE_WIDTH_2000 : IMAGE_WIDTH_80B;
	this->tileWidth = this->mode == MODE_2000 ? 16 : 8;
}

std::vector<std::vector<unsigned char>> MzImage::GetEncodeData(void)
{
	std::vector<std::vector<unsigned char>> encodeBufferList;
	std::vector<unsigned char> encodeBuffer;
	std::vector<unsigned char> tempImageBuffer;
	for (int y = 0; y < HEIGHT; ++ y)
	{
		// 0: �ŏ��̉摜�����A����������A�h���X�ۑ�
		int phase = 0;
		int setPhase = phase;
		int beforePlaneFlag = 0;
		bool imageLatch = false;
		int imageCount = 0;
		unsigned short vramAddress = 0;
		bool addFlag = false;
		for (int x = 0; x < WIDTH; ++ x)
		{
			unsigned int planeFlag = this->samePlaneFlag[y][x];
			// planeFlag���ς�����Ax���E�[�ɗ���
			if ((planeFlag != beforePlaneFlag) || (x == (WIDTH - 1)))
			{
				int xScale = this->mode == MODE_2000 ? 2 : 1;
				if (x == (WIDTH - 1))
				{
					if (planeFlag != 0)
					{
						GetMzImage(tempImageBuffer, x, y, planeFlag);
						++imageCount;
						addFlag = true;
					}
				}
				if (beforePlaneFlag != 0)
				{
					if (vramAddress != 0)
					{
						// �o�^����
						// �o�^�T�C�Y����
						size_t addSize = 2 + 1 + 1 + tempImageBuffer.size();
						if (encodeBuffer.size() + addSize >= ENCODE_BUFFER_MAX - 1)
						{
							encodeBuffer.push_back(0x0A);
							encodeBufferList.push_back(encodeBuffer);
							encodeBuffer.clear();
						}
						// �R�}���hC0h �`��A�h���X�o�^
						unsigned char vramAddressHigh = vramAddress >> 8;
						unsigned char vramAddressLow = vramAddress & 0xFF;
						encodeBuffer.push_back(static_cast<unsigned char>(vramAddressHigh));
						encodeBuffer.push_back(static_cast<unsigned char>(vramAddressLow));
						// �R�}���h01h�`07h �`��v���[���o�^
						encodeBuffer.push_back(static_cast<unsigned char>(beforePlaneFlag));
						// ���o�^
						encodeBuffer.push_back(static_cast<unsigned char>(imageCount));
						// �摜�o�^
						std::copy(tempImageBuffer.begin(), tempImageBuffer.end(), std::back_inserter(encodeBuffer));
						imageCount = 0;
						tempImageBuffer.clear();
						vramAddress = this->vramBase + (y * this->width + x * xScale);
					}
				}
				else
				{
					vramAddress = this->vramBase + (y * this->width + x * xScale);
				}
			}
			// �摜�擾
			if ((planeFlag != 0) && (addFlag == false))
			{
				GetMzImage(tempImageBuffer, x, y, planeFlag);
				++imageCount;
			}
			beforePlaneFlag = planeFlag;
		}
	}
	encodeBuffer.push_back(0x0B);
	encodeBufferList.push_back(encodeBuffer);
	return encodeBufferList;
}

void MzImage::GetMzImage(std::vector<unsigned char>& tempImageBuffer, int x, int y, unsigned int planeFlag)
{
	int xx = x * this->tileWidth;
	int y8 = y * 8;
	unsigned int mask = 1;
	for (int i = 1; i <= 3; ++i)
	{
		if (planeFlag & mask)
		{
			std::vector<unsigned char> image;
			image = GetMzTileImage(xx, y8, i);
			std::copy(image.begin(), image.end(), std::back_inserter(tempImageBuffer));
		}
		mask <<= 1;
	}
}

std::vector<unsigned char> MzImage::GetMzTileImage(int x, int y, int plane) const
{
	std::vector<unsigned char> tileImageBuffer;
	unsigned char* imageBuffer = this->png->GetBuffer();
	unsigned bitData = this->mode == MODE_2000 ? 0x8000 : 0x80;
	for(int yy = 0; yy < 8; ++ yy)
	{
		unsigned short data = 0;
		for(int xx = 0; xx < this->tileWidth; ++ xx)
		{
			int index = ((y + yy) * this->width + (x + xx)) * 4;
			unsigned int mask = this->maskTable[plane];
			unsigned int pixel = *reinterpret_cast<unsigned int*>(&imageBuffer[index]);
			unsigned short orData = 0;
			unsigned int planePixel = (pixel & mask) >> this->maskShift[plane];
			if(planePixel > 128)
			{
				orData = bitData;
			}
			data >>= 1;
			data |= orData;
		}
		if (this->mode == MODE_2000)
		{
			std::copy(reinterpret_cast<unsigned char*>(&data), reinterpret_cast<unsigned char*>(&data) + sizeof(data), std::back_inserter(tileImageBuffer));
		}
		else
		{
			std::copy(reinterpret_cast<unsigned char*>(&data), reinterpret_cast<unsigned char*>(&data) + 1, std::back_inserter(tileImageBuffer));
		}
	}
	return tileImageBuffer;
}

int MzImage::GetSamePlaneFlag(int x, int y) const
{
	return this->samePlaneFlag[y][x];
}

// Result:
// 0: �S������
// 0b001: �v���[�����Ⴄ 
// 0b010: �ԃv���[�����Ⴄ 
// 0b100: �΃v���[�����Ⴄ 
int MzImage::GetPlaneFlag(int x, int y) const
{
	int plane = this->mode == MODE_2000 ? 7 : 1;
	// ���ꏏ��
	if (IsSame(x, y, 1) == true)
	{
		plane -= 1;
	}
	if (this->mode == MODE_2000)
	{
		// �Ԃ��ꏏ��
		if (IsSame(x, y, 2) == true)
		{
			plane -= 2;
		}
		// �Ԃ��ꏏ��
		if (IsSame(x, y, 3) == true)
		{
			plane -= 4;
		}
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
		for (int xx = 0; xx < this->tileWidth; ++ xx)
		{
			int index = ((y + yy) * this->width + (x + xx)) * 4;
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
