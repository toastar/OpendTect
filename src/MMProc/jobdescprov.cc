/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "jobdescprov.h"

#include "cubesampling.h"
#include "iopar.h"
#include "keystrs.h"
#include "settings.h"
#include "survinfo.h"
#include "undefval.h"
#include "od_ostream.h"

const char* InlineSplitJobDescProv::sKeyMaxInlRg()  
    { return "Maximum Inline Range"; }
const char* InlineSplitJobDescProv::sKeyMaxCrlRg()
    { return "Maximum Crossline Range"; }


JobDescProv::JobDescProv( const IOPar& iop )
	: inpiopar_(*new IOPar(iop))
{
}


JobDescProv::~JobDescProv()
{
    delete &inpiopar_;
}


KeyReplaceJobDescProv::KeyReplaceJobDescProv( const IOPar& iop,
						const char* key, int nrjobs )
	: JobDescProv(iop)
	, key_(key)
	, nrjobs_(nrjobs)
{
}


void KeyReplaceJobDescProv::getJob( int jid, IOPar& iop ) const
{
    iop = inpiopar_;
    iop.set( key_, objName(jid) );
}


const char* KeyReplaceJobDescProv::objName( int jid ) const
{
    objnm_ = gtObjName( jid );
    return objnm_.buf();
}


void KeyReplaceJobDescProv::dump( od_ostream& strm ) const
{
    strm << "\nKey-replace JobDescProv dump.\n"
	    "The following jobs description keys are available:\n";
    for ( int idx=0; idx<nrjobs_; idx++ )
	strm << objName( idx ) << ' ';
    strm << od_endl;
}


StringKeyReplaceJobDescProv::StringKeyReplaceJobDescProv( const IOPar& iop,
			const char* key, const BufferStringSet& nms )
    : KeyReplaceJobDescProv(iop,key,nms.size())
{
}


const char* StringKeyReplaceJobDescProv::gtObjName( int jid ) const
{
    return names_.validIdx(jid) ? names_.get(jid).buf() : "";
}


IDKeyReplaceJobDescProv::IDKeyReplaceJobDescProv( const IOPar& iop,
				const char* ky, const StepInterval<int>& rg )
	: KeyReplaceJobDescProv(iop,ky,rg.nrSteps()+1)
    	, idrg_(rg)
{
}


const char* IDKeyReplaceJobDescProv::gtObjName( int jid ) const
{
    return toString( idrg_.atIndex(jid) );
}


void IDKeyReplaceJobDescProv::dump( od_ostream& strm ) const
{
    strm << "\nID Key-replace JobDescProv: "
	    << idrg_.start << '-' << idrg_.stop << " (step "
	    << idrg_.step << ", " << nrjobs_ << " jobs)." << od_endl;
}


#define mSetInlRgDef() \
    Interval<int> dum; SI().sampling(false).hrg.get( inlrg_, dum )


InlineSplitJobDescProv::InlineSplitJobDescProv( const IOPar& iop,
					     	const char* sk )
    	: JobDescProv(iop)
    	, singlekey_(sk)
    	, inls_(0)
	, ninlperjob_( 1 )
{
    mSetInlRgDef();
    getRange( inlrg_ );
    iop.get( "Nr of Inlines per Job", ninlperjob_ );
}


InlineSplitJobDescProv::InlineSplitJobDescProv( const IOPar& iop,
			const TypeSet<int>& in, const char* sk )
    	: JobDescProv(iop)
    	, singlekey_(sk)
    	, inls_(new TypeSet<int>(in))
	, ninlperjob_( 1 )
{
    mSetInlRgDef();
}


InlineSplitJobDescProv::~InlineSplitJobDescProv()
{
    delete inls_;
}


const BufferString& getOutSubSelKey()
{
    mDefineStaticLocalObject( const BufferString, outsubselkey, 
		    ( IOPar::compKey(sKey::Output(),sKey::Subsel()) ) );
    return outsubselkey;
}

#define mGetSubselKey(s) IOPar::compKey(getOutSubSelKey().buf(),sKey::s())

void InlineSplitJobDescProv::getRange( StepInterval<int>& rg ) const
{
    rg.step = 0;

    if ( *(const char*)singlekey_ )
	inpiopar_.get( singlekey_, rg );
    else
    {
	inpiopar_.get( mGetSubselKey(FirstInl), rg.start );
	inpiopar_.get( mGetSubselKey(LastInl), rg.stop );
	inpiopar_.get( mGetSubselKey(StepInl), rg.step );
    }

    if ( rg.step < 0 ) rg.step = -rg.step;
    if ( !rg.step ) rg.step = SI().inlStep();

    //if Subsel Type == None : init rg with SI()
    BufferString typestr;
    inpiopar_.get( mGetSubselKey(Type), typestr );
    if ( typestr==sKey::None() )
	rg = SI().inlRange(true);
				
    rg.sort();

    Interval<int> maxrg( Interval<int>().setFrom(rg) );
    inpiopar_.get( sKeyMaxInlRg(), maxrg );
    if ( !mIsUdf(maxrg.start) && rg.start < maxrg.start )
	rg.start = maxrg.start;
    if ( !mIsUdf(maxrg.stop) && rg.stop > maxrg.stop )
	rg.stop = maxrg.stop;
}


int InlineSplitJobDescProv::nrJobs() const
{
    if ( inls_ ) return inls_->size();

    int nrinl = inlrg_.nrSteps() + 1;

    int ret = nrinl / ninlperjob_; 
    if ( nrinl % ninlperjob_ ) ret += 1;
    
    return ret;
}


int InlineSplitJobDescProv::firstInlNr( int jid ) const
{
    return inls_ ? (*inls_)[jid]
	         : inlrg_.start + jid * inlrg_.step * ninlperjob_;
}


int InlineSplitJobDescProv::lastInlNr( int jid ) const
{
    return firstInlNr(jid) + inlrg_.step * (ninlperjob_ - 1);
}


void InlineSplitJobDescProv::getJob( int jid, IOPar& iop ) const
{
    //TODO: verify whether we still need to support singlekey_
    Interval<float> tmprg;
    const bool isfullrange = inpiopar_.get(mGetSubselKey(ZRange), tmprg);
    iop = inpiopar_;
    if ( *(const char*)singlekey_ )
	iop.set( singlekey_, firstInlNr(jid), lastInlNr(jid), inlrg_.step );
    else
    {
	iop.set( mGetSubselKey(Type), sKey::Range() );
	iop.set( mGetSubselKey(FirstInl), firstInlNr(jid) );
	iop.set( mGetSubselKey(LastInl), lastInlNr(jid) );
    }

    if ( !isfullrange )
    {
	iop.set( mGetSubselKey(FirstCrl), SI().crlRange(true).start );
	iop.set( mGetSubselKey(LastCrl), SI().crlRange(true).stop );
	iop.set( mGetSubselKey(StepCrl), SI().crlStep() );
	iop.set( mGetSubselKey(ZRange), SI().zRange(true) );
    }
}


const char* InlineSplitJobDescProv::objName( int jid ) const
{
    const int firstinl = firstInlNr( jid );
    const int lastinl = lastInlNr( jid );
    objnm_ = ""; objnm_ += firstinl;
    if ( lastinl > firstinl )
	{ objnm_ += "-"; objnm_ += lastinl; }

    return objnm_.buf();
}


void InlineSplitJobDescProv::dump( od_ostream& strm ) const
{
    strm << "\nInline-split JobDescProv dump.\n";
    if ( !inls_ )
	strm << "Inline range: " << inlrg_.start << '-' << inlrg_.stop
		<< " / " << inlrg_.step;
    else
    {
	strm << "The following inlines are requested:\n";
	for ( int idx=0; idx<inls_->size(); idx++ )
	    strm << (*inls_)[idx] << ' ';
    }
    strm << od_endl;
}


#define mMMKey		"MultiMachine"
#define mNrInlPerJobKey	"Nr inline per job"

int InlineSplitJobDescProv::defaultNrInlPerJob()
{
    int nrinljob = 10;
    Settings::common().get( IOPar::compKey(mMMKey,mNrInlPerJobKey), nrinljob );
    return nrinljob;
}


void InlineSplitJobDescProv::setDefaultNrInlPerJob( int nr )
{
    if ( nr <= 0 ) return;

    Settings::common().set( IOPar::compKey(mMMKey,mNrInlPerJobKey), nr );
    Settings::common().write();
}
