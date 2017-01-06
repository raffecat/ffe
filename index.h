// index.h
// FireFly server persistent data index

#ifndef FIREFLY_INDEX_H
#define FIREFLY_INDEX_H

#include "global.h"
#include "database.h"
#include "array.h"

#define MARKER_LENGTH	8

#include <map>
#include <vector>

/*
PersistIndex process:
 - Write list of ptr:ptr instance pointers from index map

PersistData process:
 - Write data for each old instance in index map order

RestoreIndex process:
 - Read list of old pointers (preallocate)
 - Create a new instance for each and assign into old:new index map
 - Assign each into old:refcount map

RestoreData process:
 - Restore data for each instance in list order
 - Relink any pointers in the restored data (count restored references)
 - Clear the list of old instance pointers

ValidateIndex process:
 - Move the old:new index into a new local map (or copy then clear)
 - Assign each new instance into the ptr:ptr index map
 - Check if each instance ref count matches the count in the old:refcount map
 - Clear the old:refcount map
*/

class index_sentinel_t {
	int unused;
};

template<class T> class index_t
{
public:
	index_t();
	~index_t();

	typedef typename std::map<T*, T*> index_map;
	typedef typename index_map::iterator index_it;
	typedef typename index_map::value_type index_val;

	typedef typename std::vector<T*> restore_vec;
	typedef typename restore_vec::iterator restore_it;

	typedef typename std::map<T*, int32> refcount_map;
	typedef typename refcount_map::iterator refcount_it;
	typedef typename refcount_map::value_type refcount_val;

// Index management
public:
	inline void Insert( T* pInstance );
	inline void Remove( T* pInstance );
	inline T* ResolvePointer( T* pOldRef );

	// inline RESULT BuildItemList( arrayp& arResult );

// Persistent storage management
public:
	void PersistIndex( const char* szMarker );
	void PersistData( const char* szMarker );
	bool RestoreIndex( const char* szMarker );
	bool RestoreData( const char* szMarker );
	bool ValidateIndex();
	bool ValidateObjects();

// Private member variables
protected:
	index_map m_mpIdx;
	refcount_map m_mpRefCheck;
	restore_vec m_vecRestore;
};


// Construction, Destruction

template<class T> inline index_t<T>::index_t()
{
}

template<class T> inline index_t<T>::~index_t()
{
}

// Index management

template<class T> inline void index_t<T>::Insert( T* pInstance )
{
	// Add a new item to the index tagged with itself
	ASSERT( pInstance != NULL );
	m_mpIdx[ pInstance ] = pInstance;
}

template<class T> inline void index_t<T>::Remove( T* pInstance )
{
	// Remove an item from the index
	m_mpIdx.erase( pInstance );
}

template<class T> inline T* index_t<T>::ResolvePointer( T* pOldRef )
{
	// Return the address of a restored item
	// The OldRef is the previous known address of the item

	// Can save and restore a null pointer
	if( !pOldRef ) return NULL;

	// Find the old address of the item in the index map
	index_it itFind = m_mpIdx.find( pOldRef );
	if( itFind != m_mpIdx.end() )
	{
		// Found the item, so increment the new reference count
		m_mpRefCheck[pOldRef]++;

		// Return the new address of the item
		return itFind->second;
	}

	printf( "[0x%p] failed to resolve old pointer! (set to NULL)\n", pOldRef );

	// The old address was not found. This means the target value
	// was not saved (because the ref count dropped below one) or
	// the restored pointer is corrupt.
	return NULL; // (T*)-1;
}

/*
template<class T> inline RESULT index_t<T>::BuildItemList( arrayp& arResult )
{
	arResult = new array_t;
	if( !arResult ) return E_MEMORY;

	// Build the list
	index_it iterList;
	for( iterList = m_mpIdx.begin(); iterList != m_mpIdx.end(); iterList++ )
	{
		// Add this item to the list
		arResult->AppendItem( any_t( iterList->first ) );
	}

	return E_SUCCESS;
}
*/


// Persistent storage management

template<class T> void index_t<T>::PersistIndex( const char* szMarker )
{
	// Write the start-of-index marker
	g_oDatabase.WriteBytes( (byte*)szMarker, MARKER_LENGTH );

	// Write the number of items
	g_oDatabase.WriteLong( m_mpIdx.size() );

	// Persist each item's current pointer as an index
	index_it iterPersist;
	for( iterPersist = m_mpIdx.begin(); iterPersist != m_mpIdx.end(); iterPersist++ )
	{
		T* pItem = iterPersist->first;

		// Store the current pointer to the instance
		g_oDatabase.WriteLong( (uint32)pItem );
	}
}

template<class T> void index_t<T>::PersistData( const char* szMarker )
{
	// Write the start-of-data marker
	g_oDatabase.WriteBytes( (byte*)szMarker, MARKER_LENGTH );

	// Write the number of items (redundant, for validation)
	g_oDatabase.WriteLong( m_mpIdx.size() );

	// Persist each item in the index in the same order as PersistIndex
	index_it iterPersist;
	for( iterPersist = m_mpIdx.begin(); iterPersist != m_mpIdx.end(); iterPersist++ )
	{
		T* pItem = iterPersist->first;

		// Persist this item's data
		pItem->Persist();
	}
}

template<class T> bool index_t<T>::RestoreIndex( const char* szMarker )
{
	// This is used to construct an instance of an indexed object
	// without adding the new instance to the index.
	index_sentinel_t sentinel;

	// Clear all the index maps (should be unnecessary)
	m_mpIdx.clear();
	m_mpRefCheck.clear();
	m_vecRestore.clear();

	// Read and validate the start-of-index marker
	byte szBuffer[8];
	g_oDatabase.ReadBytes( (byte*)szBuffer, MARKER_LENGTH );
	if( memcmp( szBuffer, szMarker, MARKER_LENGTH ) != 0 )
	{
		printf( "expected start-of-index marker '%s' not found\n", szMarker );
		return false;
	}

	// Read the number of items to restore
	uint32 nItems = g_oDatabase.ReadLong();
	m_vecRestore.reserve( nItems );

	// Restore the list of old instance pointers
	for( uint32 i = 0; i < nItems; i++ )
	{
		// Read the old reference pointer
		T* pOldRef = (T*)g_oDatabase.ReadLong();

		// Add the pointer to the restore list
		m_vecRestore.push_back( pOldRef );

		// Add a new instance to the index for this old pointer
		T* pItem = new T( sentinel );
		if( !pItem )
		{
			printf( "[0x%p] out of memory\n", pOldRef );
			return false;
		}
		m_mpIdx[pOldRef] = pItem;

		// Add an entry to the old:refcount map for this old pointer
		m_mpRefCheck[pOldRef] = 0;
	}

	ASSERT( m_vecRestore.size() == nItems );
	ASSERT( m_mpIdx.size() == nItems );
	ASSERT( m_mpRefCheck.size() == nItems );

	return true;
}

template<class T> bool index_t<T>::RestoreData( const char* szMarker )
{
	// Read and validate the start-of-data marker
	byte szBuffer[8];
	g_oDatabase.ReadBytes( (byte*)szBuffer, MARKER_LENGTH );
	if( memcmp( szBuffer, szMarker, MARKER_LENGTH ) != 0 )
	{
		printf( "expected start-of-data marker '%s' not found\n", szMarker );
		return false;
	}

	// Read the number of items to restore
	uint32 nItems = g_oDatabase.ReadLong();
	if( nItems != (uint32)m_vecRestore.size() )
	{
		printf( "instance data count %ld does not match index size %ld\n",
			(long)nItems, (long)m_vecRestore.size() );
		return false;
	}

	// Restore the instance data for each old instance
	restore_it iterList;
	for( iterList = m_vecRestore.begin(); iterList != m_vecRestore.end(); iterList++ )
	{
		T* pOldRef = *iterList;

		// Find the new instance in the index
		T* pItem = m_mpIdx[pOldRef];
		if( !pItem )
		{
			printf( "[0x%p] expected item not found in restore index\n", pOldRef );
			return false;
		}

		// Restore the instance data
		pItem->Restore();
	}

	// No longer need the list of old instance pointers
	m_vecRestore.clear();

	return true;
}

template<class T> bool index_t<T>::ValidateIndex()
{
	// Make a local copy of the restore index and clear it
	index_map mpCopy = m_mpIdx;
	m_mpIdx.clear();

	// Validate each restored instance
	index_it iterCheck;
	for( iterCheck = mpCopy.begin(); iterCheck != mpCopy.end(); ++iterCheck )
	{
		T* pOldRef = iterCheck->first;
		T* pItem = iterCheck->second;

		// Check if the reference count matches reality
		int32 nActualRefs = m_mpRefCheck[pOldRef];
		int32 nStoredRefs = pItem->GetRefCount();
		if( nActualRefs != nStoredRefs )
		{
			if( nActualRefs < 1 )
			{
				printf( "[0x%p] actual ref count %ld does not match stored count %ld (recovered)\n",
					pOldRef, (long)nActualRefs, (long)nStoredRefs );

				// Make the reference count valid
				pItem->SetRefCount( 1 );

				// We could delete the item, but we'd have to defer the delete until
				// we finish the integrity checks because the delete will change
				// the ref count of any contained items (eg, array elements, mapping
				// keys/values, etc. which will cause later mismatches..)

				// Move the item to the 'recovered' container after restore
				g_oDatabase.RecoverLostValue( any_t( pItem ) );
			}
			else
			{
				printf( "[0x%p] actual ref count %ld does not match stored count %ld (fixed)\n",
					pOldRef, (long)nActualRefs, (long)nStoredRefs );

				// Fix the reference count in the item
				pItem->SetRefCount( nActualRefs );
			}
		}

		// Add the item back into the main index
		m_mpIdx[pItem] = pItem;
	}

	// No longer need the ref count map
	m_mpRefCheck.clear();

	return true;
}

template<class T> bool index_t<T>::ValidateObjects()
{
	// Validate each restored object
	index_it iterCheck;
	for( iterCheck = m_mpIdx.begin(); iterCheck != m_mpIdx.end(); ++iterCheck )
	{
		// Validate the restored object
		iterCheck->first->Validate();
	}

	return true;
}

#endif // FIREFLY_INDEX_H
