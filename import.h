// import.h
// FireFly server import/export module

#ifndef FIREFLY_IMPORT_H
#define FIREFLY_IMPORT_H

#include "global.h"
#include "any.h"
#include "index.h"

#define BUFFER_SIZE 4096 // assumed in fscanf call

class import_t
{
public:
	import_t();
	~import_t();

// Public interface
public:
	RESULT Import( const char* szFilename );
	RESULT Export( const char* szFilename );

// Callback
public:
	void WriteExport(stringp sOutput);
	void WriteExportBreak();

protected:
//	RESULT DoExport(arrayp& arSchedule);

// Private member variables
protected:
    FILE* m_pFile;
	char m_pBuffer[BUFFER_SIZE + 1];
};

// Construction, Destruction

inline import_t::import_t()
{
}

inline import_t::~import_t()
{
}

#endif // FIREFLY_IMPORT_H
