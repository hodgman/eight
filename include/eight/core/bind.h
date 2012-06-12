//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
namespace eight {
//------------------------------------------------------------------------------

class CallBuffer;
typedef void (FnTask)( void* o, void* b, uint size );
typedef void (FnPushCall)( CallBuffer& q, void* o, void* b, uint s );

struct DataBinding
{
	const char* name;
	memptr memvar;
	uint offset, size;
};
struct MethodBinding
{
	const char* name;
	memfuncptr memfuncptr;
	FnTask* task;
	FnPushCall* push;
};
struct FunctionBinding
{
	const char* name;
	callback func;
};

struct TypeBinding
{
	const DataBinding*     data;
	uint                   dataCount;
	const MethodBinding*   method;
	uint                   methodCount;
	const FunctionBinding* function;
	uint                   functionCount;
};

template<class T> struct Reflect;

//------------------------------------------------------------------------------

#define eiBindStruct( a )																\
	const TypeBinding& Reflect##a();													\
	template<> struct Reflect<a> {														\
		static const TypeBinding& Binding() { return Reflect##a(); } };					\
	inline const TypeBinding& Reflect##a()												\
	eiBindStruct_Internal(a)															//

#define eiBindClass( a )																\
	inline const TypeBinding& Reflect##a()												\
	{ return a::Reflect(); }															\
	template<> struct Reflect<a> {														\
		static const TypeBinding& Binding() { return Reflect##a(); } };					\
	const TypeBinding& a::Reflect()														\
	eiBindStruct_Internal(a)															//


//------------------------------------------------------------------------------
#define eiBindStruct_Internal( a )														\
	{																					\
		typedef a T;																	\
		const DataBinding*     s_data = 0; uint s_dataSize = 0;	/* defaults */			\
		const MethodBinding*   s_meth = 0; uint s_methSize = 0;							\
		const FunctionBinding* s_func = 0; uint s_funcSize = 0;							\
		{																				//
# define eiBeginData()																	\
			const static DataBinding s_data[] = {	/* overwrites name of default */	//
#   define eiBindData( a )																\
				{ #a, (memptr)&T::a, offsetof(T,a), MemberSize(&T::a) },				//
#   define eiBindReference( a )															\
				{ #a,             0,             0, sizeof(void*) },					//
# define eiEndData()																	\
			};																			\
			const static uint s_dataSize = ArraySize(s_data);							//

# define eiBeginMethods()																\
			const static MethodBinding s_meth[] = {	/* overwrites name of default */	//
#   define eiBindMethod( a )															\
				{ #a, (memfuncptr)&T::a,												\
				&MsgFuncs<Tag##a,T>::Call, &MsgFuncs<Tag##a,T>::Push },					//
# define eiEndMethods()																	\
			};																			\
			const static uint s_methSize = ArraySize(s_meth);							//

# define eiBeginFunctions()																\
			const static DataBinding s_func[] = {	/* overwrites name of default */	//
#   define eiBindFunction( a )															\
				{ #a, (callback)&a },													//
# define eiEndFunctions()																\
			};																			\
			const static uint s_funcSize = ArraySize(s_func);							//

#define eiEndBind( )																	\
			const static TypeBinding s_binding =										\
			{																			\
				s_data, s_dataSize,														\
				s_meth, s_methSize,														\
				s_func, s_funcSize,														\
			};																			\
			return s_binding;															\
		}																				\
	}																					//
//------------------------------------------------------------------------------
#define eiBind( a )																		\
	static const TypeBinding& Reflect();												//
//------------------------------------------------------------------------------

template<class M, class C> uint MemberSize(M C::*) { return sizeof(M); };
/*
template<class T>          struct IsCallbacktMemFunc                  { static const bool value = false; }
template<class R, class C> struct IsCallbacktMemFunc<R (C::*)(void*)> { static const bool value = true; }
template<class T>          struct SizeOfCallbackReturn                     { static const uint value = 0; }
template<class C>          struct SizeOfCallbackReturn<void (C::*)(void*)> { static const uint value = 0; }
template<class R, class C> struct SizeOfCallbackReturn<R    (C::*)(void*)> { static const uint value = sizeof(R); }
template<class T>          struct IsMemFunc                  { static const bool value = false; };
template<class R, class C> struct IsMemFunc<R (C::*)(...)>   { static const bool value = true; };
*/
/*
template<class T>          bool IsMemFunc(T)               { return false; }
template<class R, class C> bool IsMemFunc(R (C::*)(...)) { return true; }

template<class T>          bool IsCallbackMemFunc(T)               { return false; }
template<class R, class C> bool IsCallbackMemFunc(R (C::*)(void*)) { return true; }
template<class T>          uint SizeOfCallbackReturn(T)                  { return 0; }
template<class C>          uint SizeOfCallbackReturn(void (C::*)(void*)) { return 0; }
template<class R, class C> uint SizeOfCallbackReturn(R    (C::*)(void*)) { return sizeof(R); }
template<class R>          uint SizeOfCallbackReturn(R(*)(void*)) { return sizeof(R); }
*/
//	namespace internal { class class_ { char member; void func(); };


//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
