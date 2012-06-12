#include <eight/core/bind.h>
#include <eight/core/message.h>
#include <eight/core/debug.h>
#include <eight/core/test.h>
#include <eight/core/thread/timedloop.h>
#include <ctime>
#include <vector>

using namespace eight;

/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/

namespace { 
	class Test1
	{
	public:
		eiBind(Test);
		int DoStuff() { return 0; }
		void FooBar(int*, const float*) { }
	};
	struct Test2 {};
}

eiMessage( Test1, DoStuff );

eiBindClass( Test1 )
	eiBeginMethods()
		eiBindMethod( DoStuff )
	eiEndMethods();
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
	(t.*((void(Test1::*)(void*))b.method[0].memfuncptr))(0);
	(void)(b.dataCount + a.dataCount);
}
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
