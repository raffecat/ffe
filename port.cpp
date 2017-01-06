// port.cpp
// FireFly server 'port' data type

#include "port.h"
#include "database.h"
#include "string.h"

#ifndef SIGIGN
#define SIGIGN SIG_IGN
#endif

#include <errno.h>
#include <netinet/tcp.h>

fd_set port_t::m_fdsHandles;
fd_set port_t::m_fdsWriteHandles;
portp port_t::m_pCurrent = NULL;
static int m_hasWriters = 0;

/* Broken Pipe signal handler
void CDECL port_t::SigBrokenPipe( int sig )
{
	if( m_pCurrent )
	{
		printf( "[%d] broken pipe, closed", (int)m_pCurrent->m_fdSocket );

		m_pCurrent->Close();
	}
	else
	{
		printf( "[unk] broken pipe, ignored" );
	}
}
*/

RESULT port_t::Initialise()
{
#ifdef WIN32
	// Start windows sockets
	WSADATA wsaData;
	WORD nRequest = MAKEWORD( 1, 0 );
	if( WSAStartup( nRequest, &wsaData ) != 0 )
	{
		printf( "Failed to initialise windows sockets" );
		return E_SOCKET;
	}

	// Check that the required version is supported
	if ( LOBYTE( wsaData.wVersion ) != 1 ||
	     HIBYTE( wsaData.wVersion ) != 0 )
	{
		printf( "WinSock version 1.0 required" );
		WSACleanup();
		return E_SOCKET;
	}
#else
	// Install a signal handler for Broken Pipe signals
	//signal( SIGPIPE, SigBrokenPipe );
	signal( SIGPIPE, SIGIGN );
#endif
    FD_ZERO( &m_fdsHandles );
	FD_ZERO( &m_fdsWriteHandles );
	m_hasWriters = 0;

    return E_SUCCESS;
}

RESULT port_t::Shutdown()
{
#ifdef WIN32
	WSACleanup();
#endif

    return E_SUCCESS;
}

RESULT port_t::Select( time_t nTimeout )
{
	struct timeval tvTime = { (long) nTimeout, 0 };

    fd_set fdsRead = m_fdsHandles;
	//fd_set fdsWrite = m_fdsWriteHandles;
	// fd_set fdsExcept = m_fdsHandles;

	// Must select on fdsRead and fdsWrite, otherwise writing large
	// amounts of data will stall until the next read!
    // int nReady = select( FD_SETSIZE, &fdsRead, &fdsWrite, &fdsExcept, &tvTime );

	// For Win32, any non-null handle set specified must have at least
	// one handle in it according to the documentation. Stupid restriction.
    int nReady;
	if (m_hasWriters)
	{
		nReady = select( FD_SETSIZE, &fdsRead, &m_fdsWriteHandles, NULL, &tvTime );
		//printf( "** has %d writers.\n", m_hasWriters );
	}
	else
	{
		nReady = select( FD_SETSIZE, &fdsRead, NULL, NULL, &tvTime );
	}

	// Clear all the write handles every time we wake up
	// This is the only safe way to clean them up, because if we ever
	// miss one (eg. the thread writing to a socket is killed) we will
	// use up 100% cpu polling that socket until we're restarted.
	// So instead Write() must put the socket back into the set
	// every time it returns with write data outstanding.
	FD_ZERO( &m_fdsWriteHandles );
	m_hasWriters = 0;

	if( nReady < 1 ) return E_FALSE;
	return E_SUCCESS;
}

RESULT port_t::Listen( int32 nPort )
{
    // Ensure port number is valid
    if( nPort < 1 ) return E_RANGE;

    // Create a new socket
    m_fdSocket = socket( AF_INET, SOCK_STREAM, 0 );
    if( m_fdSocket == INVALID_SOCKET ) return E_SOCKET;

    // Enable address re-use (so we can use any-address)
    int nOption = 1;
    if( setsockopt( m_fdSocket, SOL_SOCKET, SO_REUSEADDR,
        (char*)&nOption, sizeof(nOption) ) == -1 )
    {
		Close();
        return E_SOCKET;
    }

    // Bind to any-address on specified port
    struct sockaddr_in addrListen;
    addrListen.sin_family = AF_INET;
    addrListen.sin_addr.s_addr = INADDR_ANY;
    addrListen.sin_port = htons((unsigned short)nPort);
    if( bind( m_fdSocket, (struct sockaddr*)&addrListen,
        sizeof(addrListen) ) == -1 )
    {
        Close();
        return E_SOCKET;
    }

    // Set connection queue size
    if( listen( m_fdSocket, 8 ) == -1 )
    {
        Close();
        return E_SOCKET;
    }

	// Set up the socket options
	RESULT nResult = SetupSocket();
	if( ISERROR(nResult) ) return nResult;

    return E_SUCCESS;
}

RESULT port_t::Accept( portp& ptResult )
{
	SOCKET fdNew;

    // Accept the new connection
	m_pCurrent = this;
    fdNew = accept( m_fdSocket, (struct sockaddr*) 0, 0 );
	m_pCurrent = NULL;

	// Check the return value
	if( fdNew == INVALID_SOCKET )
	{
#ifdef WIN32
		if( WSAGetLastError() != WSAEWOULDBLOCK )
#else
		if( errno != EAGAIN && errno != EINTR )
#endif
		{
			// Something has gone horribly wrong
			Close();
			return E_CLOSED;
		}

		// Write would block - try again later
		return E_FALSE;
	}

	// Create the new socket
	ptResult = new port_t;
	if( !ptResult )
	{
#ifdef WIN32
		closesocket( fdNew );
#else
        close( fdNew );
#endif
		return E_MEMORY;
	}
	ptResult->m_fdSocket = fdNew;

	// Set up the socket options
	RESULT nResult = ptResult->SetupSocket();
	if( ISERROR(nResult) ) return nResult;

    // Set no-delay mode (TODO: should do internal buffering or use TCP_CORK)
    int noDelay = 1;
    if (setsockopt( fdNew, IPPROTO_TCP, TCP_NODELAY, (char*)&noDelay, sizeof(noDelay) ) == -1) {
        Close();
        return E_SOCKET;
    }

	return E_SUCCESS;
}

RESULT port_t::Connect( stringp sAddress, int32 nPort )
{
	return E_NOT_IMPLEMENTED;
}

RESULT port_t::Read( int32 nSize, stringp& sResult )
{
	RESULT nResult = ReadData();
	if( ISERROR(nResult) ) return nResult;

	// If not reading data, succeed unless port is closed and empty
	if( nSize < 1 )
	{
		if( IsClosed() && !m_sReadBuf.size() )
			return E_CLOSED;
		return E_SUCCESS;
	}

	// Check if sufficient data is available
	if( (int32)m_sReadBuf.size() >= nSize )
	{
		// Create the result string
		sResult = new string_t( m_sReadBuf.c_str(), nSize );
		if( !sResult ) return E_MEMORY;

		// Remove the read data from the input buffer
		m_sReadBuf = m_sReadBuf.c_str() + nSize;

		return E_SUCCESS;
	}

    if( IsClosed() )
		return E_CLOSED;
	return E_FALSE;
}

RESULT port_t::ReadLine( stringp& sResult )
{
	RESULT nResult = ReadData();
	if( ISERROR(nResult) ) return nResult;

	// Check if sufficient data is available
	long nEnd = m_sReadBuf.find_first_of( "\n\r" );
	if( nEnd != std::basic_string<char>::npos )
	{
		// Create the result string
		sResult = new string_t( m_sReadBuf.c_str(), nEnd );
		if( !sResult ) return E_MEMORY;

		// Handle any type of line terminator
		if( nEnd + 1 < (int32)m_sReadBuf.length() )
		{
			if( (m_sReadBuf[nEnd] == '\n' && m_sReadBuf[nEnd+1] == '\r') ||
				(m_sReadBuf[nEnd] == '\r' && m_sReadBuf[nEnd+1] == '\n') ) nEnd++;
		}

		// Remove the read data from the input buffer
		// Use a temp string because basic_string uses memcpy not memmove
		std::basic_string<char> strTemp = m_sReadBuf.c_str() + nEnd + 1;
		m_sReadBuf = strTemp;

		return E_SUCCESS;
	}

    if( IsClosed() )
		return E_CLOSED;
	return E_FALSE;
}

RESULT port_t::Write( stringp sValue )
{
	if( IsClosed() )
		return E_CLOSED;

	int32 nLen = sValue->Length();
	if( nLen < 1 ) return E_SUCCESS;

	// Calculate the length remaining to write
	size_t nWritten, nWrite = nLen - m_nWritten;
	ASSERT( nWrite > 0 );
	const char* szData = sValue->CString() + m_nWritten;

	m_pCurrent = this;
#ifdef WIN32
	if( m_nPort == FILE_PORT_FLAG )
	{
		// Write to the file
		nWritten = WRITE( m_fdSocket, szData, nWrite );
	}
	else
	{
		// Write to the socket
		nWritten = send( m_fdSocket, szData, nWrite, 0 );
	}
#else
    // Write the data to the port
    nWritten = write( m_fdSocket, szData, nWrite );
#endif
	m_pCurrent = NULL;

	// Check the return value
	if( nWritten == (size_t)-1 )
	{
#ifdef WIN32
		if( WSAGetLastError() != WSAEWOULDBLOCK )
#else
		if( errno != EAGAIN && errno != EINTR )
#endif
		{
			// Something has gone horribly wrong
			Close();
			return E_CLOSED;
		}

		// Write would block - try again later
		nWritten = 0;
	}

#ifdef WIN32
	// Send the data immediately
	//_commit( m_fdSocket );
#endif

	if( m_nWritten + (long)nWritten < nLen )
	{
		// Could not write all the data
		m_nWritten += nWritten;

		// Add the socket to the write handle set
		// Must do this every time we leave outstanding data
		FD_SET( m_fdSocket, &m_fdsWriteHandles );
		m_hasWriters++;

		//printf( "** added writer, now has %d writers.\n", m_hasWriters );

		return E_FALSE;
	}

	// All data successfully written
	m_nWritten = 0;

	return E_SUCCESS;
}

RESULT port_t::Close()
{
	m_pCurrent = NULL; // prevent potential infinite loop

	if( m_fdSocket != INVALID_SOCKET )
	{
#ifdef WIN32
		if( m_nPort == FILE_PORT_FLAG )
		{
			// Close the file
			CLOSE( m_fdSocket );
		}
		else
		{
			// Close the socket
	        FD_CLR( m_fdSocket, &m_fdsHandles );
			FD_CLR( m_fdSocket, &m_fdsWriteHandles );
			closesocket( m_fdSocket );
		}
#else
        FD_CLR( m_fdSocket, &m_fdsHandles );
        FD_CLR( m_fdSocket, &m_fdsWriteHandles );
        close( m_fdSocket );
#endif
    }

    // Mark the port as closed
    m_fdSocket = INVALID_SOCKET;

	return E_SUCCESS;
}


// Private implementation

RESULT port_t::ReadData()
{
	size_t nRead;

	// Check the socket for new data
	m_pCurrent = this;
	if( m_fdSocket != INVALID_SOCKET )
	{
#ifdef WIN32
		if( m_nPort == FILE_PORT_FLAG )
		{
			// Read from the file
			nRead = READ( m_fdSocket, (char*)m_pReadChunk, READ_CHUNK );
		}
		else
		{
			// Read from the socket
			nRead = recv( m_fdSocket, (char*)m_pReadChunk, READ_CHUNK, 0);
		}
#else
		// Read any new data from the port
		nRead = read( m_fdSocket, (char*)m_pReadChunk, READ_CHUNK );
#endif

		// Check the return value
		if( nRead == 0 ) Close();
		else if( nRead == (size_t)-1 )
		{
#ifdef WIN32
			if( WSAGetLastError() != WSAEWOULDBLOCK )
#else
			if( errno != EAGAIN && errno != EINTR )
#endif
			{
				// Something has gone horribly wrong
				Close();
				return E_CLOSED;
			}

			// Read would block - try again later
			nRead = 0;
		}

		if( nRead ) m_sReadBuf.append( (char*)m_pReadChunk, nRead );
	}
	m_pCurrent = NULL;

	return E_SUCCESS;
}

RESULT port_t::SetupSocket()
{
	// Make sure out-of-band data stays that way
    int nOption = 0;
    if( setsockopt( m_fdSocket, SOL_SOCKET, SO_OOBINLINE,
        (char*)&nOption, sizeof(nOption) ) == -1 )
    {
		Close();
        return E_SOCKET;
    }

    // Set non-blocking flag so we can poll/select
#ifdef WIN32
	{
		unsigned long nValue = 1;
		if( ioctlsocket( m_fdSocket, FIONBIO, &nValue ) != 0 )
		{
			Close();
			return E_SOCKET;
		}
	}
#else
    if( fcntl( m_fdSocket, F_SETFL, O_NONBLOCK ) == -1 )
    {
        Close();
        return E_SOCKET;
    }
#endif

	// Add the socket to the handle set
    FD_SET( m_fdSocket, &m_fdsHandles );

	return E_SUCCESS;
}


// Persistent storage management

void port_t::Persist()
{
	// Write the reference count
	g_oDatabase.WriteLong( m_nRef );
}

void port_t::Restore()
{
	// Read the reference count
	m_nRef = g_oDatabase.ReadLong();
}
