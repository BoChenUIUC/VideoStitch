#include <opencv/cv.h>
#include <opencv/cvaux.h>
#include <opencv/highgui.h>
#include "VideoVolumeAnalyzer.h"

using std::cerr;
using std::endl;
using namespace cv;

VideoVolumeAnalyzer::VideoVolumeAnalyzer() {
    fgDetector = new ImageDifference();
    mFrameCounter = 0;
}

VideoVolumeAnalyzer::~VideoVolumeAnalyzer() {
    // Clean up
    delete fgDetector;
    fgDetector = NULL;

    list<FeatureTracker*>::iterator iter;
    for ( iter = activeTrackers.begin(); iter != activeTrackers.end(); iter++ ) {
        delete *iter;
        *iter = NULL;
    } for ( iter = deadTrackers.begin(); iter != deadTrackers.end(); iter++ ) {
        delete *iter;
        *iter = NULL;
    }
}

void VideoVolumeAnalyzer::deleteTrackers() {
    list<FeatureTracker*>::iterator iter;
    for ( iter = deadTrackers.begin(); iter != deadTrackers.end(); iter++ ) {
        delete *iter;
        *iter = NULL;
    }
    deadTrackers.clear();   
}

void VideoVolumeAnalyzer::trackFeatures(Rect &boundary) {
    deleteTrackers();

    if (activeTrackers.size() > 0) {
        vector<Point2f> points;
        vector<Point2f> matchedPoints, backMatchPoints;
        vector<uchar> matchStatus, backMatchStatus;
        vector<float> err;

        TermCriteria termcrit(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 10, 0.03);
        Size winSize(20, 20);

        // Include the last positions of registered point trackers to compute optical flow
        list<FeatureTracker*>::iterator iter;
        for ( iter = activeTrackers.begin(); iter != activeTrackers.end(); iter++ ) 
            points.push_back((*iter)->getLastPoint());

        // forward matching
        calcOpticalFlowPyrLK(prevGray, currGray, points, matchedPoints, matchStatus, err,
                             winSize, 3, termcrit, 0, 0.001);

        // backward matching
        calcOpticalFlowPyrLK(currGray, prevGray, matchedPoints, backMatchPoints, backMatchStatus, err,
                             winSize, 3, termcrit, 0, 0.001);

        // check distance
        int offset = 0;
        for ( iter = activeTrackers.begin(); iter != activeTrackers.end(); iter++ ) {
            if (matchStatus[offset] && backMatchStatus[offset]) {
                Vec2f backMatchDisp = points[offset] - backMatchPoints[offset];
                double d = norm(backMatchDisp);

                if (d < 1 && boundary.contains(matchedPoints[offset]))
                    (*iter)->move(matchedPoints[offset]);
                else {
                    deadTrackers.push_back(*iter);  // copy inactive tracker to another container
                    *iter = NULL;
                }
            } else {
                deadTrackers.push_back(*iter);      // copy inactive tracker to another container
                *iter = NULL;
            }

            offset++;
        }
        activeTrackers.remove(NULL);
    }

    //  init new trackers by finding new feature points periodically
    if ( mFrameCounter % 2 == 0 ) {
        const int maxCorner = 3000;
        const double qualityLevel = 0.01;
        const double minDistance = 5;
        vector<Point> features;
        std::vector<KeyPoint> keypoints;

        Mat enhanced;
        equalizeHist(currGray, enhanced);

        int minHessian = 200;
        Ptr<SURF> detector = SURF::create( minHessian ) ;

        if (!fgMask.empty()) {
            // mask out neighborhood of existing trackers
            list<FeatureTracker*>::iterator iter;
            for ( iter = activeTrackers.begin(); iter != activeTrackers.end(); iter++ ) {
                circle(fgMask, (*iter)->getLastPoint(), 10, Scalar(), -1);
            }
            goodFeaturesToTrack(enhanced, features, maxCorner, qualityLevel, minDistance, fgMask, 3, false);
            detector->detect(enhanced, keypoints, fgMask);
        } else {
            goodFeaturesToTrack(enhanced, features, maxCorner, qualityLevel, minDistance, 3, false);
            detector->detect(enhanced, keypoints);
        }

        detector.release();

        // register new trackers
        for (size_t i = 0; i < features.size(); i++) {
            FeatureTracker *t = new FeatureTracker(features[i]);
            activeTrackers.push_back(t);
        }
        for (size_t i =0; i < keypoints.size(); i++) {
            FeatureTracker *t = new FeatureTracker(keypoints[i].pt);
            activeTrackers.push_back(t);
        }
    }

    if (mFrameCounter % 30 == 0) {
        cerr << "[DEBUG] tracking feature # " << activeTrackers.size() << endl;
    }

    mFrameCounter++;
}

void VideoVolumeAnalyzer::process(Mat &frame) {
    Rect frameBoundary(0, 0, frame.cols, frame.rows);

    try {
        image = frame;
        cvtColor(frame, currGray, CV_BGR2GRAY);

        fgMask = Mat::ones(frame.rows, frame.cols, CV_8UC1);

        trackFeatures(frameBoundary);

        swap(currGray, prevGray);

    } catch ( cv::Exception &e ) {
        const char *errMsg = e.what();
        cerr << "[ERROR] process(): " << errMsg << endl;
    }
}

void VideoVolumeAnalyzer::draw() {
    list<FeatureTracker*>::iterator iter;
    for (iter = activeTrackers.begin(); iter != activeTrackers.end(); iter++)
        (*iter)->draw(image, (*iter)->getColor());

    imshow("Analyzer", image);
    //imwrite("tracker.png", image);
}

Mat& VideoVolumeAnalyzer::getImage() {
    return image;
}

list<FeatureTracker*> VideoVolumeAnalyzer::getActiveTrackers() {
    list<FeatureTracker*> rtnList;
    list<FeatureTracker*>::iterator iter;
    for (iter = activeTrackers.begin(); iter != activeTrackers.end(); iter++)
        if( (*iter)->getPoints().size() >= 2 || mFrameCounter < 2)
            rtnList.push_back( (*iter) );

    return rtnList;
}
