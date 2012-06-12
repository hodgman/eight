#include <eight/core/message.h>
#include <eight/core/debug.h>
#include <eight/core/test.h>
#include <ctime>
#include <vector>

using namespace eight;


/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/*
template<class T, class N> struct _        { T data; N next; };
template<class T>          struct _<T,Nil> { T data; };

template<class T> struct ArgList { T data; };
template<>        struct ArgList<Nil> {};
*/


void testy()
{
	FutureArg<int> one( 1 );
	const FutureArg<int>& two = FutureArg<int>( FutureIndex() );
	ArgStore<void, int, int, float> a = {{one,two,3.0f}};
	ArgStore<void, int, int> b = {{1,2}};
	ArgStore<void, int> c = {{1}};
	ArgStore<void> d = {};
}

FutureCall* CallBuffer::Push( FnTask* task, void* obj, uint argSize, uint returnSize )
{
	FutureCall* data = a.Alloc<FutureCall>();
	data->task = task;
	data->obj = obj;
	data->args = argSize ? a.Alloc(argSize) : 0;
	data->argSize = argSize;
	data->next = 0;
	data->returnSize = returnSize;
	if( !returnSize )
		data->returnIndex = 0;
	else
	{
		data->returnIndex = m_nextReturnIndex++;
		m_returnSize.Align( 16 );//todo
		m_returnSize.Alloc( returnSize );
	}
	Insert(data);
	return data;
};


/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/

namespace { 
	class Test
	{
	public:
		int DoStuff() { return 0; }
		void FooBar(int*, const float*) { }
	};
}

eiMessage( Test, DoStuff );
eiMessage( Test, FooBar );

template<class T, class F, class A>
void Call( F& f, T* t, A* a )
{
//	(t.*f)();
}

eiTEST( Message )
{
	Test t;
	char buf[1024];
	StackAlloc s(buf,1024);
	CallBuffer q(s);
	Push_Test_DoStuff( q, &t, 0, 0 );
	ArgStore<void, int*, const float*> args;
	Push_Test_FooBar( q, &t, &args, sizeof(ArgStore<void, int*, const float*>) );
	Call_Test_FooBar(    &t, &args, sizeof(ArgStore<void, int*, const float*>) );
	LuaPush_Test_FooBar( q, &t, 0 );

	eiPush( q, t, Test::DoStuff );
	eiPush( q, t, Test::FooBar, (int*)0, (float*)0 );
}
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
