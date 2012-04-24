#define _BSD_SOURCE
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <sstream>
#include <tr1/cstdint>
#include <netinet/in.h>
#include <endian.h>
#include "options.hpp"
#include "server.hpp"

namespace server
{
using std::string;
using std::stringstream;
using std::cerr;
using std::cout;
using std::endl;
using std::vector;

static int server_fd = -1;

void start ()
{
	int port;
	stringstream (options::get ("port").value) >> port;
	if (port < 1 || port > 65535)
	{
		cerr << "ERROR: Invalid port " << port << "." << endl;
		options::help_and_exit (2);
	}
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons (port);
	addr.sin_addr.s_addr = INADDR_BROADCAST;
	
	int ret = server_fd = socket (AF_INET, SOCK_DGRAM, 0);
	if (ret < 0)
	{
		cerr << "ERROR: Unable to create socket: " << strerror (errno) << endl;
		exit (1);
	}
	
	const int one = 1;
	setsockopt (server_fd, SOL_SOCKET, SO_REUSEADDR,
		&one, sizeof (one));
	setsockopt (server_fd, SOL_SOCKET, SO_BROADCAST,
		&one, sizeof (one));
	
	ret = connect (server_fd, (struct sockaddr *) &addr, sizeof (addr));
	if (ret < 0)
	{
		cerr << "ERROR: Unable to connect to port " << port <<
			": " << strerror (errno) << endl;
		exit (1);
	}
}

void send (vector<cv::Point3f> points)
{
	int chunks = points.size () * 3;
	uint64_t* dp = new uint64_t[chunks];
	
	for (unsigned i = 0; i < points.size (); i++)
	{
		cv::Point3f& pt = points[i];
		
		int o = i * 3;
		
		#define ENCODE(x) (htobe64 ((int64_t)(x * 1000.)))
		dp[o + 0] = ENCODE (pt.x);
		dp[o + 1] = ENCODE (pt.y);
		dp[o + 2] = ENCODE (pt.z);
		#undef ENCODE
	}
	
	::send (server_fd, dp, chunks * sizeof (uint64_t), 0);
}

}
