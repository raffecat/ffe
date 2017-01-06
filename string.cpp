// string.cpp
// FireFly server 'string' data type

#include "string.h"
#include "database.h"
#include "array.h"

static const char* c_pHexChars = "0123456789abcdef";

arrayp string_t::Split(stringp sDelim, int32 limit, bool collapse) const
{
	if (limit <= 0) limit = 2147483647; // max positive int32.

	str_size start = 0;
	str_size nDelimLen = sDelim ? sDelim->m_str.length() : 1;
	const char* szDelim = sDelim ? sDelim->m_str.c_str() : " ";

	arrayp arResult = new array_t;

    // Don't get stuck searching for nothing.
    if (nDelimLen != 0) {
        // Find all occurences of the sFind string.
        int32 found = 0;
        while (found < limit) {
            str_size pos = m_str.find( szDelim, start );
            if (pos == std_str::npos) break;

            // Ignore empty spans if asked to collapse.
            int32 nLen = (int32)pos - (int32)start;
            if (nLen > 0 || !collapse) {
                // Append the span to the result.
                stringp sValue = (nLen > 0) ? new string_t(m_str.c_str() + start, nLen) : new string_t();
                arResult->AppendItem(any_t(sValue)); // new ref
                sValue->Release();
				ASSERT(sValue->GetRefCount() > 0);
                found++;
            }

            start = pos + nDelimLen;
        }
    }

	// Append the rest of the string.
	int32 nLen = (int32)m_str.length() - (int32)start;
	if (nLen > 0) {
		stringp sValue = new string_t(m_str.c_str() + start, nLen);
		arResult->AppendItem(any_t(sValue)); // new ref
		sValue->Release();
		ASSERT(sValue->GetRefCount() > 0);
	}

	return arResult;
}

static char szHex[5] { '\\', 'x', '0', '0', '\0' };

stringp string_t::Encode() const
{
	stringp sResult = new string_t( "\"" );
	const byte* szStr = (const byte*) m_str.c_str();
	 
	while (*szStr) {
		// Find the next non-printable character
		const byte* szStart = szStr;
		while (*szStr >= 32 && *szStr < 127 && *szStr != '"' && *szStr != '\\') szStr++;
		if (szStr > szStart) sResult->m_str.append( (const char*)szStart, (str_size)(szStr - szStart) );

		// Encode a non-printable character
		byte ch = *szStr++;
		if (!ch) break; // end of string
		switch (ch) {
			case '"': sResult->m_str.append( "\\\"" ); continue;
			case '\\': sResult->m_str.append( "\\\\" ); continue;
			case '\n': sResult->m_str.append( "\\n" ); continue;
			case '\r': sResult->m_str.append( "\\r" ); continue;
			case '\t': sResult->m_str.append( "\\t" ); continue;
			case '\f': sResult->m_str.append( "\\f" ); continue;
			case '\v': sResult->m_str.append( "\\v" ); continue;
			case '\b': sResult->m_str.append( "\\b" ); continue;
			default: {
				szHex[2] = c_pHexChars[ch >> 4];
				szHex[3] = c_pHexChars[ch & 15];
				sResult->m_str.append( szHex );
				continue;
			}
		}
	}

	sResult->m_str.append( "\"" );

	return sResult;
}

// Persistent storage management

void string_t::Persist()
{
	// Write the reference count
	g_oDatabase.WriteLong( m_nRef );

	// Write the string length
	long nLength = m_str.length();
	g_oDatabase.WriteLong( nLength );

	// Write the string data
	const byte *pBuffer = (const byte*)m_str.c_str();
	g_oDatabase.WriteBytes( pBuffer, nLength );
}

void string_t::Restore()
{
	// Read the reference count
	m_nRef = g_oDatabase.ReadLong();

	// Read the string length
	uint32 nLength = g_oDatabase.ReadLong();

	// Allocate the string buffer
	m_str.resize( nLength );

	// Read the string data into the string
	byte* pBuffer = (byte*) const_cast<char*>( m_str.data() );
	g_oDatabase.ReadBytes( pBuffer, nLength );
}
