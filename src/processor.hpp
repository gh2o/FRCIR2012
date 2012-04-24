#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

namespace processor
{
using cv::Mat;
using std::string;
using std::vector;

void process (Mat &frame, vector<cv::Point3f>& points);
}
