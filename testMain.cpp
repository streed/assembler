#include "testMain.h"
#ifdef TESTING
using std::cout;
using std::endl;

void testMain( int argc, char **argv )
{
	testList( argc, argv );
	testParser( argc, argv );
}

void testList( int argc, char **argv )
{
	cout << "Tests for the list class..." << endl;

	cout << "Test list insert." << endl;
	testListInsert();
	cout << "Test list get using []'s." << endl;
	testListGet();
	cout << "Test the replace method of the list." << endl;
	testListReplace();
	cout << "Test the addUnique method." << endl;
	testListAddUnique();

	cout << "All Tests Passed." << endl;
}

void testParser( int argc, char **argv )
{
	cout << "Tests for the parser class..." << endl;

	cout << "Test 'in' opcode" << endl;
	testParserIN();
	cout << "Test 'out' opcode" << endl;
	testParserOUT();
	cout << "Test 'beq' opcode" << endl;
	testParserBEQ();
	cout << "Test parsing out label" << endl;
	testParserLabel();
	
	cout << "Test single register replacement and substitution for 'in'" << endl;
	testParserSingleRegisterReplacementINX();
	cout << "Test single register replacement and substitution for 'out'" << endl;
	testParserSingleRegisterReplacementOUTX();
	cout << "Test two register replacement and substitution" << endl;
	testParserTwoRegisterReplacementX();
	cout << "Test three register replacement and substitution, with one variable." << endl;
	testParserVariableRegisterReplacementX();
	cout << "Test three register replacement and substitution, with two variable." << endl;
	testParserVariableRegisterReplacementXY();
	cout << "Test three register replacement and substitution, with three variable." << endl;
	testParserVariableRegisterReplacementXYZ();
	cout << "Test two register and offset replacement." << endl;
	testParserTwoRegisterReplacementOffset();

	cout << "All Tests Passed." << endl;
}
#endif