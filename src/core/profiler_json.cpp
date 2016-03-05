
#include <eight/core/profiler.h>
#include <eight/core/debug.h>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/filestream.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"

using namespace eight;
using namespace rapidjson;

static FILE* g_fp = 0;
static FileStream* g_fs;
static Writer<FileStream>* g_w;
int g_profilersRunning = 0;
int g_profileFramesLeft = 0;

#define STRING(x) String(x, sizeof(x)-1)
void BeginEvents( void*, bool disjoint )
{
	if( g_fp )
		return;
	g_fp = fopen( "profile.json", "wt" );
	if( !g_fp )
		return;

	g_fs = new FileStream( g_fp );
	g_w = new Writer<FileStream>( *g_fs );

	Writer<FileStream>& w = *g_w;
	w.StartObject();
	w.STRING( "traceEvents" );
	w.StartArray();
}
void EndEvents( void* )
{
	if( --g_profilersRunning !=  0 )
		return;
	if( !g_w )
		return;
	Writer<FileStream>& w = *g_w;
	/*
	for( uint i=0; i!=3; ++i )
	{
		char buf[32] = "Pool 0";
		buf[5] = '0'+i;
		w.StartObject();
		w.STRING( "name" ); w.STRING( "thread_name" );
		w.STRING( "ph" );   w.String( "M", 1 );
		w.STRING( "pid" );  w.Int( 0 );
		w.STRING( "tid" );  w.Int( i );
		w.STRING( "args" );
		w.StartObject();
		w.STRING( "name" ); w.String( buf, strlen( buf ) );
		w.EndObject();
		w.EndObject();
	}*/
	w.StartObject();
	w.STRING( "name" ); w.STRING( "thread_name" );
	w.STRING( "ph" );   w.String( "M", 1 );
	w.STRING( "pid" );  w.Int( -1 );
	w.STRING( "tid" );  w.Int( 99 );
	w.STRING( "args" );
	w.StartObject();
	w.STRING( "name" ); w.STRING( "GPU" );
	w.EndObject();
	w.EndObject();
	
	w.EndArray();
	w.STRING( "otherData" );
	w.StartObject();
	w.STRING( "origin" ); w.STRING( "eight" );
	w.EndObject();
	w.EndObject();

	delete g_w;
	delete g_fs;
	fclose( g_fp );
	g_fp = 0;
	g_fs = 0;
	g_w = 0;
}
void RecordEvent( void*, int tid, int pid, const char* name, double time, ProfileCallback::Type type )
{
	if( !g_w )
		return;
	Writer<FileStream>& w = *g_w;

	double scale = 1000000;

	const char* phase = 0;
	switch( type )
	{
	case ProfileCallback::Begin:  phase = "B"; break;
	case ProfileCallback::End:    phase = "E"; break;
	case ProfileCallback::Marker: phase = "I"; break;
	default: eiASSERT( false );
	}

	w.StartObject();
	w.STRING( "name" ); w.String( name, strlen( name ) );
	//w.STRING( "cat" );  w.STRING( "GPU" );
	w.STRING( "ph" );   w.String( phase, 1 );
	w.STRING( "pid" );  w.Int( pid );
	w.STRING( "tid" );  w.Int( tid );
	w.STRING( "ts" );   w.Int64( (int64)(time * scale) );
	w.STRING( "args" ); w.StartObject(); w.EndObject();
	w.EndObject();
}
#undef STRING


ProfileCallback g_JsonProfileOut =
{
	&BeginEvents,
	&EndEvents,
	&RecordEvent,
	0
};