//------------------------------------------------------------------------------
#pragma once
#include <eight/core/traits.h>
#include <eight/core/math/arithmetic.h>
namespace eight {
//------------------------------------------------------------------------------

enum IoDir
{
	Read,
	Write
};

template<IoDir Mode, bool Compress> struct StreamState {};
template<> struct StreamState<Write, true > { typedef       ubyte* Ptr; Ptr it; const ubyte* end; uint bools_written;              StreamState(Ptr i, const ubyte* e) : it(i), end(e), bools_written(0) {} };
template<> struct StreamState<Read,  true > { typedef const ubyte* Ptr; Ptr it; const ubyte* end; uint bools_read, last_bool_pack; StreamState(Ptr i, const ubyte* e) : it(i), end(e), bools_read(0), last_bool_pack(0) {} };
template<> struct StreamState<Write, false> { typedef       ubyte* Ptr; Ptr it; const ubyte* end;                                  StreamState(Ptr i, const ubyte* e) : it(i), end(e) {} };
template<> struct StreamState<Read,  false> { typedef const ubyte* Ptr; Ptr it; const ubyte* end;                                  StreamState(Ptr i, const ubyte* e) : it(i), end(e) {} };

template<IoDir Mode, bool Compress> struct StreamIterator
{	
	StreamIterator( typename StreamState<Mode,Compress>::Ptr data, uint size ) : state(data, size) {}

	template<bool C>
	StreamIterator& operator=( const StreamIterator<Mode,C>& o ) { memcpy(&state, &o.state, min(sizeof(StreamState<Mode,Compress>),sizeof(StreamState<Mode,C>))); }
	                  void read ( bool& );
	                  void write( bool  );
	template<class T> void read (       T& );
	template<class T> void write( const T& );
	template<class T> uint read ( T*, uint& read, uint max_read );//returns number of items that were not read (if max_read wasn't large enough)
	template<class T> void write( T*, uint count );
private:
	StreamState<Mode,Compress> state;
	//These templates are required to stop the *var_int funcs being compiled with non-integer types
	template<class T, bool isInt> struct  ReadVarInt { static void Call(StreamIterator&,       T&) {eiSTATIC_ASSERT(0)} };
	template<class T, bool isInt> struct WriteVarInt { static void Call(StreamIterator&, const T&) {eiSTATIC_ASSERT(0)} };
	template<class T> struct  ReadVarInt<T,true> { static void Call(StreamIterator& s,       T& d) {d=s.read_var_int<T>();} };
	template<class T> struct WriteVarInt<T,true> { static void Call(StreamIterator& s, const T& d) {s.write_var_int<T>(d);} };
	template<class T> T    read_var_int();
	template<class T> void write_var_int( T );
	void read_var_bool( bool& );
	void write_var_bool( bool );
};
template<bool C, class T> StreamIterator<Write,C>& operator<<( StreamIterator<Write,C>&, const T& bytes );
template<bool C, class T> StreamIterator<Read, C>& operator>>( StreamIterator<Read, C>&,       T& bytes );

typedef StreamIterator<Read,  true > ByteInput;
typedef StreamIterator<Write, true > ByteOutput;
typedef StreamIterator<Read,  false> RawByteInput;
typedef StreamIterator<Write, false> RawByteOutput;

//------------------------------------------------------------------------------
#include "bytestream.hpp"
} // namespace eight
//------------------------------------------------------------------------------
