// import.cpp
// FireFly server import/export module

#include "import.h"
#include "database.h"
#include "string.h"
#include "array.h"
#include "object.h"
#include "any_impl.h"
#include "parse.h"


// Public interface

static char buf[4096+1];

RESULT import_t::Import( const char* szFilename )
{
	// Read the import file into a buffer.
	int fd = OPEN( szFilename, O_RDONLY | O_BINARY );
	if (fd <= 0) {
		printf("import: failed to open import file.\n");
		return E_INTERNAL;
	}
	stringp source = new string_t();
	for (;;) {
		int n = READ( fd, buf, sizeof(buf)-1 );
		if (n < 1) break;
		buf[n] = '\0';
		source->AppendCStr(buf);
	}
	CLOSE(fd);

	// Parse the file and import everything.
	any_t mResult;
	RESULT nResult = Parse( source, true, mResult );
	if (ISERROR(nResult)) {
		printf( "import: error parsing import file: %d\n", (int)nResult );
	} else {
		// Display any errors that were returned.
		if (mResult.IsString()) {
			puts(stringp(mResult)->CString());
		}
	}

	// Clean up.
	source->Release();
	printf("import: done.\n");
	return E_SUCCESS;
}

RESULT import_t::Export( const char* szFilename )
{
	m_pFile = fopen( szFilename, "wt" );
	if (m_pFile) {
		printf("export: exporting database...\n");

		g_oDatabase.RootObject()->Export();

		fclose(m_pFile);
		m_pFile = NULL;

		printf("export: done.\n");
	}
	else
	{
		printf("export: failed to open output file.\n");
	}

	return E_SUCCESS;
}

void import_t::WriteExport(stringp sOutput)
{
	if (m_pFile)
	{
		fwrite( sOutput->CString(), 1, sOutput->Length(), m_pFile );
	}
}

void import_t::WriteExportBreak()
{
	fwrite( "\n", 1, 1, m_pFile );

	printf("."); // show progress.
}
