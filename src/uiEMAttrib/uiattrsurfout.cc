/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2004
________________________________________________________________________

-*/


#include "uiattrsurfout.h"

#include "array2dinterpolimpl.h"
#include "attriboutput.h"
#include "ioobjctxt.h"
#include "emsurfacetr.h"
#include "emsurfaceauxdata.h"
#include "emsurfauxdataio.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"

#include "uiarray2dinterpol.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uidlggroup.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uibatchjobdispatchersel.h"
#include "uistrings.h"
#include "od_helpids.h"

using namespace Attrib;

uiAttrSurfaceOut::uiAttrSurfaceOut( uiParent* p, const DescSet& ad,
				    const NLAModel* n, const DBKey& mid )
    : uiAttrEMOut( p, ad, n, mid, "Calculate Horizon Data" )
    , interpol_(0)
{
    setHelpKey( mODHelpKey(mAttrSurfaceOutHelpID) );
    setCtrlStyle( RunAndClose );

    attrnmfld_ = new uiGenInput( pargrp_, uiStrings::sAttribName(),
				 StringInpSpec() );
    attrnmfld_->setElemSzPol( uiObject::Wide );
    attrnmfld_->attach( alignedBelow, attrfld_ );

    filludffld_ = new uiGenInput( pargrp_, tr("Fill undefined parts"),
				  BoolInpSpec(false) );
    filludffld_->valuechanged.notify( mCB(this,uiAttrSurfaceOut,fillUdfSelCB) );
    filludffld_->attach( alignedBelow, attrnmfld_ );

    settingsbut_ = new uiPushButton( pargrp_, uiStrings::sSettings(),
				 mCB(this,uiAttrSurfaceOut,settingsCB), false);
    settingsbut_->display( false );
    settingsbut_->attach( rightOf, filludffld_ );

    objfld_ = new uiIOObjSel( pargrp_, mIOObjContext(EMHorizon3D),
			      uiStrings::phrCalculate(tr("on Horizon")) );
    objfld_->attach( alignedBelow, filludffld_ );
    objfld_->selectionDone.notify( mCB(this,uiAttrSurfaceOut,objSelCB) );
    pargrp_->setHAlignObj( objfld_ );

    batchjobfld_->setSensitive( false );
}


uiAttrSurfaceOut::~uiAttrSurfaceOut()
{
}


void uiAttrSurfaceOut::fillUdfSelCB( CallBacker* )
{
    const bool isdisplay = filludffld_->getBoolValue();
    settingsbut_->display( isdisplay );
    if ( settingsbut_->isDisplayed() )
    {
	InverseDistanceArray2DInterpol* tempinterpol =
					new InverseDistanceArray2DInterpol;
	const float defradius = 10*(SI().inlDistance()+SI().crlDistance());
	tempinterpol->setSearchRadius( defradius );
	tempinterpol->setFillType( Array2DInterpol::ConvexHull );
	tempinterpol->setStepSize( 1 );
	tempinterpol->setMaxHoleSize( mUdf(float) );
	interpol_ = tempinterpol;
    }
}


void uiAttrSurfaceOut::settingsCB( CallBacker* )
{
    uiSingleGroupDlg<uiArray2DInterpolSel> dlg( this,
                new uiArray2DInterpolSel( 0, true, true, false, interpol_));
    dlg.setCaption( uiStrings::sInterpolation() );
    dlg.setTitleText( uiStrings::sSettings() );
    
    if ( !dlg.go() )
	return;

    IOPar iop;
    dlg.getDlgGroup()->fillPar( iop );
    iop.get( sKey::Name(), methodname_ );

    if ( interpol_ ) delete interpol_;

    interpol_ = dlg.getDlgGroup()->getResult();
}


void uiAttrSurfaceOut::attribSel( CallBacker* )
{
    attrnmfld_->setText( attrfld_->getInput() );
    objSelCB(0);
}


void uiAttrSurfaceOut::objSelCB( CallBacker* )
{
}


void uiAttrSurfaceOut::getJobName( BufferString& jobnm ) const
{
    const IOObj* ioobj = objfld_->ioobj( true );
    if ( ioobj )
	jobnm.add( ioobj->name() );

    const FixedString attrnm = attrnmfld_->text();
    if ( !attrnm.isEmpty() )
	jobnm.add( " ").add( attrnm.buf() );
}


bool uiAttrSurfaceOut::prepareProcessing()
{
    const IOObj* ioobj = objfld_->ioobj();
    if ( !ioobj ) return false;

    const FixedString attrnm = attrnmfld_->text();
    if ( attrnm.isEmpty() )
    {
	uiMSG().error( tr("Provide output attribute name") );
	return false;
    }

    return uiAttrEMOut::prepareProcessing();
}


bool uiAttrSurfaceOut::fillPar( IOPar& iopar )
{
    if ( !uiAttrEMOut::fillPar( iopar ) )
	return false;

    const IOObj* ioobj = objfld_->ioobj();
    if ( !ioobj ) return false;

    if ( settingsbut_->isDisplayed() )
	fillGridPar( iopar );

    fillOutPar( iopar, Output::surfkey(),
		LocationOutput::surfidkey(), ioobj->key().toString() );

    BufferString attrnm = attrnmfld_->text();
    if ( attrnm.isEmpty() )
	attrnm = attrfld_->getInput();

    BufferString attrfnm =
	EM::SurfaceAuxData::getFileName( *ioobj, attrnm );
    if ( !attrfnm.isEmpty() )
    {
	const int val = uiMSG().askOverwrite( tr("Horizon data with "
            "this attribute name already exists."
	    "\n\nDo you want to overwrite?") );
	if ( val==0 )
	    return false;
    }
    else
    {
	attrfnm = EM::SurfaceAuxData::getFreeFileName( *ioobj );
	const bool res =
	    EM::dgbSurfDataWriter::writeDummyHeader( attrfnm, attrnm );
	if ( !res )
	{
	    uiMSG().error(tr("Cannot save Horizon data to: %1").arg(attrfnm));
	    return false;
	}
    }

    iopar.set( sKey::Target(), attrnm );
    return true;
}


void uiAttrSurfaceOut::fillGridPar( IOPar& par ) const
{
    IOPar gridpar, iopar;
    if ( interpol_ )
    {
	gridpar.set( sKey::Name(), methodname_ );
	interpol_->fillPar( gridpar );
    }

    iopar.mergeComp( gridpar, "Grid" );
    par.merge( iopar );
}
