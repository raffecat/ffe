// thread.h
// FireFly server 'thread' data type

#ifndef FIREFLY_THREAD_H
#define FIREFLY_THREAD_H

#include "global.h"
#include "index.h"
#include "any.h"

// Debug
#include "function.h"
#include "string.h"


// Thread states
#define TS_STOPPED		0
#define TS_READY		1
#define TS_SLEEP		2
#define TS_READ			3
#define TS_READLN		4
#define TS_WRITE		5
#define TS_ACCEPT		6

// Machine-status-word for thread
class msw_t
{
public:
	msw_t();
	~msw_t();

public:
    objectp m_oContext;
    functionp m_fnCode;
    int32 m_nIP;
    class any_t *m_pRegisters;
    class msw_t *m_pCaller;
	int m_nRegisters;
	int m_nReturnReg;
	int m_nCatchReg;
	int32 m_nCatchHandler;
};

class thread_t  
{
public:
	thread_t();
	thread_t(const index_sentinel_t&);
	~thread_t();

// Public interface
public:
	void SetupThread( functionp fnMain, objectp oContext, arrayp arArgs );
	void AbortThread( const any_t& mValue );
	RESULT Execute( int& nRemain );

	int32 GetState() { return m_nState; }
	int32 IsStopped() { return m_nState == TS_STOPPED; }
	int32 IsBlocked() { return m_nState != TS_READY; }

	any_t GetExitValue() { return m_mResult; }
	any_t GetMainFunc() { return any_t( m_fnMain, m_oContext ); }

// Private helper functions
protected:
	void SetupCall( objectp oContext, functionp fnCode, arrayp arArgs, int nReturnReg );
	void FinishCall( const any_t& mValue, bool bSetResult );
	arrayp BuildRuntimeList( any_t* pReg, uint32*& pInst, int nArgs );
	arrayp BuildErrorInfo( RESULT nError );
	arrayp BuildCallStack();

    // This would return const any_t& and dereference registers in-place,
	// but compiled code depends on having (arr,index) and (obj,attr) references
    // captured in registers for ops like += to work!
	any_t Dereference( any_t* pReg, int nReg );

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
    int32 m_nCallDepth;
    int32 m_nState;
    time_t m_nStateTime;
	int32 m_nStateReg;
    portp m_ptStatePort;
    stringp m_sStateStr;
    objectp m_oContext;
    functionp m_fnMain;
    msw_t *m_mswCurrent;
	any_t m_mResult;
};

// Construction, Destruction

inline thread_t::thread_t() : m_nRef(1),
	m_nCallDepth(0), m_nState(TS_STOPPED), m_nStateTime(0), m_nStateReg(0),
	m_ptStatePort(NULL), m_sStateStr(NULL), m_oContext(NULL), m_fnMain(NULL),
	m_mswCurrent(NULL)
{
#ifdef DEBUG_REF
	const char* szName = "unknown";
	if( m_fnMain && m_fnMain->m_sName ) szName = m_fnMain->m_sName->CString();
	printf( "[thread %08x create: %d refs]\n", this, szName, (int)m_nRef );
#endif
	g_oIdxThread.Insert( this );
}

inline thread_t::thread_t(const index_sentinel_t&) : m_nRef(0),
	m_nCallDepth(0), m_nState(TS_STOPPED), m_nStateTime(0), m_nStateReg(0),
	m_ptStatePort(NULL), m_sStateStr(NULL), m_oContext(NULL), m_fnMain(NULL),
	m_mswCurrent(NULL)
{
}

// Reference counting

inline void thread_t::AddRef()
{
#ifdef DEBUG_REF
	const char* szName = "unknown";
	if( m_fnMain && m_fnMain->m_sName ) szName = m_fnMain->m_sName->CString();
	printf( "[thread %08x (%s) addref: %d refs]\n", this, szName, m_nRef+1 );
#endif
	ASSERT( m_nRef > 0 );
	m_nRef++;
}

inline void thread_t::Release()
{
#ifdef DEBUG_REF
	const char* szName = "unknown";
	if( m_fnMain && m_fnMain->m_sName ) szName = m_fnMain->m_sName->CString();
	printf( "[thread %08x (%s) release: %d refs]\n", this, szName, m_nRef-1 );
#endif
	ASSERT( m_nRef > 0 );
	if( --m_nRef == 0 ) delete this;
}

#endif // FIREFLY_THREAD_H
