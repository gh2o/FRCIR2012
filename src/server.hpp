#include <vector>
#include <opencv2/opencv.hpp>

namespace server
{

void start ();
void send (std::vector<cv::Point3f> points);

}
