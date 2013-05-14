//------------------------------------------------------------------------------
#pragma once
#include <eight/core/alloc/scope.h>
#include <eight/core/alloc/new.h>
#include <eight/core/types.h>
#include <eight/core/tuple.h>
namespace eight {
//------------------------------------------------------------------------------

template<class T>
class Soa : NonCopyable
{
	typedef typename TupleToTupleOfPointers<T>::Type Type;
public:
	Soa(Scope& a, uint size) : m_size(size)
	{
		m.ForEach(Init(m, a, size));
	}

	void Set( uint index, const T& tuple )
	{
		eiASSERT( index < m_size );
		tuple.ForEach(Setter(Raw(), index));
	}
	void Get( uint index, T& tuple ) const
	{
		eiASSERT( index < m_size );
		tuple.ForEach(Getter(Raw(), index));
	}

	class Proxy
	{
	public:
		typename const T::A& A() const { return s.m.a[index]; }
		typename       T::A& A()       { return s.m.a[index]; }
		typename const T::B& B() const { return s.m.b[index]; }
		typename       T::B& B()       { return s.m.b[index]; }
		typename const T::C& C() const { return s.m.c[index]; }
		typename       T::C& C()       { return s.m.c[index]; }
		typename const T::D& D() const { return s.m.d[index]; }
		typename       T::D& D()       { return s.m.d[index]; }
		typename const T::E& E() const { return s.m.e[index]; }
		typename       T::E& E()       { return s.m.e[index]; }

		Proxy& operator=( const T& tuple )
		{
			s.Set( index, tuple );
			return *this;
		}
		operator T() const
		{
			T tuple;
			s.Get( index, tuple );
			return tuple;
		}
	//	friend T& operator=( T& tuple, const Proxy& );

		Proxy(Soa& d, uint i) : s(d), index(i) {}
	private:
		Soa& s;
		uint index;
	};
	const Proxy operator[](uint index) const { return Proxy(*this, index); }
	      Proxy operator[](uint index)       { return Proxy(*this, index); }
	
	       uint Length() const { return m_size; }
	static uint Components() { return Type::length; }
	const void*const* Raw() const { return (const void*const*)&m.a; }
	      void**      Raw()       { return (      void**     )&m.a; }

	typename       Type::A BeginA()       { return m.a; }
	typename const Type::A BeginA() const { return m.a; }
	typename       Type::B BeginB()       { return m.b; }
	typename const Type::B BeginB() const { return m.b; }
	typename       Type::C BeginC()       { return m.c; }
	typename const Type::C BeginC() const { return m.c; }
	typename       Type::D BeginD()       { return m.d; }
	typename const Type::D BeginD() const { return m.d; }
	typename       Type::E BeginE()       { return m.e; }
	typename const Type::E BeginE() const { return m.e; }

	typename       Type::A EndA()       { return m.a + m_size; }
	typename const Type::A EndA() const { return m.a + m_size; }
	typename       Type::B EndB()       { return m.b + m_size; }
	typename const Type::B EndB() const { return m.b + m_size; }
	typename       Type::C EndC()       { return m.c + m_size; }
	typename const Type::C EndC() const { return m.c + m_size; }
	typename       Type::D EndD()       { return m.d + m_size; }
	typename const Type::D EndD() const { return m.d + m_size; }
	typename       Type::E EndE()       { return m.e + m_size; }
	typename const Type::E EndE() const { return m.e + m_size; }
private:
	Type m;
	uint m_size;
	struct Init { Init(Type& m, Scope& a, uint size) : m(m), a(a), size(size) {}
		Type& m; Scope& a; uint size;
		template<class Y>
		void operator()( Y*& member )
		{
			member = eiAllocArray(a, Y, size);
		}
	};
	struct Getter { Getter(const void*const* raw, uint index) : raw(raw), index(index) {}
		const void*const* raw; uint index;
		template<class Y>
		void operator()( Y& member )
		{
			const Y* data = (const Y*)*raw;
			member = data[index];
			++raw;
		}
	};
	struct Setter { Setter(void** raw, uint index) : raw(raw), index(index) {}
		void** raw; uint index;
		template<class Y>
		void operator()( const Y& member )
		{
			Y* data = (Y*)*raw;
			data[index] = member;
			++raw;
		}
	};
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------