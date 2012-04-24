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
	
	protected:
	virtual void check_if_ready (std::string errmsg);
};

extern Source *source;

void setup ();

}
