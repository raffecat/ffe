// function.cpp
// FireFly server 'function' data type

#include "function.h"
#include "database.h"
#include "string.h"
#include "array.h"

// Interface to compiler
RESULT ParseFunction( stringp sSource, stringp& sErrors, functionp fnResult );


// Construction, Destruction

function_t::~function_t()
{
#ifdef DEBUG_REF
	printf( "[function %08x destroy: %d refs]\n", this, m_nRef );
#endif
	g_oIdxFunction.Remove( this );

	// Free program memory
	if( m_sSource ) m_sSource->Release();
	if( m_sName ) m_sName->Release();
	if( m_arConstants ) m_arConstants->Release();
	if( m_pCode ) FREE( m_pCode );
	if( m_pLines ) FREE( m_pLines );
}

// Persistent storage management

void function_t::Persist()
{
	// Write the reference count
	g_oDatabase.WriteLong( m_nRef );

	// Save the useful bits first
	g_oDatabase.WriteLong( (uint32)m_sSource );
	g_oDatabase.WriteLong( (uint32)m_sName );
	g_oDatabase.WriteLong( (uint32)m_arConstants );
	g_oDatabase.WriteLong( m_nArgs );
	g_oDatabase.WriteLong( m_nLocals );
	g_oDatabase.WriteLong( m_nCodeSize );
	g_oDatabase.WriteLong( m_nLineSize );

	// Save code and line data
	if( m_nCodeSize ) g_oDatabase.WriteBytes( (byte*)m_pCode, m_nCodeSize * sizeof(uint32) );
	if( m_nLineSize ) g_oDatabase.WriteBytes( (byte*)m_pLines, m_nLineSize * sizeof(int32) );
}

void function_t::Restore()
{
	// Read the reference count
	m_nRef = g_oDatabase.ReadLong();

	// Restore the useful bits first
	m_sSource = g_oIdxString.ResolvePointer( (stringp)g_oDatabase.ReadLong() );
	m_sName = g_oIdxString.ResolvePointer( (stringp)g_oDatabase.ReadLong() );
	m_arConstants = g_oIdxArray.ResolvePointer( (arrayp)g_oDatabase.ReadLong() );

	// Recover from lost references.
	if (!m_sSource) m_sSource = new string_t("missing source");
	if (!m_sName) m_sName = new string_t("recovered");
	if (!m_arConstants) {
		// Constant access is range-checked, so this will do.
		m_arConstants = new array_t;
	}

	m_nArgs = g_oDatabase.ReadLong();
	m_nLocals = g_oDatabase.ReadLong();
	m_nCodeSize = g_oDatabase.ReadLong();
	m_nLineSize = g_oDatabase.ReadLong();

	// Restore code and line data
	if( m_nCodeSize )
	{
		m_pCode = new uint32[m_nCodeSize];
		ASSERT( m_pCode != NULL );
		g_oDatabase.ReadBytes( (byte*)m_pCode, m_nCodeSize * sizeof(uint32) );
	}
	if( m_nLineSize )
	{
		m_pLines = new int32[m_nLineSize];
		ASSERT( m_pLines != NULL );
		g_oDatabase.ReadBytes( (byte*)m_pLines, m_nLineSize * sizeof(int32) );
	}

	// Ensure we build a valid function.
	if (!m_pCode) { m_pCode = new uint32[1]; m_pCode[0] = 0; }
	if (!m_pLines) { m_pLines = new int32[1]; m_pLines[0] = 0; }
}
