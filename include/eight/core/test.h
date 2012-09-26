//------------------------------------------------------------------------------
#pragma once
namespace eight {
//------------------------------------------------------------------------------

bool RunProtected( void(*pfn)(void*), void* arg );//return false if the function crashed
bool RunProtected( void(*pfn)() );//return false if the function crashed

bool RunUnitTest( const char* name, void(*pfn)(), const char* file, int line, const char* function );

void TestFailure( const char* file, int line, const char* function, const char* message, ... );

void EnterTest();
bool ExitTest();
bool InTest();

#define eiTEST(name)		\
	void eiTest_##name()	//

#define eiRUN_TEST(name, errorCount)												\
{																					\
	void eiTest_##name();															\
	if( !RunUnitTest(#name, &eiTest_##name, __FILE__, __LINE__, "eiTest_"#name) )	\
	{																				\
		++errorCount;																\
	}																				\
}																					//

void InitCrashHandler();

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
