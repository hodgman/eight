//------------------------------------------------------------------------------
#pragma once
//#pragma optimize("",off)//!!!!!!!!!!!!!!!!

#include <eight/core/message.h>
#include <eight/core/typeinfo.h>
#include <eight/lua/lua.h>
#include <eight/lua/lua_api.h>
#include <eight/core/profiler.h>
#include <eight/core/alloc/interface.h>
#include <eight/core/traits.h>
#include <eight/core/macro.h>
#include <utility>
namespace eight {
//------------------------------------------------------------------------------
/* C++ objects are bound to Lua as raw pointers without OO-style metatables.
 * Lifetimes are managed on the C++ side as usual.
 * For optimized builds, light userdata objects are used to store these pointers.
 * For development builds, full-userdata objects containing LuaPointer instances
 *  are used, so that type information can be validated.
 * If a C++ value needs to be Lua Garbage Collected, it can be put in a
 * LuaBox userdata, which otherwise acts like a LuaPointer.
 */

//to quickly identify full-userdata objects, their metatables contain the 's_metaId' pointer as a light-userdata key.

//------------------------------------------------------------------------------
namespace internal {
//------------------------------------------------------------------------------

#if !defined(eiBUILD_NO_LUA_TYPESAFETY)
struct LuaPointer
{
	static const char* const s_metaId;
	u32 typeHash;
	const char* typeName;
	void* pointer;
};
#endif

struct LuaBoxBase
{
	static const char* const s_metaId;
	u32 typeHash;
	const char* typeName;
};
template<class T> struct LuaBox : public LuaBoxBase
{
	T value;
};

static const char s_LuaFutureMarker = 42;
inline int luaL_abs_index(lua_State* L, int i)
{
	return ((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) :
                                        lua_gettop(L) + (i) + 1);
}
extern const char* const s_cTableHandles;

namespace LuaMapping
{
	enum Type
	{
		Object = 0,
		Integer,
		Number,
		Boolean,
		Void,
	};
	struct ObjectTag  { char c; };
	struct IntegerTag { char c[2]; };
	struct NumberTag  { char c[3]; };
	struct BooleanTag { char c[4]; };
	eiSTATIC_ASSERT(Object  == (Type)(sizeof( ObjectTag)-1));
	eiSTATIC_ASSERT(Integer == (Type)(sizeof(IntegerTag)-1));
	eiSTATIC_ASSERT(Number  == (Type)(sizeof( NumberTag)-1));
	eiSTATIC_ASSERT(Boolean == (Type)(sizeof(BooleanTag)-1));
	ObjectTag  Check(...);
	IntegerTag Check(s32);
	IntegerTag Check(u32);
	NumberTag  Check(float);
	NumberTag  Check(double);
	NumberTag  Check(u64);
	NumberTag  Check(s64);
}

template<class T, int A=0> struct LuaTypeMapping_
{
	const static LuaMapping::Type value = 
		(LuaMapping::Type)(sizeof(LuaMapping::Check(*(T*)0))-1);
};
template<class T> struct LuaTypeMapping_<T,16>
{//hack for: actual parameter with __declspec(align('16')) won't be aligned
	const static LuaMapping::Type value = LuaMapping::Object;
};
template<int A> struct LuaTypeMapping_<bool,A>
{
	const static LuaMapping::Type value = LuaMapping::Boolean;
};


template<class T> struct LuaTypeMapping
{
	const static LuaMapping::Type value = LuaTypeMapping_<T,eiAlignOf(T)>::value;
};
template<> struct LuaTypeMapping<void>
{
	const static LuaMapping::Type value = LuaMapping::Void;
};
eiSTATIC_ASSERT(LuaTypeMapping<LuaMapping::Type>::value == LuaMapping::Integer);//enum
eiSTATIC_ASSERT(LuaTypeMapping<int>::value        == LuaMapping::Integer);
eiSTATIC_ASSERT(LuaTypeMapping<char>::value       == LuaMapping::Integer);
eiSTATIC_ASSERT(LuaTypeMapping<u8>::value         == LuaMapping::Integer);
eiSTATIC_ASSERT(LuaTypeMapping<s16>::value        == LuaMapping::Integer);
eiSTATIC_ASSERT(LuaTypeMapping<u32>::value        == LuaMapping::Integer);
eiSTATIC_ASSERT(LuaTypeMapping<u64>::value        == LuaMapping::Number);
eiSTATIC_ASSERT(LuaTypeMapping<s64>::value        == LuaMapping::Number);
eiSTATIC_ASSERT(LuaTypeMapping<double>::value     == LuaMapping::Number);
eiSTATIC_ASSERT(LuaTypeMapping<float>::value      == LuaMapping::Number);
eiSTATIC_ASSERT(LuaTypeMapping<bool>::value       == LuaMapping::Boolean);
eiSTATIC_ASSERT(LuaTypeMapping<void>::value       == LuaMapping::Void);
eiSTATIC_ASSERT(LuaTypeMapping<LuaBoxBase>::value == LuaMapping::Object);//struct


void PushMetatable(lua_State *L, void* name);

void LuaTypeError(lua_State* L, int i, const char* expected, const char* received);

template<class TValue> inline void LuaTypeCheck(lua_State* L, int i, u32 typeHash, const char* typeName)
{
	if( TypeHash<TValue>::Get() != typeHash )
		LuaTypeError(L, i, TypeName<TValue>::Get(), typeName);
	else
		eiASSERT( 0==strcmp(typeName,TypeName<TValue>::Get()) );
}

inline bool MetatableHasLudKey( lua_State* L, int i, void* key )
{
	if( !lua_getmetatable( L, i ) ) 
		return false;
	lua_pushlightuserdata( L, key );
	lua_rawget( L, -2 );
	int result = lua_toboolean( L, -1 );
	lua_pop( L, 2 );
	return !!result;
}

template<class T, bool Boxable> struct UserDataToPointer
{
	static T* Cast(lua_State* L, int i, int ltype)
	{
		if( ltype == LUA_TLIGHTUSERDATA )
			return (T*)lua_touserdata(L, i);
		else if( MetatableHasLudKey(L, i, (void*)LuaBox<T>::s_metaId) )
		{
			LuaBox<T>* box = (LuaBox<T>*)lua_touserdata(L, i);
			LuaTypeCheck<T>( L, i, box->typeHash, box->typeName );
			return &box->value;
		}
#if !defined(eiBUILD_NO_LUA_TYPESAFETY)
		else if( MetatableHasLudKey(L, i, (void*)LuaPointer::s_metaId) )
		{
			LuaPointer* ptr = (LuaPointer*)lua_touserdata(L, i);
			LuaTypeCheck<T>( L, i, ptr->typeHash, ptr->typeName );
			return (T*)ptr->pointer;
		}
#endif
		else
			luaL_argcheck(L, false, i, "Unknown userdata type");
		return 0;
	}
};
template<class T> struct UserDataToPointer<T,false>
{
	static T* Cast(lua_State* L, int i, int ltype)
	{
		if( ltype == LUA_TLIGHTUSERDATA )
			return (T*)lua_touserdata(L, i);
#if !defined(eiBUILD_NO_LUA_TYPESAFETY)
		else if( MetatableHasLudKey(L, i, (void*)LuaPointer::s_metaId) )
		{
			LuaPointer* ptr = (LuaPointer*)lua_touserdata(L, i);
			LuaTypeCheck<T>( L, i, ptr->typeHash, ptr->typeName );
			return (T*)ptr->pointer;
		}
#endif
		else
			luaL_argcheck(L, false, i, "Unknown userdata type");
		return 0;
	}
};
template<class T, bool AllowNil> T* CheckUserDataToPointer(lua_State* L, int i, int ltype)
{
	if( AllowNil && ltype == LUA_TNIL )
		return 0;
	if( (ltype != LUA_TUSERDATA) & (ltype != LUA_TLIGHTUSERDATA) )
		LuaTypeError(L, i, TypeName<T>::Get(), lua_typename(L, lua_type(L, i)));
	T* pointer = UserDataToPointer<T, !std::is_base_of<TagInterface,T>::value>::Cast(L, i, ltype);
	return pointer;
}

template<bool IsPointer, LuaMapping::Type> struct ValueFromLua;
template<LuaMapping::Type Y> struct ValueFromLua<true, Y>
{
	template<class T> static void Fetch(lua_State* L, int i, int ltype, T& output)
	{
		typedef StripPointer<MsgArg<T>::StorageType>::Type TValue;
		output = CheckUserDataToPointer<TValue, !IsReference<T>::value>(L, i, ltype);
		if( IsReference<T>::value )
			luaL_argcheck(L, output, i, "NULL reference!");
	}
	static void Fetch(lua_State* L, int i, int ltype, const char*& output)
	{
		luaL_argcheck(L, ltype == LUA_TSTRING, i, "Expected string");
		output = lua_tostring(L, i);
	}
};
template<> struct ValueFromLua<false, LuaMapping::Object>
{
	template<class T> static void Fetch(lua_State* L, int i, int ltype, T& output)
	{
		T* pointer = CheckUserDataToPointer<T, false>(L, i, ltype);
		luaL_argcheck(L, pointer, i, "NULL reference!");
		output = *pointer;
	}
};
template<> struct ValueFromLua<false, LuaMapping::Integer>
{
	template<class T> static void Fetch(lua_State* L, int i, int ltype, T& output)
	{
		if( ltype != LUA_TNUMBER )
			LuaTypeError(L, i, TypeName<T>::Get(), lua_typename(L, lua_type(L, i)));
		output = (T)lua_tointeger(L, i);
	}
};
template<> struct ValueFromLua<false, LuaMapping::Boolean>
{
	template<class T> static void Fetch(lua_State* L, int i, int ltype, T& output)
	{
		if( ltype != LUA_TBOOLEAN )
			LuaTypeError(L, i, TypeName<T>::Get(), lua_typename(L, lua_type(L, i)));
		output = (T)!!lua_toboolean(L, i);
	}
};
template<> struct ValueFromLua<false, LuaMapping::Number>
{
	template<class T> static void Fetch(lua_State* L, int i, int ltype, T& output)
	{
		if( ltype != LUA_TNUMBER )
			LuaTypeError(L, i, TypeName<T>::Get(), lua_typename(L, lua_type(L, i)));
		output = (T)lua_tonumber(L, i);
	}
};


template<LuaMapping::Type> struct ValueToLua {};
template<> struct ValueToLua<LuaMapping::Void>
{
	static int Push(lua_State*, Nil) { return 0; }
};
template<> struct ValueToLua<LuaMapping::Object>
{
	static int Push(lua_State* L, const char* value)
	{
		lua_pushstring(L, value);
		return 1;
	}
	static int Push(lua_State* L, FnLua* value)
	{
		lua_pushcfunction(L, value);
		return 1;
	}
	template<class T> static int Push(lua_State* L, const LuaClosure<T>& value)
	{
		int numUpvalues = PushValue( L, value.data );
		lua_pushcclosure(L, value.function, numUpvalues);
		return 1;
	}
	template<class T> static int Push(lua_State* L, const T& value)
	{
		LuaBox<T>* data = (LuaBox<T>*)lua_newuserdata(L, sizeof(LuaBox<T>));
		internal::PushMetatable(L, (void*)LuaBox<T>::s_metaId);
		lua_setmetatable(L, -2);
		typedef StripPointer<T*>::Type Type;
		data->typeHash = TypeHash<Type>::Get();
		data->typeName = TypeName<Type>::Get();
		data->value = value;
		//todo - if not POD, add destructor in __gc, and construct properly:
	//	new(&data->value) T(value);
		return 1;
	}
	template<class T> static int Push(lua_State* L, T* value)
	{
#if defined(eiBUILD_NO_LUA_TYPESAFETY)
		lua_pushlightuserdata(L, value);
#else
		LuaPointer* data = (LuaPointer*)lua_newuserdata(L, sizeof(LuaPointer));
		internal::PushMetatable(L, (void*)LuaPointer::s_metaId);
		lua_setmetatable(L, -2);
		typedef StripPointer<T*>::Type Type;
		data->typeHash = TypeHash<Type>::Get();
		data->typeName = TypeName<Type>::Get();
		data->pointer = (Type*)value;
#endif
		return 1;
	}
	//template<class T> static int Push(lua_State* L, const T* value)
	//{
	//	return Push(L, (T*)value);
	//}
};
template<> struct ValueToLua<LuaMapping::Integer>
{
	template<class T> static int Push(lua_State* L, T value)
	{
		lua_pushinteger(L, (lua_Integer)value);
		return 1;
	}
};
template<> struct ValueToLua<LuaMapping::Number>
{
	template<class T> static int Push(lua_State* L, T value)
	{
		lua_pushnumber(L, (lua_Number)value);
		return 1;
	}
};
template<> struct ValueToLua<LuaMapping::Boolean>
{
	template<class T> static int Push(lua_State* L, T value)
	{
		lua_pushboolean(L, value?0xFFFFFFFF:0);
		return 1;
	}
};

struct TuplePushHelper
{
	int count;
	lua_State* L;
	template<class T> void operator()( const T& t )
	{
		count += PushValue(L, t);
	}
};
template<class A, class B, class C, class D, class E, class F, class G, class H, class I>
int PushValue( lua_State* L, const Tuple<A,B,C,D,E,F,G,H,I>& t )
{
	TuplePushHelper f = { 0, L };
	t.ForEach(f);
	return f.count;
}
template<class T> int PushValue( lua_State* L, const T& t )
{
	return internal::ValueToLua<internal::LuaTypeMapping<T>::value>::Push(L, t);
}
template<class T> int PushValue( lua_State* L, const Maybe<T>& t )
{
	if( t.initialized )
		return PushValue(L, t.value);
	else
		return 0;
}
inline int PushValue( lua_State* L, const LuaTableTemporary& t )
{
	lua_pushvalue( L, t.stackLocation );
}
inline int PushValue( lua_State* L, const LuaTableReference& t )
{
	eiDEBUG( int stackSize = lua_gettop(L) );
	lua_pushlightuserdata(L, (void*)internal::s_cTableHandles);//Push s_cTableHandles
	lua_rawget(L, LUA_REGISTRYINDEX);  // push registry[s_cTableHandles], pop s_cTableHandles
	lua_rawgeti(L, -1, t.m_registryKey); //push registry[s_cTableHandles][registryKey]
	lua_remove(L, -2);//pop registry[s_cTableHandles]
	eiDEBUG( int stackSize2 = lua_gettop(L) );
	eiASSERT( stackSize+1 == stackSize2 );
	return 1;
}

template<class T> inline void FetchValue(lua_State* L, int i, int ltype, T& output)
{
	eiSTATIC_ASSERT( (IsSameType<T,LuaTableTemporary>::value == false) );
	eiSTATIC_ASSERT( (IsSameType<T,LuaTableReference>::value == false) );
	typedef MsgArg<T>::StorageType StorageType;
	typedef StripPointer<StorageType>::Type TValue;
	const bool storedAsPointer = IsPointer<StorageType>::value;
	internal::ValueFromLua<storedAsPointer, internal::LuaTypeMapping<TValue>::value>::Fetch(L, i, ltype, output);
}

inline void FetchValue(lua_State* L, int i, int ltype, LuaTableTemporary& output)
{
	typedef MsgArg<LuaTableTemporary>::StorageType StorageType;
	typedef StripPointer<StorageType>::Type TValue;
	luaL_argcheck(L, ltype == LUA_TTABLE, i, "Expected table");
	output.stackLocation = luaL_abs_index(L, i);
}
inline void FetchValue(lua_State* L, int i, int ltype, LuaTableReference& output)
{
	typedef MsgArg<LuaTableTemporary>::StorageType StorageType;
	typedef StripPointer<StorageType>::Type TValue;
	luaL_argcheck(L, ltype == LUA_TTABLE, i, "Expected table");
	LuaTableTemporary temp;
	temp.stackLocation = luaL_abs_index(L, i);
	output.Acquire(L, temp);
}


inline bool TryFetchFuture(lua_State* L, int i, int ltype, FutureIndex& output)
{
	if( ltype == LUA_TSTRING && lua_objlen(L, i) == 6 )
	{
		size_t len = 0;
		const char* data = lua_tolstring(L, i, &len);
		eiASSERT( len == 6 );
		if( (data[0] == 0) & (data[1] == internal::s_LuaFutureMarker) )
		{
			FutureIndex future;
			future.index  = *(u16*)&data[2];
			future.offset = *(u16*)&data[4];
			output = future;
			return true;
		}
	}
	return false;
}

struct FetchArgsPush
{
	FetchArgsPush(lua_State* L, uint startArg, uint& f) : L(L), futureMask(f), i(startArg), mask(1) {} lua_State* L; uint& futureMask; uint i, mask;
	template<class T> void operator()(T& arg)
	{
		int ltype = lua_type(L, i);
		if( TryFetchFuture(L, i, ltype, arg.index) )
			futureMask |= mask;
		else
			FetchValue(L, i, ltype, arg.value);
		mask <<= 1;
		++i;
	}
};
struct FetchArgsCall
{
	FetchArgsCall(lua_State* L, uint startArg) : L(L), i(startArg) {} lua_State* L; uint i;
	template<class T> void operator()(T& arg)
	{
		int ltype = lua_type(L, i);
		FetchValue(L, i++, ltype, arg.value);
	}
};

inline bool PushFunction(lua_State* L, const LuaGlobal& name)
{
	lua_getglobal(L, name.global);
	if(!lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return false;
	}
	return true;
}

inline bool PushFunction(lua_State* L, const LuaGlobalField& name)
{
	lua_getglobal(L, name.global);
	if(!lua_istable(L, -1))
	{
		lua_pop(L, 1);
		return false;
	}
	lua_getfield(L, -1, name.field);//get global[field]
	if(!lua_isfunction(L, -1))
	{
		lua_pop(L, 2);
		return false;
	}
	lua_remove(L, -2);//remove global from the stack
	return true;
}

inline bool PushFunction(lua_State* L, const LuaTableMethod& name)
{
	int pushed = PushValue(L, name.table);
	if( pushed != 1 )
	{
		eiASSERT( false );
		if( pushed )
			lua_pop(L, pushed);
		return false;
	}
	lua_getfield(L, -1, name.field);//get table[field]
	if(!lua_isfunction(L, -1))
	{
		lua_pop(L, 2);
		return false;
	}
	lua_remove(L, -2);//remove table from the stack
	return true;
}
inline bool PushFunction(lua_State* L, const LuaTempTableMethod& name)
{
	int pushed = PushValue(L, name.table);
	if( pushed != 1 )
	{
		eiASSERT( false );
		if( pushed )
			lua_pop(L, pushed);
		return false;
	}
	lua_getfield(L, -1, name.field);//get table[field]
	if(!lua_isfunction(L, -1))
	{
		lua_pop(L, 2);
		return false;
	}
	lua_remove(L, -2);//remove table from the stack
	return true;
}


template<class Return> typename ReturnType<Return>::Type PCall(lua_State* L, int numArgs)
{
	const int hasReturn = IsSameType<Return,void>::value ? 0 : 1;
	int returnCode = lua_pcall(L, numArgs, hasReturn, 0);
	if( returnCode )
	{
		const char* errorText = lua_tostring(L, -1);//todo - logging, recovery
		printf( "Lua error: %s\n", errorText );
//		eiASSERT(false);
		lua_pop(L, 1);
	}
	else if( hasReturn )
	{
		ReturnType<Return> r;
		FetchValue(L, -1, lua_type(L, -1), r.value);
		lua_pop(L, 1);
		return r.value;
	}
	return ReturnType<Return>::Type();
}

//------------------------------------------------------------------------------
}//end namespace internal
//------------------------------------------------------------------------------
inline LuaTableReference::LuaTableReference( lua_State* L, const LuaGlobal& name )
	: m_registryKey() 
{
	lua_getglobal(L, name.global);
	if(lua_istable(L, -1))
	{
		LuaTableTemporary temp;
		temp.stackLocation = internal::luaL_abs_index(L, -1);
		Acquire( L, temp );
	}
	lua_pop(L, 1);
}
inline void LuaTableReference::Consume( LuaTableReference& other )
{
	eiASSERT( m_registryKey == 0 );
	m_registryKey = other.m_registryKey;
	other.m_registryKey = 0;
}
inline void LuaTableReference::Acquire( lua_State* L, const LuaTableTemporary& input )
{
	eiASSERT( m_registryKey == 0 );
	extern int GetNewLuaTableKey();
	m_registryKey = GetNewLuaTableKey();
	lua_pushlightuserdata(L, (void*)internal::s_cTableHandles);//Push s_cTableHandles
	lua_rawget(L, LUA_REGISTRYINDEX);  // push registry[s_cTableHandles], pop s_cTableHandles
	lua_pushvalue(L, input.stackLocation);//push table to top of stack
	lua_rawseti(L, -2, m_registryKey);//registry[s_cTableHandles][registryKey] = table, pop table/key
	lua_pop(L, 1);//pop registry[s_cTableHandles]
}
inline void LuaTableReference::Release( lua_State* L )
{
	eiASSERT( m_registryKey != 0 );
	lua_pushlightuserdata(L, (void*)internal::s_cTableHandles);//Push s_cTableHandles
	lua_rawget(L, LUA_REGISTRYINDEX);  // push registry[s_cTableHandles], pop s_cTableHandles
	lua_pushnil(L);
	lua_rawseti(L, -2, m_registryKey);//registry[s_cTableHandles][registryKey] = nil, pop table/key
	lua_pop(L, 1);//pop registry[s_cTableHandles]
	m_registryKey = 0;
}


template<class T, memfuncptr fn, class F>
int LuaPushWrapper(lua_State* L)
{
	typedef ArgFromSig<F>::Storage Args;
	typedef Args::Type ArgTuple;
	typedef ReturnType<ArgFromSig<F>::Return> Return;
	int numArgs = lua_gettop(L);
	if( ArgTuple::length != numArgs-1 )
		luaL_error(L, "Expected %d args, received %d", ArgTuple::length+1, numArgs );
	
	CallBuffer* q = 0;//todo - get queue from globals?
	T* user = internal::CheckUserDataToPointer<T,false>(L, 1, lua_type(L, 1));
	luaL_argcheck(L, user, 1, "NULL reference!");

	eiASSERT(q);
	FutureCall* msg = q->Push( &CallWrapper<T,fn,F>, user, sizeof(Args), Return::Size() );
	Args& args = *(Args*)msg->args.Ptr();
	ArgHeader header = { 0, &Args::Info };
	args.b = header;
	args.t.ForEach( internal::FetchArgsPush(L, 2, header.futureMask) );

	if( msg->returnOffset != 0xFFFFFFFF )
	{
		FutureIndex retval = { msg->returnIndex };
		char luaFuture[6] = { 0, internal::s_LuaFutureMarker };
		*(u16*)&luaFuture[2] = retval.index;
		*(u16*)&luaFuture[4] = retval.offset;
		lua_pushlstring(L, luaFuture, 6);
		return 1;
	}
	else
		return 0;
}

template<class T, memfuncptr fn, class F>
int LuaCallWrapper(lua_State* L)
{
	typedef ArgFromSig<F>::Storage::Type ArgTuple;
	typedef ArgFromSig<F>::Return RawReturn;
	typedef ReturnType<RawReturn> Return;
	int numArgs = lua_gettop(L);
	if( ArgTuple::length != numArgs-1 )
		luaL_error(L, "Expected %d args, received %d", ArgTuple::length+1, numArgs );
	T* user = internal::CheckUserDataToPointer<T,false>(L, 1, lua_type(L, 1));
	luaL_argcheck(L, user, 1, "NULL reference!");
	ArgTuple a;
	Return r;
	a.ForEach( internal::FetchArgsCall(L, 2) );
	Call( r, a, user, (F)fn );
	return internal::PushValue(L, r.value);
}
template<callback fn, class F>
int LuaFnCallWrapper(lua_State* L)
{
	typedef ArgFromSig<F>::Storage::Type ArgTuple;
	typedef ArgFromSig<F>::Return RawReturn;
	typedef ReturnType<RawReturn> Return;
	int numArgs = lua_gettop(L);
	if( ArgTuple::length != numArgs )
		luaL_error(L, "Expected %d args, received %d", ArgTuple::length, numArgs );
	ArgTuple a;
	Return r;
	a.ForEach( internal::FetchArgsCall(L, 1) );
	Call( r, a, (F)fn );
	return internal::PushValue(L, r.value);
}


template<class Return, class T>
typename ReturnType<Return>::Type PCall(lua_State* L, const T& name)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	if(internal::PushFunction(L, name))
	{
		return internal::PCall<Return>(L, 0);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A>
typename ReturnType<Return>::Type PCall(lua_State* L, const T& name, const A& a)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		return internal::PCall<Return>(L, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B>
typename ReturnType<Return>::Type PCall(lua_State* L, const T& name, const A& a, const B& b)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		return internal::PCall<Return>(L, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B, class C>
typename ReturnType<Return>::Type PCall(lua_State* L, const T& name, const A& a, const B& b, const C& c)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		args +=    internal::PushValue(L, c);
		return internal::PCall<Return>(L, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B, class C, class D>
typename ReturnType<Return>::Type PCall(lua_State* L, const T& name, const A& a, const B& b, const C& c, const D& d)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		args +=    internal::PushValue(L, c);
		args +=    internal::PushValue(L, d);
		return internal::PCall<Return>(L, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B, class C, class D, class E>
typename ReturnType<Return>::Type PCall(lua_State* L, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		args +=    internal::PushValue(L, c);
		args +=    internal::PushValue(L, d);
		args +=    internal::PushValue(L, e);
		return internal::PCall<Return>(L, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B, class C, class D, class E, class F>
typename ReturnType<Return>::Type PCall(lua_State* L, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		args +=    internal::PushValue(L, c);
		args +=    internal::PushValue(L, d);
		args +=    internal::PushValue(L, e);
		args +=    internal::PushValue(L, f);
		return internal::PCall<Return>(L, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B, class C, class D, class E, class F, class G>
typename ReturnType<Return>::Type PCall(lua_State* L, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		args +=    internal::PushValue(L, c);
		args +=    internal::PushValue(L, d);
		args +=    internal::PushValue(L, e);
		args +=    internal::PushValue(L, f);
		args +=    internal::PushValue(L, g);
		return internal::PCall<Return>(L, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B, class C, class D, class E, class F, class G, class H>
typename ReturnType<Return>::Type PCall(lua_State* L, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		args +=    internal::PushValue(L, c);
		args +=    internal::PushValue(L, d);
		args +=    internal::PushValue(L, e);
		args +=    internal::PushValue(L, f);
		args +=    internal::PushValue(L, g);
		args +=    internal::PushValue(L, h);
		return internal::PCall<Return>(L, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I>
typename ReturnType<Return>::Type PCall(lua_State* L, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		args +=    internal::PushValue(L, c);
		args +=    internal::PushValue(L, d);
		args +=    internal::PushValue(L, e);
		args +=    internal::PushValue(L, f);
		args +=    internal::PushValue(L, g);
		args +=    internal::PushValue(L, h);
		args +=    internal::PushValue(L, i);
		return internal::PCall<Return>(L, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
typename ReturnType<Return>::Type PCall(lua_State* L, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i, const J& j)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		args +=    internal::PushValue(L, c);
		args +=    internal::PushValue(L, d);
		args +=    internal::PushValue(L, e);
		args +=    internal::PushValue(L, f);
		args +=    internal::PushValue(L, g);
		args +=    internal::PushValue(L, h);
		args +=    internal::PushValue(L, i);
		args +=    internal::PushValue(L, j);
		return internal::PCall<Return>(L, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K>
typename ReturnType<Return>::Type PCall(lua_State* L, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i, const J& j, const K& k)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		args +=    internal::PushValue(L, c);
		args +=    internal::PushValue(L, d);
		args +=    internal::PushValue(L, e);
		args +=    internal::PushValue(L, f);
		args +=    internal::PushValue(L, g);
		args +=    internal::PushValue(L, h);
		args +=    internal::PushValue(L, i);
		args +=    internal::PushValue(L, j);
		args +=    internal::PushValue(L, k);
		return internal::PCall<Return>(L, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L>
typename ReturnType<Return>::Type PCall(lua_State* _, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i, const J& j, const K& k, const L& l)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(_);
	if(internal::PushFunction(_, name))
	{
		int args = internal::PushValue(_, a);
		args +=    internal::PushValue(_, b);
		args +=    internal::PushValue(_, c);
		args +=    internal::PushValue(_, d);
		args +=    internal::PushValue(_, e);
		args +=    internal::PushValue(_, f);
		args +=    internal::PushValue(_, g);
		args +=    internal::PushValue(_, h);
		args +=    internal::PushValue(_, i);
		args +=    internal::PushValue(_, j);
		args +=    internal::PushValue(_, k);
		args +=    internal::PushValue(_, l);
		return internal::PCall<Return>(_, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
typename ReturnType<Return>::Type PCall(lua_State* _, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i, const J& j, const K& k, const L& l, const M& m)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(_);
	if(internal::PushFunction(_, name))
	{
		int args = internal::PushValue(_, a);
		args +=    internal::PushValue(_, b);
		args +=    internal::PushValue(_, c);
		args +=    internal::PushValue(_, d);
		args +=    internal::PushValue(_, e);
		args +=    internal::PushValue(_, f);
		args +=    internal::PushValue(_, g);
		args +=    internal::PushValue(_, h);
		args +=    internal::PushValue(_, i);
		args +=    internal::PushValue(_, j);
		args +=    internal::PushValue(_, k);
		args +=    internal::PushValue(_, l);
		args +=    internal::PushValue(_, m);
		return internal::PCall<Return>(_, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N>
typename ReturnType<Return>::Type PCall(lua_State* _, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i, const J& j, const K& k, const L& l, const M& m, const N& n)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(_);
	if(internal::PushFunction(_, name))
	{
		int args = internal::PushValue(_, a);
		args +=    internal::PushValue(_, b);
		args +=    internal::PushValue(_, c);
		args +=    internal::PushValue(_, d);
		args +=    internal::PushValue(_, e);
		args +=    internal::PushValue(_, f);
		args +=    internal::PushValue(_, g);
		args +=    internal::PushValue(_, h);
		args +=    internal::PushValue(_, i);
		args +=    internal::PushValue(_, j);
		args +=    internal::PushValue(_, k);
		args +=    internal::PushValue(_, l);
		args +=    internal::PushValue(_, m);
		args +=    internal::PushValue(_, n);
		return internal::PCall<Return>(_, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
typename ReturnType<Return>::Type PCall(lua_State* _, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i, const J& j, const K& k, const L& l, const M& m, const N& n, const O& o)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(_);
	if(internal::PushFunction(_, name))
	{
		int args = internal::PushValue(_, a);
		args +=    internal::PushValue(_, b);
		args +=    internal::PushValue(_, c);
		args +=    internal::PushValue(_, d);
		args +=    internal::PushValue(_, e);
		args +=    internal::PushValue(_, f);
		args +=    internal::PushValue(_, g);
		args +=    internal::PushValue(_, h);
		args +=    internal::PushValue(_, i);
		args +=    internal::PushValue(_, j);
		args +=    internal::PushValue(_, k);
		args +=    internal::PushValue(_, l);
		args +=    internal::PushValue(_, m);
		args +=    internal::PushValue(_, n);
		args +=    internal::PushValue(_, o);
		return internal::PCall<Return>(_, args);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P>
typename ReturnType<Return>::Type PCall(lua_State* _, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i, const J& j, const K& k, const L& l, const M& m, const N& n, const O& o, const P& p)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(_);
	if(internal::PushFunction(_, name))
	{
		int args = internal::PushValue(_, a);
		args +=    internal::PushValue(_, b);
		args +=    internal::PushValue(_, c);
		args +=    internal::PushValue(_, d);
		args +=    internal::PushValue(_, e);
		args +=    internal::PushValue(_, f);
		args +=    internal::PushValue(_, g);
		args +=    internal::PushValue(_, h);
		args +=    internal::PushValue(_, i);
		args +=    internal::PushValue(_, j);
		args +=    internal::PushValue(_, k);
		args +=    internal::PushValue(_, l);
		args +=    internal::PushValue(_, m);
		args +=    internal::PushValue(_, n);
		args +=    internal::PushValue(_, o);
		args +=    internal::PushValue(_, p);
		return internal::PCall<Return>(_, args);
	}
	return ReturnType<Return>::Type();
}
//template<class Return, class T, typename... A>
//typename ReturnType<Return>::Type PCall(lua_State* L, const char* global, const char* field, A... a)
//{
//	return PCall(L, LuaGlobalField(global, field), a...);
//}


//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
