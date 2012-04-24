#include <string>
#include <exception>
#include <vector>
#include <opencv2/opencv.hpp>

#define RGB(r,g,b) cv::Scalar(b,g,r)

namespace util
{
using std::string;
using std::vector;

class NamedException : public std::exception
{
	string reason;

	public:
	NamedException (string rs) throw () : reason (rs) {}
	~NamedException () throw () {}
	virtual const char* what () const throw () { return reason.c_str (); }
};

long long micros ();
vector<cv::Point2f> roundedRectToPoints (cv::RotatedRect rr);

}
