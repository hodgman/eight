
//extern uint g_defaultSpin;
template<class F> void YieldThreadUntil( F& func, uint spin, bool doJobs )
{
	BusyWait spinner(doJobs);
	while( !spinner.Try(func, spin) ) {}
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