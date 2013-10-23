#ifdef TESTING
#include "List.h"
#include <string.h>
#include <assert.h>
#include <iostream>

using std::cout;
using std::endl;

void testListInsert()
{
	List<int> t;
	t.add( 0 );
	t.add( 1 );
	t.add( 2 );

	assert( t.length() == 3 );
}
void testListGet()
{
	List<int> t;

	t.add( 0 );
	t.add( 1 );
	t.add( 2 );

	assert( t[0]->getData() == 0 );
	assert( t[1]->getData() == 1 );
	assert( t[2]->getData() == 2 );
}
void testListReplace()
{
	List<int> t;
	t.add( 0 );
	Link<int> *tt = t[0];

	t.replace( tt, 3, 1, 2, 3 );
	assert( t[0]->getData() == 1 );
	assert( t[1]->getData() == 2 );
	assert( t[2]->getData() == 3 );
}

void testListCharPointer()
{
	List<char *> t;
	char *a = new char[10];
	a[0] = 'a';
	a[1] = '\0';
	t.add( a );
	assert( strcmp( t[0]->getData(), "a" ) == 0 );
}

int check( int a, int b )
{
	return a < b ? -1 : a > b ? 1 : 0;
}

void testListAddUnique()
{

	List<int> a( check );
	a.addUnique( 1 );
	a.addUnique( 1 );
	assert( a.length() == 1 );
}

#endif
