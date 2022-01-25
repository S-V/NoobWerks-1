#pragma once

mxSTOLEN("CryEngine 3");

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

/*!
		Class TRange, can represent anything that is range between two values, mostly used for time ranges.
 */
template< class T >
struct TRange
{
	T start;
	T end;

public:
	mxFORCEINLINE TRange() {}

	mxFORCEINLINE TRange( const TRange &r )
	{ start = r.start; end = r.end; };

	mxFORCEINLINE TRange( T s,T e )
	{ start = s; end = e; };

	void Set( T s,T e ) { start = s; end = e; };
	void clear() { start = 0; end = 0; };

	//! Get length of range.
	T	Length() const { return end - start; };
	//! Check if range is empty.
	bool IsEmpty()	const { return (start == 0 && end == 0); }

	//! Check if value is inside range.
	bool IsInside( T val ) { return val >= start && val <= end; };

	void ClipValue( T &val ) const
	{
		if (val < start) val = start;
		if (val > end) val = end;
	}

	//! Compare two ranges.
	bool	operator == ( const TRange &r ) const {
		return start == r.start && end == r.end;
	}
	//! Assign operator.
	TRange&	operator =( const TRange &r ) {
		start = r.start;
		end = r.end;
		return *this;
	}
	//! Intersect two ranges.
	TRange	operator & ( const TRange &r ) const {
		return TRange( MAX(start,r.start),MIN(end,r.end) );
	}
	TRange&	operator &= ( const TRange &r )	{
		return (*this = (*this & r));
	}
	//! Concatenate two ranges.
	TRange	operator | ( const TRange &r ) const {
		return TRange( MIN(start,r.start),MAX(end,r.end) );
	}
	TRange&	operator |= ( const TRange &r )	{
		return (*this = (*this | r));
	}
	//! add new value to range.
	TRange	operator + ( T v ) const {
		T s = start, e = end;
		if (v < start) s = v;
		if (v > end) e = v;
		return TRange( s,e );
	}
	//! add new value to range.
	TRange&	operator += ( T v ) const {
		if (v < start) start = v;
		if (v > end) end = v;
		return *this;
	}
};

#undef MIN
#undef MAX


//! CRange if just TRange for floats..
typedef TRange<float> FloatRange;



template< typename SrcRangeType, typename DstRangeType >
static mxFORCEINLINE
const DstRangeType TLinearMapRange(
	// source interval
	const TRange<SrcRangeType>& srcRange,
	// target interval
	const TRange<DstRangeType>& dstRange,
	// source value (must lie inside the source range)
	const SrcRangeType& srcValue
	)
{
	const SrcRangeType src_range_width = srcRange.end - srcRange.start;
	const DstRangeType dst_range_width = dstRange.end - dstRange.start;

	return dstRange.start
		+ ((srcValue - srcRange.start) / src_range_width) * dst_range_width
		;
}


namespace Ranges
{

}//namespace Ranges
