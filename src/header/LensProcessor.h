#ifndef _H_LENS_PROCESSOR
#define _H_LENS_PROCESSOR

#include <header/VideoLoader.h>
#include <header/Params.h>
#include <header/Usage.h>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "opencv2/core/cuda.hpp"
#include "opencv2/cudawarping.hpp"

using namespace cv::cuda;

class LensProcessor {
	private:
		Mat mA;
		Mat mD;
		GpuMat mUndistortMapX;
		GpuMat mUndistortMapY;

	public:
		void undistort(Mat& frame);
		LensProcessor(map<string, Mat> calibrationData, cv::Size videoSize);
		~LensProcessor();
};

#endif // _H_LENS_PROCESSOR
