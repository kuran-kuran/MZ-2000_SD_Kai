#ifndef DITHERING_HPP
#define DITHERING_HPP

#include <vector>

class Dithering
{
public:
	struct Pixel32
	{
		unsigned char red;
		unsigned char blue;
		unsigned char green;
		unsigned char alpha;
	};
	union Color
	{
		unsigned int pixelData;
		Pixel32 pixel32;
	};
	Dithering(void);
	~Dithering(void);
	void SetPixelBuffer(void* buffer, int width, int height);
	void GetBuffer(std::vector<unsigned int>& buffer);
	void Dithering2x2(void);
private:
	std::vector<unsigned int> sourceBuffer;
	std::vector<unsigned int> ditheringBuffer;
	size_t width;
	size_t height;
	Dithering(Dithering&);
	Dithering& operator = (Dithering&);
};

#endif
