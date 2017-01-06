// array.cpp
// FireFly server 'array' data type

#include "array.h"
#include "database.h"
#include "any_impl.h"

// Construction, Destruction

array_t::array_t() : m_nRef(1)
{
#ifdef DEBUG_REF
	printf( "[array %08x create: %d refs (%d)]\n", this, m_nRef, (int)bRegister );
#endif
	g_oIdxArray.Insert(this);
}

array_t::~array_t()
{
#ifdef DEBUG_REF
	printf( "[array %08x destroy: %d refs]\n", this, m_nRef );
#endif
	g_oIdxArray.Remove( this );

	// Release each value in the array
	const array_vec& vals = m_vals;
	for (array_const_it it = vals.begin(); it != vals.end(); ++it) {
		it->Release();
	}
}


// Public interface

void array_t::AppendItem(const any_t& mValue)
{
	// Append a value to the array
	mValue.AddRef(); // new ref.
	m_vals.push_back(mValue);
}

void array_t::AppendMulti(const arrayp arValue)
{
	// Append an array to the array
	const array_vec& toAdd = arValue->m_vals;
	for (array_const_it it = toAdd.begin(); it != toAdd.end(); ++it) {
		it->AddRef(); // new ref.
		m_vals.push_back(*it);
	}
}

bool array_t::RemoveMulti(const arrayp arValue)
{
	// Add a ref in case the array we are iterating over is only being
	// kept alive by an item that we will remove. Is it possible?
	arValue->AddRef();

	// Remove multiple items from self array.
	bool found = false;
	const array_vec& toRem = arValue->m_vals;
	for (array_const_it it = toRem.begin(); it != toRem.end(); ++it) {
		if (RemoveItem(*it)) found = true;
	}

	arValue->Release();

	return found;
}

bool array_t::RemoveItem(const any_t& mValue)
{
	// Remove all matching values from the array.
	bool found = false;
	array_it it = m_vals.begin();
	while (it != m_vals.end()) {
		// Drop deleted references, replace with null.
		if (it->IsObject() && objectp(*it)->IsDeleted()) {
			it->AssignNull();
		}
		if (it->Equ(mValue)) {
			// It is safe to Release any value we find, because the caller
			// would not be searching for an object they did not hold a ref for.
			it->Release();
			it = m_vals.erase(it);
			found = true;
		} else {
			++it;
		}
	}

	return found;
}

bool array_t::RemoveFirst()
{
	// Remove the first value from the array.
	array_it it = m_vals.begin();
	if (it != m_vals.end()) {
		it->Release();
		m_vals.erase(it);
		return true;
	}
	return false;
}

int32 array_t::Find( const any_t& mValue, int32 start, int32 end )
{
	// Clamp start to both ends.
	if (start < 0) start = 0;
	if (start >= (int32)m_vals.size()) return false;

	// Clamp end to both ends.
	if (end <= start) return false;
	if (end > (int32)m_vals.size()) end = m_vals.size();

	array_it it = m_vals.begin() + start;
	array_it stop = m_vals.begin() + end;

	while(it != stop) {
		// Replace deleted refs in the array with null.
		if (it->IsDeleted()) it->AssignNull();
		if (it->Equ(mValue)) return it - m_vals.begin();
		++it;
	}

	return -1;
}

const any_t& array_t::Lookup(int32 nIndex)
{
	// Negative index is an index from the end.
	if (nIndex < 0 ) nIndex = (int32)m_vals.size() + nIndex;

	// Return null for out of range values.
	if (nIndex < 0 || nIndex >= (int32)m_vals.size()) return g_null;

	// Replace deleted refs in the array with null.
	if (m_vals[nIndex].IsDeleted()) m_vals[nIndex].AssignNull();

	return m_vals[nIndex];
}

arrayp array_t::Slice( range_t rnSlice )
{
	int32 nStart = rnSlice.nLower;
	int32 nEnd = rnSlice.nUpper;

	// Handle indices relative to end
	if( nStart < 0 ) nStart = m_vals.size() + nStart;
	if( nEnd < 0 ) nEnd = m_vals.size() + nEnd;

	// Determine the actual start and length
	if( nStart < 0 ) nStart = 0;
	if( nEnd >= (int32)m_vals.size() ) nEnd = (int32)m_vals.size() - 1;

	// Create the new array
	arrayp arResult = new array_t;

	if (nEnd >= nStart) {
		array_it itStart = m_vals.begin() + nStart;
		array_it itEnd = m_vals.begin() + nEnd + 1;

		// Insert the new values into the array
		array_vec& vals = arResult->m_vals;
		vals.insert( vals.end(), itStart, itEnd );

		// Add a ref to each inserted value
		for( array_it it = vals.begin(); it != vals.end(); ++it ) it->AddRef();
	}

	return arResult;
}

bool array_t::Assign(int32 nIndex, const any_t& mValue)
{
	// Handle index relative to end
	if (nIndex < 0) nIndex = (int32)m_vals.size() + nIndex;

	// Assign a new value into an existing element
	if (nIndex < 0 || nIndex >= (int32)m_vals.size()) return false;

	m_vals[nIndex].Assign(mValue); // new ref.
	return true;
}

stringp array_t::Join( const stringp sDelim, const stringp sFinalDelim )
{
	// Create the result string
	stringp sResult = new string_t();

	// Append an array to the array
	int nSize = m_vals.size();
	int nIndex = 0;
	for (array_const_it it = m_vals.begin(); it != m_vals.end(); ++it) {
		// Append the delimiter to the result.
		if (nIndex++) {
			if (sFinalDelim && nIndex == nSize) {
				sResult->Append(sFinalDelim);
			} else if (sDelim) {
				sResult->Append( sDelim );
			} else {
				sResult->AppendCStr( "," );
			}
		}
		stringp sTemp = it->ToString(); // new ref.
		sResult->Append(sTemp);
		sTemp->Release(); // release.
	}

	return sResult;
}


// Persistent storage management

void array_t::Persist() const
{
	// Write the reference count
	g_oDatabase.WriteLong( m_nRef );

	// Write the array size
	g_oDatabase.WriteLong( m_vals.size() );

	// Write the array elements
	for (array_const_it it = m_vals.begin(); it != m_vals.end(); ++it) {
		it->Persist();
	}
}

void array_t::Restore()
{
	// Read the reference count
	m_nRef = g_oDatabase.ReadLong();

	// Read the array size
	uint32 nSize = g_oDatabase.ReadLong();

	// Read the array elements
	for( uint32 i = 0; i < nSize; i++ ) {
		any_t mValue;
		mValue.Restore();
		m_vals.push_back( mValue ); // no change
	}
}
