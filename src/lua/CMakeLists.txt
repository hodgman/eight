
BeginLibrary( lua core luajit )

AddSourceGroup( lua
lua.cpp
lua.h
lua_api.h
bindlua.h
luadebug.cpp
 )
 
AddSourceGroup( Tilde
../../external/tilde/HostConfig.cpp
../../external/tilde/HostConfig.h
../../external/tilde/LuaDebugger.cpp
../../external/tilde/LuaDebugger.h
../../external/tilde/LuaDebuggerComms.cpp
../../external/tilde/LuaDebuggerComms.h
../../external/tilde/LuaDebuggerHost.h
../../external/tilde/LuaDebuggerProtocol.h
../../external/tilde/LuaHostWindows.cpp
../../external/tilde/LuaHostWindows.h
../../external/tilde/ReceiveMessageBuffer.h
../../external/tilde/SendMessageBuffer.h
 )
 
EndLibrary()

AddTest( test_lua lua_test_app.cpp lua )



