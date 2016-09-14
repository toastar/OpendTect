/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/


#include "uiseisiosimple.h"
#include "ui2dgeomman.h"
#include "uiseissubsel.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uimultcomputils.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uitaskrunner.h"
#include "uiseparator.h"
#include "uiscaler.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uibutton.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "seispsioprov.h"
#include "seisselection.h"
#include "seisresampler.h"
#include "survgeom2d.h"
#include "ioobjctxt.h"
#include "trckeyzsampling.h"
#include "ioobj.h"
#include "iopar.h"
#include "survinfo.h"
#include "oddirs.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "od_helpids.h"


static bool survChanged()
{
    mDefineStaticLocalObject( BufferString, survnm, );
    const bool issame = survnm.isEmpty() || survnm == SI().name();
    survnm = SI().name();
    return !issame;
}


#define mDefData(nm,geom) \
SeisIOSimple::Data& uiSeisIOSimple::data##nm() \
{ \
    static SeisIOSimple::Data* d = 0; \
    if ( !d ) d = new SeisIOSimple::Data( GetDataDir(), Seis::geom ); \
    return *d; \
}

mDefData(2d,Line)
mDefData(3d,Vol)
mDefData(ps,VolPS)


uiSeisIOSimple::uiSeisIOSimple( uiParent* p, Seis::GeomType gt, bool imp )
	: uiDialog( p, Setup( imp ? tr("Import seismics from simple flat file")
				  : tr("Export seismics to simple flat file"),
			      mNoDlgTitle,
			      imp ? mODHelpKey(mSeisIOSimpleImpHelpID)
				  : mODHelpKey(mSeisIOSimpleExpHelpID) )
			.modal(false))
	, ctxt_(*new IOObjContext(uiSeisSel::ioContext(gt,!imp)))
	, sdfld_(0)
	, havenrfld_(0)
	, haverefnrfld_(0)
	, nrdeffld_(0)
	, inldeffld_(0)
	, subselfld_(0)
	, isxyfld_(0)
	, lnmfld_(0)
	, isascfld_(0)
	, haveoffsbut_(0)
	, haveazimbut_(0)
	, pspposlbl_(0)
	, offsdeffld_(0)
	, isimp_(imp)
	, geom_(gt)
{
    setOkText( imp ? uiStrings::sImport() : uiStrings::sExport() );

    data().clear( survChanged() );
    const bool is2d = is2D();
    const bool isps = isPS();

    uiSeisSel::Setup ssu( geom_ );
    uiSeparator* sep = 0;
    if ( isimp_ )
    {
	mkIsAscFld();
	fnmfld_ = new uiFileInput( this, uiStrings::phrInput(uiStrings::sFile()
			.toLower()), uiFileInput::Setup("")
			.forread( true )
			.withexamine( true ) );
	fnmfld_->attach( alignedBelow, isascfld_ );
    }
    else
    {
	ssu.steerpol(uiSeisSel::Setup::InclSteer);
	seisfld_ = new uiSeisSel( this, ctxt_, ssu );
	seisfld_->selectionDone.notify( mCB(this,uiSeisIOSimple,inpSeisSel) );
	sep = mkDataManipFlds();
    }

    haveposfld_ = new uiGenInput( this,
			isimp_ ? tr("Traces start with a position")
			       : tr("Output a position for every trace"),
			BoolInpSpec(true) );
    haveposfld_->setValue( data().havepos_ );
    haveposfld_->valuechanged.notify( mCB(this,uiSeisIOSimple,haveposSel) );
    haveposfld_->attach( alignedBelow, isimp_ ? fnmfld_->attachObj()
					     : remnullfld_->attachObj() );
    if ( sep ) haveposfld_->attach( ensureBelow, sep );

    uiObject* attachobj = haveposfld_->attachObj();
    if ( is2d )
    {
	uiString txt= tr("%1 (preceding X/Y)").arg( isimp_ ?
		      tr("%1 included").arg( uiStrings::sTraceNumber() )
		      : tr("Include %1").arg( uiStrings::sTraceNumber() ) );
	havenrfld_ = new uiGenInput( this, txt, BoolInpSpec(true) );
	havenrfld_->setValue( data().havenr_ );
	havenrfld_->attach( alignedBelow, attachobj );
	havenrfld_->valuechanged.notify( mCB(this,uiSeisIOSimple,havenrSel) );
	txt = tr("%1 (after trace number)").arg(isimp_ ?
	      tr("Ref/SP number included") : tr("Include Ref/SP number"));
	haverefnrfld_ = new uiGenInput( this, txt, BoolInpSpec(false) );
	haverefnrfld_->setValue( data().haverefnr_ );
	haverefnrfld_->attach( alignedBelow, havenrfld_ );
	havenrfld_->valuechanged.notify( mCB(this,uiSeisIOSimple,havenrSel) );
	attachobj = haverefnrfld_->attachObj();
    }
    else
    {
	isxyfld_ = new uiGenInput( this, isimp_
		? tr("Position in file is") : tr("Position in file will be"),
		BoolInpSpec(true,tr("X Y"),tr("Inl Crl")) );
	isxyfld_->setValue( data().isxy_ );
	isxyfld_->attach( alignedBelow, attachobj );
	attachobj = isxyfld_->attachObj();
    }

    if ( !isimp_ && isps )
    {
	haveoffsbut_ = new uiCheckBox( this, uiStrings::sOffset() );
	haveoffsbut_->setChecked( data().haveoffs_ );
	haveoffsbut_->attach( alignedBelow, attachobj );
	haveazimbut_ = new uiCheckBox( this, uiStrings::sAzimuth() );
	haveazimbut_->setChecked( data().haveazim_ );
	haveazimbut_->attach( rightOf, haveoffsbut_ );
	pspposlbl_ = new uiLabel( this, tr("Include"), haveoffsbut_ );
	attachobj = haveoffsbut_;
    }

    if ( isimp_ )
    {
	if ( !is2d )
	{
	    inldeffld_ = new uiGenInput( this,
                                         tr("Inline definition: start, step"),
				IntInpSpec(data().inldef_.start)
						.setName("Inl def start"),
				IntInpSpec(data().inldef_.step)
						.setName("Inl def step") );
	    inldeffld_->attach( alignedBelow, attachobj );
	    crldeffld_ = new uiGenInput( this,
			tr("Xline definition: start, step, # per inline"),
			   IntInpSpec(data().crldef_.start)
						.setName("Crl def start"),
			   IntInpSpec(data().crldef_.step)
						.setName("Crl def step"),
			   IntInpSpec(data().nrcrlperinl_)
						.setName("per inl") );
	    crldeffld_->attach( alignedBelow, inldeffld_ );
	    attachobj = crldeffld_->attachObj();
	}
	else
	{
	    nrdeffld_ = new uiGenInput( this,
		    tr("%1 definition: start, step")
			.arg( uiStrings::sTraceNumber() ),
		    IntInpSpec(data().nrdef_.start).setName("Trc def start"),
		    IntInpSpec(data().nrdef_.step).setName("Trc def step") );
	    nrdeffld_->attach( alignedBelow, havenrfld_ );
	    startposfld_ = new uiGenInput( this,
			    tr("Start position (X, Y, %1)")
				.arg(uiStrings::sTraceNumber() ),
					  PositionInpSpec(data().startpos_) );
	    startposfld_->attach( alignedBelow, haveposfld_ );
	    stepposfld_ = new uiGenInput( this, tr("Step in X/Y/Number"),
					 PositionInpSpec(data().steppos_) );
	    stepposfld_->attach( alignedBelow, startposfld_ );
	    startnrfld_ = new uiGenInput( this, uiString::emptyString(),
					 IntInpSpec(data().nrdef_.start) );
	    startnrfld_->setElemSzPol( uiObject::Small );
	    startnrfld_->attach( rightOf, startposfld_ );
	    stepnrfld_ = new uiGenInput( this, uiString::emptyString(),
					IntInpSpec(data().nrdef_.step) );
	    stepnrfld_->setElemSzPol( uiObject::Small );
	    stepnrfld_->attach( rightOf, stepposfld_ );
	    attachobj = stepposfld_->attachObj();
	}
	if ( isps )
	{
	    haveoffsbut_ = new uiCheckBox( this, uiStrings::sOffset(),
					 mCB(this,uiSeisIOSimple,haveoffsSel) );
	    haveoffsbut_->attach( alignedBelow, attachobj );
	    haveoffsbut_->setChecked( data().haveoffs_ );
	    haveazimbut_ = new uiCheckBox( this, uiStrings::sAzimuth() );
	    haveazimbut_->attach( rightOf, haveoffsbut_ );
	    haveazimbut_->setChecked( data().haveazim_ );
	    pspposlbl_ = new uiLabel( this, tr("Position includes"),
                                      haveoffsbut_ );
	    const float stopoffs =
			data().offsdef_.atIndex(data().nroffsperpos_-1);
	    offsdeffld_ = new uiGenInput( this,
			   tr("Offset definition: start, stop, step"),
			   FloatInpSpec(data().offsdef_.start).setName("Start"),
			   FloatInpSpec(stopoffs).setName("Stop"),
			   FloatInpSpec(data().offsdef_.step).setName("Step") );
	    offsdeffld_->attach( alignedBelow, haveoffsbut_ );
	    attachobj = offsdeffld_->attachObj();
	}
    }

    havesdfld_ = new uiGenInput( this,
		isimp_ ? tr("File start contains sampling info")
		       : tr("Put sampling info in file start"),
		BoolInpSpec(true)
			.setName("Info in file start Yes",0)
			.setName("Info in file start No",1) );
    havesdfld_->setValue( data().havesd_ );
    havesdfld_->attach( alignedBelow, attachobj );
    havesdfld_->valuechanged.notify( mCB(this,uiSeisIOSimple,havesdSel) );

    if ( isimp_ )
    {
	uiString txt = tr("Sampling info: start, step %1 and #samples").
		       arg(SI().getUiZUnitString(true));
	SamplingData<float> sd( data().sd_ );
	if ( SI().zIsTime() )
	    { sd.start *= 1000; sd.step *= 1000; }
	sdfld_ = new uiGenInput( this, txt,
			DoubleInpSpec(sd.start).setName("SampInfo start"),
			DoubleInpSpec(sd.step).setName("SampInfo step"),
			IntInpSpec(data().nrsamples_).setName("Nr samples") );
	sdfld_->attach( alignedBelow, havesdfld_ );
	sep = mkDataManipFlds();
	if ( !isps ) ssu.enabotherdomain( true );
	seisfld_ = new uiSeisSel( this, ctxt_, ssu );
	seisfld_->attach( alignedBelow, remnullfld_ );
	if ( is2d )
	{
	    lnmfld_ = new uiSeis2DLineNameSel( this, false );
	    lnmfld_->fillWithAll();
	    lnmfld_->attach( alignedBelow, seisfld_ );
	    seisfld_->selectionDone.notify( mCB(this,uiSeisIOSimple,lsSel) );
	}
    }
    else
    {
	mkIsAscFld();
	isascfld_->attach( alignedBelow, havesdfld_ );
	fnmfld_ = new uiFileInput( this, uiStrings::phrOutput(uiStrings::sFile()
			.toLower()), uiFileInput::Setup("")
			.forread( false )
			.withexamine( false ) );
	fnmfld_->attach( alignedBelow, isascfld_ );
    }

    fnmfld_->setDefaultSelectionDir( data().fname_ );
    postFinalise().notify( mCB(this,uiSeisIOSimple,initFlds) );
}


void uiSeisIOSimple::mkIsAscFld()
{
    isascfld_ = new uiGenInput( this, tr("File type"),
			BoolInpSpec(true,uiStrings::sASCII(),tr("Binary")) );
    isascfld_->valuechanged.notify( mCB(this,uiSeisIOSimple,isascSel) );
    isascfld_->setValue( data().isasc_ );
}


uiSeparator* uiSeisIOSimple::mkDataManipFlds()
{
    uiSeparator* sep = new uiSeparator( this, "sep inp and outp pars" );
    if ( isimp_ )
	sep->attach( stretchedBelow, sdfld_ );
    else
    {
	subselfld_ = uiSeisSubSel::get( this, Seis::SelSetup(geom_)
						.onlyrange(false) );
	subselfld_->attachObj()->attach( alignedBelow, seisfld_ );
    }

    scalefld_ = new uiScaler( this, uiStrings::sEmptyString(), true );
    scalefld_->attach( alignedBelow, isimp_ ? sdfld_->attachObj()
					   : subselfld_->attachObj() );
    if ( isimp_ ) scalefld_->attach( ensureBelow, sep );
    remnullfld_ = new uiGenInput( this, tr("Null traces"),
				  BoolInpSpec(true,tr("Discard"),tr("Pass")) );
    remnullfld_->attach( alignedBelow, scalefld_ );

    multcompfld_ = new uiGenInput( this, tr("Component to export"),
				   StringListInpSpec() );
    multcompfld_->display( false );
    multcompfld_->setSensitive( false );

    if ( !isimp_ )
    {
	multcompfld_->attach( alignedBelow, remnullfld_ );
	sep->attach( stretchedBelow, multcompfld_ );
    }

    return sep;
}


void uiSeisIOSimple::initFlds( CallBacker* cb )
{
    havesdSel( cb );
    haveposSel( cb );
    isascSel( cb );
    haveoffsSel( cb );
    if ( !isimp_ )
	inpSeisSel( cb );
}


void uiSeisIOSimple::havesdSel( CallBacker* )
{
    if ( sdfld_ )
	sdfld_->display( !havesdfld_->getBoolValue() );
}


void uiSeisIOSimple::inpSeisSel( CallBacker* )
{
    const IOObj* ioobj = seisfld_->ioobj( true );
    if ( ioobj )
    {
	subselfld_->setInput( *ioobj );
	BufferStringSet compnms;
	SeisIOObjInfo::getCompNames( ioobj->key(), compnms );
	multcompfld_->newSpec( StringListInpSpec(compnms), 0 );
	multcompfld_->display( compnms.size()>1 );
	multcompfld_->setSensitive( compnms.size()>1 );
    }
}


void uiSeisIOSimple::lsSel( CallBacker* )
{
    if ( !lnmfld_ ) return;
    const IOObj* ioobj = seisfld_->ioobj( true );
    if ( ioobj )
	lnmfld_->setDataSet( ioobj->key() );
}



void uiSeisIOSimple::isascSel( CallBacker* )
{
    fnmfld_->enableExamine( isascfld_->getBoolValue() );
}


void uiSeisIOSimple::haveposSel( CallBacker* cb )
{
    const bool havenopos = !haveposfld_->getBoolValue();

    if ( havenrfld_ ) havenrfld_->display( !havenopos );
    if ( isxyfld_ ) isxyfld_->display( !havenopos );

    if ( isimp_ )
    {
	if ( !is2D() )
	{
	    inldeffld_->display( havenopos );
	    crldeffld_->display( havenopos );
	}
	else
	{
	    startposfld_->display( havenopos );
	    startnrfld_->display( havenopos );
	    stepposfld_->display( havenopos );
	    stepnrfld_->display( havenopos );
	}
    }

    havenrSel( cb );
    haveoffsSel( cb );
}


void uiSeisIOSimple::havenrSel( CallBacker* cb )
{
    if ( !havenrfld_ ) return;

    const bool havepos = haveposfld_->getBoolValue();
    const bool havenr = havenrfld_->getBoolValue();
    if ( nrdeffld_ )
	nrdeffld_->display( havepos && !havenr );
    if ( haverefnrfld_ )
	haverefnrfld_->display( havepos && havenr );
}


void uiSeisIOSimple::haveoffsSel( CallBacker* cb )
{
    if ( !pspposlbl_ || !haveoffsbut_ ) return;
    const bool havepos = haveposfld_->getBoolValue();
    const bool haveoffs = haveoffsbut_->isChecked();
    haveoffsbut_->display( havepos );
    haveazimbut_->display( havepos );
    pspposlbl_->display( havepos );
    if ( offsdeffld_ )
	offsdeffld_->display( !havepos || !haveoffs );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSeisIOSimple::acceptOK()
{
    if ( !isascfld_ ) return true;

    BufferString fnm( fnmfld_->fileName() );
    if ( isimp_ && !File::exists(fnm) )
	mErrRet(tr("Input file does not exist or is unreadable"))
    const IOObj* ioobj = seisfld_->ioobj( true );
    if ( !ioobj )
	return false;

    data().subselpars_.setEmpty();
    if ( is2D() )
    {
	Pos::GeomID geomid = mUdfGeomID;
	if ( !isimp_ )
	    geomid = static_cast<uiSeis2DSubSel*>(subselfld_)->selectedGeomID();
	else
	{
	    const BufferString linenm = lnmfld_->getInput();
	    if ( linenm.isEmpty() )
		mErrRet( uiStrings::phrEnter(tr("a line name")) )

	    geomid = Geom2DImpHandler::getGeomID( linenm );
	    if ( geomid == mUdfGeomID )
		return false;
	}

	data().geomid_ = geomid;
    }

    data().seiskey_ = ioobj->key();
    data().fname_ = fnm;

    data().setScaler( scalefld_->getScaler() );
    data().remnull_ = remnullfld_->getBoolValue();
    const bool ismulticomp = multcompfld_->sensitive();
    data().compidx_ = ismulticomp ? multcompfld_->getIntValue() : 0;

    data().isasc_ = isascfld_->getBoolValue();
    data().havesd_ = havesdfld_->getBoolValue();
    if ( sdfld_ && !data().havesd_ )
    {
	data().sd_.start = sdfld_->getFValue(0);
	data().sd_.step = sdfld_->getFValue(1);
	if ( SI().zIsTime() )
	    { data().sd_.start *= 0.001; data().sd_.step *= 0.001; }
	data().nrsamples_ = sdfld_->getIntValue(2);
    }

    data().havepos_ = haveposfld_->getBoolValue();
    data().havenr_ = data().haverefnr_ = false;
    if ( data().havepos_ )
    {
	data().isxy_ = is2D() || isxyfld_->getBoolValue();
	data().havenr_ = havenrfld_ && havenrfld_->getBoolValue();
	data().haverefnr_ = data().havenr_ && haverefnrfld_->getBoolValue();
	if ( isimp_ && nrdeffld_ && !data().havenr_ )
	{
	    data().nrdef_.start = nrdeffld_->getIntValue(0);
	    data().nrdef_.step = nrdeffld_->getIntValue(1);
	}
	if ( isPS() )
	{
	    data().haveoffs_ = haveoffsbut_->isChecked();
	    data().haveazim_ = haveazimbut_->isChecked();
	}
    }
    else if ( isimp_ )
    {
	data().haveoffs_ = false;
	if ( is2D() )
	{
	    data().startpos_ = startposfld_->getCoord();
	    data().steppos_ = stepposfld_->getCoord();
	    data().nrdef_.start = startnrfld_->getIntValue();
	    data().nrdef_.step = stepnrfld_->getIntValue();
	}
	else
	{
	    data().inldef_.start = inldeffld_->getIntValue(0);
	    data().inldef_.step = inldeffld_->getIntValue(1);
	    if ( data().inldef_.step == 0 ) data().inldef_.step = 1;
	    data().crldef_.start = crldeffld_->getIntValue(0);
	    data().crldef_.step = crldeffld_->getIntValue(1);
	    if ( data().crldef_.step == 0 ) data().crldef_.step = 1;
	    int nrcpi = crldeffld_->getIntValue(2);
	    if ( nrcpi == 0 || crldeffld_->isUndef(2) )
	    {
		uiMSG().error( tr("Please define the number"
				  " of Xlines per Inline"));
		return false;
	    }
	    data().nrcrlperinl_ = nrcpi;
	}
    }

    if ( isPS() && !data().haveoffs_ && offsdeffld_ )
    {
	data().offsdef_.start = offsdeffld_->getFValue( 0 );
	data().offsdef_.step = offsdeffld_->getFValue( 2 );
	const float offsstop = offsdeffld_->getFValue( 1 );
	data().nroffsperpos_ =
			data().offsdef_.nearestIndex( offsstop ) + 1;
    }

    if ( subselfld_ )
    {
	subselfld_->fillPar( data().subselpars_ );
	if ( !subselfld_->isAll() )
	{
	    TrcKeyZSampling cs;
	    subselfld_->getSampling( cs.hsamp_ );
	    subselfld_->getZRange( cs.zsamp_ );
	    data().setResampler( new SeisResampler(cs,is2D()) );
	}
    }

    SeisIOSimple sios( data(), isimp_ );
    uiTaskRunner dlg( this );
    const bool res = TaskRunner::execute( &dlg, sios ) && !ismulticomp;
    if ( !res )
	return res;

    const uiString msg = tr("Data successfully %1.\n\nDo you want to %2 more?")
			    .arg(isimp_ ? tr("imported") : tr("exported"))
			    .arg(isimp_ ? tr("import") : tr("export"));
    const bool ret = uiMSG().askGoOn(msg, uiStrings::sYes(),
		     tr("No, close window"));
    return !ret;
}
