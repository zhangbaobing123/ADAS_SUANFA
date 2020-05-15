#pragma once

#include "CNEllipseDetector.h"

class ImgaeProcessResult
{
private:
	CNEllipseDetector cned;
	Size sz;
	int F;
public:
	ImgaeProcessResult(int width, int height, int f);
	~ImgaeProcessResult();
	//void Initailize();
	void cal_distance(Mat gray, float& distance, float center_distance = 1, float radius=0.15);
};

