// Network Ports

// Advance: removes the current line from the buffer (up to buf->pos)

// ReadLine:
// receives and appends data to readBuf, up to its capacity;
// advances buf->pos as it scans for '\r', '\n' or IAC.
// if the buf contains IAC sequences, it elides those;
// if the buf now contains a complete line, it returns true.
// (the line is between 0 and pos)

// Write: pass in a buffer, since this is most often the source.
// Increments buf->pos as the data is written out.
// Returns true when finished, false if there is more data.

#include "result.h"

typedef struct port_t* portp;
typedef struct persist_t* persistp;
typedef struct string_t* stringp;
typedef struct buffer_t* bufferp;

#include <stddef.h>
#include <stdint.h>

#include <malloc.h>
void* card_alloc(size_t size) {
    return malloc(size);
}

#ifdef WIN32
  #include <winsock.h>
#else
  #include <sys/time.h> // timeval
  #include <sys/types.h>
  #include <sys/socket.h> // AF
  #include <netinet/in.h> // INADDR_ANY
  #define SOCKET int
  #define INVALID_SOCKET (-1)
#endif

#ifndef SIGIGN
#define SIGIGN SIG_IGN
#endif

#include <signal.h>
#include <errno.h>
#include <stdio.h>      // printf

typedef struct buffer_t {
    int32_t len;
    int32_t cap;
    char* data;
} buffer_t;

void buf_init(bufferp buf) {
    buf->len = 0;
    buf->cap = 0;
    buf->data = 0;
}

typedef struct port_t {
    buffer_t buf;
    SOCKET fd;
    int eatNL;
} port_t;

void port_init(portp self) {
    self->fd = INVALID_SOCKET;
    self->eatNL = 0;
    buf_init(&self->buf);
}

void port_final(portp self) {
    buf_final(&self->buf);
}

static fd_set m_fdsHandles;
static fd_set m_fdsWriteHandles;
static int m_hasWriters = 0;

#define IsClosed(P) ((P)->fd == INVALID_SOCKET)

RESULT port_startup()
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

RESULT port_shutdown()
{
#ifdef WIN32
    WSACleanup();
#endif
    return E_SUCCESS;
}

RESULT port_wait( time_t nTimeout )
{
    struct timeval tvTime = { nTimeout, 0 };

    fd_set fdsRead = m_fdsHandles;
    //fd_set fdsWrite = m_fdsWriteHandles;
    // fd_set fdsExcept = m_fdsHandles;

    // Must select on fdsRead and fdsWrite, otherwise writing large
    // amounts of data will stall until the next read!
    // int nReady = select( FD_SETSIZE, &fdsRead, &fdsWrite, &fdsExcept, &tvTime );

    // For Win32, any non-null handle set specified must have at least
    // one handle in it according to the documentation. Stupid restriction.
    int nReady;
    if (m_hasWriters) {
        nReady = select( FD_SETSIZE, &fdsRead, &m_fdsWriteHandles, NULL, &tvTime );
    } else {
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

RESULT port_listen( portp self, int32_t nPort )
{
    // Ensure port number is valid
    if (nPort < 1) return E_RANGE;

    // Create a new socket
    self->fd = socket( AF_INET, SOCK_STREAM, 0 );
    if (IsClosed(self)) return E_SOCKET;

    // Enable address re-use (so we can use any-address)
    int nOption = 1;
    if (setsockopt(self->fd, SOL_SOCKET, SO_REUSEADDR, (char*)&nOption, sizeof(nOption)) == -1) {
        port_close(self);
        return E_SOCKET;
    }

    // Bind to any-address on specified port
    struct sockaddr_in addrListen;
    addrListen.sin_family = AF_INET;
    addrListen.sin_addr.s_addr = INADDR_ANY;
    addrListen.sin_port = htons((unsigned short)nPort);
    if (bind(self->fd, (struct sockaddr*)&addrListen, sizeof(addrListen)) == -1) {
        port_close(self);
        return E_SOCKET;
    }

    // Set connection queue size
    if (listen(self->fd, 512) == -1) {
        port_close(self);
        return E_SOCKET;
    }

    // Set up the socket options
    RESULT err = port_init_socket(self);
    if (ISERROR(err)) return err;

    return E_SUCCESS;
}

RESULT port_accept( portp self, portp result )
{
    SOCKET fdNew;

    // Accept the new connection
    fdNew = accept(self->fd, (struct sockaddr*)0, 0);

    // Check the return value
    if (fdNew == INVALID_SOCKET) {
#ifdef WIN32
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
#else
        if (errno != EAGAIN && errno != EINTR) {
#endif
            // Something has gone horribly wrong
            port_close(self);
            return E_CLOSED;
        }

        // Accept would block - try again later
        return E_FALSE;
    }

    // Create the new socket
    result->fd = fdNew;

    // Set up the socket options
    int err = port_init_socket(self);
    if (ISERROR(err)) return err;

    return E_SUCCESS;
}

RESULT port_read( portp self, int32_t nSize, stringp result )
{
    RESULT err = port_read_data( self );
    if (ISERROR(err)) return err;

    // If not reading data, succeed unless port is closed and empty
    if (nSize < 1) {
        if (IsClosed(self) && !self->buf.len) return E_CLOSED;
        return E_SUCCESS;
    }

    // Check if sufficient data is available
    if (self->buf.len >= nSize) {
        str_assign(result, self->buf.data, nSize);
        buf_consume(&self->buf, nSize);
        return E_SUCCESS;
    }

    if (IsClosed(self)) return E_CLOSED;
    return E_FALSE;
}

RESULT port_readLine( portp self, stringp result )
{
    RESULT err = port_read_data( self );
    if (ISERROR(err)) return err;

    int32_t end = 0;

    // Eat left-over '\n' from previous line.
    if (self->eatNL && self->buf.len) {
        if (self->buf.data[0] == '\n') end++; // skip it.
    }

    // Check if we have received a complete line.
    int found = 0;
    for (; end < self->buf.len; end++) {
        if (self->buf.data[end] == '\r' || self->buf.data[end] == '\n') {
            found = 1;
            break;
        }
    }

    if (found) {
        self->eatNL = 0; // clear previous flag.
        str_assign(result, self->buf.data, end);
        if (self->buf.data[end] == '\r') {
            // expect a '\n' to follow per telnet.
            if (end < self->buf.len) {
                if (self->buf.data[end+1] == '\n') end++;
            } else {
                // when we get more data, must eat one '\n'
                self->eatNL = 1;
            }
        }
        buf_consume(&self->buf, end+1); // +1 include '\r' or '\n'
        return E_SUCCESS;
    }

    if (IsClosed(self)) return E_CLOSED;
    return E_FALSE;
}

RESULT port_write( portp self, stringp sValue )
{
    if (IsClosed(self))
        return E_CLOSED;

    if (sValue->len < 1) return E_SUCCESS;

    // Calculate the length remaining to write
    size_t nWritten, nWrite = sValue->len - self->written;
    ASSERT( nWrite > 0 );
    char* szData = sValue->CString() + self->written;

#ifdef WIN32
    // Write to the socket
    nWritten = send( self->fd, szData, nWrite, 0 );
#else
    // Write the data to the port
    nWritten = write( self->fd, szData, nWrite );
#endif

    // Check the return value
    if (nWritten == (size_t)-1)
    {
#ifdef WIN32
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
#else
        if (errno != EAGAIN && errno != EINTR) {
#endif
            // Something has gone horribly wrong
            port_close(self);
            return E_CLOSED;
        }

        // Write would block - try again later
        nWritten = 0;
    }

    if (self->written + (long)nWritten < nLen) {
        // Could not write all the data
        self->written += nWritten;

        // Add the socket to the write handle set
        // Must do this every time we leave outstanding data
        FD_SET( self->fd, &m_fdsWriteHandles );
        m_hasWriters++;

        //printf( "** added writer, now has %d writers.\n", m_hasWriters );

        return E_FALSE;
    }

    // All data successfully written
    self->written = 0;

    return E_SUCCESS;
}

RESULT port_close( portp self )
{
    if (!IsClosed(self))
    {
#ifdef WIN32
        // Close the socket
        FD_CLR( self->fd, &m_fdsHandles );
        FD_CLR( self->fd, &m_fdsWriteHandles );
        closesocket( self->fd );
#else
        FD_CLR( self->fd, &m_fdsHandles );
        FD_CLR( self->fd, &m_fdsWriteHandles );
        close( self->fd );
#endif
    }

    // Mark the port as closed
    self->fd = INVALID_SOCKET;

    return E_SUCCESS;
}


// Private implementation

RESULT port_read_data( portp self )
{
    size_t nRead;

    // Check the socket for new data
    if (!IsClosed(self)) {
        int32_t avail = self->buf.cap - self->buf.pos;
        if (avail < 0) return E_FALSE;

#ifdef WIN32
        // Read from the socket
        nRead = recv( self->fd, self->buf.data + self->buf.pos, avail, 0);
#else
        // Read any new data from the port
        nRead = read( self->fd, self->buf.data + self->buf.pos, avail );
#endif

        // Check the return value
        if (nRead == 0) port_close(self);
        else if(nRead == (size_t)-1) {
#ifdef WIN32
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
#else
            if (errno != EAGAIN && errno != EINTR) {
#endif
                // Something has gone horribly wrong
                port_close(self);
                return E_CLOSED;
            }

            // Read would block - try again later
            nRead = 0;
        } else {
            self->buf.len += (int32_t)nRead;
        }
    }

    return E_SUCCESS;
}

RESULT port_init_socket( portp self )
{
    // Make sure out-of-band data stays that way
    int nOption = 0;
    if( setsockopt( self->fd, SOL_SOCKET, SO_OOBINLINE,
        (char*)&nOption, sizeof(nOption) ) == -1 )
    {
        port_close(self);
        return E_SOCKET;
    }

    // Set non-blocking flag so we can poll/select
#ifdef WIN32
    {
        unsigned long nValue = 1;
        if( ioctlsocket( self->fd, FIONBIO, &nValue ) != 0 )
        {
            port_close(self);
            return E_SOCKET;
        }
    }
#else
    if( fcntl( self->fd, F_SETFL, O_NONBLOCK ) == -1 )
    {
        port_close(self);
        return E_SOCKET;
    }
#endif

    // Add the socket to the handle set
    FD_SET( self->fd, &m_fdsHandles );

    return E_SUCCESS;
}


// Persistent storage management

void port_persist( portp self, persistp p )
{
    // Write the reference count
    persist_write_in32(p, self->refs);
}

void port_restore( portp self, persistp p )
{
    // Read the reference count
    persist_read_int32(p, &self->refs);
}
