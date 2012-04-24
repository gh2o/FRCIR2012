#include <cmath>
#include "util.hpp"

namespace util
{

long long micros ()
{
	struct timespec now;
	clock_gettime (CLOCK_MONOTONIC, &now);
	
	long long ret = now.tv_sec * 1000000LL +
		now.tv_nsec / 1000LL;
	return ret;
}

vector<cv::Point2f> roundedRectToPoints (cv::RotatedRect rr)
{
	typedef cv::Point2f P;
	
	vector<P> ret;
	float px = rr.center.x;
	float py = rr.center.y;
	float ox = rr.size.width / 2;
	float oy = rr.size.height / 2;
	
	double rads = rr.angle * M_PI / 180;
	double vsin = sin (rads);
	double vcos = cos (rads);
	
	#define ROT(sx,sy) (P ( \
		px + (sx) * ox * vcos - (sy) * oy * vsin, \
		py + (sy) * oy * vcos + (sx) * ox * vsin  \
	))
	ret.push_back (ROT (-1, -1));
	ret.push_back (ROT (+1, -1));
	ret.push_back (ROT (+1, +1));
	ret.push_back (ROT (-1, +1));
	#undef ROT
	
	return ret;
}

}
