// function.h
// FireFly server 'function' data type

#ifndef FIREFLY_FUNCTION_H
#define FIREFLY_FUNCTION_H

#include "global.h"
#include "index.h"

class function_t  
{
public:
	function_t();
	function_t(const index_sentinel_t&);
	~function_t();

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

public:
	stringp m_sName;
	stringp m_sSource;
	arrayp m_arConstants;
	int32 m_nArgs;
	int32 m_nLocals;
	int32 m_nCodeSize;
	int32 m_nLineSize;
	uint32* m_pCode;
	int32* m_pLines;
};

// Construction, Destruction

inline function_t::function_t() : m_nRef(1),
	m_sName(NULL), m_sSource(NULL), m_arConstants(NULL),
	m_nArgs(0), m_nLocals(0), m_nCodeSize(0), m_nLineSize(0),
	m_pCode(NULL), m_pLines(NULL)
{
#ifdef DEBUG_REF
	printf( "[function %08x create: %d refs (%d)]\n", this, m_nRef, (int)bRegister );
#endif
	g_oIdxFunction.Insert( this );
}

inline function_t::function_t(const index_sentinel_t&) : m_nRef(0),
	m_sName(NULL), m_sSource(NULL), m_arConstants(NULL),
	m_nArgs(0), m_nLocals(0), m_nCodeSize(0), m_nLineSize(0),
	m_pCode(NULL), m_pLines(NULL)
{
}


// Reference counting

inline void function_t::AddRef()
{
#ifdef DEBUG_REF
	printf( "[function %08x addref: %d refs]\n", this, m_nRef+1 );
#endif
	ASSERT( m_nRef > 0 );
	m_nRef++;
}

inline void function_t::Release()
{
#ifdef DEBUG_REF
	printf( "[function %08x release: %d refs]\n", this, m_nRef-1 );
#endif
	ASSERT( m_nRef > 0 );
	if( --m_nRef == 0 ) delete this;

}

#endif // FIREFLY_FUNCTION_H
