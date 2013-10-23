#include <iostream>
#include "Parser.h"

#ifdef TESTING
#include "testMain.h"
#endif

using std::cout;
using std::endl;

int main( int argc, char **argv )
{
#ifndef TESTING
	if( argc != 2 )
	{
		cout << "Usage: " << argv[0] << " <input file>" << endl;
	}
	else
	{
		Parser parser( argv[1] );
		parser.preprocess();
		parser.parse();
	}
	return 0;
#else
	testMain( argc, argv );
	return 0;
#endif
}
