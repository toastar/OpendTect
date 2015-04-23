/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H.Payraudeau
 Date:          04/2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "attribengman.h"

#include "arrayndimpl.h"
#include "attribdatacubes.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attriboutput.h"
#include "attribparam.h"
#include "attribprocessor.h"
#include "attribprovider.h"
#include "attribstorprovider.h"

#include "binidvalset.h"
#include "trckeyzsampling.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "linesetposinfo.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "seis2ddata.h"
#include "seistrc.h"
#include "separstr.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"
#include "uistrings.h"

namespace Attrib
{

EngineMan::EngineMan()
    : inpattrset_(0)
    , procattrset_(0)
    , nlamodel_(0)
    , tkzs_(*new TrcKeyZSampling)
    , geomid_(Survey::GM().cUndefGeomID())
    , cache_(0)
    , udfval_(mUdf(float))
    , curattridx_(0)
{
}


EngineMan::~EngineMan()
{
    delete procattrset_;
    delete inpattrset_;
    delete nlamodel_;
    delete &tkzs_;
    if ( cache_ ) cache_->unRef();
}


void EngineMan::getPossibleVolume( DescSet& attribset, TrcKeyZSampling& cs,
				   const char* linename, const DescID& outid )
{
    TypeSet<DescID> desiredids(1,outid);

    uiString errmsg;
    DescID evalid = createEvaluateADS( attribset, desiredids, errmsg );
    PtrMan<Processor> proc =
			createProcessor( attribset, linename, evalid, errmsg );
    if ( !proc ) return;

    proc->computeAndSetRefZStepAndZ0();
    proc->getProvider()->setDesiredVolume( cs );
    proc->getProvider()->getPossibleVolume( -1, cs );
}


Processor* EngineMan::usePar( const IOPar& iopar, DescSet& attribset,
			      const char* linename, uiString& errmsg )
{
    int outputidx = 0;
    TypeSet<DescID> ids;
    while ( true )
    {
	BufferString outpstr = IOPar::compKey( sKey::Output(), outputidx );
	PtrMan<IOPar> outputpar = iopar.subselect( outpstr );
	if ( !outputpar )
	{
	    if ( !outputidx )
	    { outputidx++; continue; }
	    else
		break;
	}

	int attribidx = 0;
	while ( true )
	{
	    BufferString attribidstr =
			IOPar::compKey( sKey::Attributes(), attribidx );
	    int attribid;
	    if ( !outputpar->get(attribidstr,attribid) )
		break;

	    ids += DescID(attribid,false);
	    attribidx++;
	}

	outputidx++;
    }

    DescID evalid = createEvaluateADS( attribset, ids, errmsg );
    Processor* proc = createProcessor( attribset, linename, evalid, errmsg );
    if ( !proc ) return 0;

    for ( int idx=1; idx<ids.size(); idx++ )
	proc->addOutputInterest(idx);

    PtrMan<IOPar> outpar =
	iopar.subselect( IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    if ( !outpar || !tkzs_.usePar(*outpar) )
    {
	if ( !attribset.is2D() )
	    tkzs_.init();
	else
	{
	    // doesn't make much sense, but is better than nothing
	    tkzs_.set2DDef();

	    tkzs_.hrg.start.inl() = tkzs_.hrg.stop.inl() = 0;
	    Pos::GeomID geomid = Survey::GM().getGeomID( linename );
	    if ( outpar && outpar->hasKey(sKey::TrcRange()) )
	    {
		StepInterval<int> trcrg( 0, 0, 1 );
		outpar->get( sKey::TrcRange(), trcrg );
		tkzs_.hrg.setCrlRange( trcrg );
		outpar->get( sKey::ZRange(), tkzs_.zsamp_ );
	    }
	    else
	    {
		mDynamicCastGet( const Survey::Geometry2D*, geom2d,
				 Survey::GM().getGeometry(geomid) );
		if ( geom2d )
		{
		    tkzs_.hrg.setCrlRange( geom2d->data().trcNrRange() );
		    tkzs_.zsamp_ = geom2d->data().zRange();
		}
	    }
	}
    }

    //get attrib name from user reference for backward compatibility with 3.2.2
    const Attrib::Desc* curdesc = attribset.getDesc( ids[0] );
    BufferString attribname = curdesc->isStored() ? "" : curdesc->userRef();
    LineKey lkey( linename, attribname );

    SeisTrcStorOutput* storeoutp = createOutput( iopar, lkey, errmsg );
    if ( !storeoutp ) return 0;

    bool exttrctosi;
    BufferString basekey = IOPar::compKey( "Output",0 );
    if ( iopar.getYN( IOPar::compKey( basekey,SeisTrc::sKeyExtTrcToSI() ),
		      exttrctosi) )
	storeoutp->setTrcGrow( exttrctosi );

    BufferStringSet outnms;
    for ( int idx=0; idx<ids.size(); idx++ )
	outnms += new BufferString( attribset.getDesc( ids[idx] )->userRef() );

    storeoutp->setOutpNames( outnms );
    proc->addOutput( storeoutp );
    return proc;
}


Processor* EngineMan::createProcessor( const DescSet& attribset,
				       const char* linename,const DescID& outid,
				       uiString& errmsg )
{
    Desc* targetdesc = const_cast<Desc*>(attribset.getDesc(outid));
    if ( !targetdesc ) return 0;

    Processor* processor = new Processor( *targetdesc, linename, errmsg );
    if ( !processor->isOK() )
    {
	delete processor;
	return 0;
    }

    processor->addOutputInterest( targetdesc->selectedOutput() );
    return processor;
}


void EngineMan::setExecutorName( Executor* ex )
{
    if ( !ex ) return;

    BufferString usernm( getCurUserRef() );
    if ( usernm.isEmpty() || !inpattrset_ ) return;

    if ( curattridx_ < 0 || curattridx_ >= attrspecs_.size() )
	ex->setName( "Processing attribute" );

    SelSpec& ss = attrspecs_[curattridx_];
    BufferString nm( "Calculating " );
    if ( ss.isNLA() && nlamodel_ )
    {
	nm = "Applying ";
	nm += nlamodel_->nlaType(true);
	nm += ": calculating";
	if ( IOObj::isKey(usernm) )
	    usernm = IOM().nameOf( usernm );
    }
    else
    {
	const Desc* desc = inpattrset_->getDesc( ss.id() );
	if ( desc && desc->isStored() )
	    nm = "Reading from";
    }

    nm += " \"";
    nm += usernm;
    nm += "\"";

    ex->setName( nm );
}


SeisTrcStorOutput* EngineMan::createOutput( const IOPar& pars,
					    const LineKey& lkey,
					    uiString& errmsg )
{
    const FixedString typestr =
		pars.find( IOPar::compKey(sKey::Output(),sKey::Type()) );
    if ( typestr==sKey::Cube() )
    {
	SeisTrcStorOutput* outp = new SeisTrcStorOutput( tkzs_,
				    Survey::GM().getGeomID(lkey.lineName()) );
	outp->setGeometry(tkzs_);
	const bool res = outp->doUsePar( pars );
	if ( !res ) { errmsg = outp->errMsg(); delete outp; outp = 0; }
	return outp;
    }

    return 0;
}


void EngineMan::setNLAModel( const NLAModel* m )
{
    delete nlamodel_;
    nlamodel_ = m ? m->clone() : 0;
}


void EngineMan::setAttribSet( const DescSet* ads )
{
    delete inpattrset_;
    inpattrset_ = ads ? new DescSet( *ads ) : 0;
}


const char* EngineMan::getCurUserRef() const
{
    const int idx = curattridx_;
    if ( attrspecs_.isEmpty() || !attrspecs_[idx].id().isValid() ) return "";

    SelSpec& ss = const_cast<EngineMan*>(this)->attrspecs_[idx];
    if ( attrspecs_[idx].isNLA() )
    {
	if ( !nlamodel_ ) return "";
	ss.setRefFromID( *nlamodel_ );
    }
    else
    {
	if ( !inpattrset_ ) return "";
	ss.setRefFromID( *inpattrset_ );
    }
    return attrspecs_[idx].userRef();
}


const DataCubes* EngineMan::getDataCubesOutput( const Processor& proc )
{
    if ( proc.outputs_.size()==1 && !cache_ )
	return proc.outputs_[0]->getDataCubes();

    ObjectSet<const DataCubes> cubeset;
    for ( int idx=0; idx<proc.outputs_.size(); idx++ )
    {
	if ( !proc.outputs_[idx] || !proc.outputs_[idx]->getDataCubes() )
	    continue;

	const DataCubes* dc = proc.outputs_[idx]->getDataCubes();
	dc->ref();
	if ( cubeset.size() && cubeset[0]->nrCubes()!=dc->nrCubes() )
	{
	    dc->unRef();
	    continue;
	}

	cubeset += dc;
    }

    if ( cache_ )
    {
	cubeset += cache_;
	cache_->ref();
    }

    const DataCubes* output =
	!cubeset.isEmpty() ? getDataCubesOutput( cubeset ) : 0;
    deepUnRef( cubeset );
    return output;
}


const DataCubes* EngineMan::getDataCubesOutput(
				const ObjectSet<const DataCubes>& cubeset )
{
    DataCubes* output = new DataCubes;
    output->ref();
    if ( cache_ && cache_->cubeSampling().zsamp_.step != tkzs_.zsamp_.step )
    {
	TrcKeyZSampling cswithcachestep = tkzs_;
	cswithcachestep.zsamp_.step = cache_->cubeSampling().zsamp_.step;
	output->setSizeAndPos(cswithcachestep);
    }
    else
	output->setSizeAndPos(tkzs_);

    for ( int idx=0; idx<cubeset[0]->nrCubes(); idx++ )
	output->addCube(mUdf(float));

    for ( int iset=0; iset<cubeset.size(); iset++ )
    {
	const DataCubes& cubedata = *cubeset[iset];
	for ( int iinl=cubedata.getInlSz()-1; iinl>=0; iinl-- )
	{
	    const int inl = cubedata.inlsampling_.atIndex(iinl);
	    const int inlidx = output->inlsampling_.nearestIndex(inl);
	    if ( inlidx<0 || inlidx>=output->getInlSz() )
		continue;

	    for ( int icrl=cubedata.getCrlSz()-1; icrl>=0; icrl-- )
	    {
		const int crl = cubedata.crlsampling_.atIndex(icrl);
		const int crlidx = output->crlsampling_.nearestIndex(crl);
		if ( crlidx<0 || crlidx>=output->getCrlSz() )
		    continue;

		for ( int iz=cubedata.getZSz()-1; iz>=0; iz-- )
		{
		    const float inpfidx = cubedata.z0_ + iz;
		    const float outfidx = inpfidx - output->z0_;

		    if ( outfidx<0 || outfidx>output->getZSz()-1 )
			continue;

		    for ( int cubeidx=output->nrCubes()-1;cubeidx>=0;cubeidx--)
		    {
			const float val =
			    cubedata.getCube(cubeidx).get( iinl, icrl, iz );

			if ( Values::isUdf( val ) )
			    continue;

			const int outzsampidx = (int)Math::Floor( outfidx );
			output->setValue( cubeidx, inlidx, crlidx,
					  outzsampidx, val );
		    }
		}
	    }
	}
    }

    output->unRefNoDelete();
    return output;
}


void EngineMan::setAttribSpecs( const TypeSet<SelSpec>& specs )
{ attrspecs_ = specs; }


void EngineMan::setAttribSpec( const SelSpec& spec )
{
    attrspecs_.erase();
    attrspecs_ += spec;
}


void EngineMan::setTrcKeyZSampling( const TrcKeyZSampling& newcs )
{
    tkzs_ = newcs;
    tkzs_.normalise();
}


DescSet* EngineMan::createNLAADS( DescID& nladescid, uiString& errmsg,
				  const DescSet* addtoset )
{
    if ( attrspecs_.isEmpty() ) return 0;
    DescSet* descset = addtoset ? new DescSet( *addtoset )
				: new DescSet( attrspecs_[0].is2D() );

    if ( !addtoset && nlamodel_ && !descset->usePar(nlamodel_->pars()) )
    {
	errmsg = descset->errMsg();
	delete descset;
	return 0;
    }

    BufferString s;
    nlamodel_->dump(s);
    BufferString defstr( nlamodel_->nlaType(true) );
    defstr += " specification=\""; defstr += s; defstr += "\"";

    addNLADesc( defstr, nladescid, *descset, attrspecs_[0].id().asInt(),
		nlamodel_, errmsg );

    DescSet* cleanset = descset->optimizeClone( nladescid );
    delete descset;
    return cleanset;
}


void EngineMan::addNLADesc( const char* specstr, DescID& nladescid,
			    DescSet& descset, int outputnr,
			    const NLAModel* nlamdl, uiString& errmsg )
{
    RefMan<Desc> desc = PF().createDescCopy( "NN" );
    desc->setDescSet( &descset );

    if ( !desc->parseDefStr(specstr) )
    {
	errmsg = tr("Invalid definition string for NLA model:\n%1")
		    .arg( specstr );
	return;
    }
    desc->setHidden( true );

    // Need to make a Provider because the inputs and outputs may
    // not be known otherwise
    errmsg = Provider::prepare( *desc );
    if ( !errmsg.isEmpty() )
	{ return; }

    const int nrinputs = desc->nrInputs();
    for ( int idx=0; idx<nrinputs; idx++ )
    {
	const char* inpname = desc->inputSpec(idx).getDesc();
	DescID descid = descset.getID( inpname, true );
	if ( !descid.isValid() && IOObj::isKey(inpname) )
	{
	    descid = descset.getID( inpname, false );
	    if ( !descid.isValid() )
	    {
		// It could be 'storage', but it's not yet in the set ...
		PtrMan<IOObj> ioobj = IOM().get( MultiID(inpname) );
		if ( ioobj )
		{
		    Desc* stordesc =
			PF().createDescCopy( StorageProvider::attribName() );
		    stordesc->setDescSet( &descset );
		    ValParam* idpar =
			stordesc->getValParam( StorageProvider::keyStr() );
		    idpar->setValue( inpname );
		    stordesc->setUserRef( ioobj->name() );
		    descid = descset.addDesc( stordesc );
		    if ( !descid.isValid() )
		    {
			errmsg = tr("NLA input '%1' cannot be found in "
				    "the provided set.").arg( inpname );
			return;
		    }
		}
	    }
	}

	desc->setInput( idx, descset.getDesc(descid) );
    }

    if ( outputnr > desc->nrOutputs() )
    {
	errmsg = tr("Output %1 not present.").arg( toString(outputnr) );
	return;
    }

    const NLADesign& nlades = nlamdl->design();
    desc->setUserRef( *nlades.outputs[outputnr] );
    desc->selectOutput( outputnr );

    nladescid = descset.addDesc( desc );
    if ( nladescid == DescID::undef() )
	errmsg = descset.errMsg();
}


DescID EngineMan::createEvaluateADS( DescSet& descset,
				     const TypeSet<DescID>& outids,
				     uiString& errmsg )
{
    if ( outids.isEmpty() ) return DescID::undef();
    if ( outids.size() == 1 ) return outids[0];

    Desc* desc = PF().createDescCopy( "Evaluate" );
    if ( !desc ) return DescID::undef();

    desc->setDescSet( &descset );
    desc->setNrOutputs( Seis::UnknowData, outids.size() );

    desc->setHidden( true );

    const int nrinputs = outids.size();
    for ( int idx=0; idx<nrinputs; idx++ )
    {
	desc->addInput( InputSpec("Data",true) );
	Desc* inpdesc = descset.getDesc( outids[idx] );
	if ( !inpdesc ) continue;

	desc->setInput( idx, inpdesc );
    }

    desc->setUserRef( "evaluate attributes" );
    desc->selectOutput(0);

    DescID evaldescid = descset.addDesc( desc );
    if ( evaldescid == DescID::undef() )
    {
	errmsg = descset.errMsg();
	desc->unRef();
    }

    return evaldescid;
}


#define mStepEps 1e-3


Processor* EngineMan::createScreenOutput2D( uiString& errmsg,
					    Data2DHolder& output )
{
    Processor* proc = getProcessor( errmsg );
    if ( !proc )
	return 0;

    Interval<int> trcrg( tkzs_.hrg.start.crl(), tkzs_.hrg.stop.crl() );
    Interval<float> zrg( tkzs_.zsamp_.start, tkzs_.zsamp_.stop );

    TwoDOutput* attrout = new TwoDOutput( trcrg, zrg, geomid_ );
    attrout->setOutput( output );
    proc->addOutput( attrout );

    return proc;
}

#define mRg(dir) (cachecs.dir##rg)

Processor* EngineMan::createDataCubesOutput( uiString& errmsg,
					    const DataCubes* prev )
{
    if ( cache_ )
    {
	cache_->unRef();
	cache_ = 0;
    }


    if ( tkzs_.isEmpty() )
	prev = 0;
    else if ( prev )
    {
	cache_ = prev;
	cache_->ref();
	const TrcKeyZSampling cachecs = cache_->cubeSampling();
	if ( mRg(h).step != tkzs_.hrg.step
	  || (mRg(h).start.inl() - tkzs_.hrg.start.inl()) % tkzs_.hrg.step.inl()
	  || (mRg(h).start.crl() - tkzs_.hrg.start.crl()) % tkzs_.hrg.step.crl()
	  || mRg(h).start.inl() > tkzs_.hrg.stop.inl()
	  || mRg(h).stop.inl() < tkzs_.hrg.start.inl()
	  || mRg(h).start.crl() > tkzs_.hrg.stop.crl()
	  || mRg(h).stop.crl() < tkzs_.hrg.start.crl()
	  || mRg(z).start > tkzs_.zsamp_.stop + mStepEps*tkzs_.zsamp_.step
	  || mRg(z).stop < tkzs_.zsamp_.start - mStepEps*tkzs_.zsamp_.step )
	    // No overlap, gotta crunch all the numbers ...
	{
	    cache_->unRef();
	    cache_ = 0;
	}
    }

#define mAddAttrOut(todocs) \
{ \
    DataCubesOutput* attrout = new DataCubesOutput(todocs); \
    attrout->setGeometry( todocs ); \
    attrout->setUndefValue( udfval_ ); \
    proc->addOutput( attrout ); \
}

    Processor* proc = getProcessor(errmsg);
    if ( !proc )
	return 0;

    if ( !cache_ )
	mAddAttrOut( tkzs_ )
    else
    {
	const TrcKeyZSampling cachecs = cache_->cubeSampling();
	TrcKeyZSampling todocs( tkzs_ );
	if ( mRg(h).start.inl() > tkzs_.hrg.start.inl() )
	{
	    todocs.hrg.stop.inl() = mRg(h).start.inl() - tkzs_.hrg.step.inl();
	    mAddAttrOut( todocs )
	}

	if ( mRg(h).stop.inl() < tkzs_.hrg.stop.inl() )
	{
	    todocs = tkzs_;
	    todocs.hrg.start.inl() = mRg(h).stop.inl() + tkzs_.hrg.step.inl();
	    mAddAttrOut( todocs )
	}

	const int startinl = mMAX(tkzs_.hrg.start.inl(), mRg(h).start.inl() );
	const int stopinl = mMIN( tkzs_.hrg.stop.inl(), mRg(h).stop.inl() );

	if ( mRg(h).start.crl() > tkzs_.hrg.start.crl() )
	{
	    todocs = tkzs_;
	    todocs.hrg.start.inl() = startinl; todocs.hrg.stop.inl() = stopinl;
	    todocs.hrg.stop.crl() = mRg(h).start.crl() - tkzs_.hrg.step.crl();
	    mAddAttrOut( todocs )
	}

	if ( mRg(h).stop.crl() < tkzs_.hrg.stop.crl() )
	{
	    todocs = tkzs_;
	    todocs.hrg.start.inl() = startinl; todocs.hrg.stop.inl() = stopinl;
	    todocs.hrg.start.crl() = mRg(h).stop.crl() + tkzs_.hrg.step.crl();
	    mAddAttrOut( todocs )
	}

	todocs = tkzs_;
	todocs.hrg.start.inl() = startinl; todocs.hrg.stop.inl() = stopinl;
	todocs.hrg.start.crl() = mMAX( tkzs_.hrg.start.crl(),
				       mRg(h).start.crl() );
	todocs.hrg.stop.crl() = mMIN( tkzs_.hrg.stop.crl(), mRg(h).stop.crl() );

	if ( mRg(z).start > tkzs_.zsamp_.start + mStepEps*tkzs_.zsamp_.step )
	{
	    todocs.zsamp_.stop = mMAX( mRg(z).start-tkzs_.zsamp_.step,
		    		       todocs.zsamp_.start );
	    mAddAttrOut( todocs )
	}

	if ( mRg(z).stop < tkzs_.zsamp_.stop - mStepEps*tkzs_.zsamp_.step )
	{
	    todocs.zsamp_ = tkzs_.zsamp_;
	    todocs.zsamp_.start = mMIN( mRg(z).stop+tkzs_.zsamp_.step,
		    			todocs.zsamp_.stop );
	    mAddAttrOut( todocs )
	}
    }

    if ( tkzs_.isFlat() && tkzs_.defaultDir() != TrcKeyZSampling::Z )
    {
	TypeSet<BinID> positions;
	if ( tkzs_.defaultDir() == TrcKeyZSampling::Inl )
	    for ( int idx=0; idx<tkzs_.nrCrl(); idx++ )
		positions += BinID( tkzs_.hrg.start.inl(),
				    tkzs_.hrg.start.crl() +
					tkzs_.hrg.step.crl()*idx );
	if ( tkzs_.defaultDir() == TrcKeyZSampling::Crl )
	    for ( int idx=0; idx<tkzs_.nrInl(); idx++ )
		positions += BinID( tkzs_.hrg.start.inl() +
				    tkzs_.hrg.step.inl()*idx,
				    tkzs_.hrg.start.crl() );

	proc->setRdmPaths( &positions, &positions );
    }

    return proc;
}


class AEMFeatureExtracter : public Executor
{ mODTextTranslationClass(AEMFeatureExtracter);
public:
AEMFeatureExtracter( EngineMan& aem, const BufferStringSet& inputs,
		     const ObjectSet<BinIDValueSet>& bivsets )
    : Executor("Extracting attributes")
{
    const DescSet* attrset =
	aem.procattrset_ ? aem.procattrset_ : aem.inpattrset_;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	const DescID id = attrset->getID( inputs.get(idx), true );
	if ( id == DescID::undef() )
	    continue;
	SelSpec ss( 0, id );
	ss.setRefFromID( *attrset );
	aem.attrspecs_ += ss;
    }

    ObjectSet<BinIDValueSet>& bvs =
	const_cast<ObjectSet<BinIDValueSet>&>(bivsets);

    proc_ = aem.createLocationOutput( errmsg_, bvs );
}

~AEMFeatureExtracter()		{ delete proc_; }

od_int64 totalNr() const	{ return proc_ ? proc_->totalNr() : -1; }
od_int64 nrDone() const 	{ return proc_ ? proc_->nrDone() : 0; }
uiString uiNrDoneText() const
{
    return proc_ ? proc_->uiNrDoneText() : uiStrings::sEmptyString();
}

uiString uiMessage() const
{
    return !errmsg_.isEmpty()
	? errmsg_
	: (proc_ ? proc_->uiMessage() : tr("Cannot create output") );
}

int haveError( const uiString& msg )
{
    if ( !msg.isEmpty() ) errmsg_ = msg;
    return -1;
}

int nextStep()
{
    if ( !proc_ ) return haveError( uiStrings::sEmptyString() );

    int rv = proc_->doStep();
    if ( rv >= 0 ) return rv;
    return haveError( proc_->uiMessage() );
}

    uiString			errmsg_;
    Processor*			proc_;
    TypeSet<DescID>		outattribs_;
};


Executor* EngineMan::createFeatureOutput( const BufferStringSet& inputs,
				    const ObjectSet<BinIDValueSet>& bivsets )
{
    return new AEMFeatureExtracter( *this, inputs, bivsets );
}


void EngineMan::computeIntersect2D( ObjectSet<BinIDValueSet>& bivsets ) const
{
    if ( !procattrset_ || !attrspecs_.size() )
	return;

    if ( !procattrset_->is2D() )
	return;

    Desc* storeddesc = 0;
    for ( int idx=0; idx<attrspecs_.size(); idx++ )
    {
	const Desc* desc = procattrset_->getDesc( attrspecs_[idx].id() );
	if ( !desc ) continue;
	if ( desc->isStored() )
	{
	    storeddesc = const_cast<Desc*>(desc);
	    break;
	}
	else
	{
	    Desc* candidate = desc->getStoredInput();
	    if ( candidate )
	    {
		storeddesc = candidate;
		break;
	    }
	}
    }

    if ( !storeddesc )
	return;

    const LineKey lk( storeddesc->getValParam(
			StorageProvider::keyStr())->getStringValue(0) );
    const MultiID key( lk.lineName() );
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj ) return;

    const Seis2DDataSet dset( *ioobj );
    PosInfo::LineSet2DData linesetgeom;
    for ( int idx=0; idx<dset.nrLines(); idx++ )
    {
	PosInfo::Line2DData& linegeom = linesetgeom.addLine(dset.lineName(idx));
	Pos::GeomID geomid = Survey::GM().getGeomID( dset.lineName(idx) );
	const Survey::Geometry* geometry = Survey::GM().getGeometry( geomid );
	mDynamicCastGet( const Survey::Geometry2D*, geom2d, geometry )
	if ( geom2d )
	    linegeom = geom2d->data();
	if ( linegeom.positions().isEmpty() )
	{
	    linesetgeom.removeLine( dset.lineName(idx) );
	    continue;
	}
    }

    ObjectSet<BinIDValueSet> newbivsets;
    for ( int idx=0; idx<bivsets.size(); idx++ )
    {
	BinIDValueSet* newset = new BinIDValueSet(bivsets[idx]->nrVals(), true);
	ObjectSet<PosInfo::LineSet2DData::IR> resultset;
	linesetgeom.intersect( *bivsets[idx], resultset );

	for ( int idy=0; idy<resultset.size(); idy++)
	    newset->append(*resultset[idy]->posns_);

	newbivsets += newset;
    }
    bivsets = newbivsets;
}


Processor* EngineMan::createLocationOutput( uiString& errmsg,
					    ObjectSet<BinIDValueSet>& bidzvset )
{
    if ( bidzvset.size() == 0 ) return 0;

    Processor* proc = getProcessor(errmsg);
    if ( !proc )
	return 0;

    computeIntersect2D(bidzvset);
    ObjectSet<LocationOutput> outputs;
    for ( int idx=0; idx<bidzvset.size(); idx++ )
    {
	BinIDValueSet& bidzvs = *bidzvset[idx];
	LocationOutput* attrout = new LocationOutput( bidzvs );
	outputs += attrout;
    }

    if ( !outputs.size() )
        return 0;

    for ( int idx=0; idx<outputs.size(); idx++ )
	proc->addOutput( outputs[idx] );

    return proc;
}


//TODO TableOutput should later on replace at least Location- and TrcSel-Output
//AEMFeatureExtractor will also be replaced by AEMTableExtractor
class AEMTableExtractor : public Executor
{ mODTextTranslationClass(AEMTableExtractor);
public:
AEMTableExtractor( EngineMan& aem, DataPointSet& datapointset,
		   const Attrib::DescSet& descset, int firstcol )
    : Executor("Extracting attributes")
{
    const int nrinps = datapointset.nrCols();
    for ( int idx=0; idx<nrinps; idx++ )
    {
	FileMultiString fms( datapointset.colDef(idx).ref_ );
	if ( fms.size() < 2 )
	    continue;
	const DescID did( fms.getIValue(1), descset.containsStoredDescOnly() );
	if ( did == DescID::undef() )
	    continue;
	SelSpec ss( 0, did );
	ss.setRefFromID( descset );
	aem.attrspecs_.addIfNew( ss );
    }

    proc_ = aem.getTableOutExecutor( datapointset, errmsg_, firstcol );
}

~AEMTableExtractor()		{ delete proc_; }

od_int64 totalNr() const	{ return proc_ ? proc_->totalNr() : -1; }
od_int64 nrDone() const 	{ return proc_ ? proc_->nrDone() : 0; }
uiString uiNrDoneText() const
{
    return proc_ ? proc_->uiNrDoneText() : uiStrings::sEmptyString();
}

uiString uiMessage() const
{
    return !errmsg_.isEmpty()
	? errmsg_
	: (proc_ ? proc_->Task::uiMessage() : tr("Cannot create output"));
}

int haveError( const uiString& msg )
{
    if ( !msg.isEmpty() ) errmsg_ = msg;
    return -1;
}

int nextStep()
{
    if ( !proc_ ) return haveError( uiStrings::sEmptyString() );

    int rv = proc_->doStep();
    if ( rv >= 0 ) return rv;
    return haveError( proc_->uiMessage() );
}

    uiString			errmsg_;
    Processor*			proc_;
    TypeSet<DescID>		outattribs_;
};


Executor* EngineMan::getTableExtractor( DataPointSet& datapointset,
					const Attrib::DescSet& descset,
					uiString& errmsg, int firstcol,
					bool needprep )
{
    if ( needprep && !ensureDPSAndADSPrepared( datapointset, descset, errmsg ) )
	return 0;

    setAttribSet( &descset );
    AEMTableExtractor* tabex = new AEMTableExtractor( *this, datapointset,
						      descset, firstcol );
    if ( tabex && !tabex->errmsg_.isEmpty() )
	errmsg = tabex->errmsg_.getFullString();
    return tabex;
}


Processor* EngineMan::getTableOutExecutor( DataPointSet& datapointset,
					   uiString& errmsg, int firstcol )
{
    if ( !datapointset.size() ) return 0;

    Processor* proc = getProcessor(errmsg);
    if ( !proc )
	return 0;

    ObjectSet<BinIDValueSet> bidsets;
    bidsets += &datapointset.bivSet();
    computeIntersect2D( bidsets );
    TableOutput* tableout = new TableOutput( datapointset, firstcol );
    if ( !tableout ) return 0;

    proc->addOutput( tableout );

    return proc;
}


#define mErrRet(s) { errmsg = s; return 0; }

Processor* EngineMan::getProcessor( uiString& errmsg )
{
    if ( procattrset_ )
	{ delete procattrset_; procattrset_ = 0; }

    if ( !inpattrset_ || !attrspecs_.size() )
	mErrRet( tr("No attribute set or input specs") )

    TypeSet<DescID> outattribs;
    for ( int idx=0; idx<attrspecs_.size(); idx++ )
	outattribs += attrspecs_[idx].id();

    DescID outid = outattribs[0];

    errmsg = uiStrings::sEmptyString();
    bool doeval = false;
    if ( !attrspecs_[0].isNLA() )
    {
	procattrset_ = inpattrset_->optimizeClone( outattribs );
	if ( !procattrset_ ) mErrRet(tr("Attribute set not valid"));

	if ( outattribs.size() > 1 )
	{
	    doeval = true;
	    outid = createEvaluateADS( *procattrset_, outattribs, errmsg);
	}
    }
    else
    {
	DescID nlaid( SelSpec::cNoAttrib() );
	procattrset_ = createNLAADS( nlaid, errmsg );
	if ( !errmsg.isEmpty() )
	    mErrRet(errmsg)
	outid = nlaid;
    }

    Processor* proc = createProcessor(*procattrset_,
			Survey::GM().getName(geomid_), outid, errmsg);
    setExecutorName( proc );
    if ( !proc )
	mErrRet( errmsg )

    if ( doeval )
    {
	for ( int idx=1; idx<attrspecs_.size(); idx++ )
	    proc->addOutputInterest(idx);
    }

    if ( proc->getProvider() )
	proc->getProvider()->setGeomID( geomid_ );

    return proc;
}


Processor* EngineMan::createTrcSelOutput( uiString& errmsg,
					  const BinIDValueSet& bidvalset,
					  SeisTrcBuf& output, float outval,
					  Interval<float>* cubezbounds,
					  TypeSet<BinID>* trueknotspos,
					  TypeSet<BinID>* snappedpos )
{
    Processor* proc = getProcessor(errmsg);
    if ( !proc )
	return 0;

    TrcSelectionOutput* attrout	= new TrcSelectionOutput( bidvalset, outval );
    attrout->setOutput( &output );
    if ( cubezbounds )
	attrout->setTrcsBounds( *cubezbounds );
    attrout->setGeomID( geomid_ );

    proc->addOutput( attrout );
    proc->setRdmPaths( trueknotspos, snappedpos );

    return proc;
}


Processor* EngineMan::create2DVarZOutput( uiString& errmsg,
					  const IOPar& pars,
					  DataPointSet* datapointset,
					  float outval,
					  Interval<float>* cubezbounds )
{
    Processor* proc = getProcessor( errmsg );
    if ( !proc ) return 0;

    Trc2DVarZStorOutput* attrout = new Trc2DVarZStorOutput( geomid_,
							datapointset, outval );
    attrout->doUsePar( pars );
    if ( cubezbounds )
	attrout->setTrcsBounds( *cubezbounds );

    proc->addOutput( attrout );
    return proc;
}


int EngineMan::getNrOutputsToBeProcessed( const Processor& proc ) const
{
    return proc.outputs_.size();
}


#undef mErrRet
#define mErrRet(s) { errmsg = s; return false; }

bool EngineMan::ensureDPSAndADSPrepared( DataPointSet& datapointset,
					 const Attrib::DescSet& descset,
					 uiString& errmsg )
{
    BufferStringSet attrrefs;
    descset.fillInAttribColRefs( attrrefs );

    for ( int idx=0; idx<datapointset.nrCols(); idx++ )
    {
	DataColDef& dcd = datapointset.colDef( idx );

	const char* nmstr = dcd.name_.buf();
	if ( *nmstr == '[' ) // Make sure stored descs are actually in the set
	{
	    int refidx = -1;
	    for ( int ids=0; ids<attrrefs.size(); ids++ )
	    {
		FileMultiString fms( attrrefs.get(ids) );
		if ( fms[0]==nmstr )
		    { refidx = ids; break; }
	    }

	    //TODO : handle multi components stored data
	    DescID descid = DescID::undef();
	    if ( refidx > -1 )
	    {
		FileMultiString fms( attrrefs.get(refidx) );
		descid = const_cast<DescSet&>(descset).
				    getStoredID( fms[1], 0, true );
	    }
	    if ( descid == DescID::undef() )
		mErrRet( tr("Cannot find specified '%1'",
			    " in object management").arg(nmstr));

	    // Put the new DescID in coldef and in the refs
	    BufferString tmpstr;
	    const Attrib::Desc* desc = descset.getDesc( descid );
	    if ( !desc ) mErrRet("Huh?");
	    desc->getDefStr( tmpstr );
	    FileMultiString fms( tmpstr ); fms += descid.asInt();
	    attrrefs.get(refidx) = fms;
	    dcd.ref_ = fms;
	}

	if ( dcd.ref_.isEmpty() )
	{
	    int refidx = attrrefs.indexOf( nmstr ); // maybe name == ref
	    if ( refidx == -1 )
	    {
		DescID did = descset.getID( nmstr, true );
		if ( did == DescID::undef() ) // Column is not an attribute
		    continue;

		for ( int idref=0; idref< attrrefs.size(); idref++ )
		{
		    FileMultiString fms( attrrefs.get(idref) );
		    const DescID candidatid( fms.getIValue(1), false );
		    if ( did == candidatid )
			{ refidx = idref; break; }
		}
		if ( refidx < 0 ) // nmstr is not in attrrefs - period.
		    continue;
	    }

	    dcd.ref_ = attrrefs.get( refidx );
	}
    }
    return true;
}

#undef mErrRet

} // namespace Attrib
