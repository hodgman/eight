//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/scope.h>
#include <eight/core/hash.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T>
struct TypeNameGetter
{
	static const char* Get()
	{
		const char* fn = __FUNCTION__;
		const char* fn2 = typeid(T).name();
		//std::find();
		return fn2;
	}
};

template<class T>
struct TypeName
{
	static void Get(const char*& begin, const char*& end)
	{
		begin = __FUNCTION__;
		for(++begin; *begin && *(begin-1) != '<'; ++ begin);
		for(end = begin; *end; ++ end);
		for(; end > begin && *end != '>'; -- end);
	}
	static const char* Get(Scope& a)
	{
		const char* begin=0, *end=0;
		Get(begin, end);
		uint length = end-begin;
		char* buf = (char*)a.Alloc(length+1);
		memcpy(buf, begin, length);
		buf[length] = 0;
		return buf;
	}
	static void Get(char* buf, uint bufLen)
	{
		const char* begin=0, *end=0;
		Get(begin, end);
		uint length = end-begin;
		eiASSERT( length+1 <= bufLen );
		memcpy(buf, begin, length);
		buf[length] = 0;
	}
	static const char* Get()
	{
		static const char* value = 0;
		if( !value )
		{
			static char buffer[128];
			Get(buffer, 128);
			//todo - memory fence
			value = buffer;
		}
		return value;
	}
};

template<class T>
struct TypeHash
{
	static u32 Get_()
	{
		const char* begin;
		const char* end;
		TypeName<T>::Get(begin, end);
		return Fnv32a(begin, end);
	}
	static u32 Get()
	{
		static const u32 value = Get_();
		return value;
	}
};
/*
inline void test_typename()
{
	const char* name = TypeNameGetter<std::vector<std::string>>::Get();
	const char* name2 = TypeName<std::vector<std::string>>::Get();
	const char* name3 = TypeNameGetter<Asset>::Get();
}
*/
//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
