#include "eight/core/alloc/scope.h"
#include "eight/core/alloc/new.h"
#include "eight/lua/lua.h"
#include "eight/lua/lua_api.h"
#include "tilde/LuaHostWindows.h"
#include "tilde/LuaDebuggerComms.h"
#include "stdio.h"
#include "string.h"
using namespace eight;
using namespace lua;
using namespace tilde; 

Tilde* Tilde::New(Scope& a, int port)
{
	return (Tilde*)eiNew(a,LuaHostWindows)(port);
}

void Tilde::Register(lua_State* L)
{
	LuaHostWindows* self = (LuaHostWindows*)this;
	self->RegisterState("state", L);
}
void Tilde::WaitForDebuggerConnection()
{
	LuaHostWindows* self = (LuaHostWindows*)this;
	self->WaitForDebuggerConnection();
}
void Tilde::Poll()
{
	LuaHostWindows* self = (LuaHostWindows*)this;
	self->Poll();
}
//------------------------------------------------------------------------------