//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/debug.h>
#include <eight/core/noncopyable.h>

struct lua_State;

namespace eight {
	struct TypeBinding;
	class Scope;
//------------------------------------------------------------------------------

void RegisterEngine(lua_State* L);
void RegisterType(lua_State* L, const TypeBinding& type);
template<class T> void RegisterType(lua_State* L) { RegisterType(L, Reflect<T>::Binding()); }


struct LuaTableTemporary;
class LuaTableReference;
namespace internal {
	int PushValue( lua_State* L, const LuaTableTemporary& t );
	int PushValue( lua_State* L, const LuaTableReference& t );
	void FetchValue(lua_State* L, int i, int ltype, LuaTableTemporary& output);
	void FetchValue(lua_State* L, int i, int ltype, LuaTableReference& output);
}

template<class T> struct LuaClosure
{
//	FnLua* function;
	int (*function)( lua_State* );
	T data;//use T = Tuple<A,B...> for binding multiple values
};
struct LuaTableTemporary // can accept these as function arguments, but do not prolong their lifetime!!
{
private:
	friend class LuaTableReference;
	friend int internal::PushValue( lua_State* L, const LuaTableTemporary& t );
	friend void internal::FetchValue(lua_State* L, int i, int ltype, LuaTableTemporary& output);
	friend void internal::FetchValue(lua_State* L, int i, int ltype, LuaTableReference& output);
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
struct LuaTableMethod
{
	LuaTableMethod(const LuaTableReference& table, const char* field)
		: table(table), field(field) {}
	const LuaTableReference& table;
	const char* field;
};
struct LuaTempTableMethod
{
	LuaTempTableMethod(LuaTableTemporary table, const char* field)
		: table(table), field(field) {}
	LuaTableTemporary table;
	const char* field;
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
};
void SetPackageLoader(lua_State* L, int(*callback)(lua_State*), PackageLoader*);
int DefaultPackageLoaderCallback(lua_State* L);

class LuaState : NonCopyable
{
public:
	LuaState(PackageLoader* loader=0);
	~LuaState();
	operator lua_State*() { return L; }
	lua_State* const L;

	bool DoCode( const char* code );
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
