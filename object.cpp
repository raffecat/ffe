// object.cpp
// FireFly server 'object' data type

#include "object.h"
#include "database.h"
#include "index.h"
#include "any_impl.h"
#include "import.h"

extern import_t g_oImport;


// Public interface

bool object_t::Exists(stringp sName) {
	return (LookupAttrib(sName) != NULL);
}

const any_t& object_t::Lookup( stringp sName )
{
	// Retrieve the value in an existing key
	any_t* pAttrib = FindAttrib( sName );
	if (pAttrib) {
		// Replace refs to deleted objects with null.
		if (pAttrib->IsDeleted()) pAttrib->AssignNull();
		return *pAttrib;
	}
	return g_null; // Attribute not found.
}

bool object_t::Assign( stringp sName, const any_t& mValue )
{
	// Convert a child object into an object reference.
	// Unwrap a function bound to a context.
	// Replace deleted object refs with null.
	any_t mAssign;
	if (mValue.IsChild()) {
		mAssign = any_t(objectp(mValue)); // store an ObjectRef instead.
	} else if(mValue.IsFuncRef()) {
		mAssign = any_t(func_ref_t(mValue).fnValue); // store unbound function.
	} else if(mValue.IsDeleted()) {
		mAssign = g_null; // store old refs as null.
	} else {
		mAssign = mValue;
	}
	if (mAssign.IsString()) {
		ASSERT(stringp(mAssign)->GetRefCount() > 0);
	}
	ASSERT(mAssign.Type() < TYPE_FUNC_REF || mAssign.Type() == TYPE_OBJECT_REF);
	// Search this object for the attribute.
	any_t* pAttrib = LookupAttrib(sName);
	if (pAttrib) {
		// Cannot replace a child object.
		if (mAssign.IsString() && stringp(mAssign)->CompareCStr("~~UNSET~CHILD~~") == 0) {
			// This is a hack to let us fix child references that should not be child references!
			// Fall through without checking IsChild.
		} else {
			if (pAttrib->IsChild()) return false; // E_REPLACE.
		}
		// Replace the attribute value.
		pAttrib->Assign(mAssign); // new ref
	} else {
		// Create a new attribute.
		sName->AddRef(); // new ref
		mAssign.AddRef(); // new ref
		m_mpAttribs.insert(attrib_val(sName, mAssign));
	}
	return true;
}

bool object_t::Remove(stringp sName)
{
	// Find the specified attribute
	attrib_it itAttrib = m_mpAttribs.find(sName);
	if (itAttrib != m_mpAttribs.end()) {
		// Deleting objects is a special case
		if (itAttrib->second.IsChild()) {
			// Cannot remove an object, must be deleted
			if (!objectp(itAttrib->second)->IsDeleted()) return false; // E_REPLACE.
		}

		// Release the value and delete
		itAttrib->first->Release();
		itAttrib->second.Release();
		m_mpAttribs.erase( itAttrib );
	}
	return true;
}

RESULT object_t::CreateObject(stringp sName, objectp oOwner, objectp& oChild)
{
	// Check if the specified name exists
	if( FindAttrib( sName ) ) return E_ATTRIBUTE_EXISTS;

	// Create a new object
	objectp oNew = new object_t; // new ref
	if( !oNew ) return E_MEMORY;

	// Make sure the object has an owner
	if( !oOwner ) oOwner = g_oDatabase.RootObject();
	oNew->SetOwner( oOwner );

	// Set the object's parent to this object
	RESULT nResult = oNew->SetParent( this );
	if( ISERROR(nResult) )  { oNew->Release(); return nResult; }

	// Create a new attribute
	ASSERT(sName->GetRefCount() > 0);
	sName->AddRef(); // new ref
	any_t mChild( oNew, true ); // ownership
	m_mpAttribs.insert( attrib_val( sName, mChild ) );

	// Return a borrowed reference
	oChild = oNew;

	return E_SUCCESS;
}

RESULT object_t::CreateClone( objectp oOwner, objectp& oResult )
{
	// Cannot clone the root object
	if( this == g_oDatabase.RootObject() ) return E_DENIED;

	// Create a new object
	objectp oNew = new object_t;  // new ref
	if( !oNew ) return E_MEMORY;

	// Make sure the object has an owner
	if( !oOwner ) oOwner = g_oDatabase.RootObject();
	oNew->SetOwner( oOwner );

	// Set the object's parent to this object
	RESULT nResult = oNew->SetParent( this );
	if( ISERROR(nResult) ) { oNew->Release(); return nResult; }

	// Make the object inherit this object
	nResult = oNew->AddInherit( this );
	if( ISERROR(nResult) ) { oNew->Release(); return nResult; }

	// Allocate a clone number for the new clone
	uint32 nId = g_oDatabase.AllocateCloneId();
	RT_ASSERT_RET( nId > 0 );
	oNew->SetCloneId( nId );

	// Add the object to the clone list (take ownership)
	m_pClone.push_back( oNew );

	// Return the new clone
	oResult = oNew;

	return E_SUCCESS;
}

bool object_t::DoesInherit(objectp oFind) const
{
	// Check if this object inherits the specified object
	object_const_it itInherit;
	for( itInherit = m_pInherit.begin(); itInherit != m_pInherit.end(); ++itInherit )
	{
		objectp oInherit = *itInherit;

		// Check if the specimen is on my inherit list
		if( oInherit == oFind ) return true;

		// Check if the specimen is inherited indirectly.
		if( oInherit->DoesInherit( oFind ) ) return true;
	}

	return false;
}

RESULT object_t::AddInherit( objectp oInherit )
{
	// Check if this object already inherits the specified object
	object_it itInherit;
	for( itInherit = m_pInherit.begin(); itInherit != m_pInherit.end(); itInherit++ )
	{
		if( *itInherit == oInherit ) return E_FALSE;
	}

	// Cannot inherit the root
	if( oInherit == g_oDatabase.RootObject() ) return E_DENIED;

	// Cannot create inheritance cycle
	if (oInherit->DoesInherit(this)) return E_DENIED;

	// Add inheritance and keep a ref
	m_pInherit.push_back( oInherit );
	oInherit->AddRef();

	return E_SUCCESS;
}

RESULT object_t::RemoveInherit( objectp oInherit )
{
	// Check if this object inherits the specified object
	object_it itInherit;
	for( itInherit = m_pInherit.begin(); itInherit != m_pInherit.end(); itInherit++ )
	{
		if( *itInherit == oInherit )
		{
			// Remove inheritance and release the ref
			m_pInherit.erase( itInherit );
			oInherit->Release();

			return E_SUCCESS;
		}
	}

	return E_FALSE;
}

objectp object_t::FindClone(uint32 nCloneId)
{
	// Find the clone within this object
	object_it itClone;
	for( itClone = m_pClone.begin(); itClone != m_pClone.end(); itClone++ )
	{
		if( (*itClone)->GetCloneId() == nCloneId )
		{
			return (*itClone); // borrow
		}
	}
	return NULL;
}

arrayp object_t::ListAttribs()
{
	arrayp result = new array_t;

	// Build the array of attribute names
	attrib_it itAttrib;
	for( itAttrib = m_mpAttribs.begin(); itAttrib != m_mpAttribs.end(); ++itAttrib ) {
		result->AppendItem(any_t(itAttrib->first)); // new ref
	}

	return result;
}

arrayp object_t::ListInherits()
{
	arrayp arInherits = new array_t;

	// Build the array of inherited objects
	object_it itInherit;
	for( itInherit = m_pInherit.begin(); itInherit != m_pInherit.end(); )
	{
		// Clear out deleted objects while building the list
		if( (*itInherit)->IsDeleted() )
		{
			(*itInherit)->Release();
			itInherit = m_pInherit.erase( itInherit );
		}
		else
		{
			arInherits->AppendItem( any_t(*itInherit) ); // new ref
			itInherit++;
		}
	}

	return arInherits;
}

RESULT object_t::SetParent( objectp oParent )
{
	// Cannot set the parent of the root
	if( this == g_oDatabase.RootObject() ) return E_DENIED;
	if (oParent->IsDeleted()) return E_DELETED;
	if (oParent->IsClone()) return E_CLONE;

	// Cannot set parent which creates a loop
	int nSane = 1000;
	objectp oCheck = oParent;
	while( oCheck && nSane-- )
	{
		if( oCheck == this ) return E_DENIED;
		oCheck = oCheck->GetParent();
	}
	if( !nSane ) return E_DENIED;

	// Set the new parent
	if( oParent ) oParent->AddRef();
	if( m_oParent ) m_oParent->Release();
	m_oParent = oParent;

	return E_SUCCESS;
}

RESULT object_t::MoveObject( objectp oParent, stringp sName )
{
	// Check if the destination already exists.
	if (oParent->LookupAttrib(sName)) return E_ATTRIBUTE_EXISTS;

	// Cannot move a deleted object, or a clone.
	if( this == g_oDatabase.RootObject() ) return E_DENIED;
	if (IsDeleted()) return E_DELETED;
	if (IsClone()) return E_DENIED;

	// New attribute name must be non-empty.
	stringp sNewName = sName->Trim(); // new ref (used below)
	if (!sNewName->Length()) {
		sNewName->Release();
		return E_DENIED;
	}

	// Keep the old parent (alive) until we can remove the old attribute.
	objectp oldParent = m_oParent;
	if (oldParent) oldParent->AddRef();

	// Change the parent, checking for errors.
	RESULT nResult = SetParent(oParent);
	if (ISERROR(nResult)) {
		if (oldParent) oldParent->Release();
		sNewName->Release();
		return nResult;
	}

	// Add a strong ref to self before we remove attributes.
	// This new ref will become owned by the new attribute below.
	AddRef(); // new ref (used below)

	// Remove object from its current parent.
	if (oldParent) {
		attrib_map& parentAttrs = oldParent->m_mpAttribs;
		for (attrib_it itAttrib = parentAttrs.begin(); itAttrib != parentAttrs.end(); itAttrib++) {
			if (itAttrib->second.IsChild() && objectp(itAttrib->second) == this) {
				// Release references.
				itAttrib->first->Release(); // drop ref to name
				Release(); // drop ref to self
				// Remove the attribute.
				parentAttrs.erase(itAttrib);
				break;
			}
		}
		oldParent->Release();
	}

	// Set the new attribute, taking ownership of sNewName and our AddRef above.
	any_t mChild(this, true);
	oParent->m_mpAttribs.insert(attrib_val(sNewName, mChild)); // owns both refs.

	return E_SUCCESS;
}

RESULT object_t::SetOwner( objectp oOwner )
{
	// Cannot set the owner of the root
	if( this == g_oDatabase.RootObject() ) return E_DENIED;

	// Set the new owner
	if( oOwner ) oOwner->AddRef();
	if( m_oOwner ) m_oOwner->Release();
	m_oOwner = oOwner;

	return E_SUCCESS;
}

stringp object_t::UniqueAttribName(stringp sBase)
{
	const char* szBase = sBase ? sBase->CString() : "";
	long nBaseLen = sBase ? sBase->Length() : 0;

	// Find the highest numbered key in use
	// TODO: need an algorithm that reuses numbers
	int32 nFound = 0;
	attrib_it itAttrib;
	for( itAttrib = m_mpAttribs.begin(); itAttrib != m_mpAttribs.end(); itAttrib++ )
	{
		const char* szName = itAttrib->first->CString();
		if( strncmp( szName, szBase, nBaseLen ) == 0 )
		{
			long nKey = atol( szName + nBaseLen );
			if( nKey > nFound ) nFound = nKey;
		}
	}

	// Create the new key
	stringp sResult = new string_t( szBase );
	char szNumber[33];
	sprintf( szNumber, "%ld", (long)(nFound + 1) );
	// ltoa( nFound + 1, szNumber, 10 ); // is this portable?
	sResult->AppendCStr( (const char*)szNumber );

	return sResult;
}

stringp object_t::GetPath()
{
	objectp oThis = this;
	objectp oRoot = g_oDatabase.RootObject();

	// Walk up the hierarchy to the root
	stringp sPath = new string_t();
	while( oThis != oRoot )
	{
		objectp oParent = oThis->m_oParent;
		if (oParent == NULL) return sPath; // should not happen.

		if( oThis->IsClone() )
		{
			// Add the clone number to the path
			stringp sNumber = new string_t( oThis->GetCloneId() );
			sPath->Prepend( sNumber );
			sNumber->Release();
			sPath->PrependCStr( "#" );
			oThis = oParent;
		}
		else
		{
			// Find this within its parent (linear search)
			attrib_it itAttrib, itEnd = oParent->m_mpAttribs.end();
			for( itAttrib = oParent->m_mpAttribs.begin(); itAttrib != itEnd; itAttrib++ )
			{
				if (itAttrib->second.IsChild() && objectp(itAttrib->second) == oThis)
				{
					// Prepend this object's name to the result
					sPath->Prepend( itAttrib->first );
					sPath->PrependCStr( "." );

					// Walk up the hierarchy
					oThis = oParent;
					break;
				}
			}
		}

		if (oThis != oParent) {
			// Did not find the object within its parent
			return sPath;
		}
	}

	// Add the root marker to the start
	sPath->PrependCStr( "$" );
	return sPath;
}

stringp object_t::GetName() const // new
{
	objectp oParent = m_oParent;
	if (oParent == NULL) {
		return new string_t("$"); // ownership.
	}

	if (IsClone())
	{
		stringp parentName = oParent->GetName(); // new ref.
		stringp sName = parentName->Copy(); // copy (so we can modify)
		parentName->Release(); // release.

		// Add the clone number to the name
		sName->AppendCStr("#");
		stringp sNumber = new string_t(GetCloneId()); // new.
		sName->Append(sNumber);
		sNumber->Release();
		return sName; // ownership.
	}

	// Find this within its parent (linear search)
	attrib_it itAttrib, itEnd = oParent->m_mpAttribs.end();
	for (itAttrib = oParent->m_mpAttribs.begin(); itAttrib != itEnd; ++itAttrib) {
		if (itAttrib->second.IsChild() && objectp(itAttrib->second) == this) {
			stringp sName = itAttrib->first;
			sName->AddRef(); // new ref.
			return sName; // ownership.
		}
	}

	return new string_t();
}

void object_t::SetupRoot()
{
	m_oOwner = this; AddRef(); // self-owner needs to be a ref
}

void object_t::Export()
{
	// Write the object selector
	stringp sPath = GetPath();
	sPath->PrependCStr("[");
	sPath->AppendCStr("]\n");
	g_oImport.WriteExport(sPath);
	sPath->Release();

	// Append object inheritance
	object_it itInherit;
	for( itInherit = m_pInherit.begin(); itInherit != m_pInherit.end(); itInherit++ )
	{
		objectp oInherit = (*itInherit);
		if (oInherit->IsDeleted() || oInherit->IsClone())
			continue; // skip bad inherits.

		stringp sInherit = oInherit->GetPath();
		sInherit->PrependCStr("-> ");
		sInherit->AppendCStr("\n");
		g_oImport.WriteExport(sInherit);
		sInherit->Release();

		break; // only export one base (multiple bases is deprecated)
	}

	// Append all value attributes, enqueue all children
	attrib_it itAttrib;
	for( itAttrib = m_mpAttribs.begin(); itAttrib != m_mpAttribs.end(); itAttrib++ )
	{
		const any_t& mValue = itAttrib->second;

		if (!mValue.IsChild() && !mValue.IsFunction() && !mValue.IsFuncRef())
		{
			// Write out the value
			stringp sAttrib = itAttrib->first->Copy(); // copy (so we can modify)
			sAttrib->AppendCStr(" = ");

			stringp sValue = itAttrib->second.Encode(0, true); // new.
			sAttrib->Append(sValue);
			sValue->Release();
			sAttrib->AppendCStr("\n");

			g_oImport.WriteExport(sAttrib);
			sAttrib->Release();
		}
	}

	// Append all function sources
	for( itAttrib = m_mpAttribs.begin(); itAttrib != m_mpAttribs.end(); itAttrib++ )
	{
		any_t& mValue = itAttrib->second;
		if (mValue.IsFunction() || mValue.IsFuncRef())
		{
			// Write out the function source
			stringp sValue = itAttrib->second.Encode(0, true); // new.

            int32 pos = sValue->SkipToFuncStart();
            if (pos >= 0) {
                // Replace function name with the attribute name.
                stringp sSource = sValue->SubString(pos, -1); // new.
                sValue->Release();
                sSource->Prepend(itAttrib->first);
                sValue = sSource;
            }

			sValue->PrependCStr("\n");
			sValue->AppendCStr("\n");

			g_oImport.WriteExport(sValue);
			sValue->Release();
		}
	}

	g_oImport.WriteExportBreak();

	// Export all children
	for( itAttrib = m_mpAttribs.begin(); itAttrib != m_mpAttribs.end(); itAttrib++ )
	{
		any_t& mValue = itAttrib->second;
		if (mValue.IsChild())
		{
			objectp oChild = (objectp)(mValue);
			if (oChild->IsDeleted() || oChild->IsClone())
				continue; // skip bad children.

			// Export this child
			oChild->Export();
		}
	}
}



// Construction, Destruction

object_t::~object_t()
{
#ifdef DEBUG_REF
	printf( "[object %08x destroy: %d refs]\n", this, m_nRef );
#endif
	g_oIdxObject.Remove( this );

	// Release all object state
	MakeDeleted();
}


// Persistent storage management

void object_t::Persist()
{
	// Write the reference count
	g_oDatabase.WriteLong( m_nRef );

	// Write the object access rights
	g_oDatabase.WriteLong( m_nAccess );
	g_oDatabase.WriteLong( m_nDefaultAccess );

	// Write the name, owner and parent
	g_oDatabase.WriteLong( (uint32)m_oOwner );
	g_oDatabase.WriteLong( (uint32)m_oParent );

	// Write inherited objects
	g_oDatabase.WriteLong( m_pInherit.size() );
	object_it itInherit;
	for( itInherit = m_pInherit.begin(); itInherit != m_pInherit.end(); ++itInherit )
	{
		g_oDatabase.WriteLong( (uint32)(*itInherit) );
	}

	// Write clone number for this object
	g_oDatabase.WriteLong( m_nCloneId );

	// Write clone objects
	g_oDatabase.WriteLong( m_pClone.size() );
	object_it itClone;
	for( itClone = m_pClone.begin(); itClone != m_pClone.end(); ++itClone )
	{
		g_oDatabase.WriteLong( (uint32)(*itClone) );
	}

	// Write the attributes
	g_oDatabase.WriteLong( m_mpAttribs.size() );
	attrib_it itAttrib;
	for( itAttrib = m_mpAttribs.begin(); itAttrib != m_mpAttribs.end(); ++itAttrib )
	{
		// Write the attribute name
		g_oDatabase.WriteLong( (uint32)itAttrib->first );

		// Write the access rights
		// NOT USED any more.
		g_oDatabase.WriteLong(0);

		// Write the value
		itAttrib->second.Persist();
	}
}

void object_t::Restore()
{
	// Read the reference count
	m_nRef = g_oDatabase.ReadLong();

	// Read the object access rights
	m_nAccess = g_oDatabase.ReadLong();
	m_nDefaultAccess = g_oDatabase.ReadLong();

	// Read the name, owner and parent
	m_oOwner = g_oIdxObject.ResolvePointer( (objectp)g_oDatabase.ReadLong() );
	m_oParent = g_oIdxObject.ResolvePointer( (objectp)g_oDatabase.ReadLong() );

	// Recover from broken references.
	// No owner means 'deleted', and this could be a deleted object.
	// No parent we can live with.

	// Read the inherit count
	uint32 nInherit = g_oDatabase.ReadLong();

	// Restore each inherited object
	while( nInherit-- )
	{
		// Read the inherited object (deferred pointer)
		objectp oInherit = g_oIdxObject.ResolvePointer( (objectp)g_oDatabase.ReadLong() );
		if (oInherit) {
			// Insert the object into the inherit list
			m_pInherit.push_back( oInherit );
		}
	}

	if( g_oDatabase.GetVersion() > DATABASE_VERSION_0 )
	{
		// Read the clone number for this object
		m_nCloneId = g_oDatabase.ReadLong();

		// Read the clone count
		uint32 nClone = g_oDatabase.ReadLong();

		// Restore each clone object
		while( nClone-- )
		{
			// Read the clone object (deferred pointer)
			objectp oClone = g_oIdxObject.ResolvePointer( (objectp)g_oDatabase.ReadLong() );
			if (oClone) {
				// Insert the object into the clone list
				m_pClone.push_back( oClone );
			}
		}
	}

	// Read the attribute count
	uint32 nAttribs = g_oDatabase.ReadLong();

	// Restore each attribute
	while( nAttribs-- )
	{
		// Restore the attribute name
		stringp sName = g_oIdxString.ResolvePointer( (stringp)g_oDatabase.ReadLong() );

		// Restore the attribute access rights
		// NOT USED any more: uint32 nAccess
		g_oDatabase.ReadLong();

		// Restore the attribute value
		any_t mValue;
		mValue.Restore();

		// Insert the attribute into the map.
		if (sName) {
			m_mpAttribs.insert(attrib_val(sName, mValue));
		}
	}
}

void object_t::Validate()
{
	// fix: for databases with clones in their inherit list,
	// due to an old bug pushing clones onto the inherit list.
	object_it itInherit;
	for( itInherit = m_pInherit.begin(); itInherit != m_pInherit.end(); )
	{
		objectp oInherit = *itInherit;
		if( oInherit->IsDeleted() || oInherit->IsClone() )
		{
			printf( "[0x%p] removed bad inherit: 0x%p (fixed)\n", (void*)this, (void*)oInherit );
			itInherit = m_pInherit.erase( itInherit );
			oInherit->Release();
		}
		else ++itInherit;
	}

	// Clear out deleted clones
	object_it itClone;
	for( itClone = m_pClone.begin(); itClone != m_pClone.end(); )
	{
		objectp oClone = *itClone;
		if( oClone->IsDeleted() )
		{
			printf( "[0x%p] removed bad clone: 0x%p (fixed)\n", (void*)this, (void*)oClone );
			itClone = m_pClone.erase( itClone );
			oClone->Release();
		}
		else ++itClone;
	}
}



// Protected members

any_t* object_t::LookupAttrib(stringp sName)
{
	attrib_it it = m_mpAttribs.find(sName);
	if (it != m_mpAttribs.end()) return &it->second;
	return NULL;
}

any_t* object_t::FindAttrib(stringp sName, long nDepth)
{
	// Search this object for the attribute
	any_t* pAttrib = LookupAttrib( sName );
	if( pAttrib ) return pAttrib;

	// Check for inheritance overflow
	if( nDepth > MAX_INHERIT_DEPTH ) return NULL;

	// Search each inherited object in turn
	object_it itInherit;
	for( itInherit = m_pInherit.begin(); itInherit != m_pInherit.end(); itInherit++ )
	{
		if( ! (*itInherit)->IsDeleted() )
		{
			pAttrib = (*itInherit)->FindAttrib( sName, nDepth + 1 );
			if( pAttrib ) return pAttrib;
		}
	}

	return NULL;
}

RESULT object_t::Destroy()
{
	if (this == g_oDatabase.RootObject()) return E_DENIED;
	if (IsDeleted()) return E_SUCCESS;

	// Check if this object has any children
	for( attrib_it itAttrib = m_mpAttribs.begin(); itAttrib != m_mpAttribs.end(); itAttrib++ )
	{
		if( itAttrib->second.IsChild() )
		{
			// This attribute contains an actual object
			return E_CHILDREN;
		}
	}

	// Release all object state
	MakeDeleted();

	return E_SUCCESS;
}

void object_t::MakeDeleted()
{
	// Release the object members
	if( m_oOwner ) m_oOwner->Release();
	if( m_oParent ) m_oParent->Release();
	m_oOwner = NULL;
	m_oParent = NULL;

	// Unlink from all inherited objects
	object_it itInherit;
	for( itInherit = m_pInherit.begin(); itInherit != m_pInherit.end(); itInherit++ )
	{
		(*itInherit)->Release();
		*itInherit = NULL;
	}
	m_pInherit.clear();

	// Remove all attributes
	attrib_it itAttrib;
	for( itAttrib = m_mpAttribs.begin(); itAttrib != m_mpAttribs.end(); itAttrib++ )
	{
		itAttrib->first->Release();
		itAttrib->second.Release();
	}
	m_mpAttribs.clear();

	// Detach and delete all clones (clones won't have clones themselves)
	object_it itClone;
	for( itClone = m_pClone.begin(); itClone != m_pClone.end(); itClone++ )
	{
		(*itClone)->Destroy();
		(*itClone)->Release();
		*itClone = NULL;
	}
	m_pClone.clear();
}
