
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		May 2008
 RCS:		$Id: emobjectselremoval.cc,v 1.9 2009-11-12 11:56:11 cvsumesh Exp $
________________________________________________________________________

-*/

#include "arraynd.h"
#include "binidsurface.h"
#include "emobjectselremoval.h"
#include "emobject.h"
#include "parametricsurface.h"
#include "survinfo.h"

namespace EM
{
    
EMObjectRowColSelRemoval::EMObjectRowColSelRemoval(EMObject& emobj,
						   const SectionID& secid,
						   const Selector<Coord3>& sel,
						   int nrrows, int nrcols,
       						   int startrow, int startcol )
    : emobj_(emobj)
    , sectionid_(secid)
    , selector_(sel)
    , startrow_(startrow)
    , nrrows_(nrrows)
    , startcol_(startcol)
    , nrcols_(nrcols)
{ emobj_.ref(); }


EMObjectRowColSelRemoval::~EMObjectRowColSelRemoval()
{ emobj_.unRef(); }


bool EMObjectRowColSelRemoval::doPrepare( int nrthreads )
{
    removelist_.erase();
    //TODO this is temporary extraction way of z values
    //this will get replaced by well though class structue

    const Geometry::Element* ge = emobj_.sectionGeometry( sectionid_ );
    if ( !ge ) return false;

    mDynamicCastGet(const Geometry::BinIDSurface*,surf,ge);
    if ( !surf ) return false;
    
    zvals_ = surf->getArray()->getData();

    starts_.erase();
    stops_.erase();

    starts_ += RowCol(startrow_, startcol_);
    stops_ += RowCol(startrow_+nrrows_-1, startcol_+nrcols_-1);

    finished_ = false;
    nrwaiting_ = 0;
    nrthreads_ = nrthreads;

    return true;
}


bool EMObjectRowColSelRemoval::doWork( od_int64, od_int64, int threadid )
{
    lock_.lock();

    while( !finished_ )
    {
	if ( !starts_.size() )
	{
	    nrwaiting_ ++;
	    if ( nrwaiting_==nrthreads_ )
	    {
		finished_ = true;
		lock_.signal( true );
		nrwaiting_--;
		break;
	    }

	    lock_.wait();
	    nrwaiting_--;
	}

	const int sz = starts_.size();
	if ( !sz )
	    continue;

	RowCol start = starts_[sz-1];
	RowCol stop = stops_[sz-1];

	starts_.remove( sz-1 );
	stops_.remove( sz-1 );

	lock_.unLock();

	processBlock( start, stop );

	lock_.lock();
    }

    lock_.unLock();

    return true;
}


void EMObjectRowColSelRemoval::processBlock( const RowCol& start,
					     const RowCol& stop )
{
    Coord3 up = Coord3::udf();
    Coord3 down = Coord3::udf();

    getBoundingCoords( start, stop, up, down );

    const int sel = !selector_.canDoRange() ? 1 
				: selector_.includesRange( up, down );
    if ( sel==0 || sel==3 )
	return;           // all outside or all behind projection plane

    int rowlength = stop.r() - start.r();
    int collength = stop.c() - start.c();

    if ( rowlength < 32 && collength < 32 )
	makeListGrow( start, stop, sel );
    else if ( rowlength < 32 && collength >= 32 )
    {
	lock_.lock();

	starts_ += start;
	stops_ += RowCol( stop.r(), start.c()+collength/2 );

	lock_.signal( starts_.size()>1 );

	lock_.unLock();

	processBlock( RowCol(start.r(),start.c()+1+collength/2), stop );
    }
    else if ( rowlength >=32 && collength < 32 )
    {
	lock_.lock();

	starts_ += start;
	stops_ += RowCol( start.r()+rowlength/2, stop.c() );

	lock_.signal( starts_.size()>1 );

	lock_.unLock();

	processBlock( RowCol(start.r()+1+rowlength/2,start.c()), stop );
    }
    else
    {
	lock_.lock();
	
	starts_ += start;
	stops_ += RowCol( start.r()+rowlength/2, start.c()+collength/2 );

	starts_ += RowCol( start.r(), start.c()+collength/2+1 );
	stops_ += RowCol( start.r()+rowlength/2, stop.c() );

	starts_ += RowCol( start.r()+rowlength/2+1, start.c() );
	stops_ += RowCol( stop.r(), start.c()+collength/2 );

	lock_.signal( starts_.size()>1 );

	lock_.unLock();

	processBlock( RowCol(start.r()+rowlength/2+1,start.c()+collength/2+1),
		      stop );
    }
}


void EMObjectRowColSelRemoval::getBoundingCoords( const RowCol& start,
						  const RowCol& stop,
						  Coord3& up, Coord3& down )
{
    Coord coord0 = SI().transform( BinID(start.r(),start.c()) );
    up.x = down.x = coord0.x;
    up.y = down.y = coord0.y;

    Coord coord1 = SI().transform( BinID(start.r(),stop.c()) );
    if ( up.x < coord1.x ) up.x = coord1.x;
    if ( up.y < coord1.y ) up.y = coord1.y;
    if ( coord1.x < down.x ) down.x = coord1.x;
    if ( coord1.y < down.y ) down.y = coord1.y;

    Coord coord2 = SI().transform( BinID(stop.r(),start.c()) );
    if ( up.x < coord2.x ) up.x = coord2.x;
    if ( up.y < coord2.y ) up.y = coord2.y;
    if ( coord2.x < down.x ) down.x = coord2.x;
    if ( coord2.y < down.y ) down.y = coord2.y;

    Coord coord3 = SI().transform( BinID(stop.r(),stop.c()) );
    if ( up.x < coord3.x ) up.x = coord3.x;
    if ( up.y < coord3.y ) up.y = coord3.y;
    if ( coord3.x < down.x ) down.x = coord3.x;
    if ( coord3.y < down.y ) down.y = coord3.y;

    up.z = down.z = zvals_[0];

    for ( int row=start.r(); row<=stop.r(); row++ )
    {
	int idx = (row-startrow_)*nrcols_ + (start.c()-startcol_);
	for ( int col=start.c(); col<=stop.c(); col++, idx++ )
	{
	    const float val = zvals_[idx];
	    if ( val > up.z )
		up.z = val;
	    if ( val < down.z )
		down.z = val;
	}
    }
}


void EMObjectRowColSelRemoval::makeListGrow( const RowCol& start,
    					     const RowCol& stop, int selresult )
{
    Coord3 up = Coord3::udf();
    Coord3 down = Coord3::udf();

    const Geometry::Element* ge = emobj_.sectionGeometry( sectionid_ );
    if ( !ge ) return;

     mDynamicCastGet(const Geometry::RowColSurface*,surf,ge);
     if ( !surf ) return;

    getBoundingCoords( start,stop, up, down );
    
    const StepInterval<int> rowrg( start.r(), stop.r(), surf->rowRange().step );
    const StepInterval<int> colrg( start.c(), stop.c(), surf->colRange().step );

    TypeSet<EM::SubID> ids;

    HorSampling horsampling( true );
    horsampling.set( rowrg, colrg );
    HorSamplingIterator iter( horsampling );

    BinID bid;
    
    iter.reset();
    while( iter.next(bid) )
    {
	if ( selresult != 2 )     // not all inside
	{
	    const Coord3 crd = emobj_.getPos( sectionid_,
		    			      bid.getSerialized() );
	    if ( !crd.isDefined() || !selector_.includes(crd) )
		continue;
	}
	ids += bid.getSerialized();
    }

    lock_.lock();
    removelist_.append( ids );
    lock_.unLock();
}

} // EM
