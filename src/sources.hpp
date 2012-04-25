#include <iostream>
#include <opencv2/opencv.hpp>

namespace sources
{

class Source
{
	public:
	bool ready;
	
	Source () : ready (false) {}
	virtual ~Source () {};
	
	virtual void operator>> (cv::Mat& image) = 0;
};

extern Source *source;

void setup ();

}
