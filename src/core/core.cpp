#include <eight/core/types.h>
#include <eight/core/debug.h>
#include <eight/core/test.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/thread/pool.h>
#include <eight/core/thread/fifo_mpmc.h>
#include <eight/core/thread/futex.h>
#include <eight/core/thread/threadlocal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <eight/core/macro.h>
#include <eight/core/timer/timer_impl.h>
#include <eight/core/sort/radix.h>
#include <iostream>
#include <vector>
using namespace eight;
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------



template<bool> struct IndexMeta       { typedef uint32 Type; const static int size = 32; };
template<    > struct IndexMeta<true> { typedef uint64 Type; const static int size = 64; };
template<int bits>
class Index
{
	typedef IndexMeta<(bits>=32)> Meta;
	typedef typename Meta::Type Type;
	struct Data
	{
		const static int ababits = (bits==Meta::size) ? Meta::size : Meta::size-bits;
		Type value :    bits;
		Type ababa : ababits;
	} data;
	eiSTATIC_ASSERT( sizeof(Data) == sizeof(Type) );
};

void TODO_unit_test()
{
	eiSTATIC_ASSERT( sizeof( Index< 1> ) == 4 );
	eiSTATIC_ASSERT( sizeof( Index<15> ) == 4 );
	eiSTATIC_ASSERT( sizeof( Index<31> ) == 4 );
	eiSTATIC_ASSERT( sizeof( Index<32> ) == 8 );
	eiSTATIC_ASSERT( sizeof( Index<33> ) == 8 );
}


bool eight::Assert( const char* e, int l, const char* f, const char* fn )
{
	if( InTest() )
	{
		TestFailure( f, l, fn, "Assertion failure - %s", e );
	}
	else
	{
		//TODO - dialog box
	}
	return IsDebuggerAttached();
}

#if 0
static TimerImpl g_timer;
struct PrintMsg { const char* msg; double time; };
static s32 init=0;
static u8* buf = (u8*)malloc(eiMiB(2));
static const uint s = sizeof(StackAlloc);
static StackAlloc t( buf+s, eiMiB(2)-s );
static Scope g( t, "global print" );
static FifoMpmc<PrintMsg*> f( g, 10*1024 );

struct SortByTime
{
	typedef u32 Type;
	u32 operator()( PrintMsg** in ) const { float t = (float)(*in)->time; return *(u32*)&t; }
};
#endif

static Futex futex;
void eight::Print( const char* msg )
{
#if 0
	if( InPool() || true)
	{
		/*futex.Lock();
		std::cout << msg << std::endl << std::flush;
		futex.Unlock();*/
		
		if( CurrentPoolThread().index == 0 )
		{
			std::vector<PrintMsg*> messages;
			PrintMsg* packet;
			while( f.Pop(packet) )
			{
				messages.push_back(packet);
			}
			PrintMsg thisPacket = { msg, g_timer.Elapsed() };
			messages.push_back(&thisPacket);
			uint size = messages.size();
			messages.resize( size * 2 );
			RadixSort( &messages[0], &messages[size], size, SortByTime() );
			for( uint i=0; i != size; ++i )
			{
				std::cout << messages[i]->msg << std::flush;
				//printf(messages[i]->msg);
				if( messages[i] != &thisPacket )
				{
					free((void*)messages[i]->msg);
					free(messages[i]);
				}
			}
		}
		else
		{
			uint len = strlen(msg)+1;
			PrintMsg* packet = (PrintMsg*)malloc(sizeof(PrintMsg));
			packet->msg = (char*)malloc(len);
			packet->time = g_timer.Elapsed();
			strncpy((char*)packet->msg, msg, len);
			if( !f.Push( packet ) )
			{
				printf(packet->msg);
				free((void*)packet->msg);
				free(packet);
			}
		}
	}
	else
#endif
		printf(msg);
}
void eight::Printf( const char* fmt, ... )
{
	va_list	v;
	va_start(v, fmt);
	vprintf(fmt, v);
	va_end(v);
}

struct DebugOutput_Tag{};
struct DebugOutput_Buffer{ char buffer[2048]; };
ThreadLocalStatic<DebugOutput_Buffer, DebugOutput_Tag> tlsBuffers;
void eight::DebugOutput( const char* prefix, const char* fmt, ... )
{
	//static char buffer[2048];
	DebugOutput_Buffer* tlBuffer = tlsBuffers;
	if( !tlBuffer )
		tlsBuffers = tlBuffer = new DebugOutput_Buffer;
	char* buffer = tlsBuffers->buffer;
	int len = strlen(prefix);
	memcpy(buffer, prefix, len);
	va_list	v;
	va_start(v, fmt);
	len += vsprintf(&buffer[len], fmt, v);
	va_end(v);
	buffer[len] = '\n';
	buffer[len+1] = '\0';
	eiASSERT( len+2 <= 2048 );
	eight::Print(buffer);
}

void eight::DebugOutput( int line, const char* file, const char* function, const char* fmt, ... )
{
	va_list	args;
	va_start(args, fmt);
	DebugOutput( line, file, function, fmt, args );
	va_end(args);
}
void eight::DebugOutput( int line, const char* file, const char* function, const char* fmt, va_list& args )
{
	//static char buffer[2048];
	DebugOutput_Buffer* tlBuffer = tlsBuffers;
	if( !tlBuffer )
		tlsBuffers = tlBuffer = new DebugOutput_Buffer;
	char* buffer = tlsBuffers->buffer;
	int len = sprintf(buffer, "%s(%d) : %s : ", file, line, function );
	len += vsprintf(&buffer[len], fmt, args);
	buffer[len] = '\n';
	buffer[len+1] = '\0';
	eiASSERT( len+2 <= 2048 );
	eight::Print(buffer);
}




#if 0

Low level

Resources
Texture
RenderTarget
VBO, IBO
ConstantBuffer
InstanceBuffer - may be a ConstantBuffer (used in for each instance loop), or a VBO w/ DX9 instancing data.

^^n.b. see that? a render platform choice like whether to put per-instance data into an IBO or a uniform ends up determining the form of our HLSL code.
           that means that our HLSL should be generated from many fragments. The user wants to write shaders like they're an asset, but the engine
		   wants them to be code - break up into fragments and reassemble.
		   When compiling shader permutations, all possible asset-fragment combinations can be determined statically. Engine-fragments change at runtime,
           so all their permutations must be built.

DrawCalls
Mesh        - VBO to bind, range of VBO to read, primitive type
IndexedMesh - VBO to bind, IBO to bind, range of IBO to read, primitive type
InstancedMesh - IndexedMesh + InstanceBuffer to bind

State
RasterOutput - MRT + DepthStencil
MaterialState - Blend, Textures, Uniforms, input vertex attribs
SceneState - global uniforms (proj matrix etc)
Viewport/Scissor

High level

pipeline graph (and rt pool)
shader contexts
cameras
viewports/composition
scene traversal?
material shaders
light shaders




CmdBuf
 push( vec<SceneState> )
 push( DrawCall, MaterialState, InstanceState ) : for each State in MaterialState and InstanceState as T(s)
                                                :     if m_p##T!=&s then push(m_p##T=&s)
                                                : push( DrawCall )
 ^^ for in-order cmd bufs only -- some cmd-bufs could allow re-sorting, which would delay the state-cache check until after the sort (maybe pushing the vec itself to begin with?) sorting prob not needed

 private: push( State ) -- deferred context/command buffer, or very basic vm for DX9PC/GL (unless immediate context, a CRTP)
 private: push( DrawCall ) -- as above



#endif