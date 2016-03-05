//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
#include <eight/core/noncopyable.h>
#include <eight/core/macro.h>
namespace eight {
	class Scope;
//------------------------------------------------------------------------------

#ifndef eiCPU_PROFILE
# if 1//eiDEBUG_LEVEL > 0
#  define eiCPU_PROFILE 1
# else
#  define eiCPU_PROFILE 0
# endif
#endif

#if eiCPU_PROFILE
# define eiProfile( name ) eight::ProfileScope eiJOIN(_cpuScope,__LINE__)(name)
# define eiProfileFunction() eiProfile(__FUNCTION__)
#else
# define eiProfile( name ) 
# define eiProfileFunction() 
#endif

bool PushProfileScope(const char* name);
void PopProfileScope();
struct ProfileScope : NonCopyable
{
	ProfileScope(const char* name)
	{
		b = PushProfileScope(name);
	}
	~ProfileScope()
	{
		if( b )
			PopProfileScope();
	}
private:
	bool b;
};

void InitProfiler(Scope&, uint maxEvents);

struct ProfileCallback
{
	enum Type
	{
		Begin,
		End,
		Marker,
	};
	typedef void (FnBeginEvents)(void*, bool disjoint);
	typedef void (FnEndEvents)(void*);
	typedef void (FnEvent)(void*, int tid, int pid, const char* name, double time, Type);
	FnBeginEvents* beginEvents;
	FnEndEvents*   endEvents;
	FnEvent*       event;
	void*          data;
};

bool ProfileBegin();
void ProfileEnd();
void ProfileEnd(const ProfileCallback&, Scope& a);

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
