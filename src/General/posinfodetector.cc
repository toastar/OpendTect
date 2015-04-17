/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Feb 2004
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "posinfodetector.h"
#include "trckeyzsampling.h"
#include "posidxpair2coord.h"
#include "binidsorting.h"
#include "keystrs.h"
#include "iopar.h"
#include "perthreadrepos.h"
#include "uistrings.h"

PosInfo::Detector::Detector( const Setup& su )
	: sorting_(*new BinIDSorting(su.is2d_))
	, sortanal_(0)
	, setup_(su)
	, errmsg_(tr("No positions found"))
{
    reInit();
}


PosInfo::Detector::~Detector()
{
    delete &sorting_;
    deepErase( lds_ );
}


void PosInfo::Detector::reInit()
{
    delete sortanal_;
    sortanal_ = new BinIDSortingAnalyser( setup_.is2d_ );
    cbobuf_.erase();
    deepErase( lds_ );
    inlirreg_ = crlirreg_ = false;
    allstd_ = true;
    curline_ = curseg_ = -1;
    nrpos_ = nruniquepos_ = nroffsperpos_ = 0;
    offsrg_.start = offsrg_.stop = 0;
    firstduppos_.binid_.inl() = firstaltnroffs_.binid_.inl() = mUdf(int);
    mSetUdf(distrg_.start); avgdist_ = 0;
    step_.inl() = step_.crl() = 1;
}


bool PosInfo::Detector::add( const Coord& c, const BinID& b )
{ return add( CrdBidOffs(c,b,0) ); }
bool PosInfo::Detector::add( const Coord& c, const BinID& b, float o )
{ return add( CrdBidOffs(c,b,o) ); }
bool PosInfo::Detector::add( const Coord& c, int nr )
{ return add( CrdBidOffs(c,BinID(1,nr),0) ); }
bool PosInfo::Detector::add( const Coord& c, int nr, float o )
{ return add( CrdBidOffs(c,BinID(1,nr),o) ); }


bool PosInfo::Detector::add( const Coord& c, const BinID& b, int nr, float o )
{
    CrdBidOffs cbo( c, b, o );
    if ( !setup_.isps_ )
	cbo.offset_ = 0;
    if ( setup_.is2d_ )
	 { cbo.binid_.inl() = 1; cbo.binid_.crl() = nr; }
    return add( cbo );
}


bool PosInfo::Detector::finish()
{
    if ( sortanal_ && !applySortAnal() )
	return false;

    lastcbo_ = curcbo_;
    getBinIDRanges();
    step_.inl() = getStep(true);
    step_.crl() = getStep(false);
    avgdist_ /= nruniquepos_;

    // Get rid of 0 step (for segments with one position)
    const int nrstep = crlSorted() ? step_.inl() : step_.crl();
    for ( int iln=0; iln<lds_.size(); iln++ )
    {
	LineData& ld = *lds_[iln];
	for ( int iseg=0; iseg<ld.segments_.size(); iseg++ )
	{
	    int& curstep = ld.segments_[iseg].step;
	    if ( curstep < 1 ) curstep = nrstep;
	}
    }

    return true;
}


bool PosInfo::Detector::crlSorted() const
{ return setup_.is2d_ ? false : !sorting_.inlSorted(); }

bool PosInfo::Detector::inlSorted() const
{ return setup_.is2d_ ? true : sorting_.inlSorted(); }


void PosInfo::Detector::getTrcKeySampling( TrcKeySampling& hs ) const
{ hs.start_ = start_; hs.stop = stop_; hs.step_ = step_; }

void PosInfo::Detector::getCubeData( PosInfo::CubeData& cd ) const
{
    deepErase( cd );
    if ( allstd_ )
    {
	for ( int idx=0; idx<lds_.size(); idx++ )
	    cd += new LineData( *lds_[idx] );
	return;
    }

    const bool swpd = crlSorted();
    const bool revln = !(swpd ? sorting_.inlUpward() : sorting_.crlUpward());
    const bool revnr = !(swpd ? sorting_.crlUpward() : sorting_.inlUpward());

    const int startln = revln ? lds_.size() - 1 : 0;
    const int stopln = revln ? -1 : lds_.size();
    const int stepln = revln ? -1 : 1;
    for ( int iln=startln; iln!=stopln; iln+=stepln )
    {
	const LineData& ld = *lds_[iln];
	LineData* newld = new LineData( sorting_.inlUpward()
				      ? ld.linenr_ : -ld.linenr_ );
	const int nrsegs = ld.segments_.size();
	const int startseg = revnr ? nrsegs - 1 : 0;
	const int stopseg = revnr ? -1 : nrsegs;
	const int stepseg = revnr ? -1 : 1;
	for ( int iseg=startseg; iseg!=stopseg; iseg+=stepseg )
	{
	    const LineData::Segment& seg = ld.segments_[iseg];
	    if ( !revnr )
		newld->segments_ += seg;
	    else
		newld->segments_ += LineData::Segment( -seg.stop, -seg.start,
							-seg.step );
	}
	cd += newld;
    }
}


#define mErrRet(s) { errmsg = s; return errmsg.buf(); }
#define mErrRet2(x,y) { errmsg = x; errmsg += y; return errmsg.buf(); }

const char* PosInfo::Detector::getSurvInfo( TrcKeySampling& hs,
					    Coord crd[3] ) const
{
    mDeclStaticString( errmsg );
    if ( setup_.is2d_ )
	mErrRet( "Cannot determine survey setup from a 2D line scan" )
    if ( nruniquepos_ < 3 )
	mErrRet( "Not enough unique positions found" )

    hs.start_ = start_; hs.stop = stop_; hs.step_ = step_;

    if ( hs.start_.inl() == hs.stop.inl() )
	mErrRet2("The input data contains only one in-line: ",hs.start_.inl())
    else if ( hs.start_.crl() == hs.stop.crl() )
	mErrRet2("The input data contains only one cross-line: ",
		hs.start_.crl())

    const CrdBidOffs llnstart( userCBO(llnstart_) );
    const CrdBidOffs llnstop( userCBO(llnstop_) );
    const CrdBidOffs firstcbo( userCBO(firstcbo_) );
    const CrdBidOffs lastcbo( userCBO(lastcbo_) );

    const CrdBidOffs& usecbo(
	      abs(firstcbo.binid_.inl()-llnstart.binid_.inl())
	    < abs(lastcbo.binid_.inl()-llnstart.binid_.inl())
	    ? lastcbo : firstcbo );
    Coord c[3]; BinID b[2];
    c[0] = llnstart.coord_;	b[0] = llnstart.binid_;
    c[2] = llnstop.coord_;
    c[1] = usecbo.coord_;	b[1] = usecbo.binid_;

    Pos::IdxPair2Coord b2c;
    if ( !b2c.set3Pts( c[0], c[1], c[2], b[0], b[1], llnstop.binid_.crl() ) )
	return "The input data does not contain the required information\n"
	    "to establish a relation between\nthe inline/crossline system\n"
	    "and the coordinates.";

    // what coords would have been on the corners
    crd[0] = b2c.transform( hs.start_ );
    crd[1] = b2c.transform( hs.stop );
    crd[2] = b2c.transform( BinID(hs.start_.inl(),hs.stop.crl()) );

    return 0;
}



bool PosInfo::Detector::add( const PosInfo::CrdBidOffs& cbo )
{
    if ( !sortanal_ )
	return addNext( cbo );

    cbobuf_ += cbo;
    if ( !sortanal_->add(cbo.binid_) )
	return true;

    if ( setup_.reqsorting_ && sortanal_->errMsg().isSet() )
    {
        sortanal_->errMsg();
        return false;
    }

    return applySortAnal();
}


StepInterval<int> PosInfo::Detector::getRange( bool inldir ) const
{
    return inldir ? StepInterval<int>( start_.inl(), stop_.inl(), step_.inl() )
		  : StepInterval<int>( start_.crl(), stop_.crl(), step_.crl() );
}


bool PosInfo::Detector::applySortAnal()
{
    sorting_ = sortanal_->getSorting();
    delete sortanal_; sortanal_ = 0;
    allstd_ = sorting_.crlUpward();
    if ( !setup_.is2d_ )
	allstd_ = allstd_ && sorting_.inlUpward() && inlSorted();

    if ( cbobuf_.size() )
	addFirst( cbobuf_[0] );
    else
	return false;

    for ( int idx=1; idx<cbobuf_.size(); idx++ )
	addNext( cbobuf_[idx] );

    cbobuf_.erase();
    return true;
}


void PosInfo::Detector::addFirst( const PosInfo::CrdBidOffs& cbo )
{
    curusrcbo_ = cbo;
    setCur( cbo );
    firstcbo_ = prevcbo_ = curcbo_;
    llnstart_ = llnstop_ = curlnstart_ = curcbo_;
    mincoord_.x = maxcoord_.x = curcbo_.coord_.x;
    mincoord_.y = maxcoord_.y = curcbo_.coord_.y;
    if ( setup_.isps_ )
	offsrg_.start = offsrg_.stop = cbo.offset_;
    nrpos_ = nruniquepos_ = nroffsthispos_ = 1;
    addLine();
    errmsg_ = uiStrings::sEmptyString();
}


uiString PosInfo::Detector::createPositionString(
                        const PosInfo::CrdBidOffs& cbo ) const
{
    uiString ret = uiString(tr( "%1 %2%3" ))
        .arg( setup_.is2d_ ? tr("trace number") : tr("position") )
        .arg( cbo.binid_.toString(setup_.is2d_) )
        .arg( setup_.isps_ ? tr( " (offset %1)" ).arg( cbo.offset_ )
                           : uiStrings::sEmptyString() );
    return ret;
}


bool PosInfo::Detector::addNext( const PosInfo::CrdBidOffs& cbo )
{
    bool rv = true;

    if ( setup_.reqsorting_
      && !sorting_.isValid(prevusrcbo_.binid_,cbo.binid_) )
    {
	errmsg_ = tr("Sorting inconsistency at %1.\nLast valid sorting '%2'"
                     "\nThe previous position was %3")
	        .arg( createPositionString( cbo ) )
                .arg( sorting_.description())
                .arg( createPositionString( prevusrcbo_ ));
	rv = false;
    }

    setCur( cbo );
    addPos();

    if ( curcbo_.coord_.x < mincoord_.x ) mincoord_.x = curcbo_.coord_.x;
    if ( curcbo_.coord_.x > maxcoord_.x ) maxcoord_.x = curcbo_.coord_.x;
    if ( curcbo_.coord_.y < mincoord_.y ) mincoord_.y = curcbo_.coord_.y;
    if ( curcbo_.coord_.y > maxcoord_.y ) maxcoord_.y = curcbo_.coord_.y;
    if ( setup_.isps_ )
	offsrg_.include( cbo.offset_ );

    return rv;
}


#define mDefCurLineAndSegment \
    PosInfo::LineData& curline = *lds_[curline_]; \
    PosInfo::LineData::Segment& curseg = curline.segments_[curseg_]

void PosInfo::Detector::addLine()
{
    if ( curline_ > -1 )
    {
	double distsq = curlnstart_.coord_.sqDistTo( prevcbo_.coord_ );
	if ( distsq > llnstart_.coord_.sqDistTo( llnstop_.coord_ ) )
	    { llnstart_ = curlnstart_; llnstop_ = prevcbo_; }
    }

    lds_ += new PosInfo::LineData( curcbo_.binid_.inl() );
    curline_ = lds_.size() - 1;
    lds_[curline_]->segments_ += PosInfo::LineData::Segment(
				curcbo_.binid_.crl(), curcbo_.binid_.crl(), 0 );
    curseg_ = 0;
    curlnstart_ = curcbo_;
    nroffsthispos_ = 1;
}


void PosInfo::Detector::addPos()
{
    nrpos_++;
    if ( prevcbo_.binid_.inl() != curcbo_.binid_.inl() )
	{ addLine(); nruniquepos_++; return; }

    mDefCurLineAndSegment;
    int stp = curcbo_.binid_.crl() - prevcbo_.binid_.crl();
    if ( stp == 0 )
    {
	if ( setup_.isps_ )
	    nroffsthispos_++;
	else if ( mIsUdf(firstduppos_.binid_.inl()) )
	    firstduppos_ = curcbo_;
    }
    else
    {
	nruniquepos_++;
	if ( setup_.isps_ )
	{
	    if ( nroffsperpos_ < 1 )
		nroffsperpos_ = nroffsthispos_;
	    else if ( nroffsperpos_ != nroffsthispos_
		   && mIsUdf(firstaltnroffs_.binid_.inl()) )
		firstaltnroffs_ = curcbo_;
	    nroffsthispos_ = 1;
	}
	if ( curseg.step && stp != curseg.step )
	{
	    curline.segments_ += PosInfo::LineData::Segment(
				curcbo_.binid_.crl(), curcbo_.binid_.crl(), 0 );
	    curseg_++;
	}
	else
	{
	    curseg.stop = curcbo_.binid_.crl();
	    if ( curseg.step == 0 )
		curseg.step = curseg.stop - curseg.start;
	}
	if ( setup_.is2d_ )
	{
	    const float dist = (float) curcbo_.coord_.distTo( prevcbo_.coord_ );
	    if ( mIsUdf(distrg_.start) )
		distrg_.start = distrg_.stop = dist;
	    else
		distrg_.include( dist );
	    avgdist_ += dist;
	}
    }
}


void PosInfo::Detector::setCur( const PosInfo::CrdBidOffs& cbo )
{
    prevcbo_ = curcbo_;
    prevusrcbo_ = curusrcbo_;
    curusrcbo_ = cbo;
    curcbo_ = workCBO( curusrcbo_ );
}


void PosInfo::Detector::getBinIDRanges()
{
    Interval<int> inlrg( firstcbo_.binid_.inl(), firstcbo_.binid_.inl() );
    Interval<int> crlrg( firstcbo_.binid_.crl(), firstcbo_.binid_.crl() );
    int lnstep=mUdf(int), trcstep=mUdf(int);
    for ( int iln=0; iln<lds_.size(); iln++ )
    {
	const LineData& ld = *lds_[iln];
	inlrg.include( ld.linenr_ );
	if ( iln == 0 )
	    trcstep = ld.segments_[0].step;

	const int nrseg = ld.segments_.size();
	if ( nrseg > 1 ) crlirreg_ = true;
	for ( int iseg=0; iseg<nrseg; iseg++ )
	{
	    const LineData::Segment& seg = ld.segments_[iseg];
	    crlrg.include( seg.start );
	    crlrg.include( seg.stop );
	    if ( seg.step != trcstep )
		crlirreg_ = true;
	}

	if ( iln == 1 )
	    lnstep = ld.linenr_ - lds_[0]->linenr_;
	else if ( iln > 1 && ld.linenr_ - lds_[iln-1]->linenr_ != lnstep )
	    inlirreg_ = true;
    }

    if ( crlSorted() )
    {
	Swap( inlrg, crlrg );
	Swap( inlirreg_, crlirreg_ );
    }
    if ( !sorting_.inlUpward() )
    {
	inlrg.start = -inlrg.start; inlrg.stop = -inlrg.stop;
	Swap( inlrg.start, inlrg.stop );
    }
    if ( !sorting_.crlUpward() )
    {
	crlrg.start = -crlrg.start; crlrg.stop = -crlrg.stop;
	Swap( crlrg.start, crlrg.stop );
    }

    start_.inl() = inlrg.start; start_.crl() = crlrg.start;
    stop_.inl() = inlrg.stop; stop_.crl() = crlrg.stop;
}


int PosInfo::Detector::getStep( bool inldir ) const
{
    const bool swpd = crlSorted();
    const bool lndir = swpd ? !inldir : inldir;

    int stp = mUdf(int);
    for ( int iln=(lndir?1:0); iln<lds_.size(); iln++ )
    {
	const LineData& ld = *lds_[iln];
	if ( lndir )
	{
	    const int diff = ld.linenr_ - lds_[iln-1]->linenr_;
	    if ( diff < stp ) stp = diff;
	}
	else
	{
	    for ( int iseg=0; iseg<ld.segments_.size(); iseg++ )
	    {
		const int curstep = ld.segments_[iseg].step;
		if ( curstep && curstep < stp )
		    stp = ld.segments_[iseg].step;
	    }
	}
    }

    const bool revln = !(swpd ? sorting_.inlUpward() : sorting_.crlUpward());
    const bool revnr = !(swpd ? sorting_.crlUpward() : sorting_.inlUpward());
    const bool isrev = lndir ? revln : revnr;
    if ( mIsUdf(stp) ) stp = 1;
    return isrev ? -stp : stp;
}


PosInfo::CrdBidOffs PosInfo::Detector::workCBO(
			const PosInfo::CrdBidOffs& usrcbo ) const
{
    PosInfo::CrdBidOffs workcbo( usrcbo );
    if ( setup_.is2d_ ) workcbo.binid_.inl() = 1;
    if ( allstd_ ) return workcbo;

    if ( setup_.is2d_ )
	workcbo.binid_.crl() = -workcbo.binid_.crl();
    else
    {
	if ( !sorting_.inlUpward() )
	    workcbo.binid_.inl() = -workcbo.binid_.inl();
	if ( !sorting_.crlUpward() )
	    workcbo.binid_.crl() = -workcbo.binid_.crl();
	if ( !inlSorted() )
	    Swap( workcbo.binid_.inl(), workcbo.binid_.crl() );
    }

    return workcbo;
}


PosInfo::CrdBidOffs PosInfo::Detector::userCBO(
			const PosInfo::CrdBidOffs& internalcbo ) const
{
    if ( allstd_ ) return internalcbo;

    PosInfo::CrdBidOffs usrcbo( internalcbo );

    if ( setup_.is2d_ )
	usrcbo.binid_.crl() = -usrcbo.binid_.crl();
    else
    {
	if ( !inlSorted() )
	    Swap( usrcbo.binid_.inl(), usrcbo.binid_.crl() );
	if ( !sorting_.crlUpward() )
	    usrcbo.binid_.crl() = -usrcbo.binid_.crl();
	if ( !sorting_.inlUpward() )
	    usrcbo.binid_.inl() = -usrcbo.binid_.inl();
    }

    return usrcbo;
}


template <class T>
static BufferString getRangeStr( T start, T stop )
{
    BufferString ret;
    T diff = stop - start;
    if ( mIsZero(diff,0.0001) )
	{ ret += start; ret += " [all equal]"; }
    else
    {
	if ( diff < 0 ) diff = -diff;
	ret += start; ret += " - "; ret += stop;
	ret += " (d="; ret += diff; ret += ")";
    }
    return ret;
}


template <class T>
static BufferString getStepRangeStr( T start, T stop, T step )
{
    BufferString ret;
    T diff = stop - start;
    if ( diff < 0 ) diff = -diff;
    if ( diff == 0 )
	{ ret = start; ret += " [all equal]"; }
    else
    {
	ret = start; ret += " - "; ret += stop;
	ret += " [step="; ret += step; ret += "]";
    }
    return ret;
}


void PosInfo::Detector::report( IOPar& iop ) const
{
    if ( setup_.reqsorting_ )
    {
	BufferString sortdesc( errmsg_.getFullString() );
 
	if ( sortdesc.isEmpty() )
	     sortdesc = sorting_.description();


	 iop.add( "Sorting", sortdesc );
    }

    iop.set( "Total number of positions", nrpos_ );
    iop.set( "Number of unique positions", nruniquepos_ );
    iop.set( "X-Coordinate range", getRangeStr(mincoord_.x,maxcoord_.x) );
    iop.set( "Y-Coordinate range", getRangeStr(mincoord_.y,maxcoord_.y) );
    if ( setup_.is2d_ )
    {
	iop.set( "Trace numbers",
		 getStepRangeStr(start_.crl(),stop_.crl(),step_.crl()) );
	iop.set( setup_.isps_ ? "Distance range between gathers"
			: "Trace distance range", distrg_ );
	iop.set( setup_.isps_ ? "Average gather distance"
			: "Average trace distance", avgdist_ );
	iop.setYN( "Gaps in trace numbers", crlirreg_ );
    }
    else
    {
	iop.set( "Inlines",
		getStepRangeStr(start_.inl(),stop_.inl(),step_.inl()) );
	iop.set( "Crosslines",
		getStepRangeStr(start_.crl(),stop_.crl(),step_.crl()));
	iop.setYN( "Gaps in inlines", inlirreg_ );
	iop.setYN( "Gaps in crosslines", crlirreg_ );
    }
    if ( setup_.isps_ )
    {
	iop.set( "Offsets", getRangeStr(offsrg_.start,offsrg_.stop) );
	if ( mIsUdf(firstaltnroffs_.binid_.inl()) )
	    iop.set( "Number of traces per gather", nroffsperpos_ );
	else
	{
	    iop.setYN( "Varying traces/gather", true );
	    const char* varstr = "First different traces/gather at";
	    const CrdBidOffs fao( userCBO(firstaltnroffs_) );
	    if ( setup_.is2d_ )
		iop.set( varstr, fao.binid_.crl() );
	    else
		iop.set( varstr, fao.binid_.toString() );
	}
    }
    else
    {
	if ( mIsUdf(firstduppos_.binid_.inl()) )
	    iop.set( "Duplicate positions", "None" );
	else
	{
	    const char* fdupstr = "First duplicate position at";
	    const CrdBidOffs fdp( userCBO(firstduppos_) );
	    if ( setup_.is2d_ )
		iop.set( fdupstr, fdp.binid_.crl() );
	    else
		iop.set( fdupstr, fdp.binid_.toString() );
	}
    }
}
