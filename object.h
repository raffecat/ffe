// object.h
// FireFly server 'object' data type

#ifndef FIREFLY_OBJECT_H
#define FIREFLY_OBJECT_H

#include "global.h"
#include "any.h"
#include "string.h"

#include <map>
#include <vector>

// stringp less predicate
typedef struct stringp_less_s : public std::binary_function<stringp, stringp, bool> {
	bool operator()(const stringp& x, const stringp& y) const {	return x->Less( y ); }
} stringp_less;

// attribute map
typedef std::map<stringp, any_t, stringp_less> attrib_map;
typedef attrib_map::iterator attrib_it;
typedef attrib_map::const_iterator attrib_const_it;
typedef attrib_map::value_type attrib_val;

// inheritance vector
typedef std::vector<objectp> object_vec;
typedef object_vec::iterator object_it;
typedef object_vec::const_iterator object_const_it;

class object_t  
{
public:
	object_t();
	object_t(const index_sentinel_t&);
	~object_t();

// Public interface
public:
	bool Exists(stringp sName);
	const any_t& Lookup(stringp sName);
	bool Assign(stringp sName, const any_t& mValue);
	bool Remove(stringp sName );
	RESULT AddInherit( objectp oInherit );
	RESULT RemoveInherit( objectp oInherit );
	RESULT CreateObject( stringp sName, objectp oOwner, objectp& oNew );
	RESULT CreateClone( objectp oOwner, objectp& oResult );
	arrayp ListAttribs();
	arrayp ListInherits();
	arrayp ListClones();
	inline int32 Size() const { return m_mpAttribs.size(); }
	inline objectp GetParent() const { return m_oParent; }
	inline objectp GetOwner() const { return m_oOwner; }
	RESULT SetParent( objectp oParent );
	RESULT SetOwner( objectp oOwner );
	RESULT MoveObject( objectp oParent, stringp sName );
	stringp UniqueAttribName( stringp sPrefix );
	RESULT Destroy();
	stringp GetPath();
	stringp GetName() const;
	objectp FindClone(uint32 nCloneId);
	void Export();

	bool DoesInherit(objectp oFind) const;
	inline bool IsDeleted() const { return !m_oOwner; }
	inline uint32 IsClone() const { return (m_nCloneId > 0); }
	inline uint32 GetCloneId() const { return m_nCloneId; }
	inline void SetCloneId( uint32 nId ) { m_nCloneId = nId; }
	void SetupRoot();

// Persistent storage management
public:
	void Persist();
	void Restore();
	void Validate();

	inline int32 GetRefCount() const { return m_nRef; }
	inline void SetRefCount( int32 nRef ) { m_nRef = nRef; }

// Reference counting
public:
	inline void AddRef();
	inline void Release();

// Private members
protected:
	any_t* LookupAttrib(stringp sName);
	any_t* FindAttrib(stringp sName, long nDepth = 0);
	void MakeDeleted();

// Private member variables
protected:
    int32 m_nRef;
	uint32 m_nAccess;
	uint32 m_nDefaultAccess;
	objectp m_oOwner;
	objectp m_oParent;
	attrib_map m_mpAttribs;
	object_vec m_pInherit;
	// Version 1.1.1
	uint32 m_nCloneId;
	object_vec m_pClone;
};

// Construction, Destruction

inline object_t::object_t() : m_nRef(1), m_nAccess(0),
	m_nDefaultAccess(0), m_oOwner(NULL), m_oParent(NULL), m_nCloneId(0)
{
#ifdef DEBUG_REF
	printf( "[object %08x create: %d refs]\n", this, (int)m_nRef );
#endif
	g_oIdxObject.Insert( this );
}

inline object_t::object_t(const index_sentinel_t&) : m_nRef(0), m_nAccess(0),
	m_nDefaultAccess(0), m_oOwner(NULL), m_oParent(NULL), m_nCloneId(0)
{
}

// Reference counting

inline void object_t::AddRef()
{
#ifdef DEBUG_REF
	printf( "[object %08x addref: %d refs]\n", this, m_nRef+1 );
#endif
	ASSERT( m_nRef > 0 );
	m_nRef++;
}

inline void object_t::Release()
{
#ifdef DEBUG_REF
	printf( "[object %08x release: %d refs]\n", this, m_nRef-1 );
#endif
	ASSERT( m_nRef > 0 );
	if( --m_nRef == 0 ) delete this;
}

#endif // FIREFLY_OBJECT_H
