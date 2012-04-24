#include <string>
#include <vector>
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
		processor::process (frame, points);
		
		server::send (points);
		
		ui::show ("Input", frame);
		
		int key = ui::wait ();
		if ((char) key == '\e')
			break;
	}
}
