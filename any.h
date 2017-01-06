// any.h
// FireFly server 'any' data type

#ifndef FIREFLY_ANY_H
#define FIREFLY_ANY_H

#include "global.h"

#include <vector>
#include <map>

#ifdef WIN32
#include <xutility>
#endif

class any_t  
{
public:
	// Constructors (no reference counting)
	inline any_t();
	inline any_t( const any_t& mValue );
	inline any_t( int32 nValue ) : m_nTag(TYPE_INTEGER) { m_uData.nValue = nValue; }
	inline any_t( float64 fValue ) : m_nTag(TYPE_FLOAT) { m_uData.fValue = fValue; }
	inline any_t( stringp sValue ) : m_nTag(TYPE_STRING) { m_uData.sValue = sValue; }
	inline any_t( arrayp arValue ) : m_nTag(TYPE_ARRAY) { m_uData.arValue = arValue; }
	inline any_t( mappingp mpValue ) : m_nTag(TYPE_MAPPING) { m_uData.mpValue = mpValue; }
	inline any_t( objectp oValue ) : m_nTag(TYPE_OBJECT_REF) { m_uData.oValue = oValue; }
	inline any_t( objectp oValue, bool bTrue ) : m_nTag(TYPE_OBJECT) { m_uData.oValue = oValue; }
	inline any_t( functionp fnValue ) : m_nTag(TYPE_FUNCTION) { m_uData.fnValue = fnValue; }
	inline any_t( portp ptValue ) : m_nTag(TYPE_PORT) { m_uData.ptValue = ptValue; }
	inline any_t( threadp thValue ) : m_nTag(TYPE_THREAD) { m_uData.thValue = thValue; }
	inline any_t( int32 nLower, int32 nUpper ) : m_nTag(TYPE_RANGE) { m_uData.rnValue.nLower = nLower; m_uData.rnValue.nUpper = nUpper; }
	inline any_t( functionp fnValue, objectp oContext ) : m_nTag(TYPE_FUNC_REF) { m_uData.frValue.fnValue = fnValue; m_uData.frValue.oContext = oContext; }
	inline any_t( objectp oValue, stringp sName ) : m_nTag(TYPE_ATT_NAME) { m_uData.anValue.oValue = oValue; m_uData.anValue.sName = sName; }
	inline any_t( mappingp mpValue, int32 nReg) : m_nTag(TYPE_MAP_KEY) { m_uData.mkValue.mpValue = mpValue; m_uData.mkValue.nReg = nReg; }
	inline any_t( int32 nIndex, arrayp arIterate ) : m_nTag(TYPE_FOR_EACH) { m_uData.feValue.nIndex = nIndex; m_uData.feValue.arIterate = arIterate; }
	inline ~any_t();

	// Less operator for STL map.
	bool operator<( const any_t& mOther ) const;

	// Assign operator for STL map and copy.
	inline any_t& operator=( const any_t& mValue ) { m_nTag = mValue.m_nTag; m_uData = mValue.m_uData; return *this; }

// Assignment operators (reference counting)
public:
	inline void Assign(const any_t& mValue) { mValue.AddRef(); Release(); m_nTag = mValue.m_nTag; m_uData = mValue.m_uData; }
	inline void AssignRef(const any_t& mValue) { Release(); m_nTag = mValue.m_nTag; m_uData = mValue.m_uData; }
	inline void AssignNull() { Release(); m_nTag = TYPE_NULL; }
	inline void DiscardNull() { m_nTag = TYPE_NULL; }
	inline void AssignArrIndex(arrayp arValue, int32 nIndex);
	inline void AssignArrSlice(arrayp arValue, int32 nReg);

// Operations
public:
	inline RESULT Add( const any_t& mA, const any_t& mB );
	inline RESULT Sub( const any_t& mA, const any_t& mB );
	inline RESULT Mul( const any_t& mA, const any_t& mB );
	inline RESULT Div( const any_t& mA, const any_t& mB );
	inline RESULT Mod( const any_t& mA, const any_t& mB );
	inline RESULT And( const any_t& mA, const any_t& mB );
	inline RESULT Or( const any_t& mA, const any_t& mB );
	inline RESULT Xor( const any_t& mA, const any_t& mB );
	inline RESULT Shr( const any_t& mA, const any_t& mB );
	inline RESULT Shl( const any_t& mA, const any_t& mB );
	inline RESULT Pow( const any_t& mA, const any_t& mB );
	bool Equ( const any_t& mB ) const;
	inline bool Neq( const any_t& mB ) const;
	inline bool Gt( const any_t& mB ) const;
	inline bool Lt( const any_t& mB ) const;
	inline bool GtEq( const any_t& mB ) const;
	inline bool LtEq( const any_t& mB ) const;
	inline RESULT ToInt( const any_t& mA );
	inline RESULT ToFloat( const any_t& mA );

	inline bool Bool() const;
	inline int32 Size() const;

	inline int32 Iterate() { ASSERT( m_nTag == TYPE_FOR_EACH ); return m_uData.feValue.nIndex++; }

	stringp ToString() const;
	stringp Encode(int depth = 0, bool exporting = false) const;

// Extractors
public:
	explicit inline operator int32() const { return m_uData.nValue; }
	explicit inline operator float64() const { return m_uData.fValue; }
	explicit inline operator stringp() const { return m_uData.sValue; }
	explicit inline operator arrayp() const { return m_uData.arValue; }
	explicit inline operator mappingp() const { return m_uData.mpValue; }
	explicit inline operator objectp() const { return m_uData.oValue; }
	explicit inline operator functionp() const { return m_uData.fnValue; }
	explicit inline operator portp() const { return m_uData.ptValue; }
	explicit inline operator threadp() const { return m_uData.thValue; }
	explicit inline operator func_ref_t() const { return m_uData.frValue; }
	explicit inline operator for_each_t() const { return m_uData.feValue; }

	inline arrayp array() const { return m_uData.arValue; }
	inline const range_t& range() const { return m_uData.rnValue; }
	inline const arr_index_t& arr_index() const { return m_uData.aiValue; }
	inline const arr_slice_t& arr_slice() const { return m_uData.asValue; }
	inline const map_key_t& map_key() const { return m_uData.mkValue; }
	inline const att_name_t& att_name() const { return m_uData.anValue; }

// Type checking
public:
	inline uint32 Type() const { return m_nTag; }
	inline bool IsUndefined() const { return (m_nTag == TYPE_UNDEFINED); }
	inline bool IsNull() const { return (m_nTag == TYPE_NULL); }
	inline bool IsInt() const { return (m_nTag == TYPE_INTEGER); }
	inline bool IsFloat() const { return (m_nTag == TYPE_FLOAT); }
	inline bool IsRange() const { return (m_nTag == TYPE_RANGE); }
	inline bool IsString() const { return (m_nTag == TYPE_STRING); }
	inline bool IsArray() const { return (m_nTag == TYPE_ARRAY); }
	inline bool IsMapping() const { return (m_nTag == TYPE_MAPPING); }
	inline bool IsObject() const { return (m_nTag == TYPE_OBJECT || m_nTag == TYPE_OBJECT_REF); }
	inline bool IsObjectRef() const { return (m_nTag == TYPE_OBJECT_REF); }
	inline bool IsFunction() const { return (m_nTag == TYPE_FUNCTION); }
	inline bool IsPort() const { return (m_nTag == TYPE_PORT); }
	inline bool IsThread() const { return (m_nTag == TYPE_THREAD); }
	inline bool IsFuncRef() const { return (m_nTag == TYPE_FUNC_REF); }
	inline bool IsAttName() const { return (m_nTag == TYPE_ATT_NAME); }
	inline bool IsArrIndex() const { return (m_nTag == TYPE_ARR_INDEX); }
	inline bool IsArrSlice() const { return (m_nTag == TYPE_ARR_SLICE); }
	inline bool IsMapKey() const { return (m_nTag == TYPE_MAP_KEY); }
	inline bool IsForEach() const { return (m_nTag == TYPE_FOR_EACH); }
	inline bool IsDeleted() const;
	inline bool IsChild() const { return (m_nTag == TYPE_OBJECT); }

// Persistent storage management
public:
	void Persist() const;
	bool Restore();

// Reference counting
public:
	void AddRef() const;
	void Release() const;

// Private union type
protected:
	typedef union any_u
	{
		int32 nValue;
		float64 fValue;
		range_t rnValue;
		stringp sValue;
		arrayp arValue;
		mappingp mpValue;
		objectp oValue;
		functionp fnValue;
		portp ptValue;
		threadp thValue;
		func_ref_t frValue;
		att_name_t anValue;
		arr_index_t aiValue;
		arr_slice_t asValue;
		map_key_t mkValue;
		for_each_t feValue;
	} any_u_t;

// Private member variables
protected:
	uint32 m_nTag;
	// has 4 bytes of padding here.
	any_u_t m_uData;
};

// Construction, Destruction

inline any_t::any_t() : m_nTag(TYPE_NULL)
{
}

inline any_t::any_t( const any_t& mValue )
	: m_nTag(mValue.m_nTag), m_uData(mValue.m_uData)
{
	// Don't add a ref because this is used to restore the database
	// (before deferred links have been resolved).
}

inline any_t::~any_t()
{
	// Can't release here because the copy constructor doesn't
	// add a reference.
}

const any_t g_null;

#endif // FIREFLY_ANY_H
