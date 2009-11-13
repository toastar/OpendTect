/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: seispsioprov.cc,v 1.24 2009-11-13 12:28:40 cvsbert Exp $";

#include "seispsioprov.h"
#include "seispsread.h"
#include "seispswrite.h"
#include "seispscubetr.h"
#include "seispsfact.h"
#include "seiscbvsps.h"
#include "seisselection.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "posinfo.h"
#include "filegen.h"
#include "iox.h"
#include "ioman.h"
#include "iodir.h"
#include "iopar.h"
#include "keystrs.h"


const char* SeisPSIOProvider::sKeyCubeID = "=Cube.ID";


SeisPSIOProviderFactory& SPSIOPF()
{
    static SeisPSIOProviderFactory* theinst = 0;
    if ( !theinst ) theinst = new SeisPSIOProviderFactory;
    return *theinst;
}


const SeisPSIOProvider* SeisPSIOProviderFactory::provider( const char* t ) const
{
    if ( provs_.isEmpty() )	return 0;
    else if ( !t )		return provs_[0];

    for ( int idx=0; idx<provs_.size(); idx++ )
	if ( !strcmp(t,provs_[idx]->type()) )
	    return provs_[idx];

    return 0;
}


void SeisPSIOProviderFactory::mk3DPostStackProxy( IOObj& ioobj )
{
    if ( ioobj.pars().find(SeisPSIOProvider::sKeyCubeID) )
	return;

    IOM().to( ioobj.key() );
    BufferString nm( "{" ); nm += ioobj.name(); nm += "}";
    IOX* iox = new IOX( nm );
    iox->setTranslator( mTranslKey(SeisPSCubeSeisTrc) );
    iox->setGroup( mTranslGroupName(SeisTrc) );
    iox->acquireNewKey();
    ioobj.pars().set( SeisPSIOProvider::sKeyCubeID, iox->key() );
    IOM().dirPtr()->commitChanges( &ioobj );
    iox->setOwnKey( ioobj.key() );
    IOM().dirPtr()->addObj( iox, true );
}


bool SeisPSIOProviderFactory::getLineNames( const IOObj& ioobj,
					    BufferStringSet& linenms ) const
{
    if ( provs_.isEmpty() ) return false;

    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    return prov ? prov->getLineNames( ioobj.fullUserExpr(true), linenms )
		: false;
}


SeisPS3DReader* SeisPSIOProviderFactory::get3DReader( const IOObj& ioobj,
						      int inl ) const
{
    if ( provs_.isEmpty() ) return 0;
    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    SeisPS3DReader* reader =
	prov ? prov->make3DReader( ioobj.fullUserExpr(true), inl ) : 0;

    if ( reader )
	reader->usePar( ioobj.pars() );

    return reader;
}


SeisPS2DReader* SeisPSIOProviderFactory::get2DReader( const IOObj& ioobj,
						      const char* lnm ) const
{
    if ( provs_.isEmpty() ) return 0;
    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    SeisPS2DReader* reader =
	prov ? prov->make2DReader( ioobj.fullUserExpr(true), lnm ) : 0;

    if ( reader )
	reader->usePar( ioobj.pars() );

    return reader;
}


SeisPSWriter* SeisPSIOProviderFactory::get3DWriter( const IOObj& ioobj ) const
{
    if ( provs_.isEmpty() ) return 0;
    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    SeisPSWriter* writer =
	prov ? prov->make3DWriter( ioobj.fullUserExpr(false) ) : 0;
    if ( writer )
	writer->usePar( ioobj.pars() );

    return writer;
}


SeisPSWriter* SeisPSIOProviderFactory::get2DWriter( const IOObj& ioobj,
       						    const char* lnm ) const
{
    if ( provs_.isEmpty() ) return 0;
    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    SeisPSWriter* writer =
	prov ? prov->make2DWriter( ioobj.fullUserExpr(false), lnm ) : 0;
    if ( writer )
	writer->usePar( ioobj.pars() );

    return writer;
}


SeisTrc* SeisPSReader::getTrace( const BinID& bid, int trcnr ) const
{
    SeisTrcBuf buf( true );
    if ( !getGather(bid,buf) || buf.size() <= trcnr )
	return false;
    return buf.remove( trcnr );
}


mDefSimpleTranslatorSelector(SeisPS3D,sKeySeisPS3DTranslatorGroup)
mDefSimpleTranslatorioContext(SeisPS3D,Seis)
mDefSimpleTranslatorSelector(SeisPS2D,sKeySeisPS2DTranslatorGroup)
mDefSimpleTranslatorioContext(SeisPS2D,Seis)


bool CBVSSeisPS3DTranslator::implRemove( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    BufferString fnm( ioobj->fullUserExpr(true) );
    if ( File_exists(fnm) )
	File_remove( fnm, File_isDirectory(fnm) );
    const char* res = ioobj->pars().find( SeisPSIOProvider::sKeyCubeID );
    if ( res )
	IOM().permRemove( MultiID(res) );
    return !File_exists(fnm);
}


bool CBVSSeisPS2DTranslator::implRemove( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    BufferString fnm( ioobj->fullUserExpr(true) );
    if ( File_exists(fnm) )
	File_remove( fnm, File_isDirectory(fnm) );
    return !File_exists(fnm);
}


SeisPSCubeSeisTrcTranslator::SeisPSCubeSeisTrcTranslator( const char* nm,
							  const char* unm )
	: SeisTrcTranslator(nm,unm)
    	, trc_(*new SeisTrc)
    	, psrdr_(0)
    	, inforead_(false)
    	, posdata_(*new PosInfo::CubeData)
    	, trcnr_(-1)
{
}


SeisPSCubeSeisTrcTranslator::~SeisPSCubeSeisTrcTranslator()
{
    delete psrdr_;
    delete &trc_;
    delete &posdata_;
}


const char* SeisPSCubeSeisTrcTranslator::connType() const
{
    return XConn::sType();
}

static const char* sKeyOffsNr = "Default trace nr";


bool SeisPSCubeSeisTrcTranslator::initRead_()
{
    if ( conn->ioobj )
    {
	mDynamicCastGet(IOX*,iox,conn->ioobj)
	IOObj* useioobj = iox ? iox->getIOObj() : conn->ioobj->clone();
	psrdr_ = SPSIOPF().get3DReader( *useioobj );
	useioobj->pars().get( sKeyOffsNr, trcnr_ );
	delete useioobj;
    }
    else
    {
	mDynamicCastGet(StreamConn*,sconn,conn->conn())
	if ( !sconn )
	    { errmsg = "Wrong connection from Object Manager"; return false; }
	psrdr_ = new SeisCBVSPS3DReader( sconn->fileName() );
    }
    conn->close();
    errmsg = psrdr_ ? psrdr_->errMsg() : "Cannot find PS data store type";
    if ( errmsg && *errmsg )
	return false;

    posdata_ = psrdr_->posData();
    posdata_.getInlRange( pinfo.inlrg );
    posdata_.getCrlRange( pinfo.crlrg );
    pinfo.inlrg.sort(); pinfo.crlrg.sort();
    curbinid_.inl = pinfo.inlrg.start;
    curbinid_.crl = pinfo.crlrg.start - pinfo.crlrg.step;

    TypeSet<float> offss;
    if ( !doRead(trc_,&offss) )
	return false;
    insd = trc_.info().sampling;
    innrsamples = trc_.size();
    for ( int icomp=0; icomp<trc_.nrComponents(); icomp++ )
	addComp( trc_.data().getInterpreter(icomp)->dataChar(),
		 BufferString("O=",offss[icomp]) );

    curbinid_.inl = pinfo.inlrg.start;
    curbinid_.crl = pinfo.crlrg.start - pinfo.crlrg.step;
    return true;
}


bool SeisPSCubeSeisTrcTranslator::goTo( const BinID& bid )
{
    if ( !posdata_.includes(bid.inl,bid.crl) ) return false;
    curbinid_ = bid; curbinid_.crl -= pinfo.crlrg.step;
    return true;
}


bool SeisPSCubeSeisTrcTranslator::toNext()
{
    for ( int crl=curbinid_.crl+pinfo.crlrg.step; crl<=pinfo.crlrg.stop;
	    crl+=pinfo.crlrg.step )
    {
	if ( posdata_.includes(curbinid_.inl,crl) )
	{
	    BinID bid( curbinid_.inl, crl );
	    if ( !seldata || seldata->isOK(BinID(curbinid_.inl,crl)) )
		{ curbinid_.crl = crl; return true; }
	}
    }

    curbinid_.inl += pinfo.inlrg.step;
    if ( curbinid_.inl > pinfo.inlrg.stop )
	return false;

    curbinid_.crl = pinfo.crlrg.start - pinfo.crlrg.step;
    return toNext();
}


bool SeisPSCubeSeisTrcTranslator::commitSelections_()
{
    if ( trcnr_ >= 0 ) return true;

    for ( int idx=0; idx<tarcds.size(); idx++ )
    {
	if ( tarcds[idx]->destidx >= 0 )
	    { trcnr_ = idx; break; }
    }

    return true;
}


bool SeisPSCubeSeisTrcTranslator::doRead( SeisTrc& trc, TypeSet<float>* offss )
{
    if ( !toNext() ) return false;
    SeisTrc* newtrc = 0;
    if ( trcnr_ >= 0 )
    {
	newtrc = psrdr_->getTrace( curbinid_, trcnr_ );
    }
    else
    {
	SeisTrcBuf tbuf( true );
	if ( psrdr_->getGather(curbinid_,tbuf) && !tbuf.isEmpty() )
	{
	    newtrc = new SeisTrc( *tbuf.get(0) );
	    const int trcsz = newtrc->size();
	    const DataCharacteristics trcdc(
		    newtrc->data().getInterpreter(0)->dataChar() );
	    if ( offss )
		*offss += newtrc->info().offset;
	    for ( int icomp=1; icomp<tbuf.size(); icomp++ )
	    {
		const SeisTrc& trc = *tbuf.get(icomp);
		newtrc->data().addComponent( trcsz, trcdc );
		for ( int isamp=0; isamp<trcsz; isamp++ )
		    newtrc->set( isamp, trc.get(isamp,0), icomp );
		if ( offss )
		    *offss += trc.info().offset;
	    }
	}
    }

    if ( !newtrc ) return false;
    if ( !seldata || seldata->isAll() )
	trc = *newtrc;
    else
    {
	trc.info() = newtrc->info();
	const Interval<float> zrg( seldata->zRange() );
	trc.info().sampling.start = zrg.start;
	const float sr = trc.info().sampling.step;
	const int nrsamps = (int)(zrg.width() / sr + 1.5);
	trc.reSize( nrsamps, false );
	for ( int idx=0; idx<nrsamps; idx++ )
	    trc.set( idx, newtrc->getValue( zrg.start + idx * sr, 0 ), 0 );
    }
    delete newtrc;
    return true;
}


bool SeisPSCubeSeisTrcTranslator::readInfo( SeisTrcInfo& inf )
{
    if ( !outcds ) commitSelections();
    if ( inforead_ ) return true;
    if ( !doRead(trc_) ) return false;
    inforead_ = true;
    inf = trc_.info();
    return true;
}


bool SeisPSCubeSeisTrcTranslator::read( SeisTrc& trc )
{
    if ( !outcds ) commitSelections();
    if ( inforead_ )
	{ inforead_ = false; trc = trc_; return true; }
    inforead_ = false;
    return doRead( trc );
}


bool SeisPSCubeSeisTrcTranslator::skip( int nr )
{
    for ( int idx=0; idx<nr; idx++ )
    {
	if ( !toNext() ) return false;
    }
    return true;
}
