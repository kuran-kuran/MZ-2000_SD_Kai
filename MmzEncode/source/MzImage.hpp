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
		IMAGE_WIDTH = 640,
		IMAGE_HEIGHT = 200
	};
	MzImage(void);
	~MzImage(void);
	void SetBeforeImage(Png* beforePng);
	void SetImage(Png* png);
	std::vector<unsigned char> GetMzImage16x8(int plane) const;
private:
	int GetPlaneFlag(int x, int y);
	bool IsSame(int x, int y, int plane);
	MzImage(MzImage&);
	MzImage& operator = (MzImage&);
	Png* beforePng;
	Png* png;
	int samePlaneFlag[HEIGHT][WIDTH];
};

#endif
