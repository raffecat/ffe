// any.cpp
// FireFly server 'any' data type

#include "any.h"
#include "database.h"
#include "index.h"
#include "string.h"
#include "array.h"
#include "mapping.h"
#include "object.h"
#include "function.h"
#include "port.h"
#include "thread.h"


stringp any_t::ToString() const // returns new ref.
{
	switch( m_nTag )
	{
	case TYPE_UNDEFINED:
	case TYPE_NULL:
	case TYPE_DELETED:
		return new string_t("null");

	case TYPE_INTEGER:
		return new string_t( m_uData.nValue );

	case TYPE_FLOAT:
		return new string_t( m_uData.fValue );

	case TYPE_RANGE:
		{
			stringp sResult = new string_t( m_uData.rnValue.nLower );
			sResult->AppendCStr( ".." );
			stringp sValue = new string_t( m_uData.rnValue.nUpper );
			sResult->Append( sValue );
			sValue->Release();
			return sResult;
		}

	case TYPE_STRING:
		m_uData.sValue->AddRef(); // new ref
		return m_uData.sValue;

	case TYPE_ARRAY: // FIXME: guard against cycles.
		{
			stringp sResult = new string_t( "[" );
			arrayp arIterate = m_uData.arValue; // borrow
			int32 nLen = arIterate->Size();
			for( int32 nIndex = 0; nIndex < nLen; nIndex++ ) {
				if( nIndex ) sResult->AppendCStr( ", " );
				// Convert the element to a string (recursive)
				const any_t& mValue = arIterate->Lookup( nIndex ); // borrow
				stringp sValue = mValue.Encode();
				sResult->Append( sValue );
				sValue->Release();
			}
			sResult->AppendCStr( "]" );
			return sResult;
		}

	case TYPE_MAPPING: // FIXME: guard against cycles.
		{
			// Build the list of keys
			mappingp mpIterate = m_uData.mpValue; // borrow
			arrayp arKeys = mpIterate->ListKeys(); // new
			stringp sResult = new string_t( "{" );
			int32 nLen = arKeys->Size();
			for( int32 nIndex = 0; nIndex < nLen; nIndex++ ) {
				if( nIndex ) sResult->AppendCStr( ", " );
				const any_t& mKey = arKeys->Lookup( nIndex ); // borrow
				stringp sKey = mKey.Encode();
				sResult->Append( sKey );
				sKey->Release();
				sResult->AppendCStr( " : " );
				const any_t& mValue = mpIterate->Lookup( mKey ); // borrow
				stringp sValue = mValue.Encode();
				sResult->Append( sValue );
				sValue->Release();
			}
			arKeys->Release();
			sResult->AppendCStr( "}" );
			return sResult;
		}

	case TYPE_OBJECT:
	case TYPE_OBJECT_REF:
		return m_uData.oValue->GetPath(); // new ref

	case TYPE_PORT:
		{
			int32 nPort = m_uData.ptValue->GetPort();
			stringp sResult = new string_t( nPort );
			sResult->PrependCStr( "(port:" );
			sResult->AppendCStr( ")" );
			return sResult;
		}		

	case TYPE_THREAD:
		if( m_uData.thValue->IsStopped() )
			return new string_t( "(thread:stopped)" );
		else
			return new string_t( "(thread)" );
		break;

	case TYPE_FUNCTION:
		{
			RT_ASSERT_RET( m_uData.fnValue->m_sName != NULL );
			stringp sResult = m_uData.fnValue->m_sName->Copy(); // copy
			sResult->AppendCStr( "()" );
			return sResult;
		}

	case TYPE_FUNC_REF:
		{
			RT_ASSERT_RET( m_uData.frValue.fnValue->m_sName != NULL );
			stringp sResult = m_uData.frValue.fnValue->m_sName->Copy(); // copy
			sResult->AppendCStr( "()" );
			return sResult;
		}

	default:
		return new string_t( "(error)" );
	}
}

stringp any_t::Encode(int depth, bool exporting) const // returns new ref.
{
	switch( m_nTag )
	{
	case TYPE_UNDEFINED:
	case TYPE_NULL:
	case TYPE_DELETED:
	case TYPE_PORT:
	case TYPE_THREAD:
		return new string_t( "null" );

	case TYPE_INTEGER:
		return new string_t( m_uData.nValue );

	case TYPE_FLOAT:
		return new string_t( m_uData.fValue );

	case TYPE_RANGE:
		{
			stringp sResult = new string_t( m_uData.rnValue.nLower );
			sResult->AppendCStr( ".." );
			stringp sValue = new string_t( m_uData.rnValue.nUpper );
			sResult->Append( sValue );
			sValue->Release();
			return sResult;
		}

	case TYPE_STRING:
		{
			return m_uData.sValue->Encode(); // new.
		}
		break;

	case TYPE_ARRAY:
		{
			if (depth > 100) return new string_t("[...]");
			stringp sResult = new string_t( "[" );
			arrayp arIterate = m_uData.arValue; // borrow
			int32 nLen = arIterate->Size();
			for( int32 nIndex = 0; nIndex < nLen; nIndex++ ) {
				// Convert the element to a string (recursive)
				const any_t& mValue = arIterate->Lookup( nIndex ); // borrow
				if( nIndex ) sResult->AppendCStr( ", " );
				stringp sValue = mValue.Encode(depth+1, exporting);
				sResult->Append( sValue );
				sValue->Release();
			}
			sResult->AppendCStr( "]" );
			return sResult;
		}

	case TYPE_MAPPING:
		{
			// Build the list of keys
			if (depth > 100) return new string_t("{...}");
			mappingp mpIterate = m_uData.mpValue; // borrow
			arrayp arKeys = mpIterate->ListKeys(); // new
			stringp sResult = new string_t( "{" );
			int32 nLen = arKeys->Size();
			for( int32 nIndex = 0; nIndex < nLen; nIndex++ ) {
				if( nIndex ) sResult->AppendCStr( ", " );
				const any_t& mKey = arKeys->Lookup( nIndex ); // borrow
				if (exporting) {
					// Remove deleted-object keys at export time.
					// Do not include clones as keys at export time.
					if (mKey.IsObject() && ( objectp(mKey)->IsDeleted() || objectp(mKey)->IsClone() )) continue;
				}
				stringp sKey = mKey.Encode(depth+1, exporting);
				sResult->Append( sKey );
				sKey->Release();
				sResult->AppendCStr( " : " );
				const any_t& mValue = mpIterate->Lookup( mKey ); // borrow
				stringp sValue = mValue.Encode(depth+1, exporting);
				sResult->Append( sValue );
				sValue->Release();
			}
			arKeys->Release();
			sResult->AppendCStr( "}" );
			return sResult;
		}

	case TYPE_OBJECT:
	case TYPE_OBJECT_REF:
		// Do not encode paths of deleted objects.
		if (m_uData.oValue->IsDeleted()) return new string_t( "null" );
		// Export all clone refs as null.
		if (exporting && m_uData.oValue->IsClone()) return new string_t( "null" );
		return m_uData.oValue->GetPath(); // new ref

	case TYPE_FUNCTION:
		RT_ASSERT_RET( m_uData.fnValue->m_sSource != NULL );
		return m_uData.fnValue->m_sSource->Copy(); // copy

	case TYPE_FUNC_REF:
		RT_ASSERT_RET( m_uData.frValue.fnValue->m_sSource != NULL );
		return m_uData.frValue.fnValue->m_sSource->Copy(); // copy

	default:
		return new string_t( "null" );
	}
}

// Persistent storage management

void any_t::Persist() const
{
	// Write the data type
	g_oDatabase.WriteLong( m_nTag );

	// Write the data bytes
	switch( m_nTag )
	{
	case TYPE_UNDEFINED:
	case TYPE_NULL:
	case TYPE_DELETED:
		return;
	case TYPE_INTEGER:
		g_oDatabase.WriteBytes( (byte*)&m_uData.nValue, sizeof(m_uData.nValue) );
		return;
	case TYPE_FLOAT:
		g_oDatabase.WriteBytes( (byte*)&m_uData.fValue, sizeof(m_uData.fValue) );
		return;
	case TYPE_RANGE:
		g_oDatabase.WriteBytes( (byte*)&m_uData.rnValue, sizeof(m_uData.rnValue) );
		return;
	case TYPE_STRING:
		g_oDatabase.WriteBytes( (byte*)&m_uData.sValue, sizeof(m_uData.sValue) );
		return;
	case TYPE_ARRAY:
		g_oDatabase.WriteBytes( (byte*)&m_uData.arValue, sizeof(m_uData.arValue) );
		return;
	case TYPE_MAPPING:
		g_oDatabase.WriteBytes( (byte*)&m_uData.mpValue, sizeof(m_uData.mpValue) );
		return;
	case TYPE_OBJECT:
	case TYPE_OBJECT_REF:
		g_oDatabase.WriteBytes( (byte*)&m_uData.oValue, sizeof(m_uData.oValue) );
		return;
	case TYPE_PORT:
		g_oDatabase.WriteBytes( (byte*)&m_uData.ptValue, sizeof(m_uData.ptValue) );
		return;
	case TYPE_THREAD:
		g_oDatabase.WriteBytes( (byte*)&m_uData.thValue, sizeof(m_uData.thValue) );
		return;
	case TYPE_FUNCTION:
		g_oDatabase.WriteBytes( (byte*)&m_uData.fnValue, sizeof(m_uData.fnValue) );
		return;
	case TYPE_FUNC_REF:
		g_oDatabase.WriteBytes( (byte*)&m_uData.frValue, sizeof(m_uData.frValue) );
		return;
	case TYPE_ATT_NAME:
		g_oDatabase.WriteBytes( (byte*)&m_uData.anValue, sizeof(m_uData.anValue) );
		return;
	case TYPE_ARR_INDEX:
		g_oDatabase.WriteBytes( (byte*)&m_uData.aiValue, sizeof(m_uData.aiValue) );
		return;
	case TYPE_ARR_SLICE:
		g_oDatabase.WriteBytes( (byte*)&m_uData.asValue, sizeof(m_uData.asValue) );
		return;
	case TYPE_MAP_KEY:
		g_oDatabase.WriteBytes( (byte*)&m_uData.mkValue, sizeof(m_uData.mkValue) );
		return;
	case TYPE_FOR_EACH:
		g_oDatabase.WriteBytes( (byte*)&m_uData.feValue, sizeof(m_uData.feValue) );
		return;
	}
}

bool any_t::Restore()
{
	// Read the data type
	m_nTag = g_oDatabase.ReadLong();

	// Read the data bytes
	switch( m_nTag )
	{
	case TYPE_UNDEFINED:
	case TYPE_NULL:
	case TYPE_DELETED:
		return true;
	case TYPE_INTEGER:
		g_oDatabase.ReadBytes( (byte*)&m_uData.nValue, sizeof(m_uData.nValue) );
		return true;
	case TYPE_FLOAT:
		g_oDatabase.ReadBytes( (byte*)&m_uData.fValue, sizeof(m_uData.fValue) );
		return true;
	case TYPE_RANGE:
		g_oDatabase.ReadBytes( (byte*)&m_uData.rnValue, sizeof(m_uData.rnValue) );
		return true;
	case TYPE_STRING:
		g_oDatabase.ReadBytes( (byte*)&m_uData.sValue, sizeof(m_uData.sValue) );
		m_uData.sValue = g_oIdxString.ResolvePointer( m_uData.sValue );
		if (!m_uData.sValue) m_nTag = TYPE_NULL; // replace with null.
		return true;
	case TYPE_ARRAY:
		g_oDatabase.ReadBytes( (byte*)&m_uData.arValue, sizeof(m_uData.arValue) );
		m_uData.arValue = g_oIdxArray.ResolvePointer( m_uData.arValue );
		if (!m_uData.arValue) m_nTag = TYPE_NULL; // replace with null.
		return true;
	case TYPE_MAPPING:
		g_oDatabase.ReadBytes( (byte*)&m_uData.mpValue, sizeof(m_uData.mpValue) );
		m_uData.mpValue = g_oIdxMapping.ResolvePointer( m_uData.mpValue );
		if (!m_uData.mpValue) m_nTag = TYPE_NULL; // replace with null.
		return true;
	case TYPE_OBJECT:
	case TYPE_OBJECT_REF:
		g_oDatabase.ReadBytes( (byte*)&m_uData.oValue, sizeof(m_uData.oValue) );
		m_uData.oValue = g_oIdxObject.ResolvePointer( m_uData.oValue );
		if (!m_uData.oValue) m_nTag = TYPE_NULL; // replace with null.
		return true;
	case TYPE_FUNCTION:
		g_oDatabase.ReadBytes( (byte*)&m_uData.fnValue, sizeof(m_uData.fnValue) );
		m_uData.fnValue = g_oIdxFunction.ResolvePointer( m_uData.fnValue );
		if (!m_uData.fnValue) m_nTag = TYPE_NULL; // replace with null.
		return true;
	case TYPE_PORT:
		g_oDatabase.ReadBytes( (byte*)&m_uData.ptValue, sizeof(m_uData.ptValue) );
		m_uData.ptValue = g_oIdxPort.ResolvePointer( m_uData.ptValue );
		if (!m_uData.ptValue) m_nTag = TYPE_NULL; // replace with null.
		return true;
	case TYPE_THREAD:
		g_oDatabase.ReadBytes( (byte*)&m_uData.thValue, sizeof(m_uData.thValue) );
		m_uData.thValue = g_oIdxThread.ResolvePointer( m_uData.thValue );
		if (!m_uData.thValue) m_nTag = TYPE_NULL; // replace with null.
		return true;
	case TYPE_FUNC_REF:
		g_oDatabase.ReadBytes( (byte*)&m_uData.frValue, sizeof(m_uData.frValue) );
		m_uData.frValue.fnValue = g_oIdxFunction.ResolvePointer( m_uData.frValue.fnValue );
		m_uData.frValue.oContext = g_oIdxObject.ResolvePointer( m_uData.frValue.oContext );
		if (!m_uData.frValue.fnValue) m_nTag = TYPE_NULL; // replace with null.
		if (!m_uData.frValue.oContext) m_nTag = TYPE_NULL; // replace with null.
		return true;
	case TYPE_ATT_NAME:
		g_oDatabase.ReadBytes( (byte*)&m_uData.anValue, sizeof(m_uData.anValue) );
		m_uData.anValue.oValue = g_oIdxObject.ResolvePointer( m_uData.anValue.oValue );
		m_uData.anValue.sName = g_oIdxString.ResolvePointer( m_uData.anValue.sName );
		if (!m_uData.anValue.oValue) m_nTag = TYPE_NULL; // replace with null.
		if (!m_uData.anValue.sName) m_nTag = TYPE_NULL; // replace with null.
		return true;
	case TYPE_ARR_INDEX:
		g_oDatabase.ReadBytes( (byte*)&m_uData.aiValue, sizeof(m_uData.aiValue) );
		m_uData.aiValue.arValue = g_oIdxArray.ResolvePointer( m_uData.aiValue.arValue );
		if (!m_uData.aiValue.arValue) m_nTag = TYPE_NULL; // replace with null.
		return true;
	case TYPE_ARR_SLICE:
		g_oDatabase.ReadBytes( (byte*)&m_uData.asValue, sizeof(m_uData.asValue) );
		m_uData.asValue.arValue = g_oIdxArray.ResolvePointer( m_uData.asValue.arValue );
		if (!m_uData.asValue.arValue) m_nTag = TYPE_NULL; // replace with null.
		return true;
	case TYPE_MAP_KEY:
		g_oDatabase.ReadBytes( (byte*)&m_uData.mkValue, sizeof(m_uData.mkValue) );
		m_uData.mkValue.mpValue = g_oIdxMapping.ResolvePointer( m_uData.mkValue.mpValue );
		if (!m_uData.mkValue.mpValue) m_nTag = TYPE_NULL; // replace with null.
		return true;
	case TYPE_FOR_EACH:
		g_oDatabase.ReadBytes( (byte*)&m_uData.feValue, sizeof(m_uData.feValue) );
		m_uData.feValue.arIterate = g_oIdxArray.ResolvePointer( m_uData.feValue.arIterate );
		if (!m_uData.feValue.arIterate) m_nTag = TYPE_NULL; // replace with null.
		return true;
	default:
		// Unknown data type was encountered
		return false;
	}
}


// Reference counting

void any_t::AddRef() const
{
	switch( m_nTag )
	{
	case TYPE_STRING:
		m_uData.sValue->AddRef();
		return;
	case TYPE_ARRAY:
		m_uData.arValue->AddRef();
		return;
	case TYPE_MAPPING:
		m_uData.mpValue->AddRef();
		return;
	case TYPE_OBJECT:
	case TYPE_OBJECT_REF:
		m_uData.oValue->AddRef();
		return;
	case TYPE_FUNCTION:
		m_uData.fnValue->AddRef();
		return;
	case TYPE_PORT:
		m_uData.ptValue->AddRef();
		return;
	case TYPE_THREAD:
		m_uData.thValue->AddRef();
		return;
	case TYPE_FUNC_REF:
		m_uData.frValue.fnValue->AddRef();
		m_uData.frValue.oContext->AddRef();
		return;
	case TYPE_ATT_NAME:
		m_uData.anValue.oValue->AddRef();
		m_uData.anValue.sName->AddRef();
		return;
	case TYPE_ARR_INDEX:
		m_uData.aiValue.arValue->AddRef();
		return;
	case TYPE_ARR_SLICE:
		m_uData.asValue.arValue->AddRef();
		return;
	case TYPE_MAP_KEY:
		m_uData.mkValue.mpValue->AddRef();
		return;
	case TYPE_FOR_EACH:
		m_uData.feValue.arIterate->AddRef();
		return;
	}
}

void any_t::Release() const
{
	switch( m_nTag )
	{
	case TYPE_STRING:
		m_uData.sValue->Release();
		return;
	case TYPE_ARRAY:
		m_uData.arValue->Release();
		return;
	case TYPE_MAPPING:
		m_uData.mpValue->Release();
		return;
	case TYPE_OBJECT:
	case TYPE_OBJECT_REF:
		m_uData.oValue->Release();
		return;
	case TYPE_FUNCTION:
		m_uData.fnValue->Release();
		return;
	case TYPE_PORT:
		m_uData.ptValue->Release();
		return;
	case TYPE_THREAD:
		m_uData.thValue->Release();
		return;
	case TYPE_FUNC_REF:
		m_uData.frValue.fnValue->Release();
		m_uData.frValue.oContext->Release();
		return;
	case TYPE_ATT_NAME:
		m_uData.anValue.oValue->Release();
		m_uData.anValue.sName->Release();
		return;
	case TYPE_ARR_INDEX:
		m_uData.aiValue.arValue->Release();
		return;
	case TYPE_ARR_SLICE:
		m_uData.asValue.arValue->Release();
		return;
	case TYPE_MAP_KEY:
		m_uData.mkValue.mpValue->Release();
		return;
	case TYPE_FOR_EACH:
		m_uData.feValue.arIterate->Release();
		return;
	}
}
