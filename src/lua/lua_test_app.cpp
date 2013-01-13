//------------------------------------------------------------------------------
#include <eight/core/test.h>
#include <eight/core/application.h>
using namespace eight;
//------------------------------------------------------------------------------

eiENTRY_POINT( test_main )
int test_main( int argc, char** argv )
{
	int errorCount = 0;
	eiRUN_TEST( Lua, errorCount );
	return errorCount;
}

//------------------------------------------------------------------------------
