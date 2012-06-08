
extern uint g_defaultSpin;
template<class F> void YieldThreadUntil( F& func, uint spin )
{
	if( !spin )
		spin = g_defaultSpin;
	bool sleep = false;
	while( true )
	{
		for( uint i=0; i!=spin; ++i )
		{
			for( uint j=0; j!=spin; ++j )
			{
				if( func() )
					return;
				YieldHardThread();
			}
			if( func() )
				return;
			YieldSoftThread();
		}
		if( func() )
			return;
		YieldToOS( sleep );
		sleep = !sleep;
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
