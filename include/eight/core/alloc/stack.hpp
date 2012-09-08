//------------------------------------------------------------------------------
//inline StackAlloc::StackAlloc()
//                      : begin(), end(), cursor(), owner() {}
inline StackAlloc::StackAlloc( void* begin, void* end )
                      : begin((u8*)begin), end((u8*)end), cursor((u8*)begin), owner() {}
inline StackAlloc::StackAlloc( void* begin, uint size )
                      : begin((u8*)begin), end((u8*)begin+size), cursor((u8*)begin), owner() {}
/*inline bool StackAlloc::Valid() const
{
	return begin != end;
}*/
inline void StackAlloc::Clear()
{
	cursor = begin;
}
inline bool StackAlloc::Align( uint align )
{
	u8* null = (u8*)0;
	ptrdiff_t offset = cursor - null;
	offset = ((offset + align-1) / align) * align;
	u8* newCursor = null + offset;
	if( newCursor <= end )
	{
		cursor = newCursor;
		return  true;
	}
	return false;
}
//bool IsClear() const { return cursor == begin; }
inline const u8* StackAlloc::Begin() const { return begin; }
inline u8* StackAlloc::Alloc( u32 required, u32 reserve )
{
	eiASSERT( reserve >= required );
	if( cursor + reserve <= end )
	{
		u8* allocation = cursor;
		cursor += required;
		eiASSERT( allocation );
		eiDEBUG( memset( allocation, 0xCDCDCDCD, required ); )
		return allocation;
	}
	return 0;
}
inline u8* StackAlloc::Alloc( u32 size )
{
	return Alloc( size, size );
}
template<class T> inline T* StackAlloc::Alloc()
{
	return (T*)Alloc( sizeof(T) );
}
template<class T> inline T* StackAlloc::Alloc( u32 count )
{
	return (T*)Alloc( sizeof(T)*count );
}
inline void StackAlloc::Unwind(const void* p)
{
	eiASSERT( p>=begin && p<end && p<=cursor );
	eiDEBUG( uint bytes = cursor-(u8*)p );
	cursor = (u8*)p;
	eiDEBUG( memset( cursor, 0xFEFEFEFE, bytes ) );
}
inline const u8* StackAlloc::Mark() const
{
	return cursor;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

inline void StackMeasure::Clear()
{
	cursor = 0;
}
inline void StackMeasure::Align( uint align )
{
	cursor = ((cursor + align-1) / align) * align;
}
inline u32 StackMeasure::Alloc( u32 size )
{
	u32 mark = cursor;
	cursor += size;
	return mark;
}
template<class T>
u32 StackMeasure::Alloc()
{
	return Alloc( sizeof(T) );
}
template<class T>
u32 StackMeasure::Alloc( u32 count )
{
	return Alloc( sizeof(T)*count );
}
//------------------------------------------------------------------------------