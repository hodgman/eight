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

//------------------------------------------------------------------------------
} // namespace eight
//------------------------------------------------------------------------------
