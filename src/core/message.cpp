#include <eight/core/message.h>
#include <eight/core/debug.h>
#include <eight/core/bit/twiddling.h>
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
/*

void testy()
{
	FutureArg<int> one( 1 );
	const FutureArg<int>& two = FutureArg<int>( FutureIndex() );
	ArgStore<void, int, int, float> a = {0,{one,two,3.0f}};
	ArgStore<void, int, int> b = {0,{1,2}};
	ArgStore<void, int> c = {0,{1}};
	ArgStore<void> d = {};
}*/

void CallBuffer::Insert(FutureCall* data)
{
	eiASSERT( (!m_first) == (!m_last) );
	eiASSERT( !data->next );
	if( !m_first )
	{
		m_first = m_last = data;
	}
	else
	{
		eiASSERT( !m_last->next );
		m_last->next = data;
		m_last = data;
	}
}

FutureCall* CallBuffer::Push( FnTask* call, void* obj, uint argSize, uint returnSize )
{
	FutureCall* data = a.Alloc<FutureCall>();
	data->call = call;
	data->obj = obj;
	data->args = argSize ? a.Alloc(argSize) : 0;
	data->argSize = argSize;
	data->next = 0;
	if( !returnSize )
		data->returnOffset = 0xFFFFFFFF;
	else
	{
		m_returnSize.Align( 16 );//todo
		uint offset = m_returnSize.Alloc( returnSize );
		eiASSERT( offset < 0xFFFFFFFF );
		data->returnOffset = offset;
		data->returnIndex = m_nextReturnIndex++;
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
		int DoStuff() { return 42; }
		void FooBar(int i, const float*) { eiASSERT(i==42||i==41); }
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

	{
		ArgStore<int> args = { {0, &ArgStore<int>::Info}, {} };
		Push_Test_DoStuff( q, &t, &args, sizeof(args) );
	}
	{
		ArgStore<void, int, const float*> args = { {0, &ArgStore<void, int, const float*>::Info}, {42, 0} };
		Push_Test_FooBar( q, &t, &args, sizeof(ArgStore<void, int, const float*>) );

		CallBlob call = { &args, 0 };
		Call_Test_FooBar( &t, &call, sizeof(CallBlob) );
	}

//	LuaPush_Test_FooBar( q, &t, 0 );

	FutureIndex stuff = eiPush( q, t, Test::DoStuff );
	eiPush( q, t, Test::FooBar, 41, (float*)0 );
	eiPush( q, t, Test::FooBar, stuff, (float*)0 );

	const uint returnSize = q.ReturnValuesSize();
	const uint returnCount = q.ReturnValuesCount();
	uint* const returnOffsets = s.Alloc<uint>( returnCount );
	u8* const returnData = s.Alloc( returnSize );
	for( FutureCall* i = q.First(), *last = q.Last(); i; i != last ? i = i->next.Ptr() : i = 0 )
	{
		CallBlob call = { 0, 0 };
		bool hasArgs = !!i->args;
		if( hasArgs )
			call.arg = &*i->args;
		if( hasArgs && i->args->futureMask )
		{
			ArgHeader& args = *i->args;
			call.arg = &args;
			u32 futureMask = args.futureMask;
			args.futureMask = 0;
			u8* argTuple = (u8*)(&args + 1);
			uint numArgs = 0;
			uint offsets[MaxArgs];
			uint sizes[MaxArgs];
			eiASSERT( args.info && (void*)args.info != (void*)0xcdcdcdcd );
			args.info(numArgs, offsets, sizes);
			do// for each bit in futureMask
			{
				uint idx = LeastSignificantBit(futureMask);//find index of lsb set
				futureMask &= futureMask - 1; // clear the least significant bit set
				eiASSERT( idx < numArgs );//future bits should match up with arg indices
				uint offset = offsets[idx];//look up where this item is inside the tuple
				uint size   = sizes[idx];//and how many bytes to copy it
				FutureIndex* future = (FutureIndex*)&argTuple[offset];//if the bit was set, assume the item isn't an arg, but a future index
				uint futureIndex = future->index;
				uint futureOffset = future->offset;
				eiASSERT( futureIndex < returnCount );
				eiASSERT( returnOffsets[futureIndex] != 0xFFFFFFFF );
				u8* returnValue = returnData + returnOffsets[futureIndex];
				eiASSERT( offset < numArgs );
				memcpy( &argTuple[offset], returnValue, size );//copy the return value into the call's arg tuple
			}while( futureMask );
		}
		eiASSERT( i->call );
		eiASSERT( i->returnOffset < returnSize || i->returnOffset == 0xFFFFFFFF );
		eiASSERT( i->returnIndex < returnCount || i->returnOffset == 0xFFFFFFFF );
		uint returnOffset = i->returnOffset;
		if( returnOffset != 0xFFFFFFFF )
		{
			returnOffsets[i->returnIndex] = returnOffset;
			call.out = returnData + returnOffset;
		}
		i->call( i->obj, &call, sizeof(call) );
	}

}
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
