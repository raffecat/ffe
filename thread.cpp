// thread.cpp
// FireFly server 'thread' data type

#include "thread.h"
#include "database.h"
#include "string.h"
#include "array.h"
#include "mapping.h"
#include "object.h"
#include "function.h"
#include "port.h"
#include "opcode.h"
#include "any_impl.h"
#include "parse.h"
#include "runtime.h"

#define EXEC_TIMEOUT		1000000
#define FORCED_YIELD_TICKS	1000

#define reset		goto interp_reset;
#define nextop		goto interp_nextop;
#define yield		goto interp_yield;

#define CheckError( nExpr ) { nResult = nExpr; if( ISERROR(nResult) ) goto interp_raise; }
#define RaiseError( nError ) { nResult = nError; goto interp_raise; }

#ifdef RUNTIME_ASSERTIONS
#define RT_ASSERT(X) if( !(X) ) RaiseError( E_ASSERT )
#else
#define RT_ASSERT(X) ASSERT(X)
#endif

extern const char *g_szOpcodes[];
extern const char* g_szSystemErrors[];

const char* g_szErrorMessage = NULL;


// msw_t implementation
msw_t::msw_t() : m_oContext(NULL), m_fnCode(NULL), m_nRegisters(0),
	m_nCatchReg(0), m_nCatchHandler(0)
{
}

msw_t::~msw_t()
{
	// Release references to objects
	if( m_oContext ) m_oContext->Release();
	if( m_fnCode ) m_fnCode->Release();
	m_oContext = NULL;
	m_fnCode = NULL;

	if( m_nRegisters )
	{
		// Release the contents of all registers
		for( int i = 0; i < m_nRegisters; i++ )
		{
			m_pRegisters[i].Release();
		}

		delete [] m_pRegisters;
		m_pRegisters = NULL;
		m_nRegisters = 0;
	}
}

thread_t::~thread_t()
{
#ifdef DEBUG_REF
	const char* szName = "unknown";
	if( m_fnMain && m_fnMain->m_sName ) szName = m_fnMain->m_sName->CString();
	printf( "[thread %08x destroy: %ld refs]\n", this, (long)m_nRef );
#endif
	g_oIdxThread.Remove( this );

	ASSERT( m_nState == TS_STOPPED );
	ASSERT( m_mswCurrent == NULL );

	if( m_oContext ) m_oContext->Release();
	if( m_fnMain ) m_fnMain->Release();
	m_oContext = NULL;
	m_fnMain = NULL;

	// State members are real references
	if( m_ptStatePort ) m_ptStatePort->Release();
	if( m_sStateStr ) m_sStateStr->Release();
	m_ptStatePort = NULL;
	m_sStateStr = NULL;

	m_mResult.Release();
}


// Public interface

void thread_t::SetupThread( functionp fnMain, objectp oContext, arrayp arArgs )
{
	m_oContext = oContext;
	m_fnMain = fnMain;

	oContext->AddRef();
	fnMain->AddRef();

	// Create the initial machine status word
	SetupCall( oContext, fnMain, arArgs, -1 );

	// Mark the thread as ready
	m_nState = TS_READY;
}

void thread_t::AbortThread( const any_t& mValue )
{
	if( m_mswCurrent )
	{
		// End the current function and set return value
		FinishCall( mValue, true );

		// End all functions in the call stack
		while( m_mswCurrent )
		{
			FinishCall( g_null, false );
		}
	}
}

RESULT thread_t::Execute( int& nRemain )
{
	RESULT nResult = E_SUCCESS;

	// In case we were killed while not active
	if( m_nState == TS_STOPPED ) return E_STOPPED;

	RT_ASSERT_RET( m_mswCurrent != NULL );
	RT_ASSERT_RET( m_mswCurrent->m_fnCode != NULL );
	RT_ASSERT_RET( m_mswCurrent->m_pRegisters != NULL );

//if( m_mswCurrent ) printf( "context switch: %p '%s' (state %d)\n", this, m_mswCurrent->m_fnCode->m_sName->CString(), (int)m_nState );
//else printf( "context switch: %p unknown (state %d)\n", this, (int)m_nState );

	int nTicks = EXEC_TIMEOUT;
	int nOp, nA, nB, nC;
	byte* pRead;

	// Set up the interpreter (must do before checking state)
	uint32* pCode = m_mswCurrent->m_fnCode->m_pCode;
	any_t* pReg = m_mswCurrent->m_pRegisters;
	uint32* pInst = &pCode[m_mswCurrent->m_nIP];

	int impending_doom = 0; // will die unless thread yields.

	// Check if the thread is ready to execute
	switch( m_nState )
	{
	case TS_READY:
		break;

	case TS_SLEEP:
		{
			// Check if the wake-up time has arrived
			time_t nTime = time( NULL );
			if( nTime < m_nStateTime )
			{
				nRemain = (int)( m_nStateTime - nTime );
				return E_FALSE;
			}
			m_nState = TS_READY;
		}
		break;

	case TS_READ:
		{
			// Poll the blocking port for activity
			int32 nSize = (int32)m_nStateTime;
			stringp sResult = NULL;
			CheckError( m_ptStatePort->Read( nSize, sResult ) );
			if( nResult == E_FALSE ) return E_FALSE;
			// Release the state port
			m_ptStatePort->Release();
			m_ptStatePort = NULL;
			// Assign the new string to the target register
			m_mswCurrent->m_pRegisters[m_nStateReg].AssignRef( any_t(sResult) ); // ownership
			m_nState = TS_READY;
		}
		break;

	case TS_READLN:
		{
			// Poll the blocking port for activity
			stringp sResult = NULL;
			CheckError( m_ptStatePort->ReadLine( sResult ) );
			if( nResult == E_FALSE ) return E_FALSE;
			// Release the state port
			m_ptStatePort->Release();
			m_ptStatePort = NULL;
			// Assign the new string to the target register
			m_mswCurrent->m_pRegisters[m_nStateReg].AssignRef( any_t(sResult) ); // ownership
			m_nState = TS_READY;
		}
		break;

	case TS_WRITE:
		{
			// Retry (resume) the write operation
			CheckError( m_ptStatePort->Write( m_sStateStr ) );
			if( nResult == E_FALSE ) return E_FALSE;
			// Release the state port and state string
			m_ptStatePort->Release();
			m_sStateStr->Release();
			m_ptStatePort = NULL;
			m_sStateStr = NULL;
			m_nState = TS_READY;
		}
		break;

	case TS_ACCEPT:
		{
			// Poll the blocking port for activity
			portp ptResult = NULL;
			CheckError( m_ptStatePort->Accept( ptResult ) );
			if( nResult == E_FALSE ) return E_FALSE;
			// Release the state port
			m_ptStatePort->Release();
			m_ptStatePort = NULL;
			// Assign the new port to the target register
			m_mswCurrent->m_pRegisters[m_nStateReg].AssignRef( any_t(ptResult) ); // ownerhsip
			m_nState = TS_READY;
		}
		break;

	default:
		// Unknown current state or already stopped
		m_nState = TS_STOPPED;
		return E_FALSE;
	}

interp_reset:
	// Set up the interpreter
	pCode = m_mswCurrent->m_fnCode->m_pCode;
	pReg = m_mswCurrent->m_pRegisters;
	pInst = &pCode[m_mswCurrent->m_nIP];

interp_nextop:
	if( !nTicks-- ) goto forced_yield;

	// Fetch and decode the next opcode
	pRead = (byte*)pInst++;
	nOp = pRead[0];
	nA = pRead[1];
	nB = pRead[2];
	nC = pRead[3];

//if( nOp < NUM_OPCODES ) printf( "[%4d] %s (%d) %d %d %d (%d)\n", (int)(pInst - pCode), g_szOpcodes[nOp], nOp, nA, nB, nC );
//else printf( "[%4d] unknown (%d) %d %d %d (%d)\n", (int)(pInst - pCode), nOp, nA, nB, nC );

	switch( nOp )
	{
	case OP_COPY:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].Assign( mB ); // new ref
		}
		nextop;

	case OP_ASSIGN:
		{
			any_t mB = Dereference( pReg, nB );
			if (mB.IsString()) {
				ASSERT(stringp(mB)->GetRefCount() > 0);
			}
			// REQUIRES mA to be a reference.
			// Assign a value to a member or local
			switch( pReg[nA].Type() )
			{
			case TYPE_ATT_NAME:
				{
					objectp oContext = pReg[nA].att_name().oValue; // borrow
					stringp sName = pReg[nA].att_name().sName; // borrow
					if (!oContext->Assign(sName, mB)) { // new refs
						RaiseError(E_REPLACE);
					}
				}
				nextop;
			case TYPE_ARR_INDEX:
				pReg[nA].arr_index().arValue->Assign( pReg[nA].arr_index().nIndex, mB ); // new ref
				nextop;
			case TYPE_ARR_SLICE:
				RaiseError(E_TYPE); // was broken.
			case TYPE_MAP_KEY:
				pReg[nA].map_key().mpValue->Assign( pReg[ pReg[nA].map_key().nReg ], mB ); // new ref
				nextop;
			default:
				// Assign to local variable (register)
				RT_ASSERT( nA > 0 && nA < MAX_LOCALS );
				pReg[nA].Assign( mB ); // new ref
			}
		}
		nextop;

    case OP_ADD:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			CheckError( pReg[nA].Add( mB, mC ) );
		}
        nextop;

    case OP_SUB:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			CheckError( pReg[nA].Sub( mB, mC ) );
		}
        nextop;

    case OP_MUL:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			CheckError( pReg[nA].Mul( mB, mC ) );
		}
        nextop;

    case OP_DIV:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			CheckError( pReg[nA].Div( mB, mC ) );
		}
        nextop;

    case OP_MOD:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			CheckError( pReg[nA].Mod( mB, mC ) );
		}
        nextop;

    case OP_AND:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			CheckError( pReg[nA].And( mB, mC ) );
		}
        nextop;

    case OP_OR:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			CheckError( pReg[nA].Or( mB, mC ) );
		}
        nextop;

    case OP_XOR:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			CheckError( pReg[nA].Xor( mB, mC ) );
		}
        nextop;

    case OP_SHR:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			CheckError( pReg[nA].Shr( mB, mC ) );
		}
        nextop;

    case OP_SHL:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			CheckError( pReg[nA].Shl( mB, mC ) );
		}
        nextop;

    case OP_POW:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			CheckError( pReg[nA].Pow( mB, mC ) );
		}
        nextop;

    case OP_EQ:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			pReg[nA].AssignRef(any_t(int32( mB.Equ(mC) ? 1 : 0 )));
		}
        nextop;

    case OP_NEQ:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			pReg[nA].AssignRef(any_t(int32( mB.Neq(mC) ? 1 : 0 )));
		}
        nextop;

    case OP_GT:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			pReg[nA].AssignRef(any_t(int32( mB.Gt(mC) ? 1 : 0 )));
		}
        nextop;

    case OP_LT:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			pReg[nA].AssignRef(any_t(int32( mB.Lt(mC) ? 1 : 0 )));
		}
        nextop;

    case OP_GE:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			pReg[nA].AssignRef(any_t(int32( mB.GtEq(mC) ? 1 : 0 )));
		}
        nextop;

    case OP_LE:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			pReg[nA].AssignRef(any_t(int32( mB.LtEq(mC) ? 1 : 0 )));
		}
        nextop;

    case OP_IF:
		{
			any_t mA = Dereference( pReg, nA );
			int nBC = ((unsigned short*)pRead)[1];
			if (mA.Bool()) pInst = &pCode[nBC];
		}
        nextop;

    case OP_IFZ:
		{
			any_t mA = Dereference( pReg, nA );
			int nBC = ((unsigned short*)pRead)[1];
			if (!mA.Bool()) pInst = &pCode[nBC];
		}
        nextop;

	case OP_GOTO:
		{
			int nBC = ((unsigned short*)pRead)[1];
			pInst = &pCode[nBC];
		}
        nextop;

    case OP_RETURN:
		if( nA != INVALID_REGISTER )
		{
			any_t mA = Dereference( pReg, nA );
			FinishCall( mA, false );
		}
		else
		{
			// No return value specified
			FinishCall( g_null, false );
		}
		if( m_nState == TS_STOPPED ) return E_STOPPED;
        reset; // context has changed

    case OP_EXIT:
		if( nA != INVALID_REGISTER )
		{
			any_t mA = Dereference( pReg, nA );
			AbortThread( mA );
		}
		else
		{
			// No return value specified
			AbortThread( g_null );
		}
        return E_STOPPED;

    case OP_DELETE:
		{
			any_t mA = Dereference( pReg, nA );
			if( !mA.IsObject() ) RaiseError( E_TYPE );
			CheckError( objectp(mA)->Destroy() );
		}
        nextop;

    case OP_INVOKE: // reg, nargs, ret
		{
			any_t mA = Dereference( pReg, nA );
			switch( mA.Type() )
			{
			case TYPE_FUNC_REF:
				{
					// Check the function
					if( !func_ref_t(mA).fnValue->m_sName ) RaiseError( E_INTERNAL );
					// Build the argument list
					arrayp arList = BuildRuntimeList( pReg, pInst, nB );
					// Save the instruction pointer
					m_mswCurrent->m_nIP = pInst - pCode;
					// Invoke the function
					SetupCall( func_ref_t(mA).oContext, func_ref_t(mA).fnValue, arList, nC );
					arList->Release();
				}
				reset; // context has changed

			case TYPE_FUNCTION:
				RaiseError( E_CONTEXT );

			default:
				RaiseError( E_INVOKE );
			}
		}
        nextop;

    case OP_THREAD: // reg, nargs, ret
		{
			any_t mA = Dereference( pReg, nA );
			switch( mA.Type() )
			{
			case TYPE_FUNC_REF:
				{
					// Check the function
					if( !func_ref_t(mA).fnValue->m_sName ) RaiseError( E_INTERNAL );
					// Create the new thread object
					threadp thResult = new thread_t;
					// Assign the thread object to the result reg
					pReg[nC].AssignRef( any_t(thResult) ); // ownership
					// Build the argument list
					arrayp arList = BuildRuntimeList( pReg, pInst, nB );
					// Launch the new thread
					thResult->SetupThread( func_ref_t(mA).fnValue, func_ref_t(mA).oContext, arList );
					arList->Release();
					// Add the thread to the scheduler list
					arrayp arDispatch = g_oDatabase.DispatchList(); // borrow
					arDispatch->AppendItem( any_t(thResult) ); // new ref
				}
				break;

			case TYPE_FUNCTION:
				RaiseError( E_CONTEXT );

			default:
				RaiseError( E_TYPE );
			}
		}
        nextop;

    case OP_RANGE:
		{
			int32 nLower = 0;
			int32 nUpper = -1;
			if( nB != INVALID_REGISTER )
			{
				any_t mB = Dereference( pReg, nB );
				if( !mB.IsInt() ) RaiseError( E_TYPE );
				nLower = int32(mB);
			}
			if( nC != INVALID_REGISTER )
			{
				any_t mC = Dereference( pReg, nC );
				if( !mC.IsInt() ) RaiseError( E_TYPE );
				nUpper = int32(mC);
			}
			pReg[nA].AssignRef( any_t(nLower, nUpper) ); // ownership
		}
        nextop;

    case OP_ARRAY: // ret, args
		{
			arrayp arList = BuildRuntimeList( pReg, pInst, nB ); // new array
			pReg[nA].AssignRef( any_t(arList) ); // ownership
		}
        nextop;

    case OP_INDEX: // ret, target, index
		{
			any_t mB = Dereference( pReg, nB ); // borrow target
			any_t mC = Dereference( pReg, nC ); // borrow index

			switch( mB.Type() )
			{
			case TYPE_OBJECT:
			case TYPE_OBJECT_REF:
				{
					// Build an att_name_t for Dereference and OP_ASSIGN.
					if( mC.IsString() ) {
						pReg[nA].Assign( any_t(objectp(mB), stringp(mC)) );
						nextop;
					}
				}
				break;

			case TYPE_ARRAY:
				{
					if (mC.IsRange()) {
                        // Slice the array now, since we no longer support assignment to sliced arrays.
                        // This used to generate arr_slice_t in reg[A] for Dereference.
                        pReg[nA].AssignRef(arrayp(mB)->Slice(mC.range())); // ownership of new array.
						nextop;
					} else if (mC.IsInt()) {
						// Build an arr_index_t for Dereference and OP_ASSIGN.
						pReg[nA].AssignArrIndex( arrayp(mB), int32(mC) );
						nextop;
					}
				}
				break;

			case TYPE_MAPPING:
				{
                    // Since we capture register nC here, we must also dereference it in-place
                    // so a map_key_t Dereference or an OP_ASSIGN can access it directly.
                    pReg[nC].Assign(mC); // new ref.
                    
					// Build a map_key_t for Dereference and OP_ASSIGN.
					pReg[nA].Assign( any_t(mappingp(mB), int32(nC)) );
					nextop;
				}
				break;

			case TYPE_STRING:
				{
					switch( mC.Type() )
					{
					case TYPE_INTEGER:
						// Extract a character substring
						{
							stringp sResult = stringp(mB)->SubString( int32(mC), int32(mC) ); // new
							pReg[nA].AssignRef( any_t(sResult) ); // ownership
						}
						nextop;

					case TYPE_RANGE:
						// Extract a substring
						{
							stringp sResult = stringp(mB)->SubString( mC.range().nLower, mC.range().nUpper ); // new
							pReg[nA].AssignRef( any_t(sResult) ); // ownership
						}
						nextop;

					default:
						break;
					}
				}
				break;

			default:
				break;
			}
			pReg[nA].AssignNull(); // invalid lookups result in null.
		}
		nextop;

    case OP_CATCH:
		{
			int nBC = ((unsigned short*)pRead)[1];
			if( nBC )
			{
				// Set up the catch handler
				// Need to implement a stack for nesting these
				// (but only because the parser doesn't build a parse tree)
				// Actually just allocate a register for each catch block and use them as a stack
				RT_ASSERT( m_mswCurrent->m_nCatchHandler == 0 );
				m_mswCurrent->m_nCatchReg = nA;
				m_mswCurrent->m_nCatchHandler = nBC;
			}
			else
			{
				// Restore the previous catch handler
				// This has already been done if an error was caught..
				// So emit this before the goto, at the end of the try..catch body!
				m_mswCurrent->m_nCatchReg = 0;
				m_mswCurrent->m_nCatchHandler = 0;
			}
		}
        nextop;

    case OP_FOREACH: // ctrl, local, target
		{
			any_t mC = Dereference( pReg, nC );
			uint32 nGoto = ((unsigned short*)pInst)[1];
			pInst++;

            // Must dereference mC in-place so OP_NEXTEACH doesn't have to
            // do it every time through the loop.
            pReg[nC].Assign(mC); // new ref.

			arrayp arIterate = NULL;
			switch( mC.Type() )
			{
			case TYPE_OBJECT:
			case TYPE_OBJECT_REF:
				// Build a list of keys in the object.
				arIterate = objectp(mC)->ListAttribs(); // new array
				break;

			case TYPE_ARRAY:
				arIterate = arrayp(mC); // borrow
				arIterate->AddRef(); // new ref.
				break;

			case TYPE_MAPPING:
				// Build a list of keys in the mapping
				arIterate = mappingp(mC)->ListKeys(); // new array
				break;
		
			default:
				// Jump over the loop body if type cannot be iterated over.
				pInst = &pCode[nGoto];
				nextop;
			}

			// Check if there are any elements to iterate.
			if (arIterate->Size() < 1) {
				// Jump over the loop body if no elements.
				arIterate->Release();
				pInst = &pCode[nGoto];
			} else {
				// Set up the iterator.
				pReg[nA].AssignRef( any_t(1, arIterate) ); // ownership
				// Get the first element.
				any_t mValue = arIterate->Lookup(0); // borrow
				pReg[nB].Assign( mValue ); // new ref
			}

		}
		nextop;

	case OP_NEXTEACH: // ctrl, local, target
		{
			if (!pReg[nA].IsForEach()) RaiseError(E_INTERNAL); // stomped register.
			switch( pReg[nC].Type() )
			{
			case TYPE_OBJECT:
			case TYPE_OBJECT_REF:
			case TYPE_ARRAY:
			case TYPE_MAPPING:
				{
					// Retrieve the foreach iterator bits
					RT_ASSERT( nA != INVALID_REGISTER );
					RT_ASSERT( nB != INVALID_REGISTER );
					arrayp arIterate = for_each_t(pReg[nA]).arIterate; // borrow
					int32 nIndex = pReg[nA].Iterate();
					uint32 nGoto = ((unsigned short*)pInst)[1];
					pInst++;
					// Check if more items to iterate over
					if (nIndex < arIterate->Size()) {
						// Extract the next value from the array
						any_t mValue = arIterate->Lookup(nIndex); // borrow
						pReg[nB].Assign( mValue ); // new ref
						// Jump back to the loop body
						pInst = &pCode[nGoto];
					}
				}
				break;

			default:
				RaiseError( E_TYPE ); // should not happen.
			}
		}
		nextop;

    case OP_THIS:
		{
			// Copy a reference to "this" into the destination
			pReg[nA].Assign( pReg[0] ); // new ref
		}
        nextop;

    case OP_ACCESS:
		{
			any_t mB = Dereference( pReg, nB );
			// nC should contain a string because we always emit an OP_CONST
			// before OP_ACCESS. Verify here to avoid a crash/hack.
			if (mB.IsObject() && pReg[nC].IsString()) {
				// Build an att_name_t
				pReg[nA].Assign( any_t(objectp(mB), stringp(pReg[nC])) ); // new refs
			} else {
				pReg[nA].AssignNull(); // (non-object).(anything) == null
			}
		}
        nextop;

    case OP_LOOKUP:
		{
			int nBC = ((unsigned short*)pRead)[1];
			any_t mName = m_mswCurrent->m_fnCode->m_arConstants->Lookup(nBC); // borrow
			RT_ASSERT( mName.IsString() );
			// reg[0] should be 'this' but guard against hack.
			if (pReg[0].IsObject()) {
				// Build an att_name_t
				pReg[nA].Assign( any_t(objectp(pReg[0]), stringp(mName)) ); // new refs
			}
		}
        nextop;

    case OP_NOT:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef(any_t(int32( mB.Bool() ? 0 : 1 ))); // ownership
		}
        nextop;

    case OP_BITNOT:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsInt() ) RaiseError( E_TYPE );
			pReg[nA].AssignRef( any_t(int32( ~int32(mB) )) );
		}
        nextop;

    case OP_NEG:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsInt() ) RaiseError( E_TYPE );
			pReg[nA].AssignRef(any_t( int32(0 - int32(mB)) ));
		}
        nextop;

    case OP_CONST:
		{
			int nBC = ((unsigned short*)pRead)[1];
			any_t mValue = m_mswCurrent->m_fnCode->m_arConstants->Lookup(nBC); // borrow
			pReg[nA].Assign( mValue ); // new ref
		}
        nextop;

    case OP_SETNULL:
		{
			pReg[nA].AssignNull();
		}
        nextop;

	case OP_SETROOT:
		{
			pReg[nA].Assign( any_t( g_oDatabase.RootObject() ) ); // new ref
		}
        nextop;

    case OP_SWITCH:
		{
			// Retrieve the switch info array
			any_t mA = Dereference( pReg, nA );
			int nBC = ((unsigned short*)pRead)[1];
			any_t mInfo = m_mswCurrent->m_fnCode->m_arConstants->Lookup(nBC); // borrow
			RT_ASSERT( mInfo.IsArray() );
			arrayp arInfo = arrayp(mInfo); // borrow

			// Find the matching case value
			int32 nSize = arInfo->Size();
			for (int32 i = 1; i < nSize; i += 2) {
				if (mA.Equ(arInfo->Lookup(i))) {
					// Branch to the address for this value
					pInst = &pCode[ int32(arInfo->Lookup(i+1)) ];
					nextop;
				}
			}

			// No matching value, use the default address
			pInst = &pCode[ int32(arInfo->Lookup(0)) ];
		}
        nextop;

    case OP_NEW_OBJECT:
		{
			// Create an object owned by the context of this thread
			// Arguments may be ( string full_path ) or
			// ( string relative_path, object parent ) or ( object template )
			any_t mB = Dereference( pReg, nB );
			if( mB.IsObject() )
			{
				if( nC != INVALID_REGISTER ) RaiseError( E_ARGS ); // too many arguments

				// Create a new instance of a template object
				objectp oNew = NULL;
				CheckError( objectp(mB)->CreateClone( m_oContext, oNew ) ); // result is borrowed!
				pReg[nA].Assign( any_t(oNew) ); // keep a new ref
			}
			else if( mB.IsString() )
			{
				// Resolve the context object if supplied
				objectp oContext = NULL;
				if( nC != INVALID_REGISTER )
				{
					any_t mC = Dereference( pReg, nC ); // borrow
					if( !mC.IsObject() ) RaiseError( E_TYPE );
					oContext = objectp(mC); // borrow
				}

				// Attempt to create the object
				objectp oNew = NULL;
				CheckError( g_oDatabase.FindCreateObject( stringp(mB), oContext, m_oContext, true, oNew ) ); // borrow
				if( nResult == E_FALSE ) RaiseError( E_OBJECT_EXISTS );

				pReg[nA].Assign( any_t(oNew) ); // new ref
			}
			else RaiseError( E_TYPE );
		}
        nextop;

    case OP_NEW_ARRAY:
		{
			// Check if a size has been specified
			int32 nSize = 0;
			if( nB != INVALID_REGISTER )
			{
				any_t mB = Dereference( pReg, nB );
				if( !mB.IsInt() ) RaiseError( E_TYPE );
				nSize = int32(mB);
			}
			// Create the array
			arrayp arNew = new array_t();
			if (nSize > 1000) nSize = 1000; // apply some limits.
            while( nSize-- ) arNew->AppendItem(g_null);
			pReg[nA].AssignRef( any_t(arNew) ); // ownership
		}
        nextop;

    case OP_NEW_MAPPING:
		{
			mappingp mpNew = new mapping_t;
			pReg[nA].AssignRef( any_t(mpNew) ); // ownership
		}
        nextop;

    case PF_UNDEFINEDP: // never used any of these.
    case PF_NULLP:
    case PF_DELETEDP:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t(int32( (mB.IsNull() || mB.IsUndefined() || mB.IsDeleted()) ? 1 : 0 )) );
		}
        nextop;

    case PF_INTEGERP:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t(int32( mB.IsInt() ? 1 : 0 )) );
		}
        nextop;

    case PF_REALP:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t(int32( mB.IsFloat() ? 1 : 0 )) );
		}
        nextop;

    case PF_NUMBERP:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t(int32( (mB.IsInt() || mB.IsFloat()) ? 1 : 0 )) );
		}
        nextop;

    case PF_RANGEP:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t(int32( mB.IsRange() ? 1 : 0 )) );
		}
        nextop;

    case PF_STRINGP:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t(int32( mB.IsString() ? 1 : 0 )) );
		}
        nextop;

    case PF_ARRAYP:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t(int32( mB.IsArray() ? 1 : 0 )) );
		}
        nextop;

    case PF_MAPPINGP:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t(int32( mB.IsMapping() ? 1 : 0 )) );
		}
        nextop;

    case PF_OBJECTP:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t(int32( mB.IsObject() ? 1 : 0 )) );
		}
        nextop;

    case PF_FUNCTIONP:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t(int32( (mB.IsFunction() || mB.IsFuncRef()) ? 1 : 0 )) );
		}
        nextop;

    case PF_PORTP:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t(int32( mB.IsPort() ? 1 : 0 )) );
		}
        nextop;

    case PF_THREADP:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t(int32( mB.IsThread() ? 1 : 0 )) );
		}
        nextop;

    case PF_CLONEP:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t(int32( (mB.IsObject() && objectp(mB)->IsClone()) ? 1 : 0 )) );
		}
        nextop;

    case PF_CHILDP:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t(int32( mB.IsChild() ? 1 : 0 )) );
		}
        nextop;

    case PF_TO_INT:
		{
			any_t mB = Dereference( pReg, nB );
			switch( mB.Type() )
			{
			case TYPE_INTEGER:
				pReg[nA].AssignRef( mB );
				break;

			case TYPE_FLOAT:
				pReg[nA].AssignRef( any_t(int32( floor( double(float64(mB)) ) )) );
				break;

			case TYPE_STRING:
				{
					int nParsed = 0;
					long nValue = 0;
					sscanf( stringp(mB)->CString(), "%ld%n", &nValue, &nParsed );
					if( nParsed ) pReg[nA].AssignRef( any_t(int32(nValue)) );
					else pReg[nA].AssignNull();
				}
				break;

			default:
				pReg[nA].AssignNull();
			}
		}
		nextop;

    case PF_TO_FLOAT:
		{
			any_t mB = Dereference( pReg, nB );
			switch( mB.Type() )
			{
			case TYPE_INTEGER:
				pReg[nA].AssignRef( any_t(float64( double(int32(mB)) )) );
				break;

			case TYPE_FLOAT:
				pReg[nA].AssignRef( mB );
				break;

			case TYPE_STRING:
				{
					int nParsed = 0;
					double dValue = 0.0;
					sscanf( stringp(mB)->CString(), "%lf%n", &dValue, &nParsed );
					if( nParsed ) pReg[nA].AssignRef( any_t(float64(dValue)) );
					else pReg[nA].AssignNull();
				}
				break;

			default:
				pReg[nA].AssignNull();
			}
		}
        nextop;

    case PF_TO_STR:
		// Coerce any non-string value into a display string
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef(any_t(mB.ToString())); // ownership of new
		}
        nextop;

	case PF_VERSION:
		{
			stringp sVersion = new string_t( FIREFLY_VERSION_STRING ); // new
			pReg[nA].AssignRef(any_t(sVersion)); // ownership
		}
		nextop;

	case PF_SLEEP:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsInt() ) RaiseError( E_TYPE );
			if( int32(mB) > 0 )
			{
				m_nState = TS_SLEEP;
				m_nStateTime = time(NULL) + int32(mB);
				yield;
			}
			pReg[nA].AssignNull(); // TODO: remove the need for this?
		}
		nextop;

	case PF_LISTEN:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsInt() ) RaiseError( E_TYPE );
			// Create the new port
			portp ptNew = new port_t;
			pReg[nA].AssignRef( any_t(ptNew) ); // ownership
			// Begin listening on the specified TCP/IP port
			CheckError( ptNew->Listen( int32(mB) ) );
		}
		nextop;

	case PF_ACCEPT:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsPort() ) RaiseError( E_TYPE );
			// Accept on the port
			portp ptTarget = portp(mB); // borrow
			portp ptNew = NULL;
			CheckError( ptTarget->Accept( ptNew ) );
			if( nResult == E_FALSE )
			{
				// Block the thread until a connection arrives
				m_ptStatePort = ptTarget; ptTarget->AddRef(); // new ref
				m_nStateReg = nA;
				m_nState = TS_ACCEPT;
				yield;
			}
			pReg[nA].AssignRef( any_t(ptNew) ); // ownership
		}
		nextop;

	case PF_WRITE:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			if( !mB.IsPort() ) RaiseError( E_TYPE );
			if( !mC.IsString() ) RaiseError( E_TYPE );
			// Write data to the port
			portp ptTarget = portp(mB);
			stringp sValue = stringp(mC);
			CheckError( ptTarget->Write( sValue ) );
			if( nResult == E_FALSE )
			{
				// Block the thread until write is allowed
				m_ptStatePort = ptTarget; ptTarget->AddRef(); // new ref
				m_sStateStr = sValue; sValue->AddRef(); // new ref
				m_nState = TS_WRITE;
				yield;
			}
			pReg[nA].AssignNull(); // TODO: remove the need for this?
		}
		nextop;

	case PF_WRITE_TEXT:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			if( !mB.IsPort() ) RaiseError( E_TYPE );
			if( !mC.IsString() ) RaiseError( E_TYPE );
			// Create a copy of the string and replace "\n" with "\r\n"
			stringp sCopy = stringp(mC)->ReplaceNewline(); // new.
			// Write text to the port
			portp ptTarget = portp(mB);
			nResult = ptTarget->Write( sCopy );
			if( nResult == E_FALSE )
			{
				// Block the thread until write is allowed
				m_ptStatePort = ptTarget; ptTarget->AddRef(); // new ref
				m_sStateStr = sCopy; // ownership
				m_nState = TS_WRITE;
				yield;
			}
			sCopy->Release(); // release before checking for error
			CheckError( nResult );
			pReg[nA].AssignNull(); // TODO: remove the need for this?
		}
		nextop;

	case PF_READ:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			if( !mB.IsPort() ) RaiseError( E_TYPE );
			if( !mC.IsInt() ) RaiseError( E_TYPE );
			// Read data from the port
			portp ptTarget = portp(mB);
			int32 nSize = int32(mC);
			stringp sResult = NULL;
			CheckError( ptTarget->Read( nSize, sResult ) );
			if( nResult == E_FALSE )
			{
				// Block the thread until read is allowed
				m_ptStatePort = ptTarget; ptTarget->AddRef(); // new ref
				m_nStateTime = nSize;
				m_nStateReg = nA;
				m_nState = TS_READ;
				yield;
			}
			pReg[nA].AssignRef( any_t(sResult) ); // ownership
		}
		nextop;

	case PF_READLN:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsPort() ) RaiseError( E_TYPE );
			// Read a line from the port
			portp ptTarget = portp(mB);
			stringp sResult = NULL;
			CheckError( ptTarget->ReadLine( sResult ) );
			if( nResult == E_FALSE )
			{
				// Block the thread until read is allowed
				m_ptStatePort = ptTarget; ptTarget->AddRef(); // new ref
				m_nStateReg = nA;
				m_nState = TS_READLN;
				yield;
			}
			pReg[nA].AssignRef( any_t(sResult) ); // ownership
		}
		nextop;

	case PF_CLOSE:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsPort() ) RaiseError( E_TYPE );
			CheckError( portp(mB)->Close() );
			pReg[nA].AssignNull(); // TODO: remove the need for this?
		}
		nextop;

	case PF_TRIM:
		{
			any_t mB = Dereference( pReg, nB );
			if (mB.IsString()) pReg[nA].AssignRef(any_t( stringp(mB)->Trim() )); // ownership of new
			else pReg[nA].AssignNull();
		}
		nextop;

	case PF_LOWER:
		{
			any_t mB = Dereference( pReg, nB );
			if (mB.IsString()) pReg[nA].AssignRef(any_t( stringp(mB)->ToLower() )); // ownership of new
			else pReg[nA].AssignNull();
		}
		nextop;

	case PF_UPPER:
		{
			any_t mB = Dereference( pReg, nB );
			if (mB.IsString()) pReg[nA].AssignRef(any_t( stringp(mB)->ToUpper() )); // ownership of new
			else pReg[nA].AssignNull();
		}
		nextop;

	case PF_CAPITALISE:
		{
			any_t mB = Dereference( pReg, nB );
			if (mB.IsString()) {
				pReg[nA].AssignRef(any_t( stringp(mB)->Capitalise() )); // ownership of new
			} else {
				pReg[nA].AssignNull();
			}
		}
		nextop;

	case PF_REVERSE:
		{
			any_t mB = Dereference( pReg, nB );
			if (mB.IsString()) pReg[nA].AssignRef(any_t( stringp(mB)->Reverse() )); // ownership of new
			else pReg[nA].AssignNull();
		}
		nextop;

	case PF_SEARCH: // 2-3 args
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			// Extract the 'start' register
			int nReg = *((byte*)pInst);
			pInst++;
			int32 nStart = 0;
			if( nReg != INVALID_REGISTER )
			{
				any_t mStart = Dereference( pReg, nReg );
				if( !mStart.IsInt() ) RaiseError( E_TYPE );
				nStart = int32(mStart);
			}
			if (mB.IsString()) {
				if (mC.IsString()) {
					pReg[nA].AssignRef(any_t(int32( stringp(mB)->Search( stringp(mC), nStart ) )));
				} else {
					pReg[nA].AssignRef(any_t(int32( -1 )));
				}
			} else if (mB.IsArray()) {
				pReg[nA].AssignRef(any_t(int32( arrayp(mB)->Find( mC, nStart, arrayp(mB)->Size() ) )));
			} else {
				pReg[nA].AssignRef(any_t(int32( -1 )));
			}
		}
		nextop;

	case PF_RSEARCH: // 2-3 args
		{
			// TODO: This should handle array searches too..
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			if( mB.IsString() && mC.IsString() ) {
				// Extract the 'start' register
				int nReg = *((byte*)pInst);
				pInst++;
				int32 nStart = -1;
				if( nReg != INVALID_REGISTER )
				{
					any_t mStart = Dereference( pReg, nReg );
					if( !mStart.IsInt() ) RaiseError( E_TYPE );
					nStart = int32(mStart);
				}
				pReg[nA].AssignRef( any_t(int32( stringp(mB)->ReverseSearch( stringp(mC), nStart ) )) ); // ownership
			} else {
				pReg[nA].AssignRef(any_t(int32( -1 )));
			}
		}
		nextop;

  case PF_SKIP: // 1-3 args
    {
      any_t mB = Dereference( pReg, nB ); // source text
      if( !mB.IsString() ) RaiseError( E_TYPE );
      int32 nStart = 0;
      if( nC != INVALID_REGISTER )
      {
        // Extract the start offset
        any_t mC = Dereference( pReg, nC );
        if( !mC.IsInt() ) RaiseError( E_TYPE );
        nStart = int32(mC);
      }
      // Extract the 'charset' register
      int nReg = *((byte*)pInst);
      pInst++;
      stringp sCharSet = NULL;
      if( nReg != INVALID_REGISTER )
      {
        any_t mCharSet = Dereference( pReg, nReg );
        if( !mCharSet.IsString() ) RaiseError( E_TYPE );
        sCharSet = stringp(mCharSet);
      }
      pReg[nA].AssignRef( any_t(int32( stringp(mB)->Skip(
        sCharSet, nStart ) )) ); // ownership
    }
    nextop;

  case PF_SKIPTO: // 1-3 args
    {
      any_t mB = Dereference( pReg, nB ); // source text
      if( !mB.IsString() ) RaiseError( E_TYPE );
      int32 nStart = 0;
      if( nC != INVALID_REGISTER )
      {
        // Extract the start offset
        any_t mC = Dereference( pReg, nC );
        if( !mC.IsInt() ) RaiseError( E_TYPE );
        nStart = int32(mC);
      }
      // Extract the 'charset' register
      int nReg = *((byte*)pInst);
      pInst++;
      stringp sCharSet = NULL;
      if( nReg != INVALID_REGISTER )
      {
        any_t mCharSet = Dereference( pReg, nReg );
        if( !mCharSet.IsString() ) RaiseError( E_TYPE );
        sCharSet = stringp(mCharSet);
      }
      pReg[nA].AssignRef( any_t(int32( stringp(mB)->SkipTo(
        sCharSet, nStart ) )) ); // ownership
    }
    nextop;

	case PF_REPLACE: // 3 args
		{
			any_t mB = Dereference( pReg, nB ); // source text
			any_t mC = Dereference( pReg, nC ); // find pattern
			int nReg = *((byte*)pInst);
			pInst++;
			if (mB.IsString() && mC.IsString()) {
				// Extract the 'replace with' register
				any_t mReplace = Dereference( pReg, nReg );
				if( mReplace.IsString() ) {
					pReg[nA].AssignRef(stringp(mB)->Replace( stringp(mC), stringp(mReplace) )); // ownership of new
					nextop;
				} else {
					// Replace is null/invalid; replace with "".
					stringp sEmpty = new string_t();
					pReg[nA].AssignRef(stringp(mB)->Replace( stringp(mC), sEmpty )); // ownership of new
					sEmpty->Release();
					nextop;
				}
			}
			pReg[nA].Assign(mB); // pass through source text.
		}
		nextop;

	case PF_SPLIT: // 1-4 args
		{
			any_t mB = Dereference( pReg, nB ); // source text
			if (!mB.IsString()) {
                pReg[nA].Assign(mB); // pass through.
                nextop;
            }
			stringp sDelim = NULL;
			if (nC != INVALID_REGISTER) {
				any_t mC = Dereference( pReg, nC ); // delimiter
				if (mC.IsString()) {
                    sDelim = stringp(mC); // borrow
                }
			}
			// Extract the 'maximum count' register
			int32 nCount = 0;
			int nReg = ((byte*)pInst)[0];
			if (nReg != INVALID_REGISTER) {
				any_t mCount = Dereference( pReg, nReg );
				if (mCount.IsInt()) {
                    nCount = int32(mCount);
                }
			}
			// Extract the 'collapse flag' register
			int32 nCollapse = 0;
			nReg = ((byte*)pInst)[1];
            // Special handling for already-compiled code, which does not have the 4th
            // argument. Luckily instead the compiled code has zero, which is "this".
			if (nReg != 0 && nReg < m_mswCurrent->m_nRegisters) {
                any_t mCount = Dereference( pReg, nReg );
                if (mCount.IsInt()) {
                    nCollapse = int32(mCount);
                }
			}
			pInst++;
			// Split the string into an array
			arrayp arResult = stringp(mB)->Split( sDelim, nCount, nCollapse != 0 ); // new
			pReg[nA].AssignRef( any_t(arResult) ); // ownership
		}
		nextop;

	case PF_JOIN: // 1-3 args
		{
			any_t mB = Dereference( pReg, nB ); // source array
			if( !mB.IsArray() ) RaiseError( E_TYPE );
			stringp sDelim = NULL;
			if( nC != INVALID_REGISTER )
			{
				any_t mC = Dereference( pReg, nC ); // delimiter
				if( !mC.IsString() ) RaiseError( E_TYPE );
				sDelim = stringp(mC); // borrow
			}
			// Extract the 'final delimiter' register
			int nReg = *((byte*)pInst);
			pInst++;
			stringp sFinalDelim = NULL;
			if( nReg != INVALID_REGISTER )
			{
				any_t mFinal = Dereference( pReg, nReg );
				if( !mFinal.IsString() ) RaiseError( E_TYPE );
				sFinalDelim = stringp(mFinal);
			}
			// Join the array to form a string
			stringp sResult = arrayp(mB)->Join( sDelim, sFinalDelim ); // new
			pReg[nA].AssignRef( any_t(sResult) ); // ownership
		}
		nextop;

	case PF_CRYPT:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsString() ) RaiseError( E_TYPE );
			stringp sSalt = NULL;
			if( nC != INVALID_REGISTER )
			{
				any_t mC = Dereference( pReg, nC );
				if( !mC.IsString() ) RaiseError( E_TYPE );
				sSalt = stringp(mC); // borrow
			}
			stringp sResult = stringp(mB)->Crypt( sSalt ); // new
			pReg[nA].AssignRef( any_t(sResult) ); // ownership
		}
		nextop;

	case PF_SIZEOF:
		{
			any_t mB = Dereference( pReg, nB );
			pReg[nA].AssignRef( any_t( mB.Size() ) ); // num
		}
		nextop;

	case PF_ADD_INHERIT:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			if( !mB.IsObject() ) RaiseError( E_TYPE );
			if( !mC.IsObject() ) RaiseError( E_TYPE );
			CheckError( objectp(mB)->AddInherit( objectp(mC) ) );
			pReg[nA].AssignNull(); // TODO: remove the need for this?
		}
		nextop;

	case PF_REMOVE_INHERIT:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			if( !mB.IsObject() ) RaiseError( E_TYPE );
			if( !mC.IsObject() ) RaiseError( E_TYPE );
			CheckError( objectp(mB)->RemoveInherit( objectp(mC) ) );
			pReg[nA].AssignNull(); // TODO: remove the need for this?
		}
		nextop;

	case PF_INHERIT_LIST:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsObject() ) RaiseError( E_TYPE );
			arrayp arInherits = objectp(mB)->ListInherits();
			pReg[nA].AssignRef( any_t(arInherits) ); // ownership
		}
		nextop;

	case PF_KEYS:
		{
			any_t mB = Dereference( pReg, nB );
			switch( mB.Type() )
			{
			case TYPE_OBJECT:
			case TYPE_OBJECT_REF:
				{
					arrayp arKeys = objectp(mB)->ListAttribs(); // new
					pReg[nA].AssignRef( any_t(arKeys) ); // ownership
				}
				nextop;

			case TYPE_MAPPING:
				{
					arrayp arKeys = mappingp(mB)->ListKeys(); // new
					pReg[nA].AssignRef( any_t(arKeys) ); // ownership
				}
				nextop;

			default:
				// return an empty list of keys.
				arrayp arNew = new array_t(); // new
				pReg[nA].AssignRef( any_t(arNew) ); // ownership
			}
		}
		nextop;

	case PF_EXISTS:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			switch( mB.Type() )
			{
			case TYPE_OBJECT:
			case TYPE_OBJECT_REF:
				if (mC.IsString()) {
					pReg[nA].AssignRef(any_t(int32( objectp(mB)->Exists(stringp(mC)) ? 1 : 0 )));
				} else {
					pReg[nA].AssignRef(any_t(int32( 0 )));
				}
				nextop;

			case TYPE_MAPPING:
				pReg[nA].AssignRef(any_t(int32( mappingp(mB)->Exists(mC) ? 1 : 0 )));
				nextop;

			default:
				pReg[nA].AssignRef(any_t(int32( 0 )));
			}
		}
		nextop;

	case PF_REMOVE:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			int32 success = 0;
			switch (mB.Type())
			{
			case TYPE_OBJECT:
			case TYPE_OBJECT_REF:
				if (mC.IsString()) {
					if (objectp(mB)->Remove(stringp(mC))) success = 1;
				}
				break;

			case TYPE_MAPPING:
				if (mappingp(mB)->Remove(mC)) success = 1;
				break;

			case TYPE_ARRAY:
				if (arrayp(mB)->RemoveItem(mC)) success = 1;
				break;

			default:
				break;
			}
			pReg[nA].AssignRef(any_t(success));
		}
		nextop;

	case PF_PARENT:
		{
			any_t mB = Dereference( pReg, nB );
			if (mB.IsObject()) {
				objectp oParent = objectp(mB)->GetParent(); // borrow
				if( oParent ) {
					pReg[nA].Assign( any_t(oParent) ); // new ref
					nextop;
				}
			}
			pReg[nA].AssignNull();
		}
		nextop;

	case PF_OBJECT_PATH:
		{
			any_t mB = Dereference( pReg, nB );
			if (mB.IsObject()) {
				stringp sPath = objectp(mB)->GetPath(); // new
				if( sPath ) {
					pReg[nA].AssignRef( any_t(sPath) ); // ownership
					nextop;
				}
			}
			pReg[nA].AssignNull();
		}
		nextop;

	case PF_FIND_OBJECT:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsString() ) RaiseError( E_TYPE );
			// Resolve the context object if supplied
			objectp oContext = NULL;
			if( nC != INVALID_REGISTER )
			{
				any_t mC = Dereference( pReg, nC );
				if( !mC.IsObject() ) RaiseError( E_TYPE );
				oContext = objectp(mC); // borrow
			}

			// Attempt to find the object
			objectp oFind = NULL;
			CheckError( g_oDatabase.FindCreateObject( stringp(mB), oContext, NULL, false, oFind ) ); // borrow
			if( oFind )	pReg[nA].Assign( any_t(oFind) ); // new ref
			else pReg[nA].AssignNull();
		}
		nextop;

	case PF_UNIQUE_NAME:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsObject() ) RaiseError( E_TYPE );
			stringp sBase = NULL;
			if( nC != INVALID_REGISTER )
			{
				any_t mC = Dereference( pReg, nC );
				if( !mC.IsString() ) RaiseError( E_TYPE );
				sBase = (stringp)mC;
			}
			stringp sResult = objectp(mB)->UniqueAttribName( sBase ); // new
			pReg[nA].AssignRef( any_t(sResult) ); // ownership
		}
		nextop;
	
	case PF_SHUTDOWN:
		{
			g_bRunning = false;
			pReg[nA].AssignNull(); // TODO: remove the need for this?
		}
		yield;

	case PF_KILL_THREAD:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsThread() ) RaiseError( E_TYPE );
			threadp(mB)->AbortThread(g_null);
			if( m_nState == TS_STOPPED ) return E_STOPPED; // in case we killed self
			pReg[nA].AssignNull(); // TODO: remove the need for this?
		}
		nextop;

	case PF_EXIT_VALUE:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsThread() ) RaiseError( E_TYPE );
			pReg[nA].Assign( threadp(mB)->GetExitValue() );
		}
		nextop;

	case PF_THREAD_STOPPED:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsThread() ) RaiseError( E_TYPE );
			pReg[nA].AssignRef( any_t( threadp(mB)->IsStopped() ) );
		}
		nextop;

	case PF_THREAD_BLOCKED:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsThread() ) RaiseError( E_TYPE );
			pReg[nA].AssignRef( any_t( threadp(mB)->IsBlocked() ) );
		}
		nextop;

	case PF_THREAD_LIST:
		{
			pReg[nA].Assign( any_t(g_oDatabase.DispatchList()) ); // new ref
		}
		nextop;

	case PF_THREAD_STACK:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsThread() ) RaiseError( E_TYPE );
			arrayp arStack = threadp(mB)->BuildCallStack();
			pReg[nA].AssignRef( any_t(arStack) ); // ownership
		}
		nextop;

	case PF_THREAD_FUNCTION:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsThread() ) RaiseError( E_TYPE );
			pReg[nA].Assign( threadp(mB)->GetMainFunc() ); // new ref
		}
		nextop;

	case PF_THREAD_STATE:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsThread() ) RaiseError( E_TYPE );
			pReg[nA].AssignRef( any_t( threadp(mB)->GetState() ) );
		}
		nextop;

	case PF_YIELD:
		yield;

	case PF_RANDOM:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsInt() ) {
				pReg[nA].AssignRef(any_t(int32(0)));
			}
			else if( int32(mB) < 1 )
			{
				// Select a random float between 0 and <1
				pReg[nA].AssignRef( any_t(float64( float64(rand()) / float64(RAND_MAX) )) );
			}
			else
			{
				// Select a random integer between 0 and nB
				pReg[nA].AssignRef( any_t(int32( rand() % int32(mB) )) );
			}
		}
		nextop;

	case PF_FUNCTION_NAME:
		{
			any_t mB = Dereference( pReg, nB );
			stringp sName = NULL;
			switch( mB.Type() )
			{
			case TYPE_FUNCTION: sName = functionp(mB)->m_sName; break; // borrow
			case TYPE_FUNC_REF: sName = func_ref_t(mB).fnValue->m_sName; break; // borrow
			default: break;
			}
			if( sName ) pReg[nA].Assign( any_t(sName) ); // new ref
			else pReg[nA].AssignNull();
		}
		nextop;

	case PF_FUNCTION_CONTEXT:
		{
			any_t mB = Dereference( pReg, nB );
			if (mB.IsFuncRef()) {
				objectp oContext = func_ref_t(mB).oContext; // borrow
				if( oContext ) {
					pReg[nA].Assign( any_t(oContext) ); // new ref
					nextop;
				}
			}
			pReg[nA].AssignNull();
		}
		nextop;

	case PF_PARSE:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsString() ) RaiseError( E_TYPE );
			bool importing = false;
			if( nC != INVALID_REGISTER ) {
				any_t mC = Dereference( pReg, nC );
				if( !mC.IsInt() ) RaiseError( E_TYPE );
				importing = !int32(mC);
			}
			any_t mResult;
			CheckError( Parse( stringp(mB), importing, mResult ) );
			pReg[nA].AssignRef(mResult); // ownership
		}
		nextop;

	case PF_ENCODE:
		// Convert any value into a parseable string
		{
			any_t mB = Dereference( pReg, nB );
			stringp sResult = mB.Encode(); // new
			pReg[nA].AssignRef( any_t(sResult) ); // ownership
		}
        nextop;

	case PF_CONSOLE:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsString() ) RaiseError( E_TYPE );
			stringp sOutput = stringp(mB);
			printf( "%s\n", sOutput->CString() );
			pReg[nA].AssignNull(); // TODO: remove the need for this?
		}
		nextop;

	case PF_TIME:
		{
			// Retrieve the current time
			pReg[nA].AssignRef( int32( time(NULL) ) ); // num
		}
		nextop;

	case PF_CTIME:
		{
			// Use the specified time or retrieve the current time
			time_t nTime;
			if( nB != INVALID_REGISTER ) {
				any_t mB = Dereference( pReg, nB );
				if( mB.IsInt() ) nTime = (int32)mB;
				else RaiseError( E_TYPE );
			}
			else nTime = (int32)time(NULL);
			// Format the time as a string
			stringp sResult = string_t::CTime( nTime ); // new
			pReg[nA].AssignRef( any_t(sResult) ); // ownership
		}
		nextop;

	case PF_STRFTIME:
		{
			any_t mB = Dereference( pReg, nB );
			if( !mB.IsString() ) RaiseError( E_TYPE );
			// Use the specified time or retrieve the current time
			time_t nTime;
			if( nC != INVALID_REGISTER )
			{
				any_t mC = Dereference( pReg, nC );
				if( mC.IsInt() ) nTime = (int32)mC;
				else RaiseError( E_TYPE );
			}
			else nTime = (int32)time(NULL);
			// Format the time as a string
			stringp sResult = string_t::FmtTime( (stringp)mB, nTime ); // new
			pReg[nA].AssignRef( any_t(sResult) ); // ownership
		}
		nextop;

	case PF_LOCALTIME:
		{
			// Use the specified time or retrieve the current time
			time_t nTime;
			if( nB != INVALID_REGISTER )
			{
				any_t mB = Dereference( pReg, nB );
				nTime = (int32)mB;
			}
			else nTime = (int32)time(NULL);
			// Convert the UTC time to local timezone and split into bits
			struct tm* pTime = localtime( &nTime );
			if( !pTime ) RaiseError( E_RANGE );
			// Build an array of time components
			arrayp arResult = new array_t; // new
			arResult->AppendItem( any_t( int32(pTime->tm_year + 1900) ) ); // year (1900-)
			arResult->AppendItem( any_t( int32(pTime->tm_mon + 1) ) ); // month (1-12)
			arResult->AppendItem( any_t( int32(pTime->tm_mday) ) ); // day (1-31)
			arResult->AppendItem( any_t( int32(pTime->tm_hour) ) ); // hour (0-23)
			arResult->AppendItem( any_t( int32(pTime->tm_min) ) ); // minute (0-59)
			arResult->AppendItem( any_t( int32(pTime->tm_sec) ) ); // second (0-59)
			arResult->AppendItem( any_t( int32(pTime->tm_yday) ) ); // days since 1 Jan (0-365)
			arResult->AppendItem( any_t( int32(pTime->tm_wday) ) ); // weekdays since Sun (0-6)
			arResult->AppendItem( any_t( int32(pTime->tm_isdst) ) ); // daylight savings (0/1)
			pReg[nA].AssignRef( any_t(arResult) ); // ownership
		}
		nextop;

	case PF_UTCTIME:
		{
			// Use the specified time or retrieve the current time
			time_t nTime;
			if( nB != INVALID_REGISTER )
			{
				any_t mB = Dereference( pReg, nB );
				nTime = (int32)mB;
			}
			else nTime = (int32)time(NULL);
			// Split the UTC time into bits
			struct tm* pTime = gmtime( &nTime );
			if( !pTime ) RaiseError( E_RANGE );
			// Build an array of time components
			arrayp arResult = new array_t; // new
			arResult->AppendItem( any_t( int32(pTime->tm_year + 1900) ) ); // year (1900-)
			arResult->AppendItem( any_t( int32(pTime->tm_mon + 1) ) ); // month (1-12)
			arResult->AppendItem( any_t( int32(pTime->tm_mday) ) ); // day (1-31)
			arResult->AppendItem( any_t( int32(pTime->tm_hour) ) ); // hour (0-23)
			arResult->AppendItem( any_t( int32(pTime->tm_min) ) ); // minute (0-59)
			arResult->AppendItem( any_t( int32(pTime->tm_sec) ) ); // second (0-59)
			arResult->AppendItem( any_t( int32(pTime->tm_yday) ) ); // days since 1 Jan (0-365)
			arResult->AppendItem( any_t( int32(pTime->tm_wday) ) ); // weekdays since Sun (0-6)
			arResult->AppendItem( any_t( int32(pTime->tm_isdst) ) ); // daylight savings (0/1)
			pReg[nA].AssignRef( any_t(arResult) ); // ownership
		}
		nextop;

	case PF_MOVE_OBJECT: // 3 args (object, new-parent, new-name)
		{
			any_t mB = Dereference(pReg, nB);
			if (!mB.IsObject()) RaiseError(E_TYPE);
			any_t mC = Dereference(pReg, nC);
			if (!mC.IsObject()) RaiseError(E_TYPE);
			// Extract the new-name register
			int nReg = *((byte*)pInst);
			pInst++;
			any_t mName = Dereference(pReg, nReg);
			if (!mName.IsString()) RaiseError(E_TYPE);
			// Move the object
			CheckError(objectp(mB)->MoveObject(objectp(mC), stringp(mName)));
			pReg[nA].AssignNull();
		}
		nextop;

	case PF_OBJECT_NAME:
		{
			any_t mB = Dereference( pReg, nB );
			if (mB.IsObject()) {
				stringp sName = objectp(mB)->GetName(); // new
				if (sName) {
					pReg[nA].AssignRef(any_t(sName)); // ownership
					nextop;
				}
			}
			pReg[nA].AssignNull();
		}
		nextop;

	case PF_APPEND:
		{
			any_t mB = Dereference( pReg, nB );
			any_t mC = Dereference( pReg, nC );
			switch( mB.Type() )
			{
			case TYPE_ARRAY:
				arrayp(mB)->AppendItem(mC);
				break;

			default:
				RaiseError( E_TYPE );
			}
			pReg[nA].AssignNull(); // TODO: remove the need for this?
		}
		nextop;

	case PF_SLICE:
		{
			// Clamped version of string slicing, excluding the 'end' character.
			any_t mB = Dereference( pReg, nB ); // string
			any_t mC = Dereference( pReg, nC ); // start
			int nReg = *((byte*)pInst);
			pInst++;
			if (mB.IsString()) {
				if (!mC.IsInt()) RaiseError(E_TYPE);
				int32 start = int32(mC);
				if (nReg != INVALID_REGISTER) {
					// substring with start and end specified.
					any_t mEnd = Dereference( pReg, nReg ); // end
					if (!mEnd.IsInt()) RaiseError(E_TYPE);
					int32 end = int32(mEnd);
					if (start < 0) start = 0;
					if (end <= start) {
						// the result is the empty string.
						pReg[nA].AssignRef(any_t( new string_t() )); // ownership
						nextop;
					} else {
						// slice the string using inclusive endpoints.
						stringp sResult = stringp(mB)->SubString( start, end - 1 ); // new
						pReg[nA].AssignRef(any_t( sResult )); // ownership
						nextop;
					}
				} else {
					// substring with start only.
					if (start < 0) start = 0;
					stringp sResult = stringp(mB)->SubString( start, -1 ); // new
					pReg[nA].AssignRef(any_t( sResult )); // ownership
					nextop;
				}
			}
			// Not a string, return null.
			pReg[nA].AssignNull();
		}
		nextop;

	case OP_NULL:
	default:
		// Invalid opcode
		RaiseError( E_NOT_IMPLEMENTED );
	}

interp_raise:
	{
		// Save the instruction pointer (required for building error info)
		m_mswCurrent->m_nIP = pInst - pCode;

		// Build the error info array (must do before unwinding the call stack)
		arrayp arError = BuildErrorInfo( nResult );

		// Check for a try..catch handler in the call stack
		while (m_mswCurrent && !m_mswCurrent->m_nCatchHandler) {
			// Unwind the stack one invocation
			FinishCall( g_null, false );
		}

		// Did we find one?
		if( m_mswCurrent )
		{
			if( m_mswCurrent->m_nCatchReg )
			{
				// Put the error info in the 'catch' local
				m_mswCurrent->m_pRegisters[m_mswCurrent->m_nCatchReg].AssignRef(any_t(arError) ); // ownership
			}
			else arError->Release(); // but we just built it :(

			// Jump to the catch handler
			m_mswCurrent->m_nIP = m_mswCurrent->m_nCatchHandler;
			m_mswCurrent->m_nCatchHandler = 0; // MUST restore enclosing catch here
			m_nState = TS_READY; // may be in a blocked state
			reset;
		}

		// Uncaught error, look for a global error handler
		objectp oRoot = g_oDatabase.RootObject(); // borrow
		stringp sName = new string_t( "catch" ); // new ref
		const any_t& mErrorHandler = oRoot->Lookup(sName); // borrow
		sName->Release(); // release
		if (mErrorHandler.IsFunction()) {
			// Invoke the global error handler on this thread
			SetupCall( oRoot, functionp(mErrorHandler), arError, -1 );
			arError->Release(); // not needed any more (passed as args)
			m_nState = TS_READY; // was stopped during stack unwind
			reset;
		} else {
			// Report error calling global error handler
			printf( "no global error handler: %d '%s'\n", (int)nResult, (nResult < 0 ? g_szSystemErrors[-nResult] : "") );
		}

		// Clean up
		if( arError ) arError->Release();

if( m_mswCurrent ) printf( "STOP thread: %p '%s' (state %d)\n", this, m_mswCurrent->m_fnCode->m_sName->CString(), (int)m_nState );
else printf( "STOP thread: %p unknown (state %d)\n", this, (int)m_nState );

		// Stop the thread
		m_nState = TS_STOPPED;
		AbortThread(g_null);
	}
	return E_STOPPED;

forced_yield:
	if (impending_doom)
	{
		// the thread ignored our dire threat, kill it.
		AbortThread(g_null);
	    return E_STOPPED;
	}
	else
	{
		// give the thread 1000 more ticks to handle the exception before we kill it.
		impending_doom = 1;
		nTicks = FORCED_YIELD_TICKS;
		RaiseError( E_NO_YIELD );
	}

interp_yield:
	// Save the instruction pointer
	m_mswCurrent->m_nIP = pInst - pCode;

//if( m_mswCurrent ) printf( "yield: %p '%s' (state %d)\n", this, m_mswCurrent->m_fnCode->m_sName->CString(), (int)m_nState );
//else printf( "yield: %p unknown (state %d)\n", this, (int)m_nState );

	return E_SUCCESS;
}


// Private helper functions

void DebugDump(functionp fn)
{
    int32 nCount;
	int32 nLen = fn->m_nCodeSize;
	uint32* code = fn->m_pCode;
    int op, a, b, c, bc;

	printf( ":::: %s\n", fn->m_sName->CString() );

    for( nCount = 0; nCount < nLen; nCount++ )
    {
		byte* pIn = (byte*)&code[nCount];
        op = pIn[0];
        a = pIn[1];
        b = pIn[2];
        c = pIn[3];

		unsigned short* pInShort = (unsigned short*)&code[nCount];
		bc = pInShort[1];

        if( op < NUM_OPCODES )
        {
            printf( "[%4d] %3d %12s %d = %d %d (%d)\n", (int)nCount, op, g_szOpcodes[op], a, b, c, bc );
        }
        else
        {
            printf( "[%4d] %3d unknown      %d = %d %d (%d)\n", (int)nCount, op, a, b, c, bc );
        }
    }
}

void thread_t::SetupCall( objectp oContext, functionp fnCode, arrayp arArgs, int nReturnReg )
{
	// Check argument count
	int nArgs = 0;
	if( arArgs ) nArgs = arArgs->Size();
	int nExpect = fnCode->m_nArgs;

	// Create a new machine status word
	msw_t *mswNew = new msw_t;

	// Allocate registers for 'this', all arguments and local vars.
	RT_ASSERT_RET( fnCode->m_nLocals >= nExpect + 1 );
	any_t* pRegisters = new any_t[ fnCode->m_nLocals ];

	// Populate the new msw
	mswNew->m_oContext = oContext;
	mswNew->m_fnCode = fnCode;
	mswNew->m_nIP = 0;
	mswNew->m_pCaller = m_mswCurrent;
	mswNew->m_pRegisters = pRegisters;
	mswNew->m_nRegisters = fnCode->m_nLocals;
	mswNew->m_nReturnReg = nReturnReg;

	oContext->AddRef(); // msw ref
	fnCode->AddRef(); // msw ref

	// DebugDump(fnCode);

	// Register zero is always 'this'
	pRegisters[0] = any_t(oContext);
	oContext->AddRef(); // new ref

	// Assign the arguments provided to registers.
    // Do not assign more registers than the function expects (ignore the rest)
	int32 i, nMax = (nExpect < nArgs) ? nExpect : nArgs;
	for (i = 0; i < nMax; i++) {
		const any_t& mValue = arArgs->Lookup(i); // borrow
		pRegisters[i + 1] = mValue;
		mValue.AddRef(); // new ref
	}

	// Make this the current msw
	m_mswCurrent = mswNew;
	m_nCallDepth++;
}

void thread_t::FinishCall( const any_t& mValue, bool bSetResult )
{
	// Restore the previous msw
	msw_t* pFinished = m_mswCurrent;
	m_mswCurrent = pFinished->m_pCaller;
	m_nCallDepth--;

	// Extract the return value if specified
	if( pFinished->m_nReturnReg > -1 && m_mswCurrent )
	{
		// Caller is expecting a return value
		m_mswCurrent->m_pRegisters[pFinished->m_nReturnReg].Assign( mValue ); // new ref
	}

	// Set the thread exit value if necessary
	if( bSetResult || !m_mswCurrent )
	{
		// Set the thread's exit value
		m_mResult.Assign( mValue ); // new ref
	}

	// Release this msw (clean-up in msw_t destructor)
	delete pFinished;

	// Check if the thread has stopped
	if( !m_mswCurrent )
	{
		m_nState = TS_STOPPED;
		ASSERT( m_nCallDepth == 0 );
	}
}

// This would return const any_t& and dereference registers in-place,
// but compiled code depends on having (arr,index) and (obj,attr) references
// captured in registers for ops like += to work!

any_t thread_t::Dereference( any_t* pReg, int nReg )
{
	ASSERT( nReg >= 0 && nReg < INVALID_REGISTER );

	switch (pReg[nReg].Type())
	{
	case TYPE_UNDEFINED:
	case TYPE_DELETED:
		// Can discard because we know it's undef or deleted.
		pReg[nReg].DiscardNull();
        break;

	case TYPE_OBJECT:
	case TYPE_OBJECT_REF:
		if (objectp(pReg[nReg])->IsDeleted()) {
			// Replace the deleted object with null and drop the ref.
			pReg[nReg].AssignNull();
		}
		break;

	case TYPE_ATT_NAME:
		{
            // We MUST NOT deref this in-place in the register file, becase ops like
            // obj.attr += [obj] depend on dereferencing, then assigning back.
			const att_name_t& atValue = pReg[nReg].att_name();
			const any_t& mValue = atValue.oValue->Lookup(atValue.sName); // borrow.
			if (mValue.IsFunction()) {
				// Always bind functions to the object we looked for them in.
                // We can deref these in-place becase no += ops modify functions.
				pReg[nReg].Assign(any_t(functionp(mValue), atValue.oValue)); // new refs.
                break;
			} else {
				return mValue; // borrow.
			}
		}
		break;

	case TYPE_ARR_INDEX:
		{
            // We MUST NOT deref this in-place in the register file, becase ops like
            // arr[n] += [obj] depend on dereferencing, then assigning back.
			const arr_index_t& aiValue = pReg[nReg].arr_index();
            return aiValue.arValue->Lookup(aiValue.nIndex); // borrow.
		}
		break;

	case TYPE_ARR_SLICE:
		{
            // Even though we no longer generate arr_slice_t values in the interpreter,
            // old running code can still have these in their register file.
            // We can deref these in-place because we do not support assignment to them.
			const arr_slice_t& asValue = pReg[nReg].arr_slice();
			pReg[nReg].AssignRef(asValue.arValue->Slice(pReg[asValue.nReg].range())); // ownership of new array.
		}
		break;

	case TYPE_MAP_KEY:
		{
            // We MUST NOT deref this in-place in the register file, becase ops like
            // map[k] += [obj] depend on dereferencing, then assigning back.
			const map_key_t& mkValue = pReg[nReg].map_key();
			return mkValue.mpValue->Lookup(pReg[mkValue.nReg]); // borrow.
		}
		break;

	default:
		break;
	}

	return pReg[nReg];
}

arrayp thread_t::BuildRuntimeList( any_t* pReg, uint32*& pInst, int nArgs )
{
	// Allocate the list object
	arrayp arList = new array_t;

	// Adjust the instruction pointer to skip arguments
	byte* pArgs = (byte*)pInst;
	int nWords = (nArgs + 3) / 4;
	pInst += nWords;

	// Extract the arguments from registers
	while (nArgs--) {
		int nReg = *pArgs++;
		const any_t& mValue = Dereference( pReg, nReg ); // borrow
		arList->AppendItem( mValue ); // new ref
	}

	return arList;
}

arrayp thread_t::BuildErrorInfo( RESULT nError )
{
	// Allocate the error info array
	arrayp arResult = new array_t;

	// Add the error number to the info array
	arResult->AppendItem( any_t(int32(nError)) );

	// Determine the error message
	const char* szError = "unknown internal error";
	if( g_szErrorMessage )
	{
		// Retrieve and clear the global error message
		szError = g_szErrorMessage;
		g_szErrorMessage = NULL;
	}
	else if( nError < 0 )
	{
		// Look up the system error message
		szError = g_szSystemErrors[-nError];
	}

	// Create the error description string
	stringp sError = new string_t( szError );

	// Add the error description to the info array
	arResult->AppendItem( any_t(sError) ); // new ref
	sError->Release();

	arrayp arCallStack = BuildCallStack();

	// Add the call stack array to the error info
	arResult->AppendItem( any_t(arCallStack) ); // new ref
	arCallStack->Release();

	return arResult;
}

arrayp thread_t::BuildCallStack()
{
	// Allocate the call stack array
	arrayp arCallStack = new array_t;

	// Build the call stack
	msw_t* mswNext = m_mswCurrent;
	while( mswNext )
	{
		// Add this function/context to the stack
		arCallStack->AppendItem( any_t( mswNext->m_fnCode, mswNext->m_oContext ) ); // new ref

		// Add the line number to the call stack
		long nLine = mswNext->m_fnCode->m_pLines[mswNext->m_nIP];
		arCallStack->AppendItem( any_t(int32(nLine)) );

		// Move up the call stack
		mswNext = mswNext->m_pCaller;
	}

	return arCallStack;
}


// Persistent storage management

void thread_t::Persist()
{
	// Write the reference count
	g_oDatabase.WriteLong( m_nRef );

	// Save the main thread state
	g_oDatabase.WriteLong( m_nCallDepth );
	g_oDatabase.WriteLong( m_nState );
	g_oDatabase.WriteLong( (uint32)m_nStateTime );
	g_oDatabase.WriteLong( m_nStateReg );
	g_oDatabase.WriteLong( (uint32)m_ptStatePort );
	g_oDatabase.WriteLong( (uint32)m_sStateStr );
	g_oDatabase.WriteLong( (uint32)m_oContext );
	g_oDatabase.WriteLong( (uint32)m_fnMain );
	m_mResult.Persist();

	// Save the msw call stack
	msw_t* mswCurrent = m_mswCurrent;
	while( mswCurrent )
	{
		g_oDatabase.WriteLong( mswCurrent->m_nIP );
		g_oDatabase.WriteLong( mswCurrent->m_nRegisters );
		g_oDatabase.WriteLong( mswCurrent->m_nReturnReg );
		g_oDatabase.WriteLong( mswCurrent->m_nCatchReg );
		g_oDatabase.WriteLong( mswCurrent->m_nCatchHandler );
		g_oDatabase.WriteLong( (uint32)mswCurrent->m_fnCode );
		g_oDatabase.WriteLong( (uint32)mswCurrent->m_oContext );

		// Save the register file
		int nReg = mswCurrent->m_nRegisters;
		while( nReg-- ) mswCurrent->m_pRegisters[nReg].Persist();

		mswCurrent = mswCurrent->m_pCaller;
	}
}

void thread_t::Restore()
{
	// Read the reference count
	m_nRef = g_oDatabase.ReadLong();

	// Restore the main thread state
	m_nCallDepth = g_oDatabase.ReadLong();
	m_nState = g_oDatabase.ReadLong();
	m_nStateTime = g_oDatabase.ReadLong();
	m_nStateReg = g_oDatabase.ReadLong();
	m_ptStatePort = g_oIdxPort.ResolvePointer( (portp)g_oDatabase.ReadLong() );
	m_sStateStr = g_oIdxString.ResolvePointer( (stringp)g_oDatabase.ReadLong() );
	m_oContext = g_oIdxObject.ResolvePointer( (objectp)g_oDatabase.ReadLong() );
	m_fnMain = g_oIdxFunction.ResolvePointer( (functionp)g_oDatabase.ReadLong() );
	m_mResult.Restore();

	// Restore the msw call stack
	bool bad = false;
	if( m_nCallDepth )
	{
		msw_t *mswCurrent = new msw_t;
		ASSERT( mswCurrent != NULL );
		m_mswCurrent = mswCurrent;
		int nMsw = m_nCallDepth;
		while( nMsw-- )
		{
			ASSERT( mswCurrent != NULL );

			// Allocate another msw if more follow
			if( nMsw )
			{
				mswCurrent->m_pCaller = new msw_t;
				ASSERT( mswCurrent->m_pCaller != NULL );
			}
			else mswCurrent->m_pCaller = NULL;

			mswCurrent->m_nIP = g_oDatabase.ReadLong();
			mswCurrent->m_nRegisters = g_oDatabase.ReadLong();
			mswCurrent->m_nReturnReg = g_oDatabase.ReadLong();
			mswCurrent->m_nCatchReg = g_oDatabase.ReadLong();
			mswCurrent->m_nCatchHandler = g_oDatabase.ReadLong();
			mswCurrent->m_fnCode = g_oIdxFunction.ResolvePointer( (functionp)g_oDatabase.ReadLong() );
			mswCurrent->m_oContext = g_oIdxObject.ResolvePointer( (objectp)g_oDatabase.ReadLong() );

			if (!mswCurrent->m_fnCode || !mswCurrent->m_oContext) bad = true;

			// Restore the register file
			int nReg = mswCurrent->m_nRegisters;
			mswCurrent->m_pRegisters = new any_t[nReg];
			ASSERT( mswCurrent->m_pRegisters != NULL );
			while( nReg-- ) mswCurrent->m_pRegisters[nReg].Restore();

			mswCurrent = mswCurrent->m_pCaller;
		}
	}

	// Recover from lost references.
	if (bad || !m_oContext || !m_fnMain) {
		m_nState = TS_STOPPED;
		return;
	}
	switch (m_nState) {
		case TS_WRITE:
			if (!m_ptStatePort || !m_sStateStr) {
				m_nState = TS_STOPPED;
				return;
			}
		case TS_READ:
		case TS_READLN:
		case TS_ACCEPT:
			if (!m_ptStatePort) {
				m_nState = TS_STOPPED;
				return;
			}
		case TS_READY:
		case TS_STOPPED:
		case TS_SLEEP:
			// OK.
			break;
		default:
			// Unknown state.
			m_nState = TS_STOPPED;
			return;
	}

}

// These must be in the same order as the E_ declarations
// in result.h (-1 * nResult used as index into this array)

const char* g_szSystemErrors[] = {
	"success",
	"internal error",
	"out of memory",
	"not implemented",
	"failed to open database",

	"object has been deleted",
	"type mismatch",
	"out of range",
	"access denied",
	"undefined attribute",
	"wrong number of arguments to function",
	"port has been closed",
	"socket error",
	"connect failed",
	"accept failed",
	"cannot invoke null",
	"cannot invoke function with no context",
	"undefined local variable",
	"key not in mapping",
	"system function not defined",
	"cannot invoke non-function value",
	"object does not exist",
	"object contains children",
	"attribute already exists",
	"object already exists",
	"debug assertion failed",
	"too deep inheritance",
	"cannot create an object within a clone",
	"cannot replace an object",
	"cannot remove an object",
	"too long without yield"
};
