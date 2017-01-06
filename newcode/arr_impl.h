// array.h
// FireFly server 'array' data type

#ifndef FIREFLY_ARRAY_H
#define FIREFLY_ARRAY_H

#include "global.h"
#include "any.h"

#include <vector>

typedef std::vector<any_t> array_vec;
typedef array_vec::iterator array_it;
typedef array_vec::const_iterator array_const_it;
typedef array_vec::reverse_iterator array_rit;

class array_t  
{
public:
	array_t( bool bRegister = true );
	array_t( arrayp arValue );
	~array_t();

// Public interface
public:
	int32 Size();
	RESULT AppendItem( any_t mValue );
	RESULT AppendMulti( const arrayp arValue );
	RESULT RemoveMulti( const arrayp arValue );
	RESULT RemoveItem( any_t mValue );
	RESULT RemoveFirst();
	RESULT LookupAt( int32 nIndex, any_t& mValue );
	RESULT LookupRange( range_t rnSlice, arrayp& arResult );
	RESULT AssignAt( int32 nIndex, const any_t& mValue );
	RESULT Join( const stringp sDelim, const stringp sFinalDelim, stringp& sResult );

// Persistent storage management
public:
	void Persist();
	void Restore();
	void Validate() { }

// Reference counting
public:
	inline void AddRef();
	inline void Release();

	inline int32 GetRefCount() { return m_nRef; }
	inline void SetRefCount( int32 nRef ) { m_nRef = nRef; }

// Private member variables
protected:
    int32 m_nRef;
	array_vec m_pValues;
};

// Construction, Destruction

inline int32 array_t::Size()
{
	// Do we stop arrays from growing too big?
	return (int32)m_pValues.size();
}

// Reference counting

inline void array_t::AddRef()
{
#ifdef DEBUG_REF
	printf( "[array %08x addref: %d refs]\n", this, m_nRef+1 );
#endif
	ASSERT( m_nRef > 0 );
	m_nRef++;
}

inline void array_t::Release()
{
#ifdef DEBUG_REF
	printf( "[mapping %08x release: %d refs]\n", this, m_nRef-1 );
#endif
	ASSERT( m_nRef > 0 );
	if( --m_nRef == 0 ) delete this;
}

#endif // FIREFLY_ARRAY_H
