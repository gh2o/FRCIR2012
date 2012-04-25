#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <ctime>
#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "options.hpp"
#include "sources.hpp"
#include "server.hpp"
#include "processor.hpp"
#include "ui.hpp"

using std::string;
using std::stringstream;

int main (int argc, char *argv[])
{
	options::parse (argc, argv);
	sources::setup ();
	server::start ();
	
	{
		options::Option& opt = options::get ("snapshots");
		if (opt && !opt.value.empty ())
		{
			for (int i = 1 ; ; i++)
			{
				stringstream ss;
				ss << opt.value << "/";
				ss << std::setw (6) << std::setfill ('0');
				ss << i;
				
				string path = ss.str ();
				if (access (path.c_str (), F_OK) == 0)
					continue;
				
				int ret = mkdir (path.c_str (), 0755);
				if (ret < 0)
				{
					std::cerr << "WARNING: Unable to create snapshots subdirectory. "
						"Snapshots disabled." << std::endl;
					break;
				}
				
				processor::snapshotDirectory = path;
				break;
			}
		}
	}
	
	cv::Mat frame, binary;
	std::vector<cv::Point3f> points;
	while (true)
	{
		points.clear ();
		
		*sources::source >> frame;
		if (frame.empty ())
		{
			std::cerr << "ERROR: Frame error." << std::endl;
			struct timespec sleep;
			sleep.tv_sec = 0;
			sleep.tv_nsec = 1000000000 / 2;
			nanosleep (&sleep, NULL);
			continue;
		}
		
		processor::process (frame, points);
		
		server::send (points);
		
		int key = ui::wait ();
		if ((char) key == '\e')
			break;
	}
}
