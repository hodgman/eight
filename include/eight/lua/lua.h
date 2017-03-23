//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/debug.h>
#include <eight/core/noncopyable.h>

struct lua_State;

namespace eight {
	struct TypeBinding;
	class Scope;
	class LuaState;
//------------------------------------------------------------------------------

void RegisterEngine(lua_State* L);
void RegisterType(lua_State* L, const TypeBinding& type);
template<class T> void RegisterType(lua_State* L) { RegisterType(L, Reflect<T>::Binding()); }//requires bind.h


struct LuaTableTemporary;
class  LuaTableReference;
struct LuaFunctionTemporary;
class  LuaFunctionReference;
namespace internal {
	int PushValue( lua_State* L, const LuaTableTemporary& t );
	int PushValue( lua_State* L, const LuaTableReference& t );
	int PushValue( lua_State* L, const LuaFunctionTemporary& t );
	int PushValue( lua_State* L, const LuaFunctionReference& t );
	void FetchValue(lua_State* L, int i, int ltype, LuaTableTemporary& output);
	void FetchValue(lua_State* L, int i, int ltype, LuaTableReference& output);
	void FetchValue(lua_State* L, int i, int ltype, LuaFunctionTemporary& output);
	void FetchValue(lua_State* L, int i, int ltype, LuaFunctionReference& output);
}

//template<class T> struct LuaClosure
//{
////	FnLua* function;
//	int (*function)( lua_State* );
//	T data;//use T = Tuple<A,B...> for binding multiple values
//};
struct LuaTableTemporary // can accept these as function arguments, but do not prolong their lifetime!!
{
	LuaTableTemporary(lua_State* L=0) : L(L) {}
	lua_State* L;
private:
	friend class LuaTableReference;
	friend struct LuaTempTableMember;
	friend int internal::PushValue( lua_State* L, const LuaTableTemporary& t );
	friend void internal::FetchValue(lua_State* L, int i, int ltype, LuaTableTemporary& output);
	int stackLocation;
};
struct LuaFunctionTemporary // can accept these as function arguments, but do not prolong their lifetime!!
{
	LuaFunctionTemporary(lua_State* L=0) : L(L) {}
	lua_State* L;

	operator bool() const { return !!L; }
	void operator()() const { eiASSERT(false && "todo"); }
private:
	friend class LuaFunctionReference;
	friend int internal::PushValue( lua_State* L, const LuaFunctionTemporary& t );
	friend void internal::FetchValue(lua_State* L, int i, int ltype, LuaFunctionTemporary& output);
	int stackLocation;
};
struct LuaGlobal
{
	LuaGlobal(const char* global)
		: global(global) {}
	const char* global;
};
struct LuaGlobalField
{
	LuaGlobalField(const char* global, const char* field)
		: global(global), field(field) {}
	const char* global;
	const char* field;
};
class LuaTableReference : NonCopyable // can convert a LuaTableTemporary to this to prolong it's lifetime.
{
public:
	~LuaTableReference() { eiASSERTMSG( m_registryKey == 0, "Always Release LuaTableReference's before they go out of scope" ); }
	LuaTableReference() : m_registryKey() {}
	LuaTableReference( LuaTableReference& o ): m_registryKey() { Consume(o); }
	LuaTableReference& operator=( LuaTableReference& o ) { Consume(o); return *this; }
	LuaTableReference( lua_State* L, const LuaGlobal& name );
	void Consume( LuaTableReference& other );
	void Acquire( lua_State* L, const LuaTableTemporary& input );
	void Release( lua_State* L );
	operator bool() const { return m_registryKey != 0; }
private:
	friend int internal::PushValue( lua_State* L, const LuaTableReference& t );
	int m_registryKey;
};
class LuaFunctionReference : NonCopyable // can convert a LuaFunctionTemporary to this to prolong it's lifetime.
{
public:
	~LuaFunctionReference() { eiASSERTMSG( m_registryKey == 0, "Always Release LuaFunctionReference's before they go out of scope" ); }
	LuaFunctionReference() : m_registryKey() {}
	LuaFunctionReference( LuaFunctionReference& o ): m_registryKey() { Consume(o); }
	LuaFunctionReference& operator=( LuaFunctionReference& o ) { Consume(o); return *this; }
	LuaFunctionReference( lua_State* L, const LuaGlobal& name );
	void Consume( LuaFunctionReference& other );
	void Acquire( lua_State* L, const LuaFunctionReference& input );
	void Acquire( lua_State* L, const LuaFunctionTemporary& input );
	void Release( lua_State* L );
	operator bool() const { return m_registryKey != 0; }
private:
	friend int internal::PushValue( lua_State* L, const LuaFunctionReference& t );
	int m_registryKey;
};
struct RaiiLuaFunctionReference
{
	LuaFunctionReference function;
	lua_State* L;
	RaiiLuaFunctionReference() : L() { }
	RaiiLuaFunctionReference(const RaiiLuaFunctionReference& f) : L() { *this = f; }
	RaiiLuaFunctionReference(const LuaFunctionTemporary& f) : L() { *this = f; }
	RaiiLuaFunctionReference& operator=(const LuaFunctionTemporary& f)
	{
		if( L )
			function.Release(L);
		function.Acquire(f.L, f);
		L = f.L;
		return *this;
	}
	RaiiLuaFunctionReference& operator=(const RaiiLuaFunctionReference& f)
	{
		if( L )
			function.Release(L);
		function.Acquire(f.L, f.function);
		L = f.L;
		return *this;
	}
	~RaiiLuaFunctionReference()
	{
		if( L )
			function.Release(L);
	}
	template<class T> T PCall()
	{
		return (T)::PCall<T>(L, function);
	}
	template<class T, class A> T PCall(const A& a)
	{
		return (T)::PCall<T>(L, function, a);
	}
	template<class T, class A, class B> T PCall(const A& a, const B& b)
	{
		return (T)::PCall<T>(L, function, a, b);
	}
};
struct LuaTableMember
{
	LuaTableMember(const LuaTableReference& table, const char* field)
		: table(table), field(field) {}
	const LuaTableReference& table;
	const char* field;
	
	template<class T> void Read(lua_State* L, T&);
	template<class T> void Write(lua_State* L, const T&);
};
struct LuaTempTableMember
{
	LuaTempTableMember(LuaTableTemporary table, const char* field)
		: table(table), field(field) {}
	LuaTableTemporary table;
	const char* field;
	
	template<class T> void Read(T&);
	template<class T> void Write(const T&);
};

struct LuaErrorHandler
{
	void* userdata;
	enum Action
	{
		Retry,
		Continue,
	};
	Action (*pfnError)(LuaState&, void* userdata);
};
struct LoadedPackage
{
	const char* filename;
	const char* bytes;
	uint numBytes;
};
struct PackageLoader
{
	void* userdata;
	LoadedPackage (*pfnLoad)(void* userdata, const char* name);
	void          (*pfnFree)(void* userdata, const LoadedPackage&);

	LuaState* owner;
};
void SetPackageLoader(lua_State* L, int(*callback)(lua_State*), PackageLoader*);
int DefaultPackageLoaderCallback(lua_State* L);

class LuaState : NonCopyable
{
public:
	LuaState(PackageLoader* loader=0, LuaErrorHandler* error=0);
	~LuaState();
	operator lua_State*() { return L; }
	lua_State* const L;

	LuaErrorHandler onError;
	
	bool Require(const char* file);
	void UnloadAll();
	bool DoCode( const char* code, const char* bufferName );
};

class Tilde : NonCopyable
{
public:
	static Tilde* New(Scope&, int port = 10000);
	void Register(lua_State*);
	void WaitForDebuggerConnection();
	void Poll();
};


//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
