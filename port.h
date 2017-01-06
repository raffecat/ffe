// port.h
// FireFly server 'port' data type

#ifndef FIREFLY_PORT_H
#define FIREFLY_PORT_H

#include "global.h"
#include "index.h"

#include <string>

#define READ_CHUNK		4096

class port_t  
{
public:
	port_t();
	port_t(const index_sentinel_t&);
	~port_t();

// Public interface
public:
	RESULT Listen( int32 nPort );
	RESULT Accept( portp& ptResult );
	RESULT Connect( stringp sAddress, int32 nPort );
	RESULT Read( int32 nSize, stringp& sResult );
	RESULT ReadLine( stringp& sResult );
	RESULT Write( stringp sValue );
	RESULT Close();
	bool IsClosed() { return (m_fdSocket == INVALID_SOCKET); }
	int32 GetPort() { return m_nPort; }

// Static interface functions
public:
	static RESULT Initialise();
	static RESULT Shutdown();
	static RESULT Select( time_t nTimeout );

// Private implementation
protected:
	RESULT ReadData();
	RESULT SetupSocket();

	static void CDECL SigBrokenPipe( int sig );

// Persistent storage management
public:
	void Persist();
	void Restore();
	void Validate() { }

	inline int32 GetRefCount() { return m_nRef; }
	inline void SetRefCount( int32 nRef ) { m_nRef = nRef; }

// Reference counting
public:
	inline void AddRef();
	inline void Release();

// Private member variables
protected:
    int32 m_nRef;
    int32 m_nPort;
	int32 m_nWritten;
	SOCKET m_fdSocket;
	std::basic_string<char> m_sReadBuf;
	byte m_pReadChunk[READ_CHUNK];

	static fd_set m_fdsHandles;
	static fd_set m_fdsWriteHandles;
	static portp m_pCurrent;
};

// Construction, Destruction

inline port_t::port_t() : m_nRef(1), m_nPort(0),
	m_nWritten(0), m_fdSocket(INVALID_SOCKET)
{
#ifdef DEBUG_REF
	printf( "[port %08x create: %d refs]\n", this, (int)m_nRef );
#endif
	g_oIdxPort.Insert( this );
}

inline port_t::port_t(const index_sentinel_t&) : m_nRef(0), m_nPort(0),
	m_nWritten(0), m_fdSocket(INVALID_SOCKET)
{
}

inline port_t::~port_t()
{
#ifdef DEBUG_REF
	printf( "[port %08x destroy: %d refs]\n", this, (int)m_nRef );
#endif
	Close();
	g_oIdxPort.Remove( this );
}

// Reference counting

inline void port_t::AddRef()
{
#ifdef DEBUG_REF
	printf( "[port %08x addref: %d refs]\n", this, m_nRef+1 );
#endif
	ASSERT( m_nRef > 0 );
	m_nRef++;
}

inline void port_t::Release()
{
#ifdef DEBUG_REF
	printf( "[port %08x release: %d refs]\n", this, m_nRef-1 );
#endif
	ASSERT( m_nRef > 0 );
	if( --m_nRef == 0 ) delete this;
}

#endif // FIREFLY_PORT_H
