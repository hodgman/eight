//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
struct lua_State;
namespace eight {
//------------------------------------------------------------------------------

template<class T> struct Reflect;// { static const TypeBinding& Binding() }

// When sending a message, if a value is not known immediately, the index of and byte-offset into a future/return value
//   can be written instead. All arguments are padded to 32bits to allow this.
// N.B. the corresponding bit in the parent ArgStore::futureMask should be set when writing these future values.
struct FutureIndex
{
	u16 index;
	u16 offset;
};

class CallBuffer;
typedef void (FnMethod)( void* obj, void* args, void* output );
typedef void (FnFunction)( void* args, void* output );
typedef FutureIndex (FnPush)( CallBuffer& q, void* obj, void* args, uint size );
typedef int (FnLua)( lua_State* );

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
	FnMethod* call;
	FnPush* push;
	FnLua* luaCall;
	FnLua* luaPush;
};
struct FunctionBinding
{
	const char* name;
	callback func;
	FnFunction* call;
	FnLua* luaCall;
};

struct TypeBinding
{
	const char*            name;
	const DataBinding*     data;
	uint                   dataCount;
	const MethodBinding*   method;
	uint                   methodCount;
	const FunctionBinding* function;
	uint                   functionCount;
	const FunctionBinding* constructor;
	uint                   constructorCount;
};

namespace internal {
	namespace LuaBindMode
	{
		enum Type
		{
			NoLuaBind  = 0,
			LuaCall = 1<<1,
			LuaPush = 1<<2,
			LuaCallAndPush = (LuaCall|LuaPush),
		};
	}
}

template<class T, memfuncptr fn, class F> void        CallWrapper(void* user, void* argBlob, void* outBlob);
template<        callback   fn, class F> void       FnCallWrapper(void* user, void* argBlob, void* outBlob);
template<class T, memfuncptr fn, class F> FutureIndex PushWrapper(CallBuffer& q, void* user, void* argBlob, uint size);
template<class T, memfuncptr fn, class F> FnMethod*   GetCallWrapper(F f=0)   { return &CallWrapper<T,fn,F>; }
template<         callback   fn, class F> FnFunction* GetFnCallWrapper(F f=0) { return &FnCallWrapper<fn,F>; }
template<class T, memfuncptr fn, class F> FnPush*     GetPushWrapper(F f=0)   { return &PushWrapper<T,fn,F>; }


template<class T, memfuncptr fn, class F> int         LuaCallWrapper(lua_State* luaState);
template<         callback   fn, class F> int       LuaFnCallWrapper(lua_State* luaState);
template<class T, memfuncptr fn, class F> int         LuaPushWrapper(lua_State* luaState);
template<int mode> struct GetLuaWrapper
{
	template<class T, memfuncptr fn> static FnLua* Call(...) { return 0; }
	template<class T, memfuncptr fn> static FnLua* Push(...) { return 0; }
};
template<> struct GetLuaWrapper<internal::LuaBindMode::LuaCall>
{
	template<class T, memfuncptr fn, class F> static FnLua* Call(F f=0) { return &LuaCallWrapper<T,fn,F>; }
	template<class T, memfuncptr fn, class F> static FnLua* Push(F f=0) { return 0; }
};
template<> struct GetLuaWrapper<internal::LuaBindMode::LuaPush>
{
	template<class T, memfuncptr fn, class F> static FnLua* Call(F f=0) { return 0; }
	template<class T, memfuncptr fn, class F> static FnLua* Push(F f=0) { return &LuaPushWrapper<T,fn,F>; }
};
template<> struct GetLuaWrapper<internal::LuaBindMode::LuaCallAndPush>
{
	template<class T, memfuncptr fn, class F> static FnLua* Call(F f=0) { return &LuaCallWrapper<T,fn,F>; }
	template<class T, memfuncptr fn, class F> static FnLua* Push(F f=0) { return &LuaPushWrapper<T,fn,F>; }
};
template<int mode> struct GetFnLuaWrapper
{
	template<callback fn, class F> static FnLua* Call(F f=0) { return &LuaFnCallWrapper<fn,F>; }
};
template<> struct GetFnLuaWrapper<internal::LuaBindMode::LuaPush>
{
	template<callback  fn> static FnLua* Call(...) { return 0; }
};

#define eiLuaFnCallWrapper(a) eight::GetFnLuaWrapper<eight::internal::LuaBindMode::LuaCall>::Call<(eight::callback)&a>(&a)
#define eiLuaCallWrapper(a  ) eight::GetLuaWrapper<  eight::internal::LuaBindMode::LuaCall>::Call<(eight::callback)&a>(&a)

//------------------------------------------------------------------------------

#define eiBindStruct( a )																\
	const TypeBinding& Reflect##a();													\
	template<> struct Reflect<a> {														\
		static const TypeBinding& Binding() { return Reflect##a(); } };					\
	const TypeBinding& Reflect##a()														\
	eiBindStruct_Internal(a)															//

#define eiBindClass( a )																\
	const TypeBinding& Reflect##a()														\
	{ return a::Reflect(); }															\
	template<> struct Reflect<a> {														\
		static const TypeBinding& Binding() { return Reflect##a(); } };					\
	const TypeBinding& a::Reflect()														\
	eiBindStruct_Internal(a)															//


#define eiBindGlobal( a )																\
	struct Globals##a;	eiBindStruct( Globals##a )										//


//-V:eiLuaBind:561

//------------------------------------------------------------------------------
#define eiBindStruct_Internal( a )														\
	{																					\
		using namespace eight;															\
		using namespace eight::internal;												\
		using namespace eight::internal::LuaBindMode;									\
		typedef a T;																	\
		const char* name = #a;															\
		const DataBinding*     _data = 0; uint _dataSize = 0;	/* defaults */			\
		const MethodBinding*   _meth = 0; uint _methSize = 0;							\
		const FunctionBinding* _func = 0; uint _funcSize = 0;							\
		const FunctionBinding* _cons = 0; uint _consSize = 0;							\
		const LuaBindMode::Type _lua = NoLuaBind;										\
		{																				//
# define eiLuaBind( mode )																\
			const LuaBindMode::Type _lua = (LuaBindMode::Type)(0 | mode);				// /* overwrites name of default */
# define eiBeginData()																	\
			const static DataBinding s_data[] = {										//
#   define eiBindData( a )																\
				{ #a, (memptr)&T::a, offsetof(T,a), MemberSize(&T::a) },				//
#   define eiBindReference( a )															\
				{ #a,             0,             0, sizeof(void*) },					//
# define eiEndData()																	\
			};																			\
			_data = s_data;																\
			_dataSize = eiArraySize(s_data);											//

# define eiBeginMethods()																\
			const static MethodBinding s_meth[] = {										//
#   define eiBindMethod( a )															\
				{ #a, (memfuncptr)&T::a,												\
				  GetCallWrapper<T,(memfuncptr)&T::a>(&T::a),							\
				  GetPushWrapper<T,(memfuncptr)&T::a>(&T::a),							\
				  GetLuaWrapper<_lua&LuaCallAndPush>::Call<T,(memfuncptr)&T::a>(&T::a),	\
				  GetLuaWrapper<_lua&LuaCallAndPush>::Push<T,(memfuncptr)&T::a>(&T::a) },//
#   define eiBindExtensionMethod( name, a )												\
				{ #name, 0,																\
				  0,																	\
				  0,																	\
				  GetFnLuaWrapper<_lua&LuaCallAndPush>::Call<(callback)&a>(&a),			\
				  0 },																	//todo - implement...
# define eiEndMethods()																	\
			};																			\
			_meth = s_meth;																\
			_methSize = eiArraySize(s_meth);											//

# define eiBeginFunctions()																\
			const static FunctionBinding s_func[] = {									//
#   define eiBindFunction( a )															\
				{ #a, (callback)&T::a,													\
				  GetFnCallWrapper<(callback)&T::a>(&T::a),								\
				  GetFnLuaWrapper<_lua&LuaCallAndPush>::Call<(callback)&T::a>(&T::a)	\
				},																		//
# define eiEndFunctions()																\
			};																			\
			_func = s_func;																\
			_funcSize = eiArraySize(s_func);											//

# define eiBeginConstructors()															\
			const static FunctionBinding s_cons[] = {									//
#   define eiBindFactoryFunction( a )													\
				{ #a, (callback)&T::a,													\
				  GetFnCallWrapper<(callback)&T::a>(&T::a),								\
				  GetFnLuaWrapper<_lua&LuaCallAndPush>::Call<(callback)&T::a>(&T::a)	\
				},																		//
# define eiEndConstructors()															\
			};																			\
			_cons = s_cons;																\
			_consSize = eiArraySize(s_cons);											//

#define eiEndBind( )																	\
			const static TypeBinding s_binding =										\
			{																			\
				name,																	\
				_data, _dataSize,														\
				_meth, _methSize,														\
				_func, _funcSize,														\
				_cons, _consSize,														\
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
