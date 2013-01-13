#include <eight/core/bind.h>
#include <eight/core/message.h>
#include <eight/core/debug.h>
#include <eight/core/test.h>
#include <eight/core/thread/timedloop.h>
#include <eight/lua/bindlua.h>
#include <ctime>
#include <vector>

using namespace eight;

/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/

namespace { 
	class Test1
	{
	public:
		eiBind(Test1);
		int DoStuff(int, Test1*, const char*) { return 0; }
		void FooBar(int*, const float*) { }
	private:
		int m_stuff;
	};
	struct Test2 {};
}

eiBindClass( Test1 )
	eiLuaBind( LuaCall )
	eiBeginMethods()
		eiBindMethod( DoStuff )
	eiEndMethods()
	eiBeginData()
		eiBindData( m_stuff )
	eiEndData()
eiEndBind();

eiBindStruct( Test2 );
eiEndBind();


eiTEST( Bind )
{
	const TypeBinding& a = ReflectTest2();
	const TypeBinding& b = ReflectTest1();

	Test1 t;
	char buf[1024];
	StackAlloc s(buf,1024);


	eiASSERT( 0==strcmp(b.method[0].name,"DoStuff") );
	(t.*((int(Test1::*)(int, Test1*, const char*))b.method[0].memfuncptr))(42, NULL, "foo");
	(void)(b.dataCount + a.dataCount);
}
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
