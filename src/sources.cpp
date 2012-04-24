#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <opencv2/opencv.hpp>
#include "options.hpp"
#include "sources.hpp"
#include "util.hpp"

namespace sources
{
using std::string;
using std::stringstream;
using std::cout;
using std::cerr;
using std::endl;
using cv::Mat;

void Source::check_if_ready (string errmsg)
{
	Mat frame;
	*this >> frame;
	
	ready = !frame.empty ();
	if (!ready)
		cerr << errmsg << endl;
}

class WebcamSource : public Source
{
	cv::VideoCapture cap;

	public:
	WebcamSource ()
	{
		int device;
		stringstream (options::get ("webcam").value) >> device;
		if (device < 0 || device > 99)
		{
			cerr << "ERROR: Webcam number must be between 0-99 inclusive." << endl;
			return;
		}
	
		cap.open (device);
		
		if (!cap.isOpened ())
		{
			cerr << "ERROR: Unable to open webcam device." << endl;
			return;
		}
		
		check_if_ready ("ERROR: Unable to fetch image from webcam device.");
	}
	
	~WebcamSource ()
	{
		cap.release ();
	}
	
	void operator>> (Mat& image)
	{
		cap >> image;
	}
};

class AxisSource : public Source
{
	string url;
	string temppath;

	public:
	AxisSource ()
	{
		// get axis options
		string ip = options::get ("axis").value;
		int quality;
		stringstream (options::get ("axisquality").value) >> quality;
		int compression = 100 - quality;
		
		// validate options
		for (unsigned i = 0; i < ip.size (); i++)
		{
			char c = ip[i];
			if (
				(c >= 'A' && c <= 'Z') ||
				(c >= 'a' && c <= 'z') ||
				(c >= '0' && c <= '9') ||
				c == '-' || c == '.'
			)
				continue;
			
			cerr << "ERROR: Invalid Axis hostname: " << ip << endl;
			return;
		}
		
		if (quality < 0 || quality > 100)
		{
			cerr << "ERROR: Quality must be between 0-100 inclusive." << endl;
			return;
		}
		
		// build a url
		{
			stringstream urlss;
			urlss << "http://" << ip << "/axis-cgi/jpg/image.cgi"
				"?resolution=640x480&compression=" << compression;
			url = urlss.str ();
		}
		
		// create a temporary file
		{
			char ctmppath[] = "/tmp/irdXXXXXX.jpg";
			int fd = mkstemps (ctmppath, 4);
			if (fd >= 0)
			{
				close (fd);
				temppath = ctmppath;
			}
			else
			{
				cerr << "ERROR: Unable to create temporary file." << endl;
				return;
			}
		}
		
		// check if we can grab a frame from the camera
		check_if_ready ("ERROR: Unable to prepare video source "
			"from Axis camera.");
	}
	
	void operator>> (Mat& image)
	{
		pid_t pid = fork ();
		
		if (pid == 0) // child
		{
			for (int fd = 3; fd < 1024; fd++)
				close (fd);
			execlp ("wget", "wget", "-q",
				"-t", "2", "-T", "5",
				"-O", temppath.c_str (), url.c_str (), NULL);
			cerr << "ERROR: Unable to invoke wget: " << strerror (errno) << endl;
			exit (1);
		}
		
		int status;
		waitpid (pid, &status, 0);
		
		if (
			!WIFEXITED (status) // abnormal exit
			||
			WEXITSTATUS (status) != 0 // error occurred
		)
		{
			cerr << "ERROR: Unable to fetch next frame from camera." << endl;
			return;
		}
		
		image = cv::imread (temppath);
	}
};

class StaticSource : public Source
{
	Mat orig;
	long long us_last;
	
	public:
	StaticSource ()
	{
		string filename = options::get ("static").value;
		orig = cv::imread (filename);
		
		check_if_ready ("ERROR: Image " + filename +
			" does not exist or cannot be read.");
		
		us_last = 0;
	}
	
	void operator>> (Mat& image)
	{
		// limit to 30 fps
		const int us_per_frame = 1000000 / 30;
		while (true)
		{
			long long us_now = util::micros ();
			long long us_next = us_last + us_per_frame;
			long long us_remaining = us_next - us_now;
			
			if (us_remaining <= 0)
			{
				if (us_next < us_now - us_per_frame) // lagging behind
					us_last = us_now; // catch up
				else
					us_last = us_next;
				break;
			}
			
			struct timespec sleep;
			sleep.tv_sec = 0;
			sleep.tv_nsec = us_remaining * 1000;
			nanosleep (&sleep, NULL);
		}
		
		orig.copyTo (image);
	}
};

Source *source = NULL;
static string source_names[] = {"webcam", "axis", "static"};
static int source_names_length = sizeof (source_names) / sizeof (source_names[0]);

void setup ()
{
	string source_name;
	for (int i = 0; i < source_names_length; i++)
	{
		string tmp = source_names[i];
		if (!options::get (tmp))
			continue;
		
		if (!source_name.empty ())
		{
			source_name = "";
			break;
		}
		
		source_name = tmp;
	}
	
	if (source_name.empty ())
	{
		cerr << "ERROR: Exactly one source must be specified." << endl;
		options::help_and_exit (2);
	}
	
	if (source_name == "webcam")
		source = new WebcamSource ();
	else if (source_name == "axis")
		source = new AxisSource ();
	else if (source_name == "static")
		source = new StaticSource ();
	
	if (!source->ready)
	{
		cerr << "ERROR: Unable to prepare video source." << endl;
		exit (1);
	}
}

}