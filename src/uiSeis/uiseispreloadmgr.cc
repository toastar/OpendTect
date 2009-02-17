/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Feb 2009
-*/

static const char* rcsID = "$Id: uiseispreloadmgr.cc,v 1.7 2009-02-17 13:17:37 cvsbert Exp $";

#include "uiseispreloadmgr.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "seispsioprov.h"
#include "seispreload.h"
#include "seis2dline.h"
#include "strmprov.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "iodirentry.h"
#include "ptrman.h"
#include "filepath.h"
#include "filegen.h"
#include "datapack.h"
#include "survinfo.h"

#include "uimsg.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uitextedit.h"
#include "uiioobjsel.h"
#include "uitaskrunner.h"
#include "uiselsurvranges.h"


uiSeisPreLoadMgr::uiSeisPreLoadMgr( uiParent* p )
    : uiDialog(p,Setup("Pre-load manager","Pre-loaded seismic data",
			mTODOHelpID))
{
    setCtrlStyle( LeaveOnly );
    uiGroup* topgrp = new uiGroup( this, "Top group" );
    listfld_ = new uiListBox( topgrp, "Loaded entries" );
    listfld_->selectionChanged.notify( mCB(this,uiSeisPreLoadMgr,selChg) );
    topgrp->setHAlignObj( listfld_ );

    const bool has2d = SI().has2D();
    const bool has3d = SI().has3D();
    uiButtonGroup* bgrp = new uiButtonGroup( topgrp, "Manip buttons" );
    bgrp->attach( rightOf, listfld_ );
#   define mAddBut(s,fn) \
    new uiPushButton( bgrp, s, mCB(this,uiSeisPreLoadMgr,fn), false )
    if ( has3d )
	mAddBut("Add Cube",cubeLoadPush);
    if ( has2d )
	mAddBut("Add Lines",linesLoadPush);
    if ( has3d )
    {
	if ( has2d )
	    mAddBut("Add 3D Pre-Stack data",ps3DPush);
	else
	    mAddBut("Add Pre-Stack data",ps3DPush);
    }
    if ( has2d )
    {
	if ( has3d )
	    mAddBut("Add 2D Pre-Stack lines",ps2DPush);
	else
	    mAddBut("Add Pre-Stack data",ps2DPush);
    }
    mAddBut("Unload Selected",unloadPush);

    infofld_ = new uiTextEdit( this, "Info" );
    infofld_->attach( alignedBelow, topgrp );
    infofld_->attach( widthSameAs, topgrp );
    infofld_->setPrefHeightInChar( 5 );

    finaliseDone.notify( mCB(this,uiSeisPreLoadMgr,fullUpd) );
}


void uiSeisPreLoadMgr::fullUpd( CallBacker* )
{
    fillList();
}


void uiSeisPreLoadMgr::fillList()
{
    listfld_->empty();
    StreamProvider::getPreLoadedIDs( ids_ );
    if ( ids_.isEmpty() ) return;

    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	const MultiID ky( ids_.get(idx) );
	PtrMan<IOObj> ioobj = IOM().get( ky );
	if ( !ioobj )
	{
	    Seis::PreLoader(ky).unLoad();
	    ids_.remove( idx ); idx--;
	    continue;
	}
	listfld_->addItem( ioobj->name() );
    }

    listfld_->setSelected( 0 );
}


void uiSeisPreLoadMgr::selChg( CallBacker* )
{
    const int selidx = listfld_->currentItem();
    if ( ids_.isEmpty() || selidx < 0 )
	{ infofld_->setText(""); return; }

    const MultiID ky( ids_.get(selidx).buf() );
    Seis::PreLoader spl( ky );
    PtrMan<IOObj> ioobj = spl.getIOObj();
    if ( !ioobj )
	{ infofld_->setText(spl.errMsg()); return; }

    SeisIOObjInfo ioinf( *ioobj );
    if ( !ioinf.isOK() )
	{ infofld_->setText("Internal error: IOObj not OK"); return; }
    const Seis::GeomType gt = ioinf.geomType();
    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( ky.buf(), fnms );
    const int nrfiles = fnms.size();
    if ( nrfiles < 1 )
	{ infofld_->setText("No files"); return; }

    BufferString disptxt( "Data type: " ); disptxt += Seis::nameOf( gt );

    switch ( gt )
    {
	case Seis::Vol:
	break;
	case Seis::Line:
	break;
	case Seis::VolPS: {
	    Interval<int> rg = spl.inlRange();
	    if ( !mIsUdf(rg.start) )
	    {
		disptxt += "\nInline range: ";
		disptxt += rg.start; disptxt += "-"; disptxt += rg.stop;
	    }
	} break;
	case Seis::LinePS:
	break;
    }

    FilePath fp( fnms.get(0) );
    disptxt += "\nDirectory: "; disptxt += fp.pathOnly();
    disptxt += "\nFile name"; if ( nrfiles > 1 ) disptxt += "s";
    disptxt += ": "; disptxt += fp.fileName();
    float totmem = 0;
    for ( int idx=0; idx<nrfiles; idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	int dpid = StreamProvider::getPreLoadedDataPackID( fnm );
	totmem += DPM(DataPackMgr::BufID()).nrKBytesOf( dpid );

	if ( idx == 0 || (idx < nrfiles-1 && idx > 3) )
	    continue;

	fp.set( fnms.get(idx) );
	disptxt += " "; disptxt += fp.fileName();
	if ( nrfiles > 5 && idx == 3 )
	    disptxt += " ...";
    }

    totmem /= 1024; const int memmb = mNINT(totmem);
    disptxt += "\nTotal memory in MB: "; disptxt += memmb;
    infofld_->setText( disptxt );
}


void uiSeisPreLoadMgr::cubeLoadPush( CallBacker* )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc);
    ctio->ctxt.trglobexpr = "CBVS";
    uiIOObjSelDlg dlg( this, *ctio );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    Seis::PreLoader spl( dlg.ioObj()->key() );
    const char* id = spl.id().buf();
    if ( StreamProvider::isPreLoaded(id,true) )
    {
	if ( !uiMSG().askGoOn("This cube is already pre-loaded.\n"
		    	      "Do you want to re-load?") )
	    return;
	spl.unLoad();
    }

    uiTaskRunner tr( this ); spl.setRunner( tr );
    if ( !spl.loadVol() )
    {
	const char* emsg = spl.errMsg();
	if ( emsg && *emsg )
	    uiMSG().error( emsg );
    }

    fullUpd( 0 );
}


class uiSeisPreLoadMgrSel2D : public uiDialog
{
public:

uiSeisPreLoadMgrSel2D( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Preload selection","Select lines/attributes",
				 mTODOHelpID))
    , ctio_(*mMkCtxtIOObj(SeisTrc))
{
    ctio_.ctxt.trglobexpr = "2D";
    IOM().to( ctio_.ctxt.getSelKey() );
    IODirEntryList del( IOM().dirPtr(), ctio_.ctxt );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	if ( del[idx]->ioobj )
	    { ctio_.setObj( del[idx]->ioobj->clone() ); break; }
    }

    lssel_ = new uiIOObjSel( this, ctio_ );
    lssel_->selectiondone.notify( mCB(this,uiSeisPreLoadMgrSel2D,lsSel) );
    uiGroup* boxgrp = new uiGroup( this, "List boxes" );
    uiLabeledListBox* lllb = new uiLabeledListBox( boxgrp, "Line(s)", true,
	    			 uiLabeledListBox::AboveMid );
    linesel_ = lllb->box();
    uiLabeledListBox* allb = new uiLabeledListBox( boxgrp, "Attribute(s)", true,
	    			 uiLabeledListBox::AboveMid );
    allb->attach( rightOf, lllb );
    attrsel_ = allb->box();
    boxgrp->setHAlignObj( allb );
    boxgrp->attach( alignedBelow, lssel_ );

    if ( ctio_.ioobj )
	lsSel(0);
}

~uiSeisPreLoadMgrSel2D()
{
    delete ctio_.ioobj; delete &ctio_;
}

void lsSel( CallBacker* )
{
    if ( !ctio_.ioobj ) return;

    lnms_.erase(); attrnms_.erase();
    Seis2DLineSet ls( *ctio_.ioobj );
    for ( int idx=0; idx<ls.nrLines(); idx++ )
    {
	lnms_.addIfNew( ls.lineName(idx) );
	attrnms_.addIfNew( ls.attribute(idx) );
    }

    linesel_->empty(); attrsel_->empty();
    linesel_->addItems( lnms_ ); attrsel_->addItems( attrnms_ );
    linesel_->selectAll(); attrsel_->selectAll();
}


bool acceptOK( CallBacker* )
{
    if ( !lssel_->commitInput(false) || !ctio_.ioobj )
    {
	uiMSG().error( "Please select a Line Set" );
	return false;
    }

    lnms_.erase(); attrnms_.erase();
    linesel_->getSelectedItems(lnms_); attrsel_->getSelectedItems(attrnms_);
    if ( lnms_.isEmpty() || attrnms_.isEmpty() )
    {
	uiMSG().error( "Please select one or more lines and attributes" );
	return false;
    }
    return true;
}

    CtxtIOObj&	ctio_;
    uiIOObjSel*	lssel_;
    uiListBox*	linesel_;
    uiListBox*	attrsel_;

    BufferStringSet	lnms_;
    BufferStringSet	attrnms_;

};


void uiSeisPreLoadMgr::linesLoadPush( CallBacker* )
{
    uiSeisPreLoadMgrSel2D dlg( this );
    if ( !dlg.go() ) return;

    Seis::PreLoader spl( dlg.ctio_.ioobj->key() );
    if ( !spl.loadLines(dlg.lnms_,dlg.attrnms_) )
    {
	const char* emsg = spl.errMsg();
	if ( emsg && *emsg )
	    uiMSG().error( emsg );
    }

    fullUpd( 0 );
}


void uiSeisPreLoadMgr::ps3DPush( CallBacker* )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisPS3D);
    ctio->ctxt.trglobexpr = "CBVS";
    uiIOObjSelDlg dlg( this, *ctio, "Select data store/part to load" );
    dlg.setCaption( "Select data store" );
    uiSelNrRange* inlrgfld = new uiSelNrRange( dlg.selGrp()->getTopGroup(),
	    				uiSelNrRange::Inl, false );
    inlrgfld->attach( centeredBelow, dlg.selGrp()->getListField() );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    Seis::PreLoader spl( dlg.ioObj()->key() );
    const char* id = spl.id().buf();
    Interval<int> inlrg; assign(inlrg,inlrgfld->getRange());
    uiTaskRunner tr( this ); spl.setRunner( tr );
    if ( !spl.loadPS3D(&inlrg) )
    {
	const char* emsg = spl.errMsg();
	if ( emsg && *emsg )
	    uiMSG().error( emsg );
    }

    fullUpd( 0 );
}


void uiSeisPreLoadMgr::ps2DPush( CallBacker* )
{
    uiMSG().error( "TODO: implement pre-load PS 2D data" );
}


void uiSeisPreLoadMgr::unloadPush( CallBacker* )
{
    const int selidx = listfld_->currentItem();
    if ( selidx < 0 ) return;

    BufferString msg( "Unload '" );
    msg += listfld_->textOfItem( selidx );
    msg += "'?\n(This will not delete the object from disk)";
    if ( !uiMSG().askGoOn( msg ) )
	return;

    Seis::PreLoader spl( MultiID(ids_.get(selidx)) );
    spl.unLoad();

    fillList();
    int newselidx = selidx;
    if ( newselidx >= ids_.size() )
	newselidx = ids_.size() - 1;
    if ( newselidx >= 0 )
	listfld_->setCurrentItem( newselidx );
    else
	selChg( 0 );
}
