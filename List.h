#pragma once
#ifndef __LIST__
#define __LIST__
#include <cstdarg>

template<class T> class Link
{
	public:
		Link();
		Link( T obj );
		// PRE: This object is defined.
		// POST: The RV is the mData member.
		T getData() { return mData; }
		// PRE: This object is defined as is nData.
		// POST: The mData will be set to nData.
		void setData( T nData ) { mData = nData; }
		// PRE: This object is defined.
		// POST: The RV is mPrev.
		Link<T> *getPrev() { return mPrev; }
		// PRE: This object, nPrev are defined.
		// POST: mPrev is set to nPrev.
		void setPrev( Link *nPrev ) { mPrev = nPrev; }
		// PRE: This object is defined.
		// POST: The RV is mNext.
		Link<T> *getNext() { return mNext; }
		// PRE: This object, nNext are defined.
		// POST: mNext is set to nNext.
		void setNext( Link *nNext ) { mNext = nNext; }
	private:
		T mData;
		Link<T> *mNext, *mPrev;
};
// PRE: This object is not defined. 
// POST: This object is defined.
template <class T> Link<T>::Link(): mNext( 0 ), mPrev( 0 ), mData( 0 )
{}

// PRE: This object is not defined.
// POST: This object is defined.
template <class T> Link<T>::Link( T obj ): mPrev( 0 ), mNext( 0 )
{
	mData = obj;
}

template <class T> class List
{
	public:
		List();
		List( int (*compare)(T a, T b) );
		void add( T obj );
		Link<T> *addUnique( T obj );
		void replace( Link<T> *link, int numInsert, ... );
		Link<T> *operator[]( int index );
		int length() { return mLength; }
	private:
		int mLength;
		Link<T> *mHead, *mTail;
		int (*mCompare)( T a, T b );
};
// PRE: This object is not defined.
// POST: This object is defined.
template <class T> List<T>::List(): mHead( 0 ),mTail( 0 ), mLength( 0 )
{}

// PRE: This object is not defined.
// POST: This object is defined.
template <class T> List<T>::List( int (*compare)( T a, T b ) ) : mHead( 0 ), mTail( 0 ), mLength( 0 ), mCompare( compare )
{}

// PRE: This object is defined and as is obj.
// POST: The list will contain obj encapsulated in a Link object
//		added to the end of this list.
template <class T> void List<T>::add( T obj )
{
	if( mHead == 0 )
	{
		mHead = new Link<T>( obj );
		mTail = mHead;
	}
	else if( mHead == mTail )
	{
		Link<T>* temp = new Link<T>( obj );
		mTail = temp;
		mHead->setNext( mTail );
		mTail->setPrev( mHead );
		mTail->setNext( 0 );
	}
	else
	{
		Link<T>* temp = new Link<T>( obj );
		mTail->setNext( temp );
		temp->setPrev( mTail );
		mTail = temp;
	}

	mLength++;
}

// PRE: This object and obj are defined.
// POST: If the obj is not in the current list then it is
//		encapsulated in a Link object and appened to the end
//		of the list, else it is not added and the RV is
//		the link that is of the same value as obj.
template <class T> Link<T> *List<T>::addUnique( T obj )
{
	Link<T> *ret = 0;
	if( length() == 0 )
	{
		add( obj );
	}
	else
	{
		Link<T> *walker = mHead;
		bool addObj = true;
		for( int i = 0; i < length() && addObj; i++ )
		{
			if( mCompare( obj, walker->getData() ) == 0 )
			{
				addObj = false;
			}
			else
				walker = walker->getNext();
		}

		if( addObj )
		{
			add( obj );
		}
		else
			ret = walker;
	}

	return ret;
}

// PRE: This object, link are defined and numInsert describes the number of
//		parameters passed to the function.
// POST: The link is replaced by a new list of numInsert elements in its place
//		and link is deleted from the list.
template <class T> void List<T>::replace( Link<T> *link, int numInsert, ... )
{
	va_list arguments;
	va_start( arguments, numInsert );

	Link<T> *tPrev = link->getPrev();
	Link<T> *tNext = link->getNext();
	Link<T> *tListHead = 0;
	Link<T> *tListTail = 0;
	for( int i = 0; i < numInsert; i++ )
	{
		Link<T> *temp = new Link<T>( va_arg ( arguments, T ) );
		if( tListHead == 0 )
		{
			tListHead = temp;
			tListHead->setPrev( tPrev );	
			tListTail = tListHead;
		}
		else if( tListHead == tListTail )
		{
			tListTail = temp;
			tListHead->setNext( tListTail );
			tListTail->setPrev( tListHead );
			tListTail->setNext( 0 );
		}
		else
		{
			tListTail->setNext( temp );
			temp->setPrev( tListTail );
			tListTail = temp;
		}
	}

	tListTail->setNext( tNext );
	
	if( link->getPrev() == 0 )
		mHead = tListHead;
	else
		link->getPrev()->setNext( tListHead );

	if( link->getNext() == 0 )
		mTail = tListTail;

	delete link;
}

// PRE: This object is defined and index is a valid index into the list.
// POST: The RV is the link at that position or 0.
template <class T> Link<T> *List<T>::operator[]( int index )
{
	int position = 0;
	Link<T> *walker = mHead;
	
	while( walker != 0 && position < index )
	{
		walker = walker->getNext();
		position++;
	}

	return walker;	
}

#ifdef TESTING
// Tests the list insertion.
void testListInsert();
// Tests the list get functionality.
void testListGet();
// Tests the replace method.
void testListReplace();
// Tests the list with char *
void testListCharPointer();
// Tests the addUnique method.
void testListAddUnique();
#endif

#endif
