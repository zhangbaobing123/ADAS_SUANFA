#include "ImgaeProcessResult.h"


ImgaeProcessResult::ImgaeProcessResult(int width, int height,int f)
{
	sz=Size(width, height);
	F = f;
	int		iThLength = 20;//过滤太短的弧
	float	fThObb = 2;
	float	fThPos = 1.0f;
	//float	fTaoCenters = 0.018f;//0.05
	int 	iNs = 16;//弦数
	float	fMaxCenterDistance = sqrt(float(sz.width*sz.width + sz.height*sz.height)) * 0.005;

	//float	fThScoreScore = 0.5f;//0.8	
	//fTaoCenters = 0.05f;
	// Other constant parameters settings.
	// Gaussian filter parameters, in pre-processing
	Size	szPreProcessingGaussKernelSize = Size(3, 3);
	double	dPreProcessingGaussSigma = 1.0;

	float	fDistanceToEllipseContour = 0.1f;	// (Sect. 3.3.1 - Validation)
	//float	fMinReliability					= 0.4f;	// Const parameters to discard bad ellipses 0.5

	// Initialize Detector with selected parameters
	cned.SetParameters(szPreProcessingGaussKernelSize,
		dPreProcessingGaussSigma,
		fThPos,
		fMaxCenterDistance,
		iThLength,
		fThObb,
		fDistanceToEllipseContour,
		0.2,
		0.2,
		iNs
		);
}


ImgaeProcessResult::~ImgaeProcessResult()
{
}

void ImgaeProcessResult::cal_distance(Mat gray, float& distance, float center_distance, float radius)
{
	Mat canny_img;
	Canny(gray, canny_img, 30, 150);
	//CNEllipseDetector cned;


	// Parameters Settings (Sect. 4.2)

	// Detect 
	Mat1b gray_clone = canny_img.clone();
	vector<Ellipsed> ellsCned_ori;
	cned.Detect(gray_clone, ellsCned_ori);

	vector<Ellipsed> ellsCned;
	for (int i = 0; i < ellsCned_ori.size(); ++i){
		if ((ellsCned_ori[i]._a<80) && (ellsCned_ori[i]._b / ellsCned_ori[i]._a)>0.25)
		{
			ellsCned.push_back(ellsCned_ori[i]);
		}
	}

	//显示部分，方便调试实际使用时去掉。
	/*Mat show(canny_img.rows, canny_img.cols,CV_8U,Scalar(255));
	cned.DrawDetectedEllipses(show, ellsCned);

	imshow("gray", gray);
	imshow("show", show);
	waitKey(1);*/

	//ell_centers存放上下两个椭圆的x和y，
	//如ell_centers[0]和ell_centers[1]存放上面所有椭圆的x和y，ell_centers[2]和ell_centers[3]存放下面所有椭圆的x和y
	vector<vector<int>> ell_centers(4);
	int first_x = 0;
	int first_y = 0;
	float threshold2 = center_distance / radius; //实际圆心距与半径之比
	float threshold3 = 0;
	//距离阈值，如果两个椭圆的中心点y的差值大于100，则两个椭圆不属于同一区域的椭圆
	int just_thres;
	int just_thres_x;
	int num = int(ellsCned.size());
	//如果没有椭圆，返回退出
	if (num == 0)
		return;
	else
	{
		Ellipsed& e = ellsCned[0];
		//以第一个椭圆为基准
		first_x = cvRound(e._xc);
		first_y = cvRound(e._yc);
		/*just_thres = (cvRound(e._a) + cvRound(e._b)) / 2;
		just_thres_x = just_thres / 2;*/

		ell_centers[0].push_back(cvRound(e._xc));
		ell_centers[1].push_back(cvRound(e._yc));

	}
	/*vector<Ellipsed> ellsCned2;*/
	for (int i = 0; i < num; ++i)
	{
		Ellipsed& e = ellsCned[i];
		just_thres = (cvRound(e._a) + cvRound(e._b)) / 2; //计算每个圆半径
		float threshold3 = (float)(pow((cvRound(e._xc) - first_x), 2) + pow((cvRound(e._yc) - first_y), 2));
		float threshold4 = sqrt(threshold3) / just_thres; //圆心距与半径之比
		/*if (1.5*threshold2 >= threshold4 && threshold4 >= 0.5*threshold2 || threshold4 <= 0.1*threshold2)
		{
		ellsCned2.push_back(ellsCned[i]);
		}

		Mat show1(canny_img.rows, canny_img.cols,CV_8U,Scalar(255));
		Mat show(canny_img.rows, canny_img.cols,CV_8U,Scalar(255));
		cned.DrawDetectedEllipses(show1, ellsCned);
		cned.DrawDetectedEllipses(show, ellsCned2);

		imshow("gray", gray);
		imshow("show1", show1);
		imshow("show", show);
		waitKey(1);*/

		//根据阈值判断是否属于同一个区域的椭圆
		if (1.5*threshold2 >= threshold4 && threshold4 >= 0.5*threshold2)
		{
			ell_centers[2].push_back(cvRound(e._xc));
			ell_centers[3].push_back(cvRound(e._yc));
		}
		if (threshold4 <= 0.1*threshold2)
		{
			ell_centers[0].push_back(cvRound(e._xc));
			ell_centers[1].push_back(cvRound(e._yc));
		}

	}
	//如果只有一个区域存在椭圆，返回。
	if (ell_centers[2].size() == 0)
		return;
	//mean中存放的是上下两个区域椭圆区域的平均中心坐标
	//如mean[0]为上面椭圆区域的x均值，mean[1]为上面椭圆区域的y均值，mean[2]为下面椭圆区域的x均值，mean[3]为下面椭圆区域的y均值
	int mean[4];
	for (int i = 0; i < 4; i++)
	{
		int sums = std::accumulate(std::begin(ell_centers[i]), std::end(ell_centers[i]), 0.0);
		mean[i] = sums / ell_centers[i].size();
	}
	//计算距离
	float dis_2 = (float)(pow((mean[0] - mean[2]), 2) + pow((mean[1] - mean[3]), 2));
	//distance = sqrt(dis_2);


	//显示部分，方便调试实际使用时去掉。

	//需添加：******************************************************************
	//1、解决一个圆多重圆问题
	//2、计算两圆心之间的像素距离。
	float center_pix_distance = sqrt(dis_2);//虚拟表示，两个圆心之间的像素距离。
	//需添加：******************************************************************

	distance = center_distance*F / center_pix_distance;
}
