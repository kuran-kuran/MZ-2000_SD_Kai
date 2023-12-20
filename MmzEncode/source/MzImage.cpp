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
		for (int x = 0; x < WIDTH; ++ x)
		{
			unsigned int planeFlag = this->samePlaneFlag[y][x];
			if (phase == 0)
			{
				if (planeFlag != 0)
				{
					// �o�^�T�C�Y
					size_t addSize = 2;
					// �`��A�h���X
					vramAddress = 0xC000 + (y * 640 + x * 2);
					setPhase = 1;
				}
			}
			if (planeFlag != 0)
			{
				// �摜�擾
				int x16 = x * 16;
				int y8 = y * 8;
				unsigned int mask = 1;
				for (int i = 1; i <= 3; ++i)
				{
					if (planeFlag & mask)
					{
						std::vector<unsigned char> image16x8 = GetMzImage16x8(x16, y8, i);
						std::copy(image16x8.begin(), image16x8.end(), std::back_inserter(tempImageBuffer));
					}
					mask <<= 1;
				}
				++ imageCount;
			}
			if (phase == 1)
			{
				// ���擾
				// ����planeFlag�������Ă���Ax���E�[�ɗ��Ă��Ȃ�
				if((planeFlag != beforePlaneFlag) || (x == (WIDTH - 1)))
				{
					// planeFlag�ς����A�܂���x���E�[�ɗ���
					// �o�^�T�C�Y
					size_t addSize = 2 + 1 + 1 + tempImageBuffer.size();
					// �o�^�T�C�Y
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
					encodeBuffer.push_back(static_cast<unsigned char>(planeFlag));
					// ���o�^
					encodeBuffer.push_back(static_cast<unsigned char>(imageCount));
					// �摜�o�^
					std::copy(tempImageBuffer.begin(), tempImageBuffer.end(), std::back_inserter(encodeBuffer));
					imageCount = 0;
					tempImageBuffer.clear();
					setPhase = 0;
				}
			}
			phase = setPhase;
			beforePlaneFlag = planeFlag;
		}
	}
	encodeBuffer.push_back(0x0B);
	encodeBufferList.push_back(encodeBuffer);
	return encodeBufferList;
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
// 0: �S������
// 0b001: �v���[�����Ⴄ 
// 0b010: �ԃv���[�����Ⴄ 
// 0b100: �΃v���[�����Ⴄ 
int MzImage::GetPlaneFlag(int x, int y) const
{
	int plane = 7;
	// ���ꏏ��
	if (IsSame(x, y, 1) == true)
	{
		plane -= 1;
	}
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
