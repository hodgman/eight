//------------------------------------------------------------------------------
#pragma once
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
	static void Get_(const char*& begin, const char*& end)
	{
		begin = __FUNCTION__;
		for(++begin; *begin && *(begin-1) != '<'; ++ begin);
		for(end = begin; *end; ++ end);
		for(; end > begin && *end != '>'; -- end);
	}
	static const char* Get()
	{
		const char* begin=0, *end=0;
		Get_(begin, end);
		uint length = end-begin;
		char* buf = (char*)malloc(length+1);
		memcpy(buf, begin, end-begin);
		buf[length] = 0;
		return buf;
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
