
//extern uint g_defaultSpin;
template<class F> void YieldThreadUntil( F& func, uint spin, bool doJobs, bool willFireWakeEvent )
{
	if( func() )
		return;
	eiProfile("Yield");
	if( willFireWakeEvent )
	{
		BusyWait<true> spinner(doJobs);
		while( !spinner.Try(func, spin) ) {}
	}
	else
	{
		BusyWait<false> spinner(doJobs);
		while( !spinner.Try(func, spin) ) {}
	}
}

struct WaitFor1
{
	WaitFor1( const Atomic& a ) : a(a) {} const Atomic& a; 
	bool operator()() const { return 1 == (s32)a; }
};

struct WaitForTrue
{
	WaitForTrue( const Atomic& a ) : a(a) {} const Atomic& a; 
	bool operator()() const { return a!=0; }
};

struct WaitForFalse
{
	WaitForFalse( const Atomic& a ) : a(a) {} const Atomic& a; 
	bool operator()() const { return !a; }
};

struct WaitForValue
{
	WaitForValue( const Atomic& a, s32 v ) : a(a), value(v) {} const Atomic& a; s32 value;
	bool operator()() const { return value == (s32)a; }
};

struct WaitForTrue64
{
	WaitForTrue64( const u64&    a ) : a((Atomic*)&a) {}
	WaitForTrue64( const s64&    a ) : a((Atomic*)&a) {} 
	WaitForTrue64( const double& a ) : a((Atomic*)&a) {} const Atomic* a; 
	bool operator()() const
	{
		return a[0]!=0 && a[1]!=0;
	}
};