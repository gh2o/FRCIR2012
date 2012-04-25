#include <sstream>
#include <opencv2/opencv.hpp>
#include "options.hpp"
#include "processor.hpp"
#include "util.hpp"
#include "ui.hpp"

namespace processor
{

class Contour
{
	void update ()
	{
		if (updated)
			return;
		updated = true;
	
		switch (points.size ())
		{
			case 1:
				_center = points[0];
				break;
			case 2:
				_center = cv::Point2f (
					(points[0].x + points[1].x) / 2.,
					(points[0].y + points[1].y) / 2.
				);
				break;
			default:
				cv::Moments moments = cv::moments (points);
				_center = cv::Point2f (
					moments.m10 / moments.m00,
					moments.m01 / moments.m00
				);
		}
		
		_area = cv::contourArea (points);
	}
	
	double _area;
	cv::Point2f _center;
	bool updated;
	
	public:
	vector<cv::Point> points;

	Contour (const vector<cv::Point>& points, bool update = true) :
		updated (false), points (points)
	{
		if (update)
			this->update ();
	}
	
	double area ()
	{
		update ();
		return _area;
	}
	
	cv::Point2f center ()
	{
		update ();
		return _center;
	}
	
	// allow sorting by area
	bool operator< (const Contour &other) const
	{
		if (!updated)
			throw util::NamedException ("contour must be updated before comparison");
		return _area < other._area;
	}
	
	// allow combination
	Contour& operator+= (const Contour& other)
	{
		points.insert (points.end (), other.points.begin (), other.points.end ());
		updated = false;
		return *this;
	}
};

static vector<cv::Point3f> createTarget ()
{
	vector<cv::Point3f> ret;
	ret.push_back (cv::Point3f (-12, -9, 0));
	ret.push_back (cv::Point3f (+12, -9, 0));
	ret.push_back (cv::Point3f (+12, +9, 0));
	ret.push_back (cv::Point3f (-12, +9, 0));
	return ret;
}

static vector<cv::Point3f> targetPoints = createTarget ();

class Processor
{
	Mat input, binary;
	Mat red, green, blue;
	Mat imask, omask, temp;
	Mat greenOverBlue, greenOverRed, brightGreen;

	void initMatrices ()
	{
		#define MATINIT(x) x.create (input.rows, input.cols, CV_8UC1)
		MATINIT (red);
		MATINIT (green);
		MATINIT (blue);
		MATINIT (imask);
		MATINIT (omask);
		MATINIT (temp);
		#undef MATINIT
	}
	
	void filterByGreen ()
	{
		cv::mixChannels (
			(Mat[]){input}, 1,
			(Mat[]){blue,green,red}, 3,
			(int[]){0,0,1,1,2,2}, 3
		);
		
		cv::compare (green, blue * ui::pref ("Blue", "Binary", 1000, 0.01, 0.90),
			greenOverBlue, cv::CMP_GT);
		cv::compare (green, red * ui::pref ("Red", "Binary", 1000, 0.01, 1.10),
			greenOverRed, cv::CMP_GT);
		cv::compare (green, 192, brightGreen, cv::CMP_GE);
		
		binary = (greenOverBlue & greenOverRed & brightGreen);
	}
	
	Mat contoursTemp;
	vector< vector<cv::Point> > contoursFound;
	vector<Contour> contours;
	
	void prepareCleanContours ()
	{
		binary.copyTo (contoursTemp);
		contours.clear ();
		contoursFound.clear ();
		
		cv::findContours (contoursTemp, contoursFound,
			CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
		
		for (unsigned i = 0; i < contoursFound.size (); i++)
		{
			vector<cv::Point>& points = contoursFound[i];
			double area = cv::contourArea (points);
			
			if (area >= ui::pref ("MinArea", "Binary", 10000, 1, 24))
				contours.push_back (Contour (points));
			else
				cv::drawContours (binary, contoursFound, i, cv::Scalar (0), CV_FILLED);
		}
		
		// sort by area
		std::sort (contours.begin (), contours.end ());
	}
	
	vector<Contour> quads;
	
	void detectQuads ()
	{
		quads.clear ();
	
		while (!contours.empty ())
		{
			// don't use reference since we're removing it anyways
			Contour contour = contours.back ();
			contours.pop_back ();
			
			// find smallest rectangle around contour
			cv::RotatedRect rr = cv::minAreaRect (contour.points);
			
			// enlarge rectangle by a bit
			double combination = ui::pref ("Combination", "Binary", 200, 0.01, 1.08);
			rr.size.width *= combination;
			rr.size.height *= combination;
			
			// convert to points
			vector<cv::Point2f> rect = util::roundedRectToPoints (rr);
			
			// eat up other contours
			for (int i = contours.size () - 1; i >= 0; i--)
			{
				Contour& other = contours[i];
				if (cv::pointPolygonTest (rect, other.center (), false) > 0) // center is inside
				{
					contour += other;
					contours.erase (contours.begin () + i);
				}
			}
			
			// get convex hull of shape
			vector<cv::Point> hull;
			cv::convexHull (contour.points, hull);
			
			// approximate the curve
			double epsilon = cv::arcLength (hull, true) *
				ui::pref ("ApproxEM", "Binary", 100, 0.01, 0.10);
			vector<cv::Point> approx;
			cv::approxPolyDP (hull, approx, epsilon, true);
			
			// skip if not quad
			if (approx.size () != 4)
				continue;
			
			// check if borders with empty middle
			//int erosions = (int) ui::pref ("ErodeD", "Binary", 20, 1, 6);
			int erosions = (int)(
				cv::arcLength (approx, true) / ui::pref ("ErodeD", "Binary", 400, 1, 68)
			);
			if (erosions == 0)
				continue;
			
			omask = cv::Scalar (0);
			cv::fillConvexPoly (omask, &(approx.front ()), approx.size (), cv::Scalar (256));
			omask.copyTo (imask);
			cv::erode (imask, imask, Mat (), cv::Point (-1, -1), erosions);
			
			int oarea = cv::countNonZero (omask);
			int iarea = cv::countNonZero (imask);
			int barea = oarea - iarea;
			
			if (iarea == 0 || oarea == 0)
				continue;
			
			temp = cv::Scalar (0);
			binary.copyTo (temp, omask);
			temp -= imask;
			
			if ((double) cv::countNonZero (temp) / barea < 0.6)
				continue;
			
			temp = cv::Scalar (0);
			cv::erode (imask, imask, Mat (), cv::Point (-1, -1), erosions / 2);
			binary.copyTo (temp, imask);
			
			if ((double) cv::countNonZero (temp) / iarea > 0.08)
				continue;
			
			// finally ...
			quads.push_back (Contour (approx));
			
			/*
			std::stringstream sso;
			sso << ((double) cv::countNonZero (temp) / iarea);
			cv::putText (temp, sso.str (), cv::Point (40, 40), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar (255));
			
			std::stringstream ss;
			ss << contour.area ();
			ui::show (ss.str (), temp);
			*/
		}
	}
	
	void rotateQuads ()
	{
		for (unsigned q = 0; q < quads.size (); q++)
		{
			Contour& quad = quads[q];
			vector<cv::Point>& pts = quad.points;
			
			double cx = quad.center ().x, cy = quad.center ().y;
			
			unsigned maxi = 0;
			double maxcos = -10; // we want v = 0, so try to max out cos(v)
			
			for (unsigned i = 0; i < 4; i++)
			{
				cv::Point& pt = pts[i];
				double x = pt.x - cx, y = pt.y - cy;
				double cos = - (x + y) / sqrt (2.0 * (x * x + y * y));
				
				if (cos > maxcos)
				{
					maxi = i;
					maxcos = cos;
				}
			}
			
			if (maxi > 0)
				std::rotate (pts.begin (), pts.begin () + maxi, pts.end ());
			
			cv::line (input, pts[0], pts[1], RGB (255, 0, 0));
			cv::line (input, pts[1], pts[2], RGB (0, 255, 0));
			cv::line (input, pts[2], pts[3], RGB (0, 0, 255));
			cv::line (input, pts[3], pts[0], RGB (0, 0, 128));
		}
	}
	
	void solvePoints (vector<cv::Point3f>& points)
	{
		double fov;
		std::stringstream (options::get ("fov").value) >> fov;
		fov *= M_PI / 180.;
		
		double focal = (double) binary.cols / (2 * tan (fov / 2));
		
		for (unsigned q = 0; q < quads.size (); q++)
		{
			Contour& quad = quads[q];
			
			vector<cv::Point2f> qpts;
			for (int i = 0; i < 4; i++)
				qpts.push_back (quad.points[i]);
			
			Mat rvec, tvec;
			cv::solvePnP (
				targetPoints, qpts,
				cv::getDefaultNewCameraMatrix (Mat::eye (3, 3, CV_64F) * focal, binary.size (), true),
				Mat (),
				rvec, tvec
			);
			
			points.push_back (cv::Point3f (
				tvec.at<double> (0),
				-tvec.at<double> (1),
				tvec.at<double> (2)
			));
		}
	}

	public:
	void process (Mat& frame, vector<cv::Point3f>& points)
	{
		input = frame;
	
		initMatrices ();
		filterByGreen ();
		prepareCleanContours ();
		detectQuads ();
		rotateQuads ();
		solvePoints (points);
		
		ui::show ("Binary", binary);
	}
};

Processor processor;

void process (Mat &frame, vector<cv::Point3f>& points)
{
	processor.process (frame, points);
}

}
