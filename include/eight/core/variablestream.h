//------------------------------------------------------------------------------
#pragma once
#include <eight/core/bytestream.h>
namespace eight {
//------------------------------------------------------------------------------

namespace core {

template<bool> class TByteStream;
typedef TByteStream<true > CByteStream;
//DECLARE_TEST( VariableStream )

class CVariableStream
{
public:
	CVariableStream( CByteStream& );
	void ReadGroupStart();
	void ReadGroupEnd();
	void WriteGroupStart();
	void WriteGroupEnd();

	template<class T> bool Read ( uint id,       T& d );
	template<class T> void Write( uint id, const T& d );
	template<class T> void NullWrite( uint id );
	
	uint Size() const { return m_Data.Size(); }
private:
	const static uint sm_NotYetRead = UINT_MAX;
	uint m_NextId;
	CByteStream& m_Data;
};

//------------------------------------------------------------------------------
#include "variablestream.hpp"
} // namespace eight
//------------------------------------------------------------------------------
