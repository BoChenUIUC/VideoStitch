#include <header/MappingProjector.h>

void MappingProjector::setCameraParams(vector<struct MutualProjectParam> params, double focalLength) {
	// Calculate R by eular angles
	for (int v=0; v<mViewCount; v++) {
		Mat r = Mat::zeros(3, 3, CV_32F);
		struct MutualProjectParam vP = params[v];
		double alpha = vP.y;
		double beta = vP.p;
		double gamma = vP.r;

		// Take camera as reference coordinate system, around: x-axis -> pitch, y-axis -> yaw, z->axis -> roll
		Mat Rz = getZMatrix(gamma);
		Mat Ry = getYMatrix(alpha);
		Mat Rx = getXMatrix(beta);
		r = Ry * Rx * Rz;
		mR.push_back( r );
	}

	// Calculate mK
	mK.resize(mViewCount);
	for (int v=0; v<mViewCount; v++) {
		mK[v] = Mat::zeros(3, 3, CV_32F);
		mK[v].at<float>(0, 0) = static_cast<float> ( focalLength );
		mK[v].at<float>(1, 1) = static_cast<float> ( focalLength );
		mK[v].at<float>(0, 2) = static_cast<float> ( mViewSize.width/2 );
		mK[v].at<float>(1, 2) = static_cast<float> ( mViewSize.height/2 );
		mK[v].at<float>(2, 2) = 1.f;
	}
}

void MappingProjector::setCameraParams(vector<Mat> Rs, vector<Mat> Ks) {
	if ( !Rs.empty() )
		mR = Rs;
	if ( !Ks.empty() ) 
		mK = Ks;
}

void MappingProjector::calcProjectionMatrix() {
	// Initialize warpers
	setupWarpers();
	logMsg(LOG_INFO, "=== Constructing UV checkup table ===");
	constructUVcheckupTable();
	logMsg(LOG_INFO, "=== Interpolate UV checkup table ===");
	interpolateUVcheckupTable();
	logMsg(LOG_INFO, "=== Done projection matrix calculation ===");
}

void MappingProjector::setupWarpers() {
	for (int v=0; v<mViewCount; v++) {
		// Use focal length as scale
		float scale = (mK[v].at<float>(0, 0) + mK[v].at<float>(1, 1))/2 ;
		mSphericalWarpers.push_back( shared_ptr<PROJECT_METHOD>( new PROJECT_METHOD( 1.f ) ) );
	}
}

void MappingProjector::buildMapsForViews() {
	for (int v=0; v<mViewCount; v++) {
		Mat uxmap, uymap;
		mMapROIs.push_back( mSphericalWarpers[v]->buildMaps(mViewSize, mK[v], mR[v], uxmap, uymap) );
		mUxMaps.push_back( uxmap );
		mUyMaps.push_back( uymap );
	}
}

void MappingProjector::updateCurrentCanvasROI() {
	// Calculate canvas ROI
	mCanvasROI = mMapROIs[0];
	for (int v=1; v<mViewCount; v++)
		mCanvasROI = mCanvasROI | mMapROIs[v];
	mCanvasROI += Size(1, 1);

	for (int v=0; v<mViewCount; v++) 
		mMapROIs[v] = Rect( mMapROIs[v].x - mCanvasROI.x, mMapROIs[v].y - mCanvasROI.y, mMapROIs[v].width+1, mMapROIs[v].height+1 );
}

void MappingProjector::renderInterestArea(Mat& outImg, vector<Mat> frames, Point2f center, float renderRange) {
	/** 
		Render area from tl(u1, v1) to br(u2, v2) 
		Using inverseMap
		u : [-PI, PI]
		v : [0, PI]
	*/
	boost::timer::cpu_timer boostTimer;

	outImg = Mat(OUTPUT_WINDOW_HEIGHT, OUTPUT_WINDOW_WIDTH, CV_8UC3);

	tuneToMap(center);

	for (int y = 0; y < OUTPUT_WINDOW_HEIGHT; y++) {
		for (int x = 0; x < OUTPUT_WINDOW_WIDTH; x++) {
			Point2f newPnt;
			getUVbyAzimuthal( (x - OUTPUT_WINDOW_WIDTH/2.f) / (OUTPUT_WINDOW_WIDTH/2.f), (y - OUTPUT_WINDOW_HEIGHT/2.f) / (OUTPUT_WINDOW_HEIGHT/2.f), center, newPnt);
			tuneToMap(newPnt);

			vector<Vec3b> pixels = getPixelsValueByUV( newPnt.x, newPnt.y, frames );
			if (pixels.size() > 0)
				outImg.at<Vec3b>(y, x) = pixels[0];
			else
				outImg.at<Vec3b>(y, x) = Vec3b(0, 0, 0);
		}
	}
	
	boostTimer.stop();
	mExecTimes.push_back( stod(boostTimer.format(3, "%w")) );
    mFrameProcessed++;
}

void MappingProjector::tuneToMap(Point2f& p) {
	while (p.x < -M_PI)
		p.x += 2*M_PI;
	while (p.x > M_PI)
		p.x -= 2*M_PI;
	while (p.y < 0)
		p.y += M_PI;
	while (p.y > M_PI) 
		p.y -= M_PI;
}

void MappingProjector::getUVbyAzimuthal(const float xOffset, const float yOffset, const Point2f center, Point2f& newPnt) {
	float phi0 = center.y - M_PI/2.f;
	float lambda0 = center.x + M_PI;

	if (xOffset == 0.f && yOffset == 0.f) {
		newPnt.x = center.x;
		newPnt.y = center.y;
		return;
	}

	float c = sqrt(xOffset * xOffset + yOffset * yOffset);
	float cosc = cos(c);
	float sinc = sqrt(1-cosc*cosc);
	float sinPhi0 = sin(phi0);
	float cosPhi0 = sqrt(1-sinPhi0*sinPhi0);
	float phi = asin( cosc * sinPhi0 + yOffset * sinc * cosPhi0 / c);
	float lambda;

	if (phi0 == M_PI/2)
		lambda = lambda0 + atan2(-yOffset, xOffset);
	else if (phi0 == -M_PI)
		lambda = lambda0 + atan2(yOffset, xOffset);
	else
		lambda = lambda0 + atan2( xOffset * sinc, c * cosPhi0 * cosc - yOffset * sinPhi0 * sinc );

	newPnt.x = lambda - M_PI;
	newPnt.y = phi + M_PI/2.f;
	//logMsg(LOG_DEBUG, stringFormat("c = %f, cosc = %f, sinc = %f, sinPhi0 = %f, cosPhi0 = %f, m = %f", c, cosc, sinc, sinPhi0, cosPhi0, cosc * sinPhi0 + yOffset * sinc * cosPhi0 / c) );
	//logMsg(LOG_DEBUG, stringFormat("(x,y) = (%f, %f), center = (%f, %f), (l, p) = (%f, %f)", xOffset, yOffset, center.x, center.y, newPnt.x, newPnt.y) );
}

void MappingProjector::defineWindowSize() {
	mOutputWindowSize = Size(OUTPUT_WINDOW_WIDTH, OUTPUT_WINDOW_HEIGHT);
}

Size MappingProjector::getOutputVideoSize() {
#ifdef OUTPUT_PANO
	return Size(OUTPUT_PANO_WIDTH, OUTPUT_PANO_HEIGHT);
#else
	return mOutputWindowSize;
#endif
	//return Size(OUTPUT_PANO_WIDTH, OUTPUT_PANO_HEIGHT);
}

void MappingProjector::checkFPS() {
	double total = 0.f;
	for (unsigned int i=0; i<mExecTimes.size(); i++) 
		total += mExecTimes[i];
	logMsg(LOG_INFO, stringFormat( "=== Average FPS is %lf === ", mExecTimes.size() / total ) );
}

MappingProjector::MappingProjector(int viewCount, Size viewSize) : 
	mFrameProcessed(0),
	mViewCount(viewCount),
	mViewSize(viewSize) {
		defineWindowSize();
}

void MappingProjector::constructUVcheckupTable() {
	/** Initialize */
	for (int y=0; y<OUTPUT_PANO_HEIGHT; y++) {
		vector<Mat> weightMatRow;
		for (int x=0; x<OUTPUT_PANO_WIDTH; x++) {
			Mat weightMat = Mat::zeros(mViewCount, 3, CV_32S);
			weightMatRow.push_back(weightMat);
		}
		mProjMap.push_back(weightMatRow);
	}

	#pragma omp parallel for collapse(3)
	for (int y=0; y<mViewSize.height; y++) {
		for (int x=0; x<mViewSize.width; x++) {
			for (int v=0; v<mViewCount; v++) {
				Point2f mapPnt = mSphericalWarpers[v]->warpPoint( Point2f(x, y), mK[v], mR[v]);
				// mapPnt: x = (-pi ~ pi), y = (0 ~ pi)
				int oX = static_cast<int> ( (mapPnt.x - (-M_PI)) * OUTPUT_PANO_WIDTH / (2 * M_PI) ) % OUTPUT_PANO_WIDTH;
				int oY = static_cast<int> ( mapPnt.y * OUTPUT_PANO_HEIGHT / (M_PI) )  % OUTPUT_PANO_HEIGHT;

				Mat weightMat = mProjMap[oY][oX];
				weightMat.at<int>(v, 0) = 1;
				weightMat.at<int>(v, 1) = x;
				weightMat.at<int>(v, 2) = y;
				mProjMap[oY][oX] = weightMat;
			}
		}
	}
}

void MappingProjector::interpolateUVcheckupTable() {
	for (int y=0; y<OUTPUT_PANO_HEIGHT; y++) {
		for (int x=0; x<OUTPUT_PANO_WIDTH; x++) {
			int totalWeight = 0;
			for (int v=0; v<mViewCount; v++) {
				if (mProjMap[y][x].at<int>(v, 0) != 0)
					totalWeight += 1;
			}

			if (totalWeight == 0) {
				if (x > 0)
					mProjMap[y][x] = mProjMap[y][x-1];
				else if (y > 0)
					mProjMap[y][x] = mProjMap[y-1][x];
				else 
					logMsg(LOG_WARNING, stringFormat("Cannot handle map (%d, %d)", x, y));
			}

		}
	}
}

void MappingProjector::projectOnCanvas(Mat& canvas, vector<Mat> frames) {
	/*
	GpuMat newFrame;
	GpuMat oriFrame(frame);
	cv::cuda::warpAffine(oriFrame, newFrame, mProjMat[vIdx], Size(frame.cols, frame.rows));
	*/
	boost::timer::cpu_timer boostTimer;

	canvas = Mat(OUTPUT_PANO_HEIGHT, OUTPUT_PANO_WIDTH, CV_8UC3);
	for (int y=0; y<canvas.rows; y++) {
		for (int x=0; x<canvas.cols; x++) {
			int totalWeight = 0;
			Mat pixelMat = mProjMap[y][x];
			Vec3b pixel = Vec3b(0, 0, 0);
			// Get weight
			for (int v=0; v<mViewCount; v++) {
				if (pixelMat.at<int>(v, 0) != 0)
					totalWeight += 1;
			}

			for (int v=0; v<mViewCount; v++) {
				if (pixelMat.at<int>(v, 0) == 0)
					continue;
				int px = pixelMat.at<int>(v, 1) ;
				int py = pixelMat.at<int>(v, 2) ;
				pixel += (static_cast<float> (pixelMat.at<int>(v, 0)) / totalWeight) * frames[v].at<Vec3b>(py, px) ;
			}
			canvas.at<Vec3b>(y, x) = pixel;
		}
	}

	boostTimer.stop();
	mExecTimes.push_back( stod(boostTimer.format(3, "%w")) );
    mFrameProcessed++;
}

vector<Vec3b> MappingProjector::getPixelsValueByUV(float u, float v, vector<Mat> frames) {
	vector<Vec3b> outputPixels;

	int checkupX = static_cast<int>( (u + M_PI) * OUTPUT_PANO_WIDTH / (2*M_PI) );
	int checkupY = static_cast<int>( v * OUTPUT_PANO_HEIGHT / M_PI );

	Mat pixelMat = mProjMap[checkupY][checkupX];

	for (int view=0; view<mViewCount; view++) {
		if (pixelMat.at<int>(view, 0) == 0)
			continue;
		int px = pixelMat.at<int>(view, 1);
		int py = pixelMat.at<int>(view, 2);
		outputPixels.push_back( frames[view].at<Vec3b>(py, px) );
	}
	
	return outputPixels;
}

int MappingProjector::rad2Deg(float r) {
	return static_cast<int> ( r * 180 / M_PI );
}

float MappingProjector::deg2Rad(int d) {
	return d * M_PI / 180 ;
}