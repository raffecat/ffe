// main.cpp
// FireFly server main program

#include "global.h"
#include "database.h"
#include "import.h"
#include "port.h"
#include "array.h"
#include "thread.h"
#include "index.h"

// Time between checkpoints in minutes
#define CHECKPOINT_DELAY		30

import_t g_oImport;
bool g_bRunning;

static time_t g_tmNextCheckpoint = 0;

// Main module constants
static const char* c_szDatabase = "persist.db";
static const char* c_szBackupDatabase = "previous.db";
static const char* c_szCrashDatabase = "crash.db";


void FatalError( RESULT nResult )
{
	// Display a fatal error
	printf( "fatal error: %d, shutting down.\n", (int)nResult );

//return; // don't write db for debugging

	// Checkpoint (save) the database
	nResult = g_oDatabase.Persist( c_szDatabase, c_szCrashDatabase );
	if( ISERROR(nResult) )
	{
		printf( "fatal error writing crash database: %d.\n", (int)nResult );
	}

	// Exit the program immediately
//getchar();
	exit( 1 );
}

// Terminate signal handler
void SigTerminate( int sig )
{
	// Reinstall signal handlers for Terminate signals
	signal( SIGINT, SigTerminate );
	signal( SIGTERM, SigTerminate );
#ifndef WIN32
	signal( SIGQUIT, SigTerminate );
#endif

	printf( "main: received terminate signal, shutting down.\n" );

	// Clear the global 'running' flag to initiate shutdown
	g_bRunning = false;
}

// Checkpoint signal handler
void SigCheckpoint( int sig )
{
	printf( "main: received checkpoint signal, scheduling checkpoint.\n" );

	// Set the next checkpoint time to now
	g_tmNextCheckpoint = time(NULL);
}

RESULT RunSynchronousFunction( const char* szName )
{
	// Create a thread for the utility function
	threadp thUtil = NULL;
	RESULT nResult = g_oDatabase.SpawnSystemThread( szName, thUtil );
	if( ISERROR(nResult) && nResult != E_NOT_DEFINED )
	{
		printf( "ERROR: failed to spawn the '%s' thread\n", szName );
		FatalError( nResult );
	}

	if( thUtil )
	{
		// Release the reference returned from SpawnSystemThread
		// This leaves only the reference in the Dispatch List
		thUtil->Release();

		// Execute the utility thread to completion
		do
		{
			// Execute the thread for its quantum
			int nRemain = 0;
			nResult = thUtil->Execute( nRemain );
			if( ISERROR(nResult) ) FatalError( nResult );
		}
		while( nResult != E_STOPPED );

		// Remove the thread from the dispatch list (and delete)
		g_oDatabase.DispatchList()->RemoveItem( any_t(thUtil) );
	}

	return E_SUCCESS;
}

int main( int argc, char* argv[] )
{
	RESULT nResult;

	// Display the startup message
	printf(FIREFLY_VERSION_STRING " (prototype).\nThis software developed by Andrew Towers (mariofrog@bigpond.com)\n");
#ifdef HEADER_DES_H
	printf("This product includes cryptographic software written by Eric Young (eay@cryptsoft.com)\n");
#endif

	// Check the size of basic data types
	if( sizeof(uint32) != 4 || sizeof(float64) != 8 )
	{
		printf( "fatal error: basic data sizes are incorrect, shutting down.\n" );
		exit( 2 );
	}

	// Check the processor endian-ness (database is big-endian (Intel))
	uint32 testVal = 0x12345678;
	byte* testBytes = (byte*)(&testVal);
	if( testBytes[0] != 0x78 || testBytes[1] != 0x56 || testBytes[2] != 0x34 || testBytes[3] != 0x12 )
	{
		printf( "fatal error: database endian-ness mismatch, shutting down.\n" );
		exit( 2 );
	}

	// Initialise the ports module
	nResult = port_t::Initialise();
	if( ISERROR(nResult) ) FatalError( nResult );

	// Seed the random number generator
	srand( (unsigned int) -time(NULL) );

	// Restore the database
	nResult = g_oDatabase.Restore( c_szDatabase );
	if( ISERROR(nResult) ) FatalError( nResult );

	// Check for import/export command
	bool bNoStart = false;
	if( argc == 3 )
	{
		if( strcmp( argv[1], "-i" ) == 0 )
		{
			nResult = g_oImport.Import( argv[2] );
			if( ISERROR(nResult) ) FatalError( nResult );
			bNoStart = true;
		}
		else if( strcmp( argv[1], "-e" ) == 0 )
		{
			nResult = g_oImport.Export( argv[2] );
			if( ISERROR(nResult) ) FatalError( nResult );
			bNoStart = true;
		}
	}

	if( !bNoStart )
	{
//printf( "+++ Ready to start interpreter, press enter.\n" );
//getchar();

		// Install signal handlers for Terminate signals
		signal( SIGINT, SigTerminate );
		signal( SIGTERM, SigTerminate );
#ifndef WIN32
		signal( SIGQUIT, SigTerminate );

		// Install a user handler for causing a checkpoint
		signal( SIGUSR1, SigCheckpoint );
#endif

		// Set up next checkpoint
		g_tmNextCheckpoint = time(NULL) + (CHECKPOINT_DELAY * 60);

		// Run the startup function
		nResult = RunSynchronousFunction( "startup" );
		if( ISERROR(nResult) ) FatalError( nResult );

		// Main loop, time-share between the threads
		printf("main: starting threads.\n");
		g_bRunning = true;
		arrayp arThreads = g_oDatabase.DispatchList();
		while( g_bRunning )
		{
			// Check that the master thread is running
			nResult = g_oDatabase.CheckMasterThread();
			if( ISERROR(nResult) )
			{
				printf( "ERROR: failed to spawn the master thread\n" );
				FatalError( nResult );
			}

			// Check when the next checkpoint is due
			time_t tmNow = time(NULL);
			time_t tmRemain = g_tmNextCheckpoint - tmNow;
			if( tmRemain <= 0 )
			{
				// Checkpoint (save) the database
				nResult = g_oDatabase.Persist( c_szDatabase, c_szBackupDatabase );
				if( ISERROR(nResult) ) FatalError( nResult );

				// Set up next checkpoint
				tmRemain = CHECKPOINT_DELAY * 60;
				g_tmNextCheckpoint = tmNow + tmRemain;
			}

			// Execute each active thread until it yields.
			int nTimeout = (int)tmRemain;
			for( int32 nThread = 0; nThread < arThreads->Size() && g_bRunning; )
			{
				// Extract the next thread from the dispatch list
				const any_t& mThread = arThreads->Lookup(nThread); // borrow
				threadp thNext = threadp(mThread);
				// Execute the thread until it yields
				int nRemain = 0;
				nResult = thNext->Execute( nRemain );
				if( ISERROR(nResult) ) FatalError( nResult );
				// Check if the thread has stopped
				if( nResult == E_STOPPED ) arThreads->RemoveItem( any_t(thNext) );
				else
				{
					nThread++;
					// Adjust the sleep time
					if( nResult == E_SUCCESS ) nTimeout = 0;
					else if( nRemain && nRemain < nTimeout ) nTimeout = nRemain;
				}
			}

//printf( "SELECT with %d threads running\n", (int)arThreads->Size() );

            // Poll the ports for activity
			if( g_bRunning )
			{
				nResult = port_t::Select( nTimeout );
				if( ISERROR(nResult) ) FatalError( nResult );
			}
		}
		printf("main: shutting down.\n");

		// Run the shutdown function
		nResult = RunSynchronousFunction( "shutdown" );
		if( ISERROR(nResult) ) FatalError( nResult );
	}

//return 0; // don't write db for debugging

	printf("main: performing final checkpoint.\n");

	// Checkpoint (save) the database
	nResult = g_oDatabase.Persist( c_szDatabase, c_szBackupDatabase );
	if( ISERROR(nResult) )
	{
		printf( "fatal error writing database: %d.\n", (int)nResult );
	}

	// Shut down the ports module
	nResult = port_t::Shutdown();
	if( ISERROR(nResult) )
	{
		printf( "fatal error shutting down ports: %d.\n", (int)nResult );
	}

	printf("main: shutdown successful.\n");
//getchar();

	return 0;
}
