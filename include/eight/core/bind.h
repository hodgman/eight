
//#pragma optimize("",off)//!!!!!!!!!!!!!!!!
//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/alloc/scope.h>
struct lua_State;
namespace eight {
//------------------------------------------------------------------------------
	struct TypeBinding;
template<class T> struct Reflect { static const TypeBinding& Binding(); typedef Nil ParentType; };

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
typedef void (FnLuaRegister)( lua_State*, const char* name, FnLua* gcFinalizer );

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
struct ConstructorBinding
{
	const char* name;
	callback funcDirect;
	FnFunction* callValue;
	FnFunction* callPlacement;
	FnFunction* callGlobalHeap;
	FnFunction* callScope;
	FnLua* luaGcNew;
	FnLua* luaScope;
};
struct EnumMemberBinding
{
	const char* name;
	int value;
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
	const ConstructorBinding* constructor;
	uint                      constructorCount;
	const EnumMemberBinding* member;
	uint                     memberCount;
	FnLuaRegister*           luaRegistration;
	FnLua*                   luaGcFinalizer;
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
template<         callback   fn, class F> int        LuaGcNewWrapper(lua_State* luaState);
template<class T, memfuncptr fn, class F> int         LuaPushWrapper(lua_State* luaState);
template<class T>                         void RegisterTypeMetatable(lua_State* luaState, const char* typeName, FnLua* gc);
template<class T>                         int           LuaDestruct(lua_State* luaState);
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
	template<callback fn, class F> static FnLua* GcNew(F f=0) { return &LuaGcNewWrapper<fn,F>; }
	template<callback fn, class F> static FnLua* Call(F f=0) { return &LuaFnCallWrapper<fn,F>; }
};
template<> struct GetFnLuaWrapper<internal::LuaBindMode::LuaPush>
{
	template<callback  fn> static FnLua* Call(...) { return 0; }
};

#define eiLuaFnCallWrapper(a) eight::GetFnLuaWrapper<eight::internal::LuaBindMode::LuaCall>::Call<(eight::callback)&a>(&a)
#define eiLuaCallWrapper(a  ) eight::GetLuaWrapper<  eight::internal::LuaBindMode::LuaCall>::Call<(eight::callback)&a>(&a)

template<class T, class A=Nil, class B=Nil, class C=Nil, class D=Nil, class E=Nil, class F=Nil, class G=Nil, class H=Nil, class I=Nil, class J=Nil>
struct ConstructorWrapper {
	static T* Placement( T* where, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j ) {
		return new(where) T(a, b, c, d, e, f, g, h, i, j); }
	static T Value( A a, B b, C c, D d, E e, F f, G g, H h, I i, J j ) {
		return T(a, b, c, d, e, f, g, h, i, j); }
	static T* Alloc( Scope& where, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j ) {
		return eiNew(where, T)(a, b, c, d, e, f, g, h, i, j); }
};
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I>
struct ConstructorWrapper<T,A,B,C,D,E,F,G,H,I,Nil> {
	static T* Placement( T* where, A a, B b, C c, D d, E e, F f, G g, H h, I i ) {
		return new(where) T(a, b, c, d, e, f, g, h, i); }
	static T Value( A a, B b, C c, D d, E e, F f, G g, H h, I i ) {
		return T(a, b, c, d, e, f, g, h, i); }
	static T* Alloc( Scope& where, A a, B b, C c, D d, E e, F f, G g, H h, I i ) {
		return eiNew(where, T)(a, b, c, d, e, f, g, h, i); }
};
template<class T, class A, class B, class C, class D, class E, class F, class G, class H>
struct ConstructorWrapper<T,A,B,C,D,E,F,G,H,Nil,Nil> {
	static T* Placement( T* where, A a, B b, C c, D d, E e, F f, G g, H h ) {
		return new(where) T(a, b, c, d, e, f, g, h); }
	static T Value( A a, B b, C c, D d, E e, F f, G g, H h ) {
		return T(a, b, c, d, e, f, g, h); }
	static T* Alloc( Scope& where, A a, B b, C c, D d, E e, F f, G g, H h ) {
		return eiNew(where, T)(a, b, c, d, e, f, g, h); }
};
template<class T, class A, class B, class C, class D, class E, class F, class G>
struct ConstructorWrapper<T,A,B,C,D,E,F,G,Nil,Nil,Nil> {
	static T* Placement( T* where, A a, B b, C c, D d, E e, F f, G g ) {
		return new(where) T(a, b, c, d, e, f, g); }
	static T Value( A a, B b, C c, D d, E e, F f, G g ) {
		return T(a, b, c, d, e, f, g); }
	static T* Alloc( Scope& where, A a, B b, C c, D d, E e, F f, G g ) {
		return eiNew(where, T)(a, b, c, d, e, f, g); }
};
template<class T, class A, class B, class C, class D, class E, class F>
struct ConstructorWrapper<T,A,B,C,D,E,F,Nil,Nil,Nil,Nil> {
	static T* Placement( T* where, A a, B b, C c, D d, E e, F f ) {
		return new(where) T(a, b, c, d, e, f); }
	static T Value( A a, B b, C c, D d, E e, F f ) {
		return T(a, b, c, d, e, f); }
	static T* Alloc( Scope& where, A a, B b, C c, D d, E e, F f ) {
		return eiNew(where, T)(a, b, c, d, e, f); }
};
template<class T, class A, class B, class C, class D, class E>
struct ConstructorWrapper<T,A,B,C,D,E,Nil,Nil,Nil,Nil,Nil> {
	static T* Placement( T* where, A a, B b, C c, D d, E e ) {
		return new(where) T(a, b, c, d, e); }
	static T Value( A a, B b, C c, D d, E e ) {
		return T(a, b, c, d, e); }
	static T* Alloc( Scope& where, A a, B b, C c, D d, E e ) {
		return eiNew(where, T)(a, b, c, d, e); }
};
template<class T, class A, class B, class C, class D>
struct ConstructorWrapper<T,A,B,C,D,Nil,Nil,Nil,Nil,Nil,Nil> {
	static T* Placement( T* where, A a, B b, C c, D d ) { eiASSERT(where);
		return new(where) T(a, b, c, d); }
	static T Value( A a, B b, C c, D d ) {
		return T(a, b, c, d); }
	static T* Alloc( Scope& where, A a, B b, C c, D d ) { eiASSERT(&where);
		return eiNew(where, T)(a, b, c, d); }
};
template<class T, class A, class B, class C>
struct ConstructorWrapper<T,A,B,C,Nil,Nil,Nil,Nil,Nil,Nil,Nil> {
	static T* Placement( T* where, A a, B b, C c ) {
		return new(where) T(a, b, c); }
	static T Value( A a, B b, C c ) {
		return T(a, b, c); }
	static T* Alloc( Scope& where, A a, B b, C c ) {
		return eiNew(where, T)(a, b, c); }
};
template<class T, class A, class B>
struct ConstructorWrapper<T,A,B,Nil,Nil,Nil,Nil,Nil,Nil,Nil,Nil> {
	static T* Placement( T* where, A a, B b ) {
		return new(where) T(a, b); }
	static T Value( A a, B b ) {
		return T(a, b); }
	static T* Alloc( Scope& where, A a, B b ) {
		return eiNew(where, T)(a, b); }
};
template<class T, class A>
struct ConstructorWrapper<T,A,Nil,Nil,Nil,Nil,Nil,Nil,Nil,Nil,Nil> {
	static T* Placement( T* where, A a ) {
		return new(where) T(a); }
	static T Value( A a ) {
		return T(a); }
	static T* Alloc( Scope& where, A a ) {
		return eiNew(where, T)(a); }
};
template<class T>
struct ConstructorWrapper<T,Nil,Nil,Nil,Nil,Nil,Nil,Nil,Nil,Nil,Nil> {
	static T* Placement( T* where ) {
		return new(where) T(); }
	static T Value() {
		return T(); }
	static T* Alloc( Scope& where ) {
		return eiNew(where, T)(); }
};

//------------------------------------------------------------------------------

#define eiBindEnum( a )				eiBindEnumRename( a, a )
#define eiBindEnumRename( a, b )														\
	const TypeBinding& Reflect##a();													\
	template<> struct Reflect<a> {														\
		static const TypeBinding& Binding() { return Reflect##a(); } };					\
	const TypeBinding& Reflect##a()														\
	eiBindStruct_Internal(typedef a T; typedef a TT, b)									//

#define eiBindNamespace( a )		eiBindNamespaceRename( a, a )
#define eiBindNamespaceRename( a, b )													\
	const TypeBinding& Reflect##a();													\
	struct Namespace##a {};																\
	template<> struct Reflect<Namespace##a> {											\
		static const TypeBinding& Binding() { return Reflect##a(); } };					\
	const TypeBinding& Reflect##a()														\
	eiBindStruct_Internal(namespace T = a; typedef Namespace##a TT, b)					//

#define eiBindStructDerived( a, p )			eiBindStruct_( a, a, p )
#define eiBindStruct( a )					eiBindStruct_( a, a, Nil )
#define eiBindStructRename( a, b )			eiBindStruct_( a, b, Nil )
#define eiBindStruct_( a, b, parent )													\
	const TypeBinding& Reflect##a();													\
	template<> struct Reflect<a> {														\
		typedef parent ParentType;														\
		static const TypeBinding& Binding() { return Reflect##a(); } };					\
	const TypeBinding& Reflect##a()														\
	eiBindStruct_Internal(typedef a T; typedef a TT, b)									//

#define eiBindClassDerived( a, p )			eiBindClass_( a, a, p )
#define eiBindClass( a )					eiBindClass_( a, a, Nil )
#define eiBindClassRename( a, b )			eiBindClass_( a, b, Nil )
#define eiBindClass_( a, b, parent )													\
	const TypeBinding& Reflect##a()														\
	{ return a::Reflect(); }															\
	template<> struct Reflect<a> {														\
		typedef parent ParentType;														\
		static const TypeBinding& Binding() { return Reflect##a(); } };					\
	const TypeBinding& a::Reflect()														\
	eiBindStruct_Internal(typedef a T; typedef a TT, b)									//


#define eiBindGlobal( a )																\
	struct Globals##a;	eiBindStruct( Globals##a )										//


//-V:eiLuaBind:561

//		using namespace eight::internal;												
//------------------------------------------------------------------------------
#define eiBindStruct_Internal( TYPEDEF, NAME )											\
	{																					\
		using namespace eight;															\
		using namespace eight::internal::LuaBindMode;									\
		TYPEDEF;																		\
		const char* name = #NAME;														\
		const DataBinding*     _data = 0; uint _dataSize = 0;	/* defaults */			\
		const MethodBinding*   _meth = 0; uint _methSize = 0;							\
		const FunctionBinding* _func = 0; uint _funcSize = 0;							\
		const ConstructorBinding* _cons = 0; uint _consSize = 0;						\
		const EnumMemberBinding* _enum = 0; uint _enumSize = 0;							\
		const eight::internal::LuaBindMode::Type _lua = NoLuaBind;						\
		FnLua* _luaGcFinalizer = 0;														\
		FnLuaRegister* _luaRegister = 0;												\
		{																				//
# define eiLuaBind( mode )																\
			const eight::internal::LuaBindMode::Type _lua =								\
				(eight::internal::LuaBindMode::Type)(0 | mode);							\
			_luaRegister = &RegisterTypeMetatable<TT>;									\
			if( !std::is_trivially_destructible<TT>::value )							\
				_luaGcFinalizer = &LuaDestruct<TT>;										//
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
#   define eiBindMethodRename( a, name )														\
				{ #name, (memfuncptr)&T::a,												\
				  GetCallWrapper<T,(memfuncptr)&T::a>(&T::a),							\
				  GetPushWrapper<T,(memfuncptr)&T::a>(&T::a),							\
				  GetLuaWrapper<_lua&LuaCallAndPush>::Call<T,(memfuncptr)&T::a>(&T::a),	\
				  GetLuaWrapper<_lua&LuaCallAndPush>::Push<T,(memfuncptr)&T::a>(&T::a) },//
#   define eiBindMethod( a )			eiBindMethodRename( a, a )					//
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
#   define eiBindFunctionRename( a, name )												\
				{ #name, (callback)&T::a,												\
				  GetFnCallWrapper<(callback)&T::a>(&T::a),								\
				  GetFnLuaWrapper<_lua&LuaCallAndPush>::Call<(callback)&T::a>(&T::a)	\
				},																		//
#   define eiBindFunction( a )			eiBindFunctionRename( a, a )					//
# define eiEndFunctions()																\
			};																			\
			_func = s_func;																\
			_funcSize = eiArraySize(s_func);											//

# define eiBeginEnumValues()															\
			const static EnumMemberBinding s_enum[] = {									//
#   define eiBindEnumValueRename( a, name )												\
				{ #name, (int)T::a, },													//
#   define eiBindEnumValue( a )			eiBindEnumValueRename( a, a )					//
# define eiEndEnumValues()																\
			};																			\
			_enum = s_enum;																\
			_enumSize = eiArraySize(s_enum);											//

# define eiBeginConstructors()															\
			const static ConstructorBinding s_cons[] = {								//
#   define eiBindConstructorRef( ... )													\
				{ #__VA_ARGS__, (callback)&ConstructorWrapper<T,__VA_ARGS__>::Placement,\
				  0,0,0,0,																\
				  GetFnLuaWrapper<_lua&LuaCallAndPush>::GcNew<(callback)&ConstructorWrapper<T,__VA_ARGS__>::Placement>(&ConstructorWrapper<T,__VA_ARGS__>::Placement),  \
				  GetFnLuaWrapper<_lua&LuaCallAndPush>::Call<(callback)&ConstructorWrapper<T,__VA_ARGS__>::Alloc>(&ConstructorWrapper<T,__VA_ARGS__>::Alloc)  \
				},			
#   define eiBindDefaultConstructorRef( ... )													\
				{ "", (callback)&ConstructorWrapper<T>::Placement,\
				  0,0,0,0,																\
				  GetFnLuaWrapper<_lua&LuaCallAndPush>::GcNew<(callback)&ConstructorWrapper<T,__VA_ARGS__>::Placement>(&ConstructorWrapper<T,__VA_ARGS__>::Placement),  \
				  GetFnLuaWrapper<_lua&LuaCallAndPush>::Call<(callback)&ConstructorWrapper<T>::Alloc>(&ConstructorWrapper<T>::Alloc)  \
				},																		//
#   define eiBindConstructorValue( ... )												\
				{ #__VA_ARGS__, (callback)&ConstructorWrapper<T,__VA_ARGS__>::Placement,\
				  0,0,0,0,																\
				  GetFnLuaWrapper<_lua&LuaCallAndPush>::Call<(callback)&ConstructorWrapper<T,__VA_ARGS__>::Value>(&ConstructorWrapper<T,__VA_ARGS__>::Value), \
				  GetFnLuaWrapper<_lua&LuaCallAndPush>::Call<(callback)&ConstructorWrapper<T,__VA_ARGS__>::Alloc>(&ConstructorWrapper<T,__VA_ARGS__>::Alloc)  \
				},																		//
#   define eiBindDefaultConstructorValue(  )												\
				{ "", (callback)&ConstructorWrapper<T>::Placement,\
				  0,0,0,0,																\
				  GetFnLuaWrapper<_lua&LuaCallAndPush>::Call<(callback)&ConstructorWrapper<T>::Value>(&ConstructorWrapper<T>::Value), \
				  GetFnLuaWrapper<_lua&LuaCallAndPush>::Call<(callback)&ConstructorWrapper<T>::Alloc>(&ConstructorWrapper<T>::Alloc)  \
				},																		//
/*
                  GetFnCallWrapper<(callback)&ConstructorWrapper<__VA_ARGS__>::Value>(&ConstructorWrapper<__VA_ARGS__>::Value),									\
				  GetFnCallWrapper<(callback)&ConstructorWrapper<__VA_ARGS__>::Placement>(&ConstructorWrapper<__VA_ARGS__>::Placement),							\
                  GetFnCallWrapper<(callback)&ConstructorWrapper<__VA_ARGS__>::Alloc<T,GlobalHeap> >(&ConstructorWrapper<__VA_ARGS__>::Alloc<T,GlobalHeap>),	\
                  GetFnCallWrapper<(callback)&ConstructorWrapper<__VA_ARGS__>::Alloc<T,Scope> >(&ConstructorWrapper<__VA_ARGS__>::Alloc<T,Scope>),				\
					  */
																						//
#   define eiBindFactoryFunction( a )													\
				{ #a, (callback)&T::a,													\
				  GetFnCallWrapper<(callback)&T::a>(&T::a),								\
                  nullptr, nullptr, nullptr,											\
				  GetFnLuaWrapper<_lua&LuaCallAndPush>::Call<(callback)&T::a>(&T::a),	\
				  nullptr,																\
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
				_enum, _enumSize,														\
				_luaRegister,															\
				_luaGcFinalizer,														\
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
