//------------------------------------------------------------------------------
#pragma once
#include <eight/core/types.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T> struct IsSigned{ const static bool value = (T(-1) < T(0)); };

template<class T> struct UnsignedType{  };
template<> struct UnsignedType<uint64>{ typedef uint64 type; };
template<> struct UnsignedType< int64>{ typedef uint64 type; };
template<> struct UnsignedType<uint32>{ typedef uint32 type; };
template<> struct UnsignedType< int32>{ typedef uint32 type; };
template<> struct UnsignedType<uint16>{ typedef uint16 type; };
template<> struct UnsignedType< int16>{ typedef uint16 type; };

template<class T> struct IsInteger { const static bool value = false;};
template<> struct IsInteger<uint64>{ const static bool value = true; };
template<> struct IsInteger< int64>{ const static bool value = true; };
template<> struct IsInteger<uint32>{ const static bool value = true; };
template<> struct IsInteger< int32>{ const static bool value = true; };
template<> struct IsInteger<uint16>{ const static bool value = true; };
template<> struct IsInteger< int16>{ const static bool value = true; };

template<class T> struct IsBoolean { const static bool value = false;
                                     static bool& cast( const T& ){static bool b;return b;}};
template<> struct IsBoolean<bool>  { const static bool value = true;
                                     static const bool& cast( const bool& b ){return b;}
                                     static bool& cast( bool& b ){return b;}};


template<class T> struct IsPointer           { const static bool value = false; };
template<class T> struct IsPointer<      T*> { const static bool value = true;  };
//template<class T> struct IsPointer<const T*> { const static bool value = true;  };

template<class T> struct IsConstPointer           { const static bool value = false; };
template<class T> struct IsConstPointer<      T*> { const static bool value = false;  };
template<class T> struct IsConstPointer<const T*> { const static bool value = true;  };

template<class T> struct IsReference           { const static bool value = false; };
template<class T> struct IsReference<      T&> { const static bool value = true;  };
//template<class T> struct IsReference<const T&> { const static bool value = true;  };

template<class T> struct IsPointerOrRef           { const static bool value = false; };
template<class T> struct IsPointerOrRef<      T*> { const static bool value = true;  };
//template<class T> struct IsPointerOrRef<const T*> { const static bool value = true;  };
template<class T> struct IsPointerOrRef<      T&> { const static bool value = true;  };
//template<class T> struct IsPointerOrRef<const T&> { const static bool value = true;  };

template<class T> struct StripPointer           { typedef T Type; };//todo - all the combinations...
template<class T> struct StripPointer<      T*> { typedef T Type; };
template<class T> struct StripPointer<const T*> { typedef T Type; };

template<class T> struct StripRef           { typedef T Type; };
template<class T> struct StripRef<      T&> { typedef T Type; };
template<class T> struct StripRef<const T&> { typedef T Type; };

template<class T> struct RefToPointer           { typedef       T  Type; static       T& Deref(      T& t){return  t;} static const T& Deref(const T& t){return  t;} };
template<class T> struct RefToPointer<      T&> { typedef       T* Type; static       T& Deref(      T* t){eiASSERTMSG(t, "Null reference"); return *t;}};
template<class T> struct RefToPointer<const T&> { typedef const T* Type; static const T& Deref(const T* t){eiASSERTMSG(t, "Null reference"); return *t;}};

template<class T> struct ConstRef           { typedef const T& Type; };
template<class T> struct ConstRef<      T&> { typedef const T& Type; };
template<class T> struct ConstRef<const T&> { typedef const T& Type; };
template<       > struct ConstRef<void>     { typedef Nil Type; };

template<class T> struct Ref           { typedef       T& Type; };
template<class T> struct Ref<      T&> { typedef       T& Type; };
template<class T> struct Ref<const T&> { typedef const T& Type; };

template<class T         > struct MemberType          { };//	typedef Nil Type; };
template<class R, class T> struct MemberType<R T::*> { typedef R Type; };

template<class T, class Y> struct IsSameType       { const static bool value = false; };
template<class T         > struct IsSameType<T, T> { const static bool value = true; };


#define eiDECLARE_FIND_MEMBER(NAME)                                                   \
namespace FindMember {                                                                \
template<class T> class NAME##_ {                                                     \
	struct Fallback { int NAME; };                                                    \
	struct Derived : T, Fallback { };                                                 \
	template<class U, U> struct Check;                                                \
	template<class U> static sizeone& Dummy(Check<int Fallback::*, &U::NAME> *);      \
	template<class U> static sizetwo& Dummy(...);                                     \
	template<bool Exists> struct Helper        { typedef decltype(&T       ::NAME) Type; static Type Pointer() { return &T       ::NAME; } }; \
	template<>            struct Helper<false> { typedef decltype(&Fallback::NAME) Type; static Type Pointer() { return &Fallback::NAME; } }; \
public:                                                                               \
	enum { exists = sizeof(Dummy<Derived>(0)) == 2 };                                 \
	typedef typename Helper<exists>::Type Type;                                       \
	static Type Pointer() { return Helper<exists>::Pointer(); }                       \
};                                                                                    \
template<class E, class R> bool NAME(E& entity, R& result)                            \
{                                                                                     \
	result = internal::TryGetMember<StripPointer<R>::Type>(                           \
		entity, FindMember::NAME##_<E>::Pointer());                                   \
	return FindMember::NAME##_<E>::exists;                                            \
}}                                                                                    //

namespace internal {
	template<bool condition> struct TryGetMember_
	{
		template<class Type, class Entity, class MemberPtr>
		static Type* GetPointer( Entity& view, MemberPtr ptr )
		{
			return &((view).*(ptr));
		}
	};
	template<> struct TryGetMember_<false> { 
		template<class T> static T* GetPointer(...){ return nullptr; }
	};

	template<class ExpectedType, class T, class MemberPtr>
	ExpectedType* TryGetMember( T& entity, MemberPtr ptr )
	{
		const bool correctType = IsSameType<ExpectedType, T>::value;
		return TryGetMember_<correctType>::GetPointer<ExpectedType>( entity, ptr );
	}
}

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
