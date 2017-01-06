// mapping.cpp
// FireFly server 'mapping' data type

#include "mapping.h"
#include "database.h"
#include "any_impl.h"


// Public interface

void mapping_t::Append(mappingp mpValue)
{
	// Add the keys in a mapping to the mapping
	const std_map& src = mpValue->m_mpValues;
	for (map_const_it it = src.begin(); it != src.end(); ++it) {
		Assign(it->first, it->second);
	}
}

const any_t& mapping_t::Lookup( const any_t& mKey )
{
	// Retrieve the value in an existing key
	map_it it = m_mpValues.find(mKey);
	if (it != m_mpValues.end()) {
		// Replace deleted refs in the mapping with null.
		if (it->second.IsDeleted()) it->second.AssignNull();

		// Return the value for this key.
		return it->second;
	}

	// Key not present, return null.
	return g_null;
}

bool mapping_t::Exists( const any_t& mKey )
{
	// Retrieve the value in an existing key
	map_it it = m_mpValues.find(mKey);
	if (it != m_mpValues.end()) {
		// Replace deleted refs in the mapping with null.
		if (it->second.IsDeleted()) it->second.AssignNull();

		// Key is present.
		return true;
	}

	// Key not present.
	return false;
}

void mapping_t::Assign(const any_t& mKey, const any_t& mValue)
{
	// Assign a new value for a key
	map_it it = m_mpValues.find(mKey);
	if (it != m_mpValues.end()) {
		// Replace the value for this key
		it->second.Assign(mValue); // new ref
	} else {
		// Add a new key-value pair
		mKey.AddRef(); // new ref
		mValue.AddRef(); // new ref
		m_mpValues.insert(map_val(mKey, mValue));
	}
}

arrayp mapping_t::ListKeys() const
{
	arrayp result = new array_t;
	const std_map& src = m_mpValues;
	for (map_const_it it = src.begin(); it != src.end(); ++it) {
		result->AppendItem(it->first); // new ref
	}
	return result;
}

arrayp mapping_t::ListValues() const
{
	arrayp result = new array_t;
	const std_map& src = m_mpValues;
	for (map_const_it it = src.begin(); it != src.end(); ++it) {
		result->AppendItem(it->second); // new ref
	}
	return result;
}

bool mapping_t::Remove(const any_t& mKey)
{
	map_it it = m_mpValues.find(mKey);
	if (it != m_mpValues.end()) {
		it->first.Release();
		it->second.Release();
		m_mpValues.erase(it);
		return true;
	}
	return false;
}


// Construction, Destruction

mapping_t::~mapping_t()
{
#ifdef DEBUG_REF
	printf( "[mapping %08x destroy: %d refs]\n", this, m_nRef );
#endif
	g_oIdxMapping.Remove( this );

	// Release each key and value in the mapping
	const std_map& src = m_mpValues;
	for (map_const_it it = src.begin(); it != src.end(); ++it) {
		it->first.Release();
		it->second.Release();
	}
}


// Persistent storage management

void mapping_t::Persist() const
{
	// Write the reference count
	g_oDatabase.WriteLong( m_nRef );

	// Write the mapping size
	g_oDatabase.WriteLong( (uint32)m_mpValues.size() );

	// Write the mapping elements
	const std_map& src = m_mpValues;
	for (map_const_it it = src.begin(); it != src.end(); ++it) {
		it->first.Persist();
		it->second.Persist();
	}
}

void mapping_t::Restore()
{
	// Read the reference count
	m_nRef = g_oDatabase.ReadLong();

	// Read the mapping size
	uint32 nSize = g_oDatabase.ReadLong();

	// Read the mapping elements
	for( uint32 i = 0; i < nSize; i++ ) {
		any_t mKey, mValue;
		mKey.Restore();
		mValue.Restore();
		m_mpValues.insert(map_val(mKey, mValue)); // no change
	}
}
