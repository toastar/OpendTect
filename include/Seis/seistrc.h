#ifndef seistrc_h
#define seistrc_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: seistrc.h,v 1.8 2001-03-19 15:42:32 bert Exp $
________________________________________________________________________

-*/

#include <seisinfo.h>
#include <tracedata.h>
#include <datachar.h>
#include <datatrc.h>
class Interpolator1D;


/*!> Seismic traces.

A seismic trace is composed of trace info and trace data. The trace data
consists of one or more components. These are represented by a set of buffers,
interpreted by DataInterpreters.

The number of samples of the components may vary, therfore the index of the
sample at the info().sampling.start can be set to non-zero. The first component
(icomp==0) has a sampleoffset of zero which cannot be set.

*/


class SeisTrc
{
public:

			SeisTrc( int ns=0, const DataCharacteristics& dc
					    = DataCharacteristics() )
			: soffs_(0), intpols_(0)
			{ data_.addComponent( ns, dc ); }
			SeisTrc( const SeisTrc& t )
			: soffs_(0), intpols_(0)	{ *this = t; }
    SeisTrc&		operator =(const SeisTrc& t);

    SeisTrcInfo&	info()		{ return info_; }
    const SeisTrcInfo&	info() const	{ return info_; }
    TraceData&		data()		{ return data_; }
    const TraceData&	data() const	{ return data_; }

    inline int		sampleOffset( int icomp ) const
			{ return soffs_ && icomp && icomp < soffs_->size()
				? (*soffs_)[icomp] : 0; }
    void		setSampleOffset(int icomp,int);
    inline float	posOffset( int icomp ) const
			{ return sampleOffset(icomp) * info_.sampling.step; }


    inline void		set( int idx, float v, int icomp )
			{ data_.setValue( idx, v, icomp ); }
    inline float	get( int idx, int icomp ) const
			{ return data_.getValue( idx, icomp ); }

    inline int		size( int icomp ) const
			{ return data_.size( icomp ); }
    inline double	getX( int idx, int icomp ) const
			{ return startPos(icomp) + idx * info_.sampling.step; }
    float		getValue(float,int icomp) const;
    const Interpolator1D* interpolator( int icomp=0 ) const;
			//!< May return null!
    void		setInterpolator(Interpolator1D*,int icomp=0);
			//!< Passed Interpolator1D becomes mine
			//!< setData() will be called with appropriate args.

    inline bool		isNull( int icomp ) const
			{ return data_.isZero(icomp); }
    inline void		zero( int icomp=-1 )
			{ data_.zero( icomp ); }
    inline bool		dataPresent( float t, int icomp ) const
			{ return info_.dataPresent( t, size(icomp),
						    sampleOffset(icomp)); }
    inline SamplingData<float>	samplingData( int icomp ) const
			{ return SamplingData<float>( startPos(icomp),
						      info_.sampling.step); }
    inline float	startPos( int icomp ) const
			{ return info_.sampling.start + posOffset(icomp); }
    inline float	samplePos( int idx, int icomp ) const
			{ return info_.samplePos( idx, sampleOffset(icomp) ); }
    inline int		nearestSample( float pos, int icomp ) const
			{ return info_.nearestSample(pos,sampleOffset(icomp)); }
    void		setStartPos(float,int icomp=0);

    bool		reSize( int sz, int icomp )
			{ data_.reSize( sz, icomp ); return data_.allOk(); }
    SampleGate		sampleGate(const Interval<float>&,bool check,
				   int icomp) const;

protected:

    TraceData		data_;
    SeisTrcInfo		info_;
    TypeSet<int>*	soffs_;
    ObjectSet<Interpolator1D>* intpols_;

};


/*!> Seismic traces conforming the DataTrace interface.

One of the components of a SeisTrc can be selected to form a DataTrace.

*/

class SeisDataTrc : public DataTrace
{
public:

			SeisDataTrc( SeisTrc& t, int comp=0 )
			: trc(t), ismutable(true), curcomp(comp)
							{}
			SeisDataTrc( const SeisTrc& t, int comp=0 )
			: trc(const_cast<SeisTrc&>(t)), ismutable(false)
			, curcomp(comp)			{}
    void		setComponent( int c )		{ curcomp = c; }
    int			component() const		{ return curcomp; }

    inline bool		set( int idx, float v )
			{ if ( ismutable ) trc.data().setValue( idx, v, curcomp );
			  return ismutable; }
    inline float	operator[]( int i ) const
			{ return trc.get( i, curcomp ); }
    int			getIndex( double val ) const
			{ return trc.info().nearestSample( val ); }
    double		getX( int idx ) const
			{ return trc.getX( idx, curcomp ); }
    float		getValue( float v ) const
			{ return trc.getValue( v, curcomp ); }

    inline int		size() const		{ return trc.size( curcomp ); }
    inline double	step() const		{ return trc.info().sampling.step; }
    double		start() const		{ return trc.startPos(curcomp); }

    bool		isMutable() const	{ return ismutable; }

protected:

    SeisTrc&		trc;
    int			curcomp;
    bool		ismutable;

    XFunctionIter*	mkIter(bool, bool) const;

};


#endif
