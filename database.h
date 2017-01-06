// database.h
// FireFly server data store interface

#ifndef FIREFLY_DATABASE_H
#define FIREFLY_DATABASE_H

#include "global.h"
#include "any.h"

class database  
{
public:
	database();
	~database();

// Database management
public:
	RESULT Persist( const char* szFilename, const char* szBackup );
	RESULT Restore( const char* szFilename );
	inline objectp RootObject();
	inline threadp MasterThread();
	inline arrayp DispatchList();
	RESULT CheckMasterThread();
	RESULT SpawnSystemThread( const char* szName, threadp& thThread );
	void RecoverLostValue( any_t mValue );
	RESULT FindCreateObject( stringp sPath, objectp oBase, objectp oOwner,
		bool bCreate, objectp& oResult );

	inline uint32 GetVersion() { return m_nVer; }
	inline uint32 AllocateCloneId() { return m_nNextClone++; }

// Data access functions
public:
	inline uint32 ReadLong();
	inline void WriteLong( uint32 nValue );
	inline void ReadBytes( byte* pBuffer, uint32 nLength );
	inline void WriteBytes( const byte* pBuffer, uint32 nLength );

// Private member variables
protected:
	objectp m_oRoot;
	threadp m_thMaster;
	arrayp m_arDispatch;
	uint32 m_nNextClone;
	uint32 m_nVer;
	int m_fd;
};

inline objectp database::RootObject()
{
	return m_oRoot;
}

inline threadp database::MasterThread()
{
	return m_thMaster;
}

inline arrayp database::DispatchList()
{
	return m_arDispatch;
}

// Data access functions

inline uint32 database::ReadLong()
{
	uint32 nValue;

	// Read an unsigned 32 bit integer
	READ( m_fd, &nValue, sizeof(uint32) );

	return nValue;
}

inline void database::WriteLong( uint32 nValue )
{
	// Write an unsigned 32 bit integer
	WRITE( m_fd, &nValue, sizeof(uint32) );
}

inline void database::ReadBytes( byte* pBuffer, uint32 nLength )
{
	// Read an array of bytes
	READ( m_fd, pBuffer, nLength );
}

inline void database::WriteBytes( const byte* pBuffer, uint32 nLength )
{
	// Write an array of bytes
	WRITE( m_fd, pBuffer, nLength );
}

#endif // FIREFLY_DATABASE_H
