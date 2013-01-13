//------------------------------------------------------------------------------
#pragma once
#include <eight/core/message.h>
#include <eight/core/types.h>
#include <eight/core/typeinfo.h>
#include <eight/lua/lua_api.h>
namespace eight {
namespace lua {
//------------------------------------------------------------------------------
/* C++ objects are bound to Lua as raw pointers without OO-style metatables.
 * Lifetimes are managed on the C++ side as usual.
 * For optimized builds, light userdata objects are used to store these pointers.
 * For development builds, full-userdata objects containing LuaPointer instances
 *  are used, so that type information can be validated.
 * If a C++ value needs to be Lua Garbage Collected, it can be put in a
 * LuaBox userdata, which otherwise acts like a LuaPointer.
 */

//to quickly identify full-userdata objects, their metatables contain the 's_metatable' pointer as a light-userdata key.

//todo - move
template<class A, class B> struct TypesEqual      { const static bool value = false; };
template<class T>          struct TypesEqual<T,T> { const static bool value = true; };

//------------------------------------------------------------------------------
namespace internal {
//------------------------------------------------------------------------------

#if !defined(eiBUILD_NO_LUA_TYPESAFETY)
struct LuaPointer
{
	static const char* const s_metatable;
	u32 typeHash;
	const char* typeName;
	void* pointer;
};
#endif

struct LuaBoxBase
{
	static const char* const s_metatable;
	u32 typeHash;
	const char* typeName;
};
template<class T> struct LuaBox : public LuaBoxBase
{
	T value;
};

static const char s_LuaFutureMarker = 42;

namespace LuaMapping
{
	enum Type
	{
		Object = 0,
		Integer,
		Number,
		Void,
	};
	struct ObjectTag  { char c; };
	struct IntegerTag { char c[2]; };
	struct NumberTag  { char c[3]; };
	eiSTATIC_ASSERT(Object  == (Type)(sizeof( ObjectTag)-1));
	eiSTATIC_ASSERT(Integer == (Type)(sizeof(IntegerTag)-1));
	eiSTATIC_ASSERT(Number  == (Type)(sizeof( NumberTag)-1));
	ObjectTag  Check(...);
	IntegerTag Check(s32);
	IntegerTag Check(u32);
	NumberTag  Check(float);
	NumberTag  Check(double);
	NumberTag  Check(u64);
	NumberTag  Check(s64);
}
template<class T> struct LuaTypeMapping
{
	const static LuaMapping::Type value = (LuaMapping::Type)(sizeof(LuaMapping::Check(*(T*)0))-1);
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

template<class T> T* UserDataToPointer(lua_State* L, int i, int ltype)
{
	if( ltype == LUA_TLIGHTUSERDATA )
		return (T*)lua_touserdata(L, i);
	else if( MetatableHasLudKey(L, i, (void*)LuaBoxBase::s_metatable) )
	{
		LuaBox<T>* box = (LuaBox<T>*)lua_touserdata(L, i);
		LuaTypeCheck<T>( L, i, box->typeHash, box->typeName );
		return &box->value;
	}
#if !defined(eiBUILD_NO_LUA_TYPESAFETY)
	else if( MetatableHasLudKey(L, i, (void*)LuaPointer::s_metatable) )
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
template<class T, bool AllowNil> T* CheckUserDataToPointer(lua_State* L, int i, int ltype)
{
	if( AllowNil && ltype == LUA_TNIL )
		return 0;
	if( (ltype != LUA_TUSERDATA) & (ltype != LUA_TLIGHTUSERDATA) )
		LuaTypeError(L, i, TypeName<T>::Get(), lua_typename(L, lua_type(L, i)));
	T* pointer = UserDataToPointer<T>(L, i, ltype);
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
	template<class T> static int Push(lua_State* L, const T& value)
	{
		LuaBox<T>* data = (LuaBox<T>*)lua_newuserdata(L, sizeof(LuaBox<T>));
		internal::PushMetatable(L, (void*)LuaBoxBase::s_metatable);
		lua_setmetatable(L, -2);
		typedef StripPointer<T*>::Type Type;
		data->typeHash = TypeHash<Type>::Get();
		data->typeName = TypeName<Type>::Get();
		data->value = value;
		return 1;
	}
	template<class T> static int Push(lua_State* L, const T* value)
	{
		return Push(L, (T*)value);
	}
	template<class T> static int Push(lua_State* L, T* value)
	{
#if defined(eiBUILD_NO_LUA_TYPESAFETY)
		lua_pushlightuserdata(L, value);
#else
		LuaPointer* data = (LuaPointer*)lua_newuserdata(L, sizeof(LuaPointer));
		internal::PushMetatable(L, (void*)LuaPointer::s_metatable);
		lua_setmetatable(L, -2);
		typedef StripPointer<T*>::Type Type;
		data->typeHash = TypeHash<Type>::Get();
		data->typeName = TypeName<Type>::Get();
		data->pointer = value;
#endif
		return 1;
	}
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

template<class T> int PushValue( lua_State* L, typename ConstRef<T>::Type t )
{
	return internal::ValueToLua<internal::LuaTypeMapping<T>::value>::Push(L, t);
}

template<class T> inline void FetchValue(lua_State* L, int i, int ltype, T& output)
{
	typedef MsgArg<T>::StorageType StorageType;
	typedef StripPointer<StorageType>::Type TValue;
	const bool storedAsPointer = IsPointer<StorageType>::value;
	internal::ValueFromLua<storedAsPointer, internal::LuaTypeMapping<TValue>::value>::Fetch(L, i, ltype, output);
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

inline bool PushGlobalFunction(lua_State* L, const char* name)
{
	lua_pushstring(L, name);
	lua_rawget(L, LUA_GLOBALSINDEX);
	if(!lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return false;
	}
	return true;
}

template<class Return> typename ReturnType<Return>::Type PCall(lua_State* L)
{
	const int hasReturn = TypesEqual<Return,void>::value ? 0 : 1;
	int returnCode = lua_pcall(L, 1, hasReturn, 0);
	if( returnCode )
	{
		const char* errorText = lua_tostring(L, -1);//dodo - print
		eiASSERT(false);
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

template<class T, memfuncptr fn, class F>
int LuaPushWrapper(lua_State* L)
{
	typedef ArgFromSig<F>::Storage Args;
	typedef Args::Type ArgTuple;
	typedef ReturnType<ArgFromSig<F>::Return> Return;
	int numArgs = lua_gettop(L);
	if( ArgTuple::length != numArgs-1 )
		luaL_error(L, "Expected %d args, received %d", ArgTuple::length, numArgs );
	
	CallBuffer* q = 0;//todo - get queue from globals?
	T* user = internal::CheckUserDataToPointer<T,false>(L, 1, lua_type(L, 1));
	luaL_argcheck(L, user, 1, "NULL reference!");
	
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
		luaL_error(L, "Expected %d args, received %d", ArgTuple::length, numArgs );
	T* user = internal::CheckUserDataToPointer<T,false>(L, 1, lua_type(L, 1));
	luaL_argcheck(L, user, 1, "NULL reference!");
	ArgTuple a;
	Return r;
	a.ForEach( internal::FetchArgsCall(L, 2) );
	Call( r, a, user, (F)fn );
	return internal::PushValue<RawReturn>(L, r.value);
}

template<class Return, class A>
typename ReturnType<Return>::Type PCall(lua_State* L, const char* name, const A& a)
{
	eiCheckLuaStack(L);
	if(internal::PushGlobalFunction(L, name))
	{
		internal::PushValue<A>(L, a);
		return internal::PCall<Return>(L);
	}
	return ReturnType<Return>::Type();
}


//------------------------------------------------------------------------------
}} // namespace eight, lua
//------------------------------------------------------------------------------
