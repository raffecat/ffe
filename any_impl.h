// any_impl.h
// FireFly server 'any' data type implementation

#ifndef FIREFLY_ANY_IMPL_H
#define FIREFLY_ANY_IMPL_H

#include "any.h"

// Need each of these for ref counting
#include "string.h"
#include "array.h"
#include "mapping.h"
#include "object.h"
#include "function.h"
#include "port.h"
#include "thread.h"

// Assign

// These need to be here so we can AddRef the array argument.

inline void any_t::AssignArrIndex(arrayp arValue, int32 nIndex) {
	arValue->AddRef(); Release(); m_nTag = TYPE_ARR_INDEX; m_uData.aiValue.arValue = arValue; m_uData.aiValue.nIndex = nIndex;
}
inline void any_t::AssignArrSlice(arrayp arValue, int32 nReg) {
	arValue->AddRef(); Release(); m_nTag = TYPE_ARR_SLICE; m_uData.asValue.arValue = arValue; m_uData.asValue.nReg = nReg;
}

inline bool any_t::IsDeleted() const {
	return (m_nTag == TYPE_DELETED || ( IsObject() && (m_uData.oValue)->IsDeleted() ));
}


// Operations

inline RESULT any_t::Add( const any_t& mA, const any_t& mB )
{
	switch (mA.m_nTag)
	{
	case TYPE_INTEGER:
		if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign( any_t( (int32)mA + (int32)mB ) );
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_FLOAT) {
			this->Assign( any_t( (int32)mA + (float64)mB ) );
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_STRING) {
			stringp temp = new string_t( (int32)mA ); // new ref.
			temp->Append( (stringp)mB );
			this->AssignRef( any_t(temp) ); // ownership
			return E_SUCCESS;
		} else {
			return E_TYPE;
		}
	case TYPE_FLOAT:
		if (mB.m_nTag == TYPE_FLOAT) {
			this->Assign( any_t( (float64)mA + (float64)mB ) );
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign( any_t( (float64)mA + (int32)mB ) );
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_STRING) {
			stringp temp = new string_t( (float64)mA ); // new ref.
			temp->Append( (stringp)mB );
			this->AssignRef( any_t(temp) ); // ownership
			return E_SUCCESS;
		} else {
			return E_TYPE;
		}
	case TYPE_STRING:
		if (mB.m_nTag == TYPE_STRING) {
			stringp a = (stringp)mA;
			stringp b = (stringp)mB;
			if (a->Length() > 0 && b->Length() > 0) {
				stringp sResult = stringp(mA)->Copy(); // new.
				sResult->Append( (stringp)mB );
				this->AssignRef(any_t(sResult)); // ownership
			} else if (a->Length()) {
				this->Assign(mA); // new ref.
			} else {
				this->Assign(mB); // new ref.
			}
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_FLOAT) {
			stringp temp = new string_t( (float64)mB ); // new.
			temp->Prepend( (stringp)mA );
			this->AssignRef( any_t(temp) ); // ownership
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_INTEGER) {
			stringp temp = new string_t( (int32)mB ); // new.
			temp->Prepend( (stringp)mA );
			this->AssignRef( any_t(temp) ); // ownership
			return E_SUCCESS;
		} else {
			// Allow all other types to be "added" to the string as a no-op,
			// so we can concatenate templates without type errors everywhere.
			this->Assign(mA); // new ref.
			return E_SUCCESS;
		}
	case TYPE_ARRAY:
		if (mB.m_nTag == TYPE_ARRAY) {
			// Concat two arrays to form new array.
			arrayp arResult = new array_t(); // new.
			arResult->AppendMulti( (arrayp)mA );
			arResult->AppendMulti( (arrayp)mB );
			this->AssignRef( any_t(arResult) ); // ownership
			return E_SUCCESS;
		}
		break;
	case TYPE_MAPPING:
		if (mB.m_nTag == TYPE_MAPPING) {
			// Concat two mappings to form new mapping.
			mappingp mpResult = new mapping_t();
			mpResult->Append( (mappingp)mA );
			mpResult->Append( (mappingp)mB );
			this->AssignRef( any_t(mpResult) ); // ownership
			return E_SUCCESS;
		}
		break;
	default:
		break;
	}
	// For any case not covered above, if mB is a string, we allow mA
	// to be "added" as a no-op for template concatenation.
	if (mB.m_nTag == TYPE_STRING) {
		this->Assign(mB); // new ref.
		return E_SUCCESS;
	} else {
		return E_TYPE;
	}
}

inline RESULT any_t::Sub( const any_t& mA, const any_t& mB )
{
	switch( mA.m_nTag )
	{
	case TYPE_INTEGER:
		if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign( any_t( (int32)mA - (int32)mB ) );
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_FLOAT) {
			this->Assign( any_t( (int32)mA - (float64)mB ) );
			return E_SUCCESS;
		}
		return E_TYPE;
	case TYPE_FLOAT:
		if (mB.m_nTag == TYPE_FLOAT) {
			this->Assign( any_t( (float64)mA - (float64)mB ) );
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign( any_t( (float64)mA - (int32)mB ) );
			return E_SUCCESS;
		}
		return E_TYPE;
	case TYPE_ARRAY:
		if (mB.m_nTag == TYPE_ARRAY) {
			// Remove array contents from copy of array.
			arrayp arResult = new array_t(); // new.
			arResult->AppendMulti( (arrayp)mA );
			arResult->RemoveMulti( (arrayp)mB );
			this->AssignRef( any_t(arResult) ); // ownership
			return E_SUCCESS;
		}
		return E_TYPE;
	default:
		return E_TYPE;
	}
}

inline RESULT any_t::Mul( const any_t& mA, const any_t& mB )
{
	switch( mA.m_nTag )
	{
	case TYPE_INTEGER:
		if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign( any_t( (int32)mA * (int32)mB ) );
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_FLOAT) {
			this->Assign( any_t( (int32)mA * (float64)mB ) );
			return E_SUCCESS;
		}
		return E_TYPE;
	case TYPE_FLOAT:
		if (mB.m_nTag == TYPE_FLOAT) {
			this->Assign( any_t( (float64)mA * (float64)mB ) );
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign( any_t( (float64)mA * (int32)mB ) );
			return E_SUCCESS;
		}
		return E_TYPE;
	default:
		return E_TYPE;
	}
}

inline RESULT any_t::Div( const any_t& mA, const any_t& mB )
{
	switch( mA.m_nTag )
	{
	case TYPE_INTEGER:
		if (mB.m_nTag == TYPE_INTEGER) {
			// Always return a float from a divide operation.
			this->Assign(any_t( (float64)(int32)mA / (float64)(int32)mB ));
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_FLOAT) {
			this->Assign(any_t( (float64)(int32)mA / (float64)mB ));
			return E_SUCCESS;
		}
		return E_TYPE;
	case TYPE_FLOAT:
		if (mB.m_nTag == TYPE_FLOAT) {
			this->Assign(any_t( (float64)mA / (float64)mB ));
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign(any_t( (float64)mA / (float64)(int32)mB ));
			return E_SUCCESS;
		}
		return E_TYPE;
	default:
		return E_TYPE;
	}
}

inline RESULT any_t::Mod( const any_t& mA, const any_t& mB )
{
	switch( mA.m_nTag )
	{
	case TYPE_INTEGER:
		if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign(any_t( (float64)fmod( (double)(int32)mA, (double)(int32)mB ) ));
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_FLOAT) {
			this->Assign(any_t( (float64)fmod( (double)(int32)mA, (double)(float64)mB ) ));
			return E_SUCCESS;
		}
		return E_TYPE;
	case TYPE_FLOAT:
		if (mB.m_nTag == TYPE_FLOAT) {
			this->Assign(any_t( (float64)fmod( (double)(float64)mA, (double)(float64)mB ) ));
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign(any_t( (float64)fmod( (double)(float64)mA, (double)(int32)mB ) ));
			return E_SUCCESS;
		}
		return E_TYPE;
	default:
		return E_TYPE;
	}
}

inline RESULT any_t::And( const any_t& mA, const any_t& mB )
{
	switch( mA.m_nTag )
	{
	case TYPE_INTEGER:
		if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign(any_t( (int32)mA & (int32)mB ));
			return E_SUCCESS;
		}
		return E_TYPE;
	default:
		return E_TYPE;
	}
}

inline RESULT any_t::Or( const any_t& mA, const any_t& mB )
{
	switch( mA.m_nTag )
	{
	case TYPE_INTEGER:
		if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign( any_t( (int32)mA | (int32)mB ) );
			return E_SUCCESS;
		}
		return E_TYPE;
	default:
		return E_TYPE;
	}
}

inline RESULT any_t::Xor( const any_t& mA, const any_t& mB )
{
	switch( mA.m_nTag )
	{
	case TYPE_INTEGER:
		if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign( any_t( (int32)mA ^ (int32)mB ) );
			return E_SUCCESS;
		}
		return E_TYPE;
	default:
		return E_TYPE;
	}
}

inline RESULT any_t::Shr( const any_t& mA, const any_t& mB )
{
	switch( mA.m_nTag )
	{
	case TYPE_INTEGER:
		if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign( any_t( (int32)mA >> (int32)mB ) );
			return E_SUCCESS;
		}
		return E_TYPE;
	default:
		return E_TYPE;
	}
}

inline RESULT any_t::Shl( const any_t& mA, const any_t& mB )
{
	switch( mA.m_nTag )
	{
	case TYPE_INTEGER:
		if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign( any_t( (int32)mA << (int32)mB ) );
			return E_SUCCESS;
		}
		return E_TYPE;
	default:
		return E_TYPE;
	}
}

inline RESULT any_t::Pow( const any_t& mA, const any_t& mB )
{
	switch( mA.m_nTag )
	{
	case TYPE_INTEGER:
		if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign(any_t( (float64)pow( (double)(int32)mA, (double)(int32)mB ) ));
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_FLOAT) {
			this->Assign(any_t( (float64)pow( (double)(int32)mA, (double)(float64)mB ) ));
			return E_SUCCESS;
		}
		return E_TYPE;
	case TYPE_FLOAT:
		if (mB.m_nTag == TYPE_FLOAT) {
			this->Assign(any_t( (float64)pow( (double)(float64)mA, (double)(float64)mB ) ));
			return E_SUCCESS;
		} else if (mB.m_nTag == TYPE_INTEGER) {
			this->Assign(any_t( (float64)pow( (double)(float64)mA, (double)(int32)mB ) ));
			return E_SUCCESS;
		}
		return E_TYPE;
	default:
		return E_TYPE;
	}
}

inline bool any_t::Equ( const any_t& mB ) const
{
	switch( m_nTag )
	{
	case TYPE_UNDEFINED:
	case TYPE_NULL:
	case TYPE_DELETED:
		if (mB.m_nTag == TYPE_UNDEFINED || mB.m_nTag == TYPE_NULL ||
			mB.m_nTag == TYPE_DELETED) return true;
		return false;
	case TYPE_INTEGER:
		if (mB.m_nTag == TYPE_INTEGER) {
			return ( int32(*this) == int32(mB) );
		} else if (mB.m_nTag == TYPE_FLOAT) {
			return ( int32(*this) == float64(mB) );
		}
		return false;
	case TYPE_FLOAT:
		if (mB.m_nTag == TYPE_FLOAT) {
			return ( float64(*this) == float64(mB) );
		} else if (mB.m_nTag == TYPE_INTEGER) {
			return ( float64(*this) == int32(mB) );
		}
		return false;
	case TYPE_RANGE:
		if (mB.m_nTag == TYPE_RANGE) {
			return ( this->range().nLower == mB.range().nLower &&
				     this->range().nUpper == mB.range().nUpper );
		}
		return false;
	case TYPE_STRING:
		if (mB.m_nTag == TYPE_STRING) {
			return ( stringp(*this)->Compare( stringp(mB) ) == 0 );
		}
		return false;
	case TYPE_ARRAY:
		if (mB.m_nTag == TYPE_ARRAY) {
			return ( arrayp(*this) == arrayp(mB) );
		}
		return false;
	case TYPE_MAPPING:
		if (mB.m_nTag == TYPE_MAPPING) {
			return ( mappingp(*this) == mappingp(mB) );
		}
		return false;
	case TYPE_OBJECT:
	case TYPE_OBJECT_REF:
		if (mB.m_nTag == TYPE_OBJECT || mB.m_nTag == TYPE_OBJECT_REF ) {
			return ( objectp(*this) == objectp(mB) );
		}
		return false;
	case TYPE_PORT:
		if (mB.m_nTag == TYPE_PORT) {
			return ( portp(*this) == portp(mB) );
		}
		return false;
	case TYPE_THREAD:
		if (mB.m_nTag == TYPE_THREAD) {
			return ( threadp(*this) == threadp(mB) );
		}
		return false;
	case TYPE_FUNCTION:
		if (mB.m_nTag == TYPE_FUNCTION) {
			return ( functionp(*this) == functionp(mB) );
		}
		return false;
	case TYPE_FUNC_REF:
		if (mB.m_nTag == TYPE_FUNC_REF) {
			return ( func_ref_t(*this).fnValue == func_ref_t(mB).fnValue &&
					 func_ref_t(*this).oContext == func_ref_t(mB).oContext );
		}
		return false;
	default:
		return false;
	}
}

inline bool any_t::Neq( const any_t& mB ) const
{
	return ! Equ( mB );
}

inline bool any_t::Gt( const any_t& mB ) const
{
	switch( m_nTag )
	{
	case TYPE_INTEGER:
		if (mB.m_nTag == TYPE_INTEGER) {
			return ( int32(*this) > int32(mB) );
		} else if (mB.m_nTag == TYPE_FLOAT) {
			return ( int32(*this) > float64(mB) );
		}
		break;
	case TYPE_FLOAT:
		if (mB.m_nTag == TYPE_FLOAT) {
			return ( float64(*this) > float64(mB) );
		} else if (mB.m_nTag == TYPE_INTEGER) {
			return ( float64(*this) > int32(mB) );
		}
		break;
	case TYPE_STRING:
		if (mB.m_nTag == TYPE_STRING) {
			return ( stringp(*this)->Compare( stringp(mB) ) > 0 );
		}
		break;
	default:
		break;
	}
	// Not sure whether to partial order these.
	return false;
}

inline bool any_t::Lt( const any_t& mB ) const
{
	switch( m_nTag )
	{
	case TYPE_INTEGER:
		if (mB.m_nTag == TYPE_INTEGER) {
			return ( int32(*this) < int32(mB) );
		} else if (mB.m_nTag == TYPE_FLOAT) {
			return ( int32(*this) < float64(mB) );
		}
		break;
	case TYPE_FLOAT:
		if (mB.m_nTag == TYPE_FLOAT) {
			return ( float64(*this) < float64(mB) );
		} else if (mB.m_nTag == TYPE_INTEGER) {
			return ( float64(*this) < int32(mB) );
		}
		break;
	case TYPE_STRING:
		if (mB.m_nTag == TYPE_STRING) {
			return ( stringp(*this)->Compare( stringp(mB) ) < 0 );
		}
		break;
	default:
		break;
	}
	// Not sure whether to partial order these.
	return false;
}

inline bool any_t::GtEq( const any_t& mB ) const
{
	return ! Lt( mB );
}

inline bool any_t::LtEq( const any_t& mB ) const
{
	return ! Gt( mB );
}

inline bool any_t::Bool() const
{
	switch( Type() )
	{
	case TYPE_INTEGER:
		return (m_uData.nValue != 0);
	case TYPE_FLOAT:
		return (m_uData.fValue != 0.0);
	case TYPE_RANGE:
		return ((m_uData.rnValue.nLower | m_uData.rnValue.nUpper) != 0);
	case TYPE_STRING:
		return (m_uData.sValue->Length() > 0);
	case TYPE_ARRAY:
		return (m_uData.arValue->Size() > 0);
	case TYPE_MAPPING:
		return (m_uData.mpValue->Size() > 0);
	case TYPE_OBJECT:
	case TYPE_OBJECT_REF:
		return !m_uData.oValue->IsDeleted();
	case TYPE_PORT:
		return !m_uData.ptValue->IsClosed();
	case TYPE_THREAD:
		return !m_uData.thValue->IsStopped();
	case TYPE_FUNCTION:
	case TYPE_FUNC_REF:
		return true;
	default:
		return false;
	}
}

inline int32 any_t::Size() const
{
	switch( Type() )
	{
	case TYPE_INTEGER:
	case TYPE_FLOAT:
	case TYPE_RANGE:
		return 1;
	case TYPE_STRING:
		return m_uData.sValue->Length();
	case TYPE_ARRAY:
		return m_uData.arValue->Size();
	case TYPE_MAPPING:
		return m_uData.mpValue->Size();
	case TYPE_OBJECT:
	case TYPE_OBJECT_REF:
		return m_uData.oValue->Size();
	case TYPE_PORT:
	case TYPE_THREAD:
	case TYPE_FUNCTION:
	case TYPE_FUNC_REF:
		return 1;
	default:
		return 0;
	}
}

inline bool any_t::operator<( const any_t& mOther ) const
{
	// Must form a partial ordering for STL map
	if( m_nTag == mOther.m_nTag )
	{
		switch( m_nTag )
		{
		case TYPE_UNDEFINED:
		case TYPE_NULL:
		case TYPE_DELETED:
			return false; // all null were created equal
		case TYPE_INTEGER:
			return m_uData.nValue < mOther.m_uData.nValue;
		case TYPE_FLOAT:
			return m_uData.fValue < mOther.m_uData.fValue;
		case TYPE_RANGE:
			if( m_uData.rnValue.nLower == mOther.m_uData.rnValue.nLower )
				return m_uData.rnValue.nUpper < mOther.m_uData.rnValue.nUpper;
			return m_uData.rnValue.nLower < mOther.m_uData.rnValue.nLower;
		case TYPE_STRING:
			// Special case - string references use value semantics
			return m_uData.sValue->Less( mOther.m_uData.sValue );
		case TYPE_ARRAY:
			return m_uData.arValue < mOther.m_uData.arValue;
		case TYPE_MAPPING:
			return m_uData.mpValue < mOther.m_uData.mpValue;
		case TYPE_OBJECT:
		case TYPE_OBJECT_REF:
			return m_uData.oValue < mOther.m_uData.oValue;
		case TYPE_PORT:
			return m_uData.ptValue < mOther.m_uData.ptValue;
		case TYPE_THREAD:
			return m_uData.thValue < mOther.m_uData.thValue;
		case TYPE_FUNCTION:
			return m_uData.fValue < mOther.m_uData.fValue;
		case TYPE_FUNC_REF:
			if( m_uData.frValue.fnValue == mOther.m_uData.frValue.fnValue )
				return m_uData.frValue.oContext < mOther.m_uData.frValue.oContext;
			return m_uData.frValue.fnValue < mOther.m_uData.frValue.fnValue;
		default:
			// Other ref types should not be used
			ASSERT( 0 && "invalid value in less operator" );
			return false;
		}
	}
	else
	{
		// Order based on type
		return m_nTag < mOther.m_nTag;
	}
}

#endif // FIREFLY_ANY_IMPL_H
