//------------------------------------------------------------------------------
#pragma once
#include <eight/core/noncopyable.h>
#include <eight/core/debug.h>
extern "C"
{
#	include <LuaJIT/src/lua.h>
#	include <LuaJIT/src/lualib.h>
#	include <LuaJIT/src/lauxlib.h>
}

namespace eight {
#if eiDEBUG_LEVEL > 1
#	define eiCheckLuaStack(L) eight::LuaStackBalanceCheck_ luaStackCheck##__LINE__(L)
	class LuaStackBalanceCheck_ : NonCopyable
	{
	public:
		LuaStackBalanceCheck_(lua_State* L) : L(L), mark(lua_gettop(L)) {}
		~LuaStackBalanceCheck_() { int newMark = lua_gettop(L); eiASSERT(newMark == mark); }
	private:
		lua_State* L;
		int mark;
	};
#else
#	define eiCheckLuaStack(L) do{}while(0)
#endif
}
//------------------------------------------------------------------------------
