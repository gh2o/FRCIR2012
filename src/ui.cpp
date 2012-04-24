#include <tr1/unordered_map>
#include "ui.hpp"
#include "options.hpp"

namespace ui
{

static options::Option* display = NULL;

bool has_display ()
{
	if (!display)
		display = &options::get ("display");
	return display->flag;
}

void show (const string& name, Mat& frame)
{
	if (has_display ())
		cv::imshow (name, frame);
}

int wait ()
{
	if (has_display ())
		return cv::waitKey (10);
	return 0;
}

double pref (const string &trackbarname, const string& winname,
	int maximum, double multiplier, double defvalue)
{
	if (!has_display ())
		return defvalue;

	static std::tr1::unordered_map<string,int> prefs;
	
	if (prefs.find (trackbarname) == prefs.end ())
	{
		prefs[trackbarname] = 0;
		cv::namedWindow (winname);
		cv::createTrackbar (trackbarname, winname, &(prefs[trackbarname]), maximum);
		cv::setTrackbarPos (trackbarname, winname, (int)(defvalue / multiplier));
		return defvalue;
	}
	
	return prefs[trackbarname] * multiplier;
}

}
