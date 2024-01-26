#ifndef MzIMAGE_HPP
#define MzIMAGE_HPP

#include <vector>
#include "Png.hpp"

class MzImage
{
public:
	enum
	{
		IMAGE_WIDTH_2000 = 640,
		IMAGE_WIDTH_80B = 320,
		IMAGE_HEIGHT = 200,
		ENCODE_BUFFER_MAX = 8192,
		SCREEN_640x200 = 0,
		SCREEN_320x200,
		MODE_16x8 = 0,
		MODE_8x8,
		MODE_8x4,
		COLOR_1 = 0,
		COLOR_8,
	};
	MzImage(void);
	~MzImage(void);
	void SetScreen(int screen);
	void SetMode(int mode);
	void SetColor(int color);
	void SetBeforeImage(Png* beforePng);
	void SetImage(Png* png);
	std::vector<std::vector<unsigned char>> GetEncodeData(void);
private:
	void GetMzImage(std::vector<unsigned char>& tempImageBuffer, int x, int y, unsigned int planeFlag);
	std::vector<unsigned char> GetMzTileImage(int x, int y, int plane) const;
	int GetSamePlaneFlag(int x, int y) const;
	void SetSamePlaneFlag(int x, int y, unsigned int flag);
	int GetPlaneFlag(int x, int y) const;
	bool IsSame(int x, int y, int plane) const;
	MzImage(MzImage&);
	MzImage& operator = (MzImage&);
	int screen;
	int mode;
	int color;
	Png* beforePng;
	Png* png;
	unsigned int* samePlaneFlag;
	int samePlaneFlagWidth;
	int samePlaneFlagHeight;
	unsigned int maskTable[4];
	unsigned int maskShift[4];
	unsigned short vramBase;
	unsigned short width;
	int tileWidth;
	int tileHeight;
};

#endif
