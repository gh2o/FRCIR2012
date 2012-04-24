#include <string>
#include <iostream>
#include <vector>
#include <getopt.h>

namespace options
{
using std::string;

class Option
{
	void fill (struct option& st);

	public:
	string option;
	string argument;
	string description;
	string value;
	int flag;
	
	Option (string option="", string argument="", string description="", string value="") :
		option (option),
		argument (argument),
		description (description),
		value (value),
		flag (0) {};
	
	operator bool () { return flag; }
	
	friend void parse (int argc, char *argv[]);
	friend std::ostream& operator<< (std::ostream& out, Option& opt);
};

void help ();
void help_and_exit (int exitcode=2);
void parse (int argc, char *argv[]);
Option& get (string option);

typedef std::vector<Option> optvec;
extern optvec options;

}
