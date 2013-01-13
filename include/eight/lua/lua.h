//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/noncopyable.h>

struct lua_State;

namespace eight {
	struct TypeBinding;
	class Scope;
namespace lua {
//------------------------------------------------------------------------------

void RegisterEngine(lua_State* L);
void RegisterType(lua_State* L, const TypeBinding& type);

struct LoadedPackage
{
	const char* filename;
	const char* bytes;
	uint numBytes;
};
struct PackageLoader
{
	void* userdata;
	LoadedPackage (*pfn)(void* userdata, const char* name);
};
void SetPackageLoader(lua_State* L, int(*callback)(lua_State*), PackageLoader*);
int DefaultPackageLoaderCallback(lua_State* L);

struct LuaState : NonCopyable
{
	LuaState(PackageLoader* loader=0);
	~LuaState();
	operator lua_State*() { return L; }
	lua_State* L;
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
}} // namespace eight, lua
//------------------------------------------------------------------------------
