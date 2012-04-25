#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>
#include <cstring>
#include <cstdlib>
#include <getopt.h>
#include "options.hpp"
#include "util.hpp"

namespace options
{
using std::cout;
using std::endl;
using std::map;

static optvec getoptions ()
{
	optvec vec;
	vec.push_back (Option ("help", "", "this help page"));
	vec.push_back (Option ("port", "PORT", "server port (default = 2228)", "2228"));
	vec.push_back (Option ("display", "", "display output locally"));
	vec.push_back (Option ("webcam", "?NUMBER", "use webcam as video source", "0"));
	vec.push_back (Option ("axis", "?IP", "use Axis camera as video source (default = 10.41.59.11)",
		"10.41.59.11"));
	vec.push_back (Option ("static", "IMAGE", "use a static image as video source"));
	vec.push_back (Option ("axisquality", "QUALITY", "image quality of Axis camera (default = 80)",
		"80"));
	vec.push_back (Option ("fov", "FOV", "horizontal angle of view of camera (default = 47)", "47"));
	vec.push_back (Option ("snapshots", "DIR", "turn on snapshots, save them in this directory"));
	return vec;
}

optvec options = getoptions ();

void Option::fill (struct option& st)
{
	st.name = option.c_str ();
	st.flag = &flag;
	st.val = 1;
	
	if (argument.empty ())
		st.has_arg = 0;
	else if (argument[0] == '?')
		st.has_arg = 2;
	else
		st.has_arg = 1;
}

std::ostream& operator<< (std::ostream& out, Option& opt)
{
	using namespace std;
	
	out << setfill (' ') << setw (4) << right;
	out << "--";
	
	stringstream option;
	option << opt.option;
	if (!opt.argument.empty ())
	{
		if (opt.argument[0] == '?')
			option << "[=" << opt.argument.substr (1) << "]";
		else
			option << "=" << opt.argument;
	}
	
	out << setfill (' ') << setw (20) << left;
	out << option.str ();
	
	out << " " << opt.description;
	
	return out;
}

static string progname;

void help_and_exit (int exitcode)
{
	help ();
	exit (exitcode);
}

void help ()
{
	cout << ("Usage: " + progname + " [OPTION]...") << endl;
	optvec::iterator it;
	for (it = options.begin (); it != options.end (); it++)
		cout << *it << endl;
}

void parse (int argc, char *argv[])
{
	progname = argv[0];
	struct option *getopt_options = new struct option[options.size () + 1];
	
	int i = 0;
	optvec::iterator it;
	for (it = options.begin (); it != options.end (); it++)
		it->fill (getopt_options[i++]);
	
	// fill last element with zeros
	memset (&(getopt_options[i]), 0, sizeof (getopt_options[i]));
	
	int getopt_value, getopt_index, needs_help = -1;
	while ((getopt_value = getopt_long (argc, argv, "",
		getopt_options, &getopt_index)) != -1)
	{
		if (getopt_value == '?') // unknown or invalid parameter
		{
			needs_help = 2;
			break;
		}
		
		Option& opt = get (getopt_options[getopt_index].name);
		if (opt.option == "help")
		{
			needs_help = 0;
			break;
		}
		
		if (optarg)
			opt.value = optarg;
	}
	
	delete[] getopt_options;
	
	if (needs_help >= 0)
		help_and_exit (needs_help);
}

Option& get (string option)
{
	optvec::iterator it;
	for (it = options.begin (); it != options.end (); it++)
		if (it->option == option)
			return *it;
	throw util::NamedException ("undefined option: " + option);
}

}
