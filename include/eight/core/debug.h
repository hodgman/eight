//------------------------------------------------------------------------------
#pragma once

#include <stdarg.h>

namespace eight {
//------------------------------------------------------------------------------

#if !defined(eiDEBUG_LEVEL)
	#if defined(eiBUILD_RETAIL)
		#define eiDEBUG_LEVEL 0
	#elif !defined(_DEBUG)
		#define eiDEBUG_LEVEL 1
	#else
		#define eiDEBUG_LEVEL 2
	#endif
#endif

	
bool IsDebuggerAttached();

//! Macro for compile-time assertion
//TODO - use the template based solution that gives better error messages
#define eiSTATIC_ASSERT(exp) typedef int assert_##_LINE_ [(exp) * 2 - 1];

bool Assert( const char* e, int l, const char* f, const char* fn );
void DebugOutput( const char* prefix, const char* fmt, ... );
void DebugOutput( int line, const char* file, const char* function, const char* fmt, ... );
void DebugOutput( int line, const char* file, const char* function, const char* fmt, va_list& );
void Print( const char* msg );
void Printf( const char* msg, ... );

#define eiRASSERT( a )		do{ if(!(a)) { if( ::eight::Assert( #a, __LINE__, __FILE__, __FUNCTION__ ) ) {eiDEBUG_BREAK;} } }while(0)

#define eiNop do{}while(0)

#define eiFatalError(x) do{}while(0)//todo

//Define the ASSERT and DEBUG keywords
#if eiDEBUG_LEVEL > 0
	#ifdef eiBUILD_GCC
		#define eiDEBUG_BREAK __asm ( ".intel_syntax noprefix\n" "int 3\n" ".att_syntax \n" )
	#elif defined eiBUILD_MSVC
		#define eiDEBUG_BREAK __debugbreak()
	#else
		#define eiDEBUG_BREAK __asm { int 3 }
	#endif
	#define eiASSERT( a )       eiRASSERT( a )
	#define eiDEBUG(  a )       a
	#define eiASSUME( a )       eiASSERT( a )
	#define eiWarn(fmt, ...) do { eight::DebugOutput("WARN: ", fmt, __VA_ARGS__); } while(0)
	#define eiInfo(name, fmt, ...) do { if(eiInfo_##name) eight::DebugOutput(#name": ", fmt, __VA_ARGS__); } while(0)
	#define eiInfoGroup(name, enabled) const static bool eiInfo_##name = enabled
#else
	#ifdef eiBUILD_MSVC
		#define eiASSUME( a ) __assume( a )
	#else
		#define eiASSUME( a ) do{}while(0) //no compiler hint available
	#endif
	#define eiDEBUG_BREAK eiNop
	#define eiASSERT( a ) eiNop
	#define eiDEBUG( a )

	#define eiWarn(...) eiNop
	#define eiInfo(...) eiNop
	#define eiInfoGroup(name, enabled)
#endif

#ifdef eiBUILD_EXCEPTIONS
#define eiTHROW( a ) throw a
#else
#define eiTHROW( a ) do{eiASSERT(false);}while(0)
#endif


//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
