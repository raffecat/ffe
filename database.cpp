// database.cpp
// FireFly server data store interface

#include "database.h"
#include "any.h"
#include "index.h"
#include "string.h"
#include "array.h"
#include "mapping.h"
#include "object.h"
#include "function.h"
#include "port.h"
#include "thread.h"

// Global class instances
database g_oDatabase;
index_t<string_t> g_oIdxString;
index_t<array_t> g_oIdxArray;
index_t<mapping_t> g_oIdxMapping;
index_t<object_t> g_oIdxObject;
index_t<function_t> g_oIdxFunction;
index_t<port_t> g_oIdxPort;
index_t<thread_t> g_oIdxThread;

// Private constants
static const byte c_szFireFlyMagic[4] = DATABASE_MAGIC;
static const uint32 c_nFireFlyFileVersion = DATABASE_VERSION;
static const uint32 c_nFireFlyFileVersion_0 = DATABASE_VERSION_0;

static const char* c_szStringIndexMarker = "{:STRI:}";
static const char* c_szArrayIndexMarker = "{:ARRI:}";
static const char* c_szMappingIndexMarker = "{:MAPI:}";
static const char* c_szObjectIndexMarker = "{:OBJI:}";
static const char* c_szFunctionIndexMarker = "{:FUNI:}";
static const char* c_szThreadIndexMarker = "{:THRI:}";
static const char* c_szPortIndexMarker = "{:PRTI:}";

static const char* c_szStringDataMarker = "{:STRD:}";
static const char* c_szArrayDataMarker = "{:ARRD:}";
static const char* c_szMappingDataMarker = "{:MAPD:}";
static const char* c_szObjectDataMarker = "{:OBJD:}";
static const char* c_szFunctionDataMarker = "{:FUND:}";
static const char* c_szThreadDataMarker = "{:THRD:}";
static const char* c_szPortDataMarker = "{:PRTD:}";


// Construction, Destruction

database::database() : m_oRoot(NULL), m_thMaster(NULL), m_arDispatch(NULL),
	m_nNextClone(1), m_nVer(c_nFireFlyFileVersion), m_fd(-1)
{
}

database::~database()
{
}

// Database management

RESULT database::CheckMasterThread()
{
	return SpawnSystemThread( "master", m_thMaster );
}

RESULT database::SpawnSystemThread( const char* szName, threadp& thThread )
{
	// Check if the thread has stopped
	if( !thThread || thThread->IsStopped() )
	{
		// Release the existing thread
		if( thThread )
		{
			m_arDispatch->RemoveItem( any_t(thThread) );
			thThread->Release();
			thThread = NULL;
		}

		// Find the specified function
		stringp sName = new string_t( szName );
		any_t mFunction = m_oRoot->Lookup(sName); // borrow
		sName->Release();
		if (!mFunction.IsFunction()) return E_NOT_DEFINED;

		// Create a new thread
		thThread = new thread_t; // byref argument is a ref
		if( !thThread ) return E_MEMORY;

		// Start the thread
		thThread->SetupThread( functionp( mFunction ), m_oRoot, NULL );

		// Add the thread to the dispatch list
		m_arDispatch->AppendItem(any_t(thThread)); // new ref
	}

	return E_SUCCESS;
}

void database::RecoverLostValue( any_t mValue )
{
	stringp sName = new string_t( "recovered" );
	any_t mContainer = m_oRoot->Lookup(sName); // borrow
	sName->Release();
	if (mContainer.IsObject()) {
		objectp container = objectp(mContainer);

		// Special handling for recovered objects
		if (mValue.IsChild()) {
			// Make the 'recovered' container the object's parent
			objectp(mValue)->SetParent(container);
		}

		// Find a unique key name
		stringp sName = container->UniqueAttribName(NULL);
		if (sName) {
			// Add the recovered value to the container using the key
			if (container->Assign(sName, mValue)) { // new ref.
				mValue.Release(); // leave only the recovered ref
			}
			sName->Release();
		}
	}
}

RESULT database::FindCreateObject( stringp sPath, objectp oBase, objectp oOwner, bool bCreate, objectp& oResult )
{
	bool bWasCreated = false;
	char* szPath = const_cast<char*>(sPath->CString()); // for strchr
	objectp oTarget;

	// Check if the path is absolute
	if( *szPath == '$' )
	{
		// Start from the root
		oTarget = g_oDatabase.RootObject(); // borrow
		szPath++;
		if( *szPath == '.' ) szPath++;
	}
	else
	{
		// Start from the base object or the root
		if( !oBase ) oBase = g_oDatabase.RootObject(); // borrow
		oTarget = oBase;
	}

	if( !*szPath || ( *szPath == '.' && !szPath[1] ) )
	{
		oResult = oTarget; // for root path ("$"), this path (".") or empty string
		return bCreate ? E_FALSE : E_SUCCESS; // not created
	}

	stringp sName = NULL;
	int nLen;
	while( szPath )
	{
		// Look for the next dot
		char* szEnd = strchr( szPath, '.' );
		if( szEnd )
		{
			nLen = szEnd - szPath;
			if( *szEnd == '.' ) szEnd++;
		}
		else nLen = 0; // rest of string

		// Check for a 'parent' symbol
		if( *szPath == '^' && (nLen == 1 || szPath[1] == 0) )
		{
			oTarget = oTarget->GetParent();
			if( !oTarget ) oTarget = g_oDatabase.RootObject();
		}
		else
		{
			// Check for a clone id
			unsigned long nCloneId = 0;
			char* szHash = strchr( szPath, '#' );
			if( szHash && (!szEnd || szHash < szEnd) )
			{
				nLen = szHash - szPath;
				sscanf( szHash + 1, "%lu", &nCloneId );
			}

			// Build a string from this segment
			if( nLen ) sName = new string_t( szPath, nLen );
			else sName = new string_t( szPath );

			// Look up the object
			any_t mChild = oTarget->Lookup(sName); // borrow
			if (!mChild.IsNull())
			{
				if( mChild.IsObject() )
				{
					oTarget = (objectp)mChild; // borrow

					if( nCloneId )
					{
						// Look up the specified clone
						objectp oClone = oTarget->FindClone( (uint32)nCloneId ); // borrow
						if (!oClone)
						{
							oResult = NULL;
							sName->Release();
							return bCreate ? E_OBJECT : E_FALSE;
						}
						oTarget = oClone;
					}
				}
				else
				{
					// The path exists and is not an object
					oResult = NULL;
					sName->Release();
					return bCreate ? E_ATTRIBUTE_EXISTS : E_FALSE;
				}
			}
			else if( bCreate )
			{
				// Check if the target is a clone
				if( oTarget->IsClone() )
				{
					// Cannot create objects inside clones
					oResult = NULL;
					sName->Release();
					return E_CLONE;
				}

				// Make sure the object has an owner
				if( !oOwner ) oOwner = g_oDatabase.RootObject();

				// Create the new object
				objectp oNew = NULL;
				RESULT nResult = oTarget->CreateObject( sName, oOwner, oNew ); // borrow
				if( ISERROR(nResult) )
				{
					oResult = NULL;
					sName->Release();
					return nResult;
				}

				oTarget = oNew; // borrow
				bWasCreated = true;
			}
			else
			{
				// Cannot find the object
				oResult = NULL;
				sName->Release();
				return E_FALSE;
			}

			sName->Release();
		}

		szPath = szEnd;
	}

	oResult = oTarget; // borrow (always)

	if( bCreate ) return bWasCreated ? E_SUCCESS : E_FALSE;
	return E_SUCCESS;
}

RESULT database::Restore( const char* szFilename )
{
	// Open the database file
	m_fd = OPEN( szFilename, O_RDONLY | O_BINARY );

	if( m_fd > 0 )
	{
		printf("init: opening database...\n");

		// Read database header
		byte pSig[4];
		ReadBytes( pSig, 4 );
		if (MEMCMP(pSig, c_szFireFlyMagic, 4) != 0)
		{
			printf("init: file is not a FireFly database.\n");
			return E_DATABASE;
		}

		// Read database version
		m_nVer = ReadLong();
		if( m_nVer < c_nFireFlyFileVersion_0 )
		{
			printf("init: file version is too old.\n");
			return E_DATABASE;
		}
		if( m_nVer > c_nFireFlyFileVersion )
		{
			printf("init: file version is too new.\n");
			return E_DATABASE;
		}

		// Read the system master references
		objectp oRoot = (objectp)ReadLong();
		threadp thMaster = (threadp)ReadLong();
		arrayp arDispatch = (arrayp)ReadLong();

		// Read the next clone id counter
		if( m_nVer > c_nFireFlyFileVersion_0 )
			m_nNextClone = ReadLong();

		// Restore index tables for all values
		g_oIdxString.RestoreIndex( c_szStringIndexMarker );
		g_oIdxArray.RestoreIndex( c_szArrayIndexMarker );
		g_oIdxMapping.RestoreIndex( c_szMappingIndexMarker );
		g_oIdxObject.RestoreIndex( c_szObjectIndexMarker );
		g_oIdxFunction.RestoreIndex( c_szFunctionIndexMarker );
		g_oIdxThread.RestoreIndex( c_szThreadIndexMarker );
		g_oIdxPort.RestoreIndex( c_szPortIndexMarker );

		// Restore data for all values
		// Must restore strings first since they alone use value semantics
		// and so their values may be required when restoring other types
		// (eg. when used as the key in a mapping)
		printf( "init: restoring string data...\n" );
		g_oIdxString.RestoreData( c_szStringDataMarker );
		printf( "init: restoring array data...\n" );
		g_oIdxArray.RestoreData( c_szArrayDataMarker );
		printf( "init: restoring mapping data...\n" );
		g_oIdxMapping.RestoreData( c_szMappingDataMarker );
		printf( "init: restoring object data...\n" );
		g_oIdxObject.RestoreData( c_szObjectDataMarker );
		printf( "init: restoring function data...\n" );
		g_oIdxFunction.RestoreData( c_szFunctionDataMarker );
		printf( "init: restoring thread data...\n" );
		g_oIdxThread.RestoreData( c_szThreadDataMarker );
		printf( "init: restoring port data...\n" );
		g_oIdxPort.RestoreData( c_szPortDataMarker );

		// Resolve the system master references
		printf( "init: resolving master references...\n" );
		m_oRoot = g_oIdxObject.ResolvePointer( oRoot );
		m_thMaster = g_oIdxThread.ResolvePointer( thMaster );
		m_arDispatch = g_oIdxArray.ResolvePointer( arDispatch );

		// We can recover if m_thMaster or m_arDispatch are lost,
		// but not if we lose track of the root object.
		if (!m_oRoot) {
			printf( "init: cannot find the root object (fatal!)\n" );
			exit(1);
		}
		if (!m_arDispatch) m_arDispatch = new array_t;

		// Validate all references after restore
		printf( "init: performing string integrity check...\n" );
		g_oIdxString.ValidateIndex();
		printf( "init: performing array integrity check...\n" );
		g_oIdxArray.ValidateIndex();
		printf( "init: performing mapping integrity check...\n" );
		g_oIdxMapping.ValidateIndex();
		printf( "init: performing object integrity check...\n" );
		g_oIdxObject.ValidateIndex();
		printf( "init: performing function integrity check...\n" );
		g_oIdxFunction.ValidateIndex();
		printf( "init: performing thread integrity check...\n" );
		g_oIdxThread.ValidateIndex();
		printf( "init: performing port integrity check...\n" );
		g_oIdxPort.ValidateIndex();

		printf( "init: validating all objects...\n" );
		g_oIdxObject.ValidateObjects();

		// Close the database file
		CLOSE( m_fd );
	}
	else
	{
		printf("init: creating database...\n");

		// Create a new database
		m_oRoot = new object_t;
		m_oRoot->SetupRoot();
		m_arDispatch = new array_t;
	}

	printf("init: done\n");

	return E_SUCCESS;
}

RESULT database::Persist( const char* szFilename, const char* szBackup )
{
	// Remove any existing backup file
	UNLINK( szBackup );

	// Rename the current database as the backup
	rename( szFilename, szBackup );

	// Create the database file
	m_fd = OPEN( szFilename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IRUSR | S_IWUSR );

	if( m_fd > 0 )
	{
//printf("persist: beginning checkpoint...\n");

		// Write database header
		WriteBytes( (const byte*)c_szFireFlyMagic, 4 );

		// Write database version
		WriteLong( c_nFireFlyFileVersion );

		// Write the system master references
		WriteLong( (uint32)m_oRoot );
		WriteLong( (uint32)m_thMaster );
		WriteLong( (uint32)m_arDispatch );

		// Write the next clone id counter
		WriteLong( m_nNextClone );

		// Persist index tables for all values
		g_oIdxString.PersistIndex( c_szStringIndexMarker );
		g_oIdxArray.PersistIndex( c_szArrayIndexMarker );
		g_oIdxMapping.PersistIndex( c_szMappingIndexMarker );
		g_oIdxObject.PersistIndex( c_szObjectIndexMarker );
		g_oIdxFunction.PersistIndex( c_szFunctionIndexMarker );
		g_oIdxThread.PersistIndex( c_szThreadIndexMarker );
		g_oIdxPort.PersistIndex( c_szPortIndexMarker );

		// Persist data for all values
		// Must persist strings first since they alone use value semantics
		// and so their values may be required when restoring other types
		// (eg. when used as the key in a mapping)
		g_oIdxString.PersistData( c_szStringDataMarker );
		g_oIdxArray.PersistData( c_szArrayDataMarker );
		g_oIdxMapping.PersistData( c_szMappingDataMarker );
		g_oIdxObject.PersistData( c_szObjectDataMarker );
		g_oIdxFunction.PersistData( c_szFunctionDataMarker );
		g_oIdxThread.PersistData( c_szThreadDataMarker );
		g_oIdxPort.PersistData( c_szPortDataMarker );

		// Close the database file
		CLOSE( m_fd );

		time_t tmNow = time(NULL);
		printf("persist: checkpoint complete %.19s\n", ctime(&tmNow) );
	}
	else
	{
		printf("persist: failed to write to database file\n\n");
		return E_DATABASE;
	}

	return E_SUCCESS;
}
