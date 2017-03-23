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
#include <eight/core/array.h>
#include <eight/core/id.h>
#include <rdestl/pair.h>
#include <rdestl/vector.h>
#include <rdestl/rde_string.h>
#include <utility>
#include <setjmp.h>
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


#if eiDEBUG_LEVEL > 0
	#define eiBUILD_LUA_RETRY_HANDLING
#endif

#ifdef eiBUILD_LUA_RETRY_HANDLING
	#define eiLUA_SETJMP 		jmp_buf _err_jmp_;setjmp(_err_jmp_)
	#define eiLUA_LONGJMP(x)    longjmp(x, 1)
	typedef jmp_buf				LUA_JMP_TYPE;
#else
	#define eiLUA_SETJMP		int _err_jmp_ = 0
	#define eiLUA_LONGJMP(x)    
	typedef int					LUA_JMP_TYPE;
#endif

#define eiLuaArgcheck(L, cond,numarg,extramsg)	\
		luaL_argcheck(L, cond, (numarg), (extramsg))

template<class K, class V>
class KeyValueArray
{
public:
	KeyValueArray(uint hint=0);
	void PushBack( const K&, const V& );
	const K& Key( uint idx ) const;
	const V& Value( uint idx ) const;
	uint Size() const;
	void Clear() { m_data.clear(); }
	const rde::pair<K,V>* Begin() const { return m_data.begin(); }
	const rde::pair<K,V>* End() const { return m_data.end(); }
private:
	rde::vector< rde::pair<K,V> > m_data;
};
template<class K, class V> KeyValueArray<K,V>::KeyValueArray(uint hint)
{
	if( hint )
		m_data.reserve(hint);
}
template<class K, class V> void KeyValueArray<K,V>::PushBack( const K& k, const V& v )
{
	m_data.push_back(rde::pair<K,V>(k,v));
}
template<class K, class V> const K& KeyValueArray<K,V>::Key( uint idx ) const
{
	return m_data[idx].first;
}
template<class K, class V> const V& KeyValueArray<K,V>::Value( uint idx ) const
{
	return m_data[idx].second;
}
template<class K, class V> uint KeyValueArray<K,V>::Size() const
{
	return m_data.size();
}
	
template<class T> struct LuaTableToValue { const static bool Value = false; static void Fetch(lua_State*, int, T&) {eiASSERT(false);} };
template<> struct LuaTableToValue<void> { const static bool Value = false; static void Fetch(...) {eiASSERT(false);} };

//------------------------------------------------------------------------------
namespace internal {
//------------------------------------------------------------------------------
	
struct TypeHierarchy
{
	u32 count;
	u32* hashes;
	template<class T> static const TypeHierarchy* Get();
};
template<class T> struct TypeHierarchyHelper
{
	const static int Count = 1 + TypeHierarchyHelper<typename Reflect<T>::ParentType>::Count;
	static void Init( u32* hashes )
	{
		*hashes = TypeHash<T>::Get();
		typedef Reflect<T>::ParentType Parent;
		TypeHierarchyHelper<Parent>::Init( hashes+1 );
	}

	static const TypeHierarchy* Get()
	{
		static u32 hashes[TypeHierarchyHelper<T>::Count];
		static TypeHierarchy s_result = 
		{
			TypeHierarchyHelper<T>::Count,
			(TypeHierarchyHelper<T>::Init(hashes), hashes)
		};
		return &s_result;
	}
};
template<> struct TypeHierarchyHelper<Nil>
{
	const static int Count = 0;
	static void Init( u32* ) {}
	static const TypeHierarchy* Get() { return 0; }
};
template<class T> static const TypeHierarchy* TypeHierarchy::Get()
{
	typedef Reflect<T>::ParentType Parent;
	return TypeHierarchyHelper<Parent>::Get();
}

int GetUniqueInt();
#if !defined(eiBUILD_NO_LUA_TYPESAFETY)
struct LuaPointer
{
	static const char* const s_metaId;
	void* pointer;
	const char* typeName;
	const TypeHierarchy* hierarchy;
	u32 typeHash;
};
template<class T> struct TLuaPointer : public LuaPointer
{
	static void* MetaName() { return (void*)&_name; }
private:
	static const int _name;
};
template<class T> const int TLuaPointer<T>::_name = GetUniqueInt();
#endif

struct LuaBoxBase
{
	static const char* const s_metaId;
	u32 typeHash;
	const char* typeName;
	const TypeHierarchy* hierarchy;
	const void* Value() const { return this+1; }
};
template<class T, bool boxable=true> struct LuaBox : public LuaBoxBase
{
	T* value;
	~LuaBox(){}

	static void* MetaName() { return (void*)&_name; }
private:
	static const int _name;
};
template<class T, bool b> const int LuaBox<T,b>::_name = GetUniqueInt();
template<bool boxable> struct LuaBox<void,boxable> : public LuaBoxBase
{
	LuaBox() { eiASSERT(false); }
	//Nil value;
	~LuaBox(){}
};
template<class T> struct LuaBox<T,false> : public LuaBoxBase
{
	LuaBox() { eiASSERT(false); }
	//Nil value;
	~LuaBox(){}
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


void PushMetatable(lua_State *L, const void* name, const char* id);
void RegisterMetatable(lua_State *L, const void* name, const char* typeName, FnLua* gc, const char* id, bool pop=true);

void LuaTypeError(lua_State* L, int i, const char* expected, const char* received);

inline bool TryLuaTypeCheck(lua_State* L, int i, u32 typeHash, const char* typeName, const TypeHierarchy* typeHierarchy, u32 expectedHash, const char* expectedName)
{
	if( expectedHash != typeHash )
	{
		if( typeHierarchy )
			for( uint i=0, end=typeHierarchy->count; i!=end; ++i )
				if( typeHierarchy->hashes[i] == expectedHash )
					return true;
		return false;
	}
	else
	{
		eiASSERT( 0==strcmp(typeName,expectedName) );
		return true;
	}
}
template<class TValue> inline bool TryLuaTypeCheck(lua_State* L, int i, u32 typeHash, const char* typeName, const TypeHierarchy* typeHierarchy)
{
	u32 expectedHash = TypeHash<TValue>::Get();
	const char* expectedName = TypeName<TValue>::Get();
	return TryLuaTypeCheck(L, i, typeHash, typeName, typeHierarchy, expectedHash, expectedName);
}
template<class TValue> inline void LuaTypeCheck(lua_State* L, int i, u32 typeHash, const char* typeName, const TypeHierarchy* typeHierarchy)
{
	if( !TryLuaTypeCheck<TValue>(L, i, typeHash, typeName, typeHierarchy) )
		LuaTypeError(L, i, TypeName<TValue>::Get(), typeName);
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
		else if( MetatableHasLudKey(L, i, (void*)LuaBoxBase::s_metaId) )
		{
			LuaBox<T>* box = (LuaBox<T>*)lua_touserdata(L, i);
			LuaTypeCheck<T>( L, i, box->typeHash, box->typeName, box->hierarchy );
			return box->value;
		}
#if !defined(eiBUILD_NO_LUA_TYPESAFETY)
		else if( MetatableHasLudKey(L, i, (void*)LuaPointer::s_metaId) )
		{
			LuaPointer* ptr = (LuaPointer*)lua_touserdata(L, i);
			LuaTypeCheck<T>( L, i, ptr->typeHash, ptr->typeName, ptr->hierarchy );
			return (T*)ptr->pointer;
		}
#endif
		else
			eiLuaArgcheck(L, false, i, "Unknown userdata type");
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
			LuaTypeCheck<T>( L, i, ptr->typeHash, ptr->typeName, ptr->hierarchy );
			return (T*)ptr->pointer;
		}
#endif
		else
			eiLuaArgcheck(L, false, i, "Unknown userdata type");
		return 0;
	}
};
template<class T, bool AllowNil> T* CheckUserDataToPointer(lua_State* L, int i, int ltype)
{
	if( AllowNil && ltype == LUA_TNIL )
		return 0;
	if( (ltype != LUA_TUSERDATA) & (ltype != LUA_TLIGHTUSERDATA) )
		LuaTypeError(L, i, TypeName<T>::Get(), lua_typename(L, lua_type(L, i)));
	T* pointer = UserDataToPointer<T, !std::is_base_of<TagInterface,T>::value && !std::is_same<void,T>::value && !std::is_abstract<T>::value>::Cast(L, i, ltype);
	return pointer;
}

template<bool IsPointer, LuaMapping::Type> struct ValueFromLua;
template<LuaMapping::Type Y> struct ValueFromLua<true, Y>
{
	template<class T> static void Fetch(lua_State* L, int i, int ltype, T& output)
	{
		typedef StripPointer<MsgArg<T>::StorageType>::Type TValue;
		if( LuaTableToValue<TValue>::Value && ltype == LUA_TTABLE )//can convert a table into the desired type
		{
			eiASSERTMSG( false, "TODO - convert table to temporarily allocated TValue, dealloc after function call?");
		}
		output = CheckUserDataToPointer<TValue, !IsReference<T>::value>(L, i, ltype);
		if( IsReference<T>::value )
			eiLuaArgcheck(L, output, i, "NULL reference!");
	}
	static void Fetch(lua_State* L, int i, int ltype, const char*& output)
	{
		if( ltype == LUA_TNIL )
			output = 0;
		else
		{
			eiLuaArgcheck(L, ltype == LUA_TSTRING, i, "Expected string");
			output = lua_tostring(L, i);
		}
	}
};
template<> struct ValueFromLua<false, LuaMapping::Object>
{
	template<class T> static void Fetch(lua_State* L, int i, int ltype, T& output)
	{
		if( LuaTableToValue<T>::Value && ltype == LUA_TTABLE )//can convert a table into the desired type
			return LuaTableToValue<T>::Fetch(L, i, output);
		T* pointer = CheckUserDataToPointer<T, false>(L, i, ltype);
		eiLuaArgcheck(L, pointer, i, "NULL reference!");
		output = *pointer;
	}
	static void Fetch(lua_State* L, int i, int ltype, rde::string& output)
	{
		if( ltype == LUA_TNIL )
			output = "";
		else
		{
			eiLuaArgcheck(L, ltype == LUA_TSTRING, i, "Expected string");
			output = lua_tostring(L, i);
		}
	}
};
template<> struct ValueFromLua<false, LuaMapping::Integer>
{
	template<class T> static void Fetch(lua_State* L, int i, int ltype, T& output)
	{
		if( ltype != LUA_TNUMBER )
			LuaTypeError(L, i, TypeName<T>::Get(), lua_typename(L, lua_type(L, i)));
		output = (T)(HideIntCastWarnings<T>::Type)lua_tointeger(L, i);
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
	static int Push(lua_State* L, const rde::string& value)
	{
		lua_pushstring(L, value.c_str());
		return 1;
	}
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
//	template<class T> static int Push(lua_State* L, const LuaClosure<T>& value)
//	{
//		int numUpvalues = PushValue( L, value.data );
//		lua_pushcclosure(L, value.function, numUpvalues);
//		return 1;
//	}
	template<class T> static int Push(lua_State* L, const T& value)
	{
		size_t size = sizeof(LuaBox<T>) + sizeof(T) + alignof(T);
		LuaBox<T>* data = (LuaBox<T>*)lua_newuserdata(L, size);
		uintptr_t align = (uintptr_t)(data+1);
		align = (align+alignof(T)-1)&~(alignof(T)-1);
		T* allocation = (T*)align;
		eiASSERT( (u8*)(allocation+1) <= ((u8*)data)+size );
		internal::PushMetatable(L, LuaBox<T>::MetaName(), internal::LuaBoxBase::s_metaId);
		lua_setmetatable(L, -2);
		typedef StripPointer<T*>::Type Type;
		data->typeHash = TypeHash<Type>::Get();
		data->typeName = TypeName<Type>::Get();
		data->hierarchy = TypeHierarchy::Get<Type>();
		data->value = allocation;
		//todo - if not POD, add destructor in __gc, and construct properly:
		new(allocation) T(value);//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		return 1;
	}
	template<class T> static int Push(lua_State* L, T* value)
	{
		if( !value )
			lua_pushnil(L);
		else
		{
#if defined(eiBUILD_NO_LUA_TYPESAFETY)
			lua_pushlightuserdata(L, value);
#else
			typedef StripPointer<T*>::Type Type;
			LuaPointer* data = (LuaPointer*)lua_newuserdata(L, sizeof(LuaPointer));
			internal::PushMetatable(L, TLuaPointer<Type>::MetaName(), internal::LuaPointer::s_metaId);
			lua_setmetatable(L, -2);
			data->typeHash = TypeHash<Type>::Get();
			data->typeName = TypeName<Type>::Get();
			data->hierarchy = TypeHierarchy::Get<T>();
			data->pointer = (Type*)value;
#endif
		}
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
template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
int PushValue( lua_State* L, const Tuple<A,B,C,D,E,F,G,H,I,J>& t )
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
	return 1;
}
inline int PushValue( lua_State* L, const LuaFunctionTemporary& t )
{
	lua_pushvalue( L, t.stackLocation );
	return 1;
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
inline int PushValue( lua_State* L, const LuaFunctionReference& t )
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
template<class K, class V>
inline int PushValue( lua_State* L, const KeyValueArray<K,V>& t )
{
	eiDEBUG( int stackSize = lua_gettop(L) );
	uint size = t.Size();
	lua_createtable(L, 0, size);
	for( uint i=0; i!=size; ++i )
	{
		PushValue(L, t.Key(i));
		PushValue(L, t.Value(i));
		lua_rawset(L, -3);
	}
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
	if( ltype == LUA_TNIL )
	{
		output.L = 0;
		output.stackLocation = 0;
	}
	else
	{
		typedef MsgArg<LuaTableTemporary>::StorageType StorageType;
		typedef StripPointer<StorageType>::Type TValue;
		eiLuaArgcheck(L, ltype == LUA_TTABLE, i, "Expected table");
		output.L = L;
		output.stackLocation = luaL_abs_index(L, i);
	}
}
inline void FetchValue(lua_State* L, int i, int ltype, LuaFunctionTemporary& output)
{
	typedef MsgArg<LuaFunctionTemporary>::StorageType StorageType;
	typedef StripPointer<StorageType>::Type TValue;
	if( ltype == LUA_TNIL )
	{
		output.L = 0;
		output.stackLocation = 0;
	}
	else
	{
		eiLuaArgcheck(L, ltype == LUA_TFUNCTION, i, "Expected function");
		output.L = L;
		output.stackLocation = luaL_abs_index(L, i);
	}
}
inline void FetchValue(lua_State* L, int i, int ltype, LuaTableReference& output)
{
	LuaTableTemporary temp = {L};
	FetchValue(L, i, ltype, temp);
	if( temp.L )
		output.Acquire(L, temp);
}
inline void FetchValue(lua_State* L, int i, int ltype, LuaFunctionReference& output)
{
	LuaFunctionTemporary temp = {L};
	FetchValue(L, i, ltype, temp);
	if( temp.L )
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

template<bool isref> struct ValidateArgHelper
{
	template<class T> static void Validate( lua_State* L, int i, const MsgArg<T>& a )
	{
		if( !a.value )
			luaL_error(L, "Argument %d is a NULL reference (%s)", i, TypeName<T>::Get() );
	}
};
template<> struct ValidateArgHelper<false>
{
	template<class T> static void Validate( lua_State*, int, const MsgArg<T>& ) {}
};
template<class T> void ValidateArg( lua_State* L, int i, const MsgArg<T>& a )
{
	ValidateArgHelper< IsReference<T>::value >::Validate(L, i, a);
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
		{
			FetchValue(L, i, ltype, arg.value);
			ValidateArg(L, i, arg);
		}
		mask <<= 1;
		++i;
	}
};
template<bool IgnoreFirst=false>
struct FetchArgsCall
{
	FetchArgsCall(lua_State* L, uint startArg) : L(L), i(startArg), ignoredFirst() {} lua_State* L; uint i; bool ignoredFirst;
	template<class T> void operator()(T& arg)
	{
		if( IgnoreFirst && !ignoredFirst )
		{
			ignoredFirst = true;
			return;
		}
		int ltype = lua_type(L, i);
		FetchValue(L, i, ltype, arg.value);
		ValidateArg(L, i, arg);
		++i;
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

inline bool PushFunction(lua_State* L, const LuaTableMember& name)
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
inline bool PushFunction(lua_State* L, const LuaTempTableMember& name)
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

inline bool PushFunction(lua_State* L, const LuaFunctionTemporary& name)
{
	return internal::PushValue( L, name ) == 1;
}
inline bool PushFunction(lua_State* L, const LuaFunctionReference& name)
{
	return internal::PushValue( L, name ) == 1;
}

inline void OnError( LuaState& L, LUA_JMP_TYPE _err_jmp_ )
{
	if( L.onError.pfnError )
	{
		switch( L.onError.pfnError( L, L.onError.userdata ) )
		{
		case LuaErrorHandler::Retry:
			eiLUA_LONGJMP(_err_jmp_);
			break;
		case LuaErrorHandler::Continue:
			break;
		default: eiASSUME(false);
		}
	}
}
inline void OnError( lua_State* L, LUA_JMP_TYPE _err_jmp_ )
{
	//TODO - get a LuaState from some hidden variable inside L
//	if( L.onError.pfnError )
//	{
//		switch( L.onError.pfnError( L, L.onError.userdata ) )
//		{
//		case LuaErrorHandler::Retry:
//			eiLUA_LONGJMP(_err_jmp_);
//			break;
//		case LuaErrorHandler::Continue:
//			break;
//		default: eiASSUME(false);
//		}
//	}
}

template<class Return, class LuaState> typename ReturnType<Return>::Type PCall(LuaState& L, int numArgs, LUA_JMP_TYPE _err_jmp_)
{
	//todo - support lua's multi-return via a tuple, etc?
	const int hasReturn = IsSameType<Return,void>::value ? 0 : 1;
	int returnCode = lua_pcall(L, numArgs, hasReturn, 0);
	if( returnCode )
	{
		const char* errorText = lua_tostring(L, -1);
		printf( "Lua error: %s\n", errorText );//todo - logging
		if(IsDebuggerAttached())
			eiDEBUG_BREAK;
		lua_pop(L, 1);
		OnError(L, _err_jmp_);
	}
	else if( hasReturn )
	{
		ReturnType<Return> r;
		FetchValue(L, -1, lua_type(L, -1), r.value);
		lua_pop(L, 1);
		return std::move(r.value);
	}
	return ReturnType<Return>::Type();
}

//------------------------------------------------------------------------------
}//end namespace internal
//------------------------------------------------------------------------------

template<class T> void LuaTableMember::Read(lua_State* L, T& output)
{
	eiCheckLuaStack(L);
	internal::PushValue( L, table );// push table
	lua_getfield(L, -1, field);//push table[field]
	internal::FetchValue(L, -1, lua_type(L, -1), output);
	lua_pop(L, 2);//pop table, table[field]
}
template<class T> void LuaTableMember::Write(lua_State* L, const T& input)
{
	eiCheckLuaStack(L);
	internal::PushValue( L, table );// push table
	internal::PushValue(L, input);
	lua_setfield(L, -2, field);//table[field] = value, pop value
	lua_pop(L, 1);//pop table
}
template<class T> void LuaTempTableMember::Read(T& output)
{
	lua_State* L = table.L;
	eiCheckLuaStack(L);
	lua_getfield(L, table.stackLocation, field);//push table[field]
	internal::FetchValue(L, -1, lua_type(L, -1), output);//output = table[field]
	lua_pop(L, 1);//pop table[field]
}
template<class T> void LuaTempTableMember::Write(const T& input)
{
	lua_State* L = table.L;
	eiCheckLuaStack(L);
	internal::PushValue(L, input);
	int tableIndex = table.stackLocation;
	if( tableIndex < 0 )
		--tableIndex;
	lua_setfield(L, tableIndex, field);//pops value
}

inline LuaTableReference::LuaTableReference( lua_State* L, const LuaGlobal& name )
	: m_registryKey() 
{
	eiCheckLuaStack(L);
	lua_getglobal(L, name.global);
	if(lua_istable(L, -1))
	{
		LuaTableTemporary temp = {L};
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
inline void LuaFunctionReference::Consume( LuaFunctionReference& other )
{
	eiASSERT( m_registryKey == 0 );
	m_registryKey = other.m_registryKey;
	other.m_registryKey = 0;
}
inline void LuaTableReference::Acquire( lua_State* L, const LuaTableTemporary& input )
{
	eiCheckLuaStack(L);
	eiASSERT( input.L );
	eiASSERT( m_registryKey == 0 );
	extern int GetNewLuaTableKey();
	m_registryKey = GetNewLuaTableKey();
	lua_pushlightuserdata(L, (void*)internal::s_cTableHandles);//Push s_cTableHandles
	lua_rawget(L, LUA_REGISTRYINDEX);  // push registry[s_cTableHandles], pop s_cTableHandles
	lua_pushvalue(L, input.stackLocation);//push table to top of stack
	lua_rawseti(L, -2, m_registryKey);//registry[s_cTableHandles][registryKey] = table, pop table/key
	lua_pop(L, 1);//pop registry[s_cTableHandles]
}
inline void LuaFunctionReference::Acquire( lua_State* L, const LuaFunctionTemporary& input )
{
	eiCheckLuaStack(L);
	eiASSERT( input.L );
	eiASSERT( m_registryKey == 0 );
	extern int GetNewLuaTableKey();
	m_registryKey = GetNewLuaTableKey();
	lua_pushlightuserdata(L, (void*)internal::s_cTableHandles);//Push s_cTableHandles
	lua_rawget(L, LUA_REGISTRYINDEX);  // push registry[s_cTableHandles], pop s_cTableHandles
	lua_pushvalue(L, input.stackLocation);//push table to top of stack
	lua_rawseti(L, -2, m_registryKey);//registry[s_cTableHandles][registryKey] = table, pop table/key
	lua_pop(L, 1);//pop registry[s_cTableHandles]
}
inline void LuaFunctionReference::Acquire( lua_State* L, const LuaFunctionReference& input )
{
	eiCheckLuaStack(L);
	eiASSERT( m_registryKey == 0 );
	extern int GetNewLuaTableKey();
	m_registryKey = GetNewLuaTableKey();
	lua_pushlightuserdata(L, (void*)internal::s_cTableHandles);//Push s_cTableHandles
	lua_rawget(L, LUA_REGISTRYINDEX);  // push registry[s_cTableHandles], pop s_cTableHandles
	lua_pushvalue(L, m_registryKey);//push input.m_registryKey to top of stack
	lua_rawget(L, LUA_REGISTRYINDEX);  // push registry[s_cTableHandles][input.m_registryKey], pop input.m_registryKey
	lua_rawseti(L, -2, m_registryKey);//registry[s_cTableHandles][registryKey] = registry[s_cTableHandles][input.m_registryKey], pop table/key
	lua_pop(L, 1);//pop registry[s_cTableHandles]
}
inline void LuaTableReference::Release( lua_State* L )
{
	eiCheckLuaStack(L);
	eiASSERT( m_registryKey != 0 );
	lua_pushlightuserdata(L, (void*)internal::s_cTableHandles);//Push s_cTableHandles
	lua_rawget(L, LUA_REGISTRYINDEX);  // push registry[s_cTableHandles], pop s_cTableHandles
	lua_pushnil(L);
	lua_rawseti(L, -2, m_registryKey);//registry[s_cTableHandles][registryKey] = nil, pop table/key
	lua_pop(L, 1);//pop registry[s_cTableHandles]
	m_registryKey = 0;
}
inline void LuaFunctionReference::Release( lua_State* L )
{
	eiCheckLuaStack(L);
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
	eiLuaArgcheck(L, user, 1, "NULL reference!");

	eiASSERT(q);
	FutureCall* msg = q->Push( &CallWrapper<T,fn,F>, user, sizeof(Args), Return::Size() );
	Args& args = *(Args*)msg->args.Ptr();
	eiSTATIC_ASSERT( MaxArgs == 10 );
	typedef CallArgMasks<ArgTuple::A, ArgTuple::B, ArgTuple::C, ArgTuple::D, ArgTuple::E, ArgTuple::F, ArgTuple::G, ArgTuple::H, ArgTuple::I, ArgTuple::J> Masks;
	ArgHeader header = { {0, Masks::isPointer, Masks::isConst, &Args::Info} };
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
	eiLuaArgcheck(L, user, 1, "NULL reference!");
	ArgTuple a;
	Return r;
	a.ForEach( internal::FetchArgsCall<false>(L, 2) );
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
	a.ForEach( internal::FetchArgsCall<false>(L, 1) );
	Call( r, a, (F)fn );
	return internal::PushValue(L, r.value);
}
template<callback fn, class F>
int LuaGcNewWrapper(lua_State* L)
{
	typedef ArgFromSig<F>::Storage::Type ArgTuple;
	typedef ArgFromSig<F>::Return RawReturn;
	typedef ReturnType<RawReturn> Return;
	int numArgs = lua_gettop(L);
	if( ArgTuple::length-1 != numArgs )
		luaL_error(L, "Expected %d args, received %d", ArgTuple::length, numArgs );
	ArgTuple a;
	Return r;
	a.ForEach( internal::FetchArgsCall<true>(L, 1) );
	
	using namespace internal;
	typedef StripPointer<RawReturn>::Type T;
		size_t size = sizeof(LuaBox<T>) + sizeof(T) + alignof(T);
		LuaBox<T>* data = (LuaBox<T>*)lua_newuserdata(L, size);
		uintptr_t align = (uintptr_t)(data+1);
		align = (align+alignof(T)-1)&~(alignof(T)-1);
		T* allocation = (T*)align;
		eiASSERT( (u8*)(allocation+1) <= ((u8*)data)+size );
		internal::PushMetatable(L, (void*)LuaBox<T>::MetaName(), internal::LuaBoxBase::s_metaId);
		lua_setmetatable(L, -2);
		typedef StripPointer<T*>::Type Type;
		data->typeHash = TypeHash<Type>::Get();
		data->typeName = TypeName<Type>::Get();
		data->hierarchy = TypeHierarchy::Get<Type>();
		data->value = allocation;

	a.a = allocation;
	Call( r, a, (F)fn );
	
	return 1;
}

template<class Return, class LuaState, class T>
typename ReturnType<Return>::Type PCall(LuaState& L, const T& name)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	eiLUA_SETJMP;
	if(internal::PushFunction(L, name))
	{
		return internal::PCall<Return>(L, 0, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A>
typename ReturnType<Return>::Type PCall(LuaState& L, const T& name, const A& a)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	eiLUA_SETJMP;
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		return internal::PCall<Return>(L, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B>
typename ReturnType<Return>::Type PCall(LuaState& L, const T& name, const A& a, const B& b)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	eiLUA_SETJMP;
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		return internal::PCall<Return>(L, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B, class C>
typename ReturnType<Return>::Type PCall(LuaState& L, const T& name, const A& a, const B& b, const C& c)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	eiLUA_SETJMP;
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		args +=    internal::PushValue(L, c);
		return internal::PCall<Return>(L, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B, class C, class D>
typename ReturnType<Return>::Type PCall(LuaState& L, const T& name, const A& a, const B& b, const C& c, const D& d)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	eiLUA_SETJMP;
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		args +=    internal::PushValue(L, c);
		args +=    internal::PushValue(L, d);
		return internal::PCall<Return>(L, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B, class C, class D, class E>
typename ReturnType<Return>::Type PCall(LuaState& L, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	eiLUA_SETJMP;
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		args +=    internal::PushValue(L, c);
		args +=    internal::PushValue(L, d);
		args +=    internal::PushValue(L, e);
		return internal::PCall<Return>(L, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B, class C, class D, class E, class F>
typename ReturnType<Return>::Type PCall(LuaState& L, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	eiLUA_SETJMP;
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		args +=    internal::PushValue(L, c);
		args +=    internal::PushValue(L, d);
		args +=    internal::PushValue(L, e);
		args +=    internal::PushValue(L, f);
		return internal::PCall<Return>(L, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B, class C, class D, class E, class F, class G>
typename ReturnType<Return>::Type PCall(LuaState& L, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	eiLUA_SETJMP;
	if(internal::PushFunction(L, name))
	{
		int args = internal::PushValue(L, a);
		args +=    internal::PushValue(L, b);
		args +=    internal::PushValue(L, c);
		args +=    internal::PushValue(L, d);
		args +=    internal::PushValue(L, e);
		args +=    internal::PushValue(L, f);
		args +=    internal::PushValue(L, g);
		return internal::PCall<Return>(L, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B, class C, class D, class E, class F, class G, class H>
typename ReturnType<Return>::Type PCall(LuaState& L, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	eiLUA_SETJMP;
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
		return internal::PCall<Return>(L, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I>
typename ReturnType<Return>::Type PCall(LuaState& L, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	eiLUA_SETJMP;
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
		return internal::PCall<Return>(L, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
typename ReturnType<Return>::Type PCall(LuaState& L, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i, const J& j)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	eiLUA_SETJMP;
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
		return internal::PCall<Return>(L, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K>
typename ReturnType<Return>::Type PCall(LuaState& L, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i, const J& j, const K& k)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(L);
	eiLUA_SETJMP;
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
		return internal::PCall<Return>(L, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L>
typename ReturnType<Return>::Type PCall(LuaState& _, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i, const J& j, const K& k, const L& l)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(_);
	eiLUA_SETJMP;
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
		return internal::PCall<Return>(_, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
typename ReturnType<Return>::Type PCall(LuaState& _, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i, const J& j, const K& k, const L& l, const M& m)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(_);
	eiLUA_SETJMP;
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
		return internal::PCall<Return>(_, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N>
typename ReturnType<Return>::Type PCall(LuaState& _, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i, const J& j, const K& k, const L& l, const M& m, const N& n)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(_);
	eiLUA_SETJMP;
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
		return internal::PCall<Return>(_, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
typename ReturnType<Return>::Type PCall(LuaState& _, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i, const J& j, const K& k, const L& l, const M& m, const N& n, const O& o)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(_);
	eiLUA_SETJMP;
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
		return internal::PCall<Return>(_, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
template<class Return, class LuaState, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P>
typename ReturnType<Return>::Type PCall(LuaState& _, const T& name, const A& a, const B& b, const C& c, const D& d, const E& e, const F& f, const G& g,
										const H& h, const I& i, const J& j, const K& k, const L& l, const M& m, const N& n, const O& o, const P& p)
{
	eiProfile("Lua PCall");
	eiCheckLuaStack(_);
	eiLUA_SETJMP;
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
		return internal::PCall<Return>(_, args, _err_jmp_);
	}
	return ReturnType<Return>::Type();
}
//template<class Return, class LuaState, class T, typename... A>
//typename ReturnType<Return>::Type PCall(lua_State* L, const char* global, const char* field, A... a)
//{
//	return PCall(L, LuaGlobalField(global, field), a...);
//}




template<class T> struct LuaTableToValue< ArrayData<T> > { const static bool Value = true; 
	static void Fetch(lua_State* L, int i, ArrayData<T>& output)
	{
		size_t len = lua_objlen(L, i);
		output.ClearToSize((uint)len);
		if( i < 0 )
			--i;
		for( size_t idx = 0; idx != len; ++idx )
		{
			lua_pushnumber(L, (lua_Number)(idx+1));//lua uses 1-based arrays
			lua_gettable(L, i);//push table[idx+1] onto the top of the stack
			internal::FetchValue(L, -1, lua_type(L, -1), output.data[idx]);
			lua_pop(L, 1);
		}
	}
};

template<class T> void RegisterTypeMetatable(lua_State* L, const char* typeName, FnLua* gc)
{
#ifndef eiBUILD_NO_LUA_TYPESAFETY
	internal::RegisterMetatable(L, internal::TLuaPointer<T>::MetaName(), typeName, 0, internal::LuaPointer::s_metaId);
#endif
	internal::RegisterMetatable(L, internal::LuaBox<T>::MetaName(), typeName, gc, internal::LuaBoxBase::s_metaId);
}

namespace internal
{
	template<class T> void Destruct( Interface<T>* object )
	{
		eiSTATIC_ASSERT((std::is_base_of<TagInterface,T>::value));
		Interface<T>::Instance_Destruct(object);
	}
	template<class T> void Destruct( T* object )
	{
		eiSTATIC_ASSERT(!(std::is_base_of<TagInterface,T>::value));
		object->~T();
	}
}
template<class T> int LuaDestruct( lua_State* L )
{
	T* object = 0;
	internal::FetchValue(L, 1, lua_type(L, 1), object);
	internal::Destruct(InterfacePointer(object));
	return 0;
}

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
