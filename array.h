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
	array_t();
	inline array_t(const index_sentinel_t&) : m_nRef(0) {}
	~array_t();

// Public interface
public:
	inline int32 Size() const { return (int32)m_vals.size(); }
	void AppendItem(const any_t& mValue);
	bool RemoveItem(const any_t& mValue);
	void AppendMulti(const arrayp arValue);
	bool RemoveMulti(const arrayp arValue);
	bool RemoveFirst();
	int32 Find(const any_t& mValue, int32 start, int32 end);
	const any_t& Lookup(int32 nIndex);
	bool Assign(int32 nIndex, const any_t& mValue);
	arrayp Slice(range_t rnSlice);
	stringp Join(const stringp sDelim, const stringp sFinalDelim);

// Persistent storage management
public:
	void Persist() const;
	void Restore();
	void Validate() { }

// Reference counting
public:
	inline void AddRef();
	inline void Release();

	inline int32 GetRefCount() const { return m_nRef; }
	inline void SetRefCount( int32 nRef ) { m_nRef = nRef; }

// Private member variables
protected:
    int32 m_nRef;
	array_vec m_vals;
};


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
