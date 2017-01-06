// string.h
// FireFly server 'string' data type

#ifndef FIREFLY_STRING_H
#define FIREFLY_STRING_H

#include "global.h"
#include "index.h"

#include <string>

typedef std::basic_string<char> std_str;
typedef std_str::size_type str_size;

class string_t
{
private:
	string_t(const string_t&) = delete; // disallowed.

public:
	string_t();
	string_t(const char*);
	string_t(const char*, long);
	string_t(const std_str&); // const for new string_t(this) in const methods.
	string_t(int32);
	string_t(uint32);
	string_t(float64);
	inline string_t(const index_sentinel_t&) : m_nRef(0) {}
	~string_t();

// Public interface
	inline const char* CString() const;
	inline stringp Copy() const { return new string_t(m_str); }

	inline void Append( stringp sValue );
	inline void Prepend( stringp sValue );
	inline void AppendCStr( const char* szValue );
	inline void PrependCStr( const char* szValue );

	inline int Compare( stringp sValue ) const;
	inline int CompareCStr( const char* szValue ) const;
	inline int32 Search( stringp sFind, int32 nStart ) const;
	inline int32 ReverseSearch( stringp sFind, int32 nStart ) const;
	inline int32 Skip( stringp sChars, int32 nStart ) const;
	inline int32 SkipTo( stringp sChars, int32 nStart ) const;
	inline int32 SkipToFuncStart() const;
	inline stringp Replace( stringp sFind, stringp sReplace ) const;
	inline stringp SubString( int32 nStart, int32 nEnd ) const;
	inline stringp Capitalise() const;
	inline stringp ToUpper() const;
	inline stringp ToLower() const;
	inline stringp Trim();
	inline stringp Reverse() const;
	inline stringp ReplaceNewline() const;
	arrayp Split( stringp sDelim, int32 limit, bool collapse ) const;
	stringp Encode() const;
	inline stringp Crypt( stringp sSalt ) const;

	inline int32 Length() const { return (int32)m_str.length(); }
	inline bool Less(const stringp other) const { return m_str < other->m_str; }

	static inline stringp CTime(time_t nTime);
	static inline stringp FmtTime(stringp sFormat, time_t nTime);

// Persistent storage management
public:
	void Persist();
	void Restore();
	void Validate() { }

// Reference counting
public:
	inline void AddRef();
	inline void Release();

	inline int32 GetRefCount() const { return m_nRef; }
	inline void SetRefCount( int32 nRef ) { m_nRef = nRef; }

protected:
	std_str m_str;
    int32 m_nRef;
};

// Construction, Destruction

inline string_t::string_t() : m_nRef(1)
{
#ifdef DEBUG_REF
	printf( "[string %08x create: %d refs (%d)]\n", this, m_nRef, (int)bRegister );
#endif
	g_oIdxString.Insert( this );
}

inline string_t::string_t( const char* szSource ) :
	m_str( szSource ), m_nRef(1)
{
#ifdef DEBUG_REF
	printf( "[string %08x create: %d refs (%d)]\n", this, m_nRef, 1 );
#endif
	g_oIdxString.Insert( this );
}

inline string_t::string_t( const char* szSource, long nLen ) :
	m_str( szSource, nLen ), m_nRef(1)
{
#ifdef DEBUG_REF
	printf( "[string %08x create: %d refs (%d)]\n", this, m_nRef, 1 );
#endif
	g_oIdxString.Insert( this );
}

inline string_t::string_t( const std_str& str ) :
	m_str( str ), m_nRef(1)
{
#ifdef DEBUG_REF
	printf( "[string %08x create: %d refs (%d)]\n", this, m_nRef, 1 );
#endif
	g_oIdxString.Insert( this );
}

inline string_t::string_t( int32 nValue ) : m_nRef(1)
{
#ifdef DEBUG_REF
	printf( "[string %08x create: %d refs (%d)]\n", this, m_nRef, 1 );
#endif
	g_oIdxString.Insert( this );

	// Create the value
	char szValue[32];
	sprintf( szValue, "%ld", (long)nValue );
	m_str = szValue;
}

inline string_t::string_t( uint32 nValue ) : m_nRef(1)
{
#ifdef DEBUG_REF
	printf( "[string %08x create: %d refs (%d)]\n", this, m_nRef, 1 );
#endif
	g_oIdxString.Insert( this );

	// Create the value
	char szValue[32];
	sprintf( szValue, "%lu", (unsigned long)nValue );
	m_str = szValue;
}

inline string_t::string_t( float64 fValue ) : m_nRef(1)
{
#ifdef DEBUG_REF
	printf( "[string %08x create: %d refs]\n", this, m_nRef );
#endif
	g_oIdxString.Insert( this );

	// Create the value
	char szValue[64];
	sprintf( szValue, "%.6g", (double)fValue );
	m_str = szValue;
}

inline string_t::~string_t()
{
#ifdef DEBUG_REF
	printf( "[string %08x destroy: %d refs]\n", this, m_nRef );
#endif
	g_oIdxString.Remove( this );
}

// Public interface

inline const char* string_t::CString() const {
	return m_str.c_str();
}

inline void string_t::Append( stringp sValue )
{
	m_str.append( sValue->m_str );
}

inline void string_t::Prepend( stringp sValue )
{
	m_str.insert( 0, sValue->m_str );
}

inline void string_t::AppendCStr( const char* szValue )
{
	m_str.append( szValue );
}

inline void string_t::PrependCStr( const char* szValue )
{
	m_str.insert( 0, szValue );
}

inline int string_t::Compare( stringp sValue ) const
{
	return m_str.compare( sValue->m_str );
}

inline int string_t::CompareCStr( const char* szValue ) const
{
	return m_str.compare( szValue );
}

inline int32 string_t::Search( stringp sFind, int32 nStart ) const
{
	return m_str.find( sFind->m_str, nStart );
}

inline int32 string_t::ReverseSearch( stringp sFind, int32 nStart ) const
{
	return m_str.rfind( sFind->m_str, nStart );
}

inline int32 string_t::Skip( stringp sChars, int32 nStart ) const
{
	if( !sChars ) return m_str.find_first_not_of( " \r\n\t", nStart );
	return m_str.find_first_not_of( sChars->m_str, nStart );
}

inline int32 string_t::SkipTo( stringp sChars, int32 nStart ) const
{
  if( !sChars ) return m_str.find_first_of( " \r\n\t", nStart );
  return m_str.find_first_of( sChars->m_str, nStart );
}

inline int32 string_t::SkipToFuncStart() const
{
  return m_str.find_first_of( "(", 0 );
}

inline stringp string_t::Replace( stringp sFind, stringp sReplace ) const
{
	stringp result = Copy();
	std_str& buf = result->m_str;
	const std_str& find = sFind->m_str;
	const std_str& repl = sReplace->m_str;

    // Don't get stuck searching for nothing.
    if (find.length() == 0) return result;
    
	// Find all occurences of the string.
	str_size start = 0;
	str_size pos;
	while ((pos = buf.find(find, start)) != std_str::npos) {
		// Replace this occurence with the replace string.
		buf.replace(pos, find.length(), repl);
		start = pos + repl.length();
	}
	return result;
}

inline stringp string_t::ReplaceNewline() const
{
	stringp result = Copy();
	std_str& buf = result->m_str;

	// Replace all occurences of newline with Telnet newline.
	str_size start = 0;
	str_size pos;
	while ((pos = buf.find("\n", start)) != std_str::npos) {
		buf.replace(pos, 1, "\r\n");
		start = pos + 2;
	}
	return result;
}

inline stringp string_t::SubString(int32 nStart, int32 nEnd) const
{
	// Negative indexes are relative to the end.
	if (nStart < 0) nStart = m_str.length() + nStart;
	if (nEnd < 0) nEnd = m_str.length() + nEnd;

	// Determine the actual start and length.
	if( nStart < 0 ) nStart = 0;
	if( nEnd >= (int32)m_str.length() ) nEnd = (int32)m_str.length() - 1;
	int32 nLen = nEnd - nStart + 1;

	// Create the substring.
	if (nLen < 1) return new string_t();
	return new string_t(m_str.c_str() + nStart, nLen);
}

inline stringp string_t::Capitalise() const
{
	ASSERT(GetRefCount() > 0);
	stringp sResult = Copy();

	// Make the first letter upper-case
	if( sResult->Length() )
	{
		char nChar = sResult->m_str[0];
		if( nChar >= 'a' && nChar <= 'z' ) sResult->m_str[0] = nChar - 32;
	}

	return sResult;
}

inline stringp string_t::ToUpper() const
{
	stringp sResult = Copy();

	// Make each lower-case letter upper-case
	int nLen = sResult->Length();
	for( int i = 0; i < nLen; i++ )
	{
		char nChar = sResult->m_str[i];
		if( nChar >= 'a' && nChar <= 'z' ) sResult->m_str[i] = nChar - 32;
	}

	return sResult;
}

inline stringp string_t::ToLower() const
{
	stringp sResult = Copy();

	// Make each upper-case letter lower-case
	int nLen = sResult->Length();
	for( int i = 0; i < nLen; i++ )
	{
		char nChar = sResult->m_str[i];
		if( nChar >= 'A' && nChar <= 'Z' ) sResult->m_str[i] = nChar + 32;
	}

	return sResult;
}

inline stringp string_t::Trim()
{
	ASSERT(GetRefCount() > 0);
	const char *szStart = m_str.c_str();
	const char *szEnd = szStart + m_str.length() - 1;
    const char *szOrg = szStart;
    const char *szOrgEnd = szEnd;

	// Skip leading spaces.
	while( *szStart == 32 || *szStart == 9 ||
		   *szStart == 10 || *szStart == 13 ) szStart++;

	// Find the last non-space character.
	while( szEnd >= szStart &&
		   ( *szEnd == 32 || *szEnd == 9 ||
		     *szEnd == 10 || *szEnd == 13 ) ) szEnd--;

    // Avoid making a new string if we did not trim anything,
    // since we trim an awful lot of strings where this is true.
    if (szStart == szOrg && szEnd == szOrgEnd) {
        AddRef();
        return this;
    }
    
	// Create a new string from the remaning data.
	if (szEnd >= szStart) {
		return new string_t( szStart, (szEnd - szStart) + 1 );
	} else {
		return new string_t();
	}
}

inline stringp string_t::Reverse() const
{
	const char *szText = m_str.c_str();

	// Create a new string to hold the result.
	stringp sResult = new string_t();

	// Reserve space in the string
	str_size nLen = m_str.length();
	sResult->m_str.append( nLen, ' ' ); // alloc hack.

	str_size nPos = 0;
	while( nLen-- ) sResult->m_str[nLen] = szText[nPos++];

	return sResult;
}

inline stringp string_t::Crypt(stringp sSalt) const
{
	// Encryt the string using the supplied salt
	const char* szCrypt;
	if( sSalt )
	{
		szCrypt = crypt( m_str.c_str(), sSalt->CString() );
	}
	else
	{
		char szSalt[3] = { 0 };
		szSalt[0] = 32 + (rand() % 95);
		szSalt[1] = 32 + (rand() % 95);

		szCrypt = crypt( m_str.c_str(), szSalt );
	}

	return new string_t( szCrypt );
}

inline stringp string_t::CTime( time_t nTime )
{
	const char* szTime = ctime( &nTime );
	if( !szTime ) return new string_t();
	return new string_t( szTime, 24 );
}

static char g_fmtTimeBuf[1025];

inline stringp string_t::FmtTime( stringp sFormat, time_t nTime )
{
	if (sFormat->Length()) {
		const struct tm* pTime = localtime( &nTime );
		if (pTime) {
			int nLen = strftime(g_fmtTimeBuf, sizeof(g_fmtTimeBuf)-1, sFormat->CString(), pTime);
			if (nLen) {
				return new string_t(g_fmtTimeBuf, (int32)nLen);
			}
		}
	}
	return new string_t();
}


// Reference counting

inline void string_t::AddRef()
{
#ifdef DEBUG_REF
	printf( "[string %08x addref: %d refs]\n", this, m_nRef+1 );
#endif
	ASSERT( m_nRef > 0 );
	m_nRef++;
}

inline void string_t::Release()
{
#ifdef DEBUG_REF
	printf( "[string %08x release: %d refs]\n", this, m_nRef-1 );
#endif
	ASSERT( m_nRef > 0 );
	if( --m_nRef == 0 ) delete this;
}

#endif // FIREFLY_STRING_H
