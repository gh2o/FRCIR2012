#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <opencv2/opencv.hpp>
#include "options.hpp"
#include "sources.hpp"
#include "server.hpp"
#include "processor.hpp"
#include "ui.hpp"

int main (int argc, char *argv[])
{
	options::parse (argc, argv);
	sources::setup ();
	server::start ();
	
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
		
		ui::show ("Input", frame);
		
		int key = ui::wait ();
		if ((char) key == '\e')
			break;
	}
}
