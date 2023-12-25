#ifndef MzIMAGE_HPP
#define MzIMAGE_HPP

#include <vector>
#include "Png.hpp"

class MzImage
{
public:
	enum
	{
		WIDTH = 40,
		HEIGHT = 25,
		IMAGE_WIDTH_2000 = 640,
		IMAGE_WIDTH_80B = 640,
		IMAGE_HEIGHT = 200,
		ENCODE_BUFFER_MAX = 8192,
		MODE_2000 = 0,
		MODE_80B
	};
	MzImage(void);
	~MzImage(void);
	void SetMode(int mode);
	void SetBeforeImage(Png* beforePng);
	void SetImage(Png* png);
	std::vector<std::vector<unsigned char>> GetEncodeData(void);
private:
	std::vector<unsigned char> GetMzImage16x8(int x, int y, int plane) const;
	std::vector<unsigned char> GetMzImage8x8(int x, int y) const;
	int GetSamePlaneFlag(int x, int y) const;
	int GetPlaneFlag(int x, int y) const;
	bool IsSame2000(int x, int y, int plane) const;
	bool IsSame80B(int x, int y, int plane) const;
	MzImage(MzImage&);
	MzImage& operator = (MzImage&);
	int mode;
	Png* beforePng;
	Png* png;
	unsigned int samePlaneFlag[HEIGHT][WIDTH];
	unsigned int maskTable[4];
};

#endif
