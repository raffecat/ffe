// mapping.h
// FireFly server 'mapping' data type

#ifndef FIREFLY_MAPPING_H
#define FIREFLY_MAPPING_H

#include "global.h"
#include "any.h"
#include "index.h"

#include <map>

typedef std::map<any_t, any_t> std_map;
typedef std_map::iterator map_it;
typedef std_map::const_iterator map_const_it;
typedef std_map::value_type map_val;

class mapping_t  
{
public:
	mapping_t();
	mapping_t(const std_map&);
	inline mapping_t(const index_sentinel_t&) : m_nRef(0) {}
	~mapping_t();

// Public interface
public:
	inline int32 Size() const;
	void Append(mappingp mpValue);
	bool Remove(const any_t& mKey);
	bool Exists(const any_t& mKey);
	const any_t& Lookup(const any_t& mKey);
	void Assign( const any_t& mKey, const any_t& mValue );
	arrayp ListKeys() const;
	arrayp ListValues() const;

// Persistent storage management
public:
	void Persist() const;
	void Restore();
	void Validate() { }

	inline int32 GetRefCount() const { return m_nRef; }
	inline void SetRefCount( int32 nRef ) { m_nRef = nRef; }

// Reference counting
public:
	inline void AddRef();
	inline void Release();

// Private member variables
protected:
    int32 m_nRef;
	std_map m_mpValues;
};

// Construction, Destruction

inline mapping_t::mapping_t() : m_nRef(1)
{
#ifdef DEBUG_REF
	printf( "[mapping %08x create: %d refs]\n", this, (int)m_nRef );
#endif
	g_oIdxMapping.Insert( this );
}

inline mapping_t::mapping_t(const std_map& mp) : m_nRef(1), m_mpValues(mp) {
#ifdef DEBUG_REF
	printf( "[mapping %08x create: %d refs]\n", this, (int)m_nRef );
#endif
	g_oIdxMapping.Insert( this );
}

inline int32 mapping_t::Size() const {
	return m_mpValues.size();
}

// Reference counting

inline void mapping_t::AddRef()
{
#ifdef DEBUG_REF
	printf( "[mapping %08x addref: %d refs]\n", this, m_nRef+1 );
#endif
	ASSERT( m_nRef > 0 );
	m_nRef++;
}

inline void mapping_t::Release()
{
#ifdef DEBUG_REF
	printf( "[mapping %08x release: %d refs]\n", this, m_nRef-1 );
#endif
	ASSERT( m_nRef > 0 );
	if( --m_nRef == 0 ) delete this;
}

#endif // FIREFLY_MAPPING_H
