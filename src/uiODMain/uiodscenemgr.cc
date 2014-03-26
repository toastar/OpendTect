/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodscenemgr.h"
#include "scene.xpm"

#include "uiattribpartserv.h"
#include "uiodapplmgr.h"
#include "uiempartserv.h"
#include "uivispartserv.h"
#include "uiwellattribpartserv.h"

#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uidockwin.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uigeninputdlg.h"
#include "uilabel.h"
#include "uimdiarea.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uiodviewer2dmgr.h"
#include "uiprintscenedlg.h"
#include "ui3dviewer.h"
#include "uiscenepropdlg.h"
#include "uistatusbar.h"
#include "uitoolbar.h"
#include "uitreeitemmanager.h"
#include "uitreeview.h"
#include "uiviscoltabed.h"
#include "uiwindowgrabber.h"

#include "ioman.h"
#include "ioobj.h"
#include "pickset.h"
#include "ptrman.h"
#include "settings.h"
#include "sorting.h"
#include "survinfo.h"
#include "timer.h"
#include "visdata.h"
#include "vissurvscene.h"
#include "vissurvobj.h"
#include "welltransl.h"

// For factories
#include "uiodbodydisplaytreeitem.h"
#include "uioddatatreeitem.h"
#include "uiodemsurftreeitem.h"
#include "uiodfaulttreeitem.h"
#include "uiodhortreeitem.h"
#include "uiodpicksettreeitem.h"
#include "uiodplanedatatreeitem.h"
#include "uiodpseventstreeitem.h"
#include "uiodrandlinetreeitem.h"
#include "uiodseis2dtreeitem.h"
#include "uiodscenetreeitem.h"
#include "uiodvolrentreeitem.h"
#include "uiodwelltreeitem.h"

#define mPosField	0
#define mValueField	1
#define mNameField	2
#define mStatusField	3

static const int cWSWidth = 600;
static const int cWSHeight = 500;
static const int cMinZoom = 1;
static const int cMaxZoom = 150;
static const char* scenestr = "Scene ";

#define mWSMCB(fn) mCB(this,uiODSceneMgr,fn)
#define mDoAllScenes(memb,fn,arg) \
    for ( int idx=0; idx<scenes_.size(); idx++ ) \
	scenes_[idx]->memb->fn( arg )

uiODSceneMgr::uiODSceneMgr( uiODMain* a )
    : appl_(*a)
    , mdiarea_(new uiMdiArea(a,"OpendTect work space"))
    , vwridx_(0)
    , lasthrot_(0), lastvrot_(0), lastdval_(0)
    , tifs_(new uiTreeFactorySet)
    , wingrabber_(new uiWindowGrabber(a))
    , activeSceneChanged(this)
    , sceneClosed(this)
    , treeToBeAdded(this)
    , viewModeChanged(this)
    , scenetimer_(new Timer)
{
    tifs_->addFactory( new uiODInlineTreeItemFactory, 1000,
		       SurveyInfo::No2D );
    tifs_->addFactory( new uiODCrosslineTreeItemFactory, 2000,
		       SurveyInfo::No2D );
    tifs_->addFactory( new uiODZsliceTreeItemFactory, 3000,
		       SurveyInfo::No2D );
    tifs_->addFactory( new uiODVolrenTreeItemFactory, 3100, SurveyInfo::No2D );
    tifs_->addFactory( new uiODRandomLineTreeItemFactory, 3500,
		       SurveyInfo::No2D );
    tifs_->addFactory( new Seis2DTreeItemFactory, 4000, SurveyInfo::Only2D );
    tifs_->addFactory( new uiODPickSetTreeItemFactory, 5000,
		       SurveyInfo::Both2DAnd3D );
    tifs_->addFactory( new uiODHorizonTreeItemFactory, 6000,
		       SurveyInfo::Both2DAnd3D );
    tifs_->addFactory( new uiODHorizon2DTreeItemFactory, 6500,
		       SurveyInfo::Only2D );
    tifs_->addFactory( new uiODFaultTreeItemFactory, 7000 );
    tifs_->addFactory( new uiODFaultStickSetTreeItemFactory, 7100,
		       SurveyInfo::Both2DAnd3D );
    tifs_->addFactory( new uiODBodyDisplayTreeItemFactory, 7500,
		       SurveyInfo::No2D );
    tifs_->addFactory( new uiODWellTreeItemFactory, 8000,
		       SurveyInfo::Both2DAnd3D );
    tifs_->addFactory( new uiODPSEventsTreeItemFactory, 8500,
		       SurveyInfo::Both2DAnd3D );

    mdiarea_->windowActivated.notify( mCB(this,uiODSceneMgr,mdiAreaChanged) );
    mdiarea_->setPrefWidth( cWSWidth );
    mdiarea_->setPrefHeight( cWSHeight );

    scenetimer_->tick.notify( mCB(this,uiODSceneMgr,sceneTimerCB) );
}


uiODSceneMgr::~uiODSceneMgr()
{
    cleanUp( false );
    delete tifs_;
    delete mdiarea_;
    delete wingrabber_;
}


void uiODSceneMgr::initMenuMgrDepObjs()
{
    if ( scenes_.isEmpty() )
	addScene(true);
}


void uiODSceneMgr::cleanUp( bool startnew )
{
    mdiarea_->closeAll();
    // closeAll() cascades callbacks which remove the scene from set

    visServ().deleteAllObjects();
    vwridx_ = 0;
    if ( startnew ) addScene(true);
}


uiODSceneMgr::Scene& uiODSceneMgr::mkNewScene()
{
    uiODSceneMgr::Scene& scn = *new uiODSceneMgr::Scene( mdiarea_ );
    scn.mdiwin_->closed().notify( mWSMCB(removeSceneCB) );
    scenes_ += &scn;
    vwridx_++;
    BufferString vwrnm( "Viewer Scene ", vwridx_ );
    scn.sovwr_->setName( vwrnm );
    return scn;
}


int uiODSceneMgr::addScene( bool maximized, ZAxisTransform* zt,
			    const char* name )
{
    Scene& scn = mkNewScene();
    const int sceneid = visServ().addScene();
    mDynamicCastGet(visSurvey::Scene*,visscene,visServ().getObject(sceneid));
    if ( visscene && scn.sovwr_->getPolygonSelector() )
	visscene->setPolygonSelector( scn.sovwr_->getPolygonSelector() );
    if ( visscene && scn.sovwr_->getSceneColTab() )
	visscene->setSceneColTab( scn.sovwr_->getSceneColTab() );

    scn.sovwr_->setSceneID( sceneid );
    BufferString title( scenestr );
    title += vwridx_;
    scn.mdiwin_->setTitle( title );
    visServ().setObjectName( sceneid, title );
    scn.sovwr_->display( true );
    scn.sovwr_->viewAll( false );
    scn.sovwr_->viewmodechanged.notify( mWSMCB(viewModeChg) );
    scn.sovwr_->pageupdown.notify( mCB(this,uiODSceneMgr,pageUpDownPressed) );
    scn.mdiwin_->display( true, false, maximized );
    actMode(0);
    treeToBeAdded.trigger( sceneid );
    initTree( scn, vwridx_ );

    if ( scenes_.size()>1 && scenes_[0] )
    {
	scn.sovwr_->setStereoType( scenes_[0]->sovwr_->getStereoType() );
	scn.sovwr_->setStereoOffset(
		scenes_[0]->sovwr_->getStereoOffset() );
	scn.sovwr_->showRotAxis( scenes_[0]->sovwr_->rotAxisShown() );
	if ( !scenes_[0]->sovwr_->isCameraPerspective() )
	    scn.sovwr_->toggleCameraType();
	visServ().displaySceneColorbar( visServ().sceneColorbarDisplayed() );
    }
    else if ( scenes_[0] )
    {
	const bool isperspective = scenes_[0]->sovwr_->isCameraPerspective();
	menuMgr().setCameraPixmap( isperspective );
	scn.sovwr_->showRotAxis( true );
	menuMgr().updateAxisMode( true );
    }

    if ( name ) setSceneName( sceneid, name );

    visServ().setZAxisTransform( sceneid, zt, 0 );

    scenetimer_->start( 50, true );
    visServ().turnSelectionModeOn( visServ().isSelectionModeOn() );
    return sceneid;
}


void uiODSceneMgr::sceneTimerCB( CallBacker* )
{
    if ( scenes_.size() > 1 )
	tile();
}


void uiODSceneMgr::removeScene( uiODSceneMgr::Scene& scene )
{
    appl_.colTabEd().setColTab( 0, mUdf(int), mUdf(int) );
    appl_.removeDockWindow( scene.dw_ );

    if ( scene.itemmanager_ )
    {
	scene.itemmanager_->askContinueAndSaveIfNeeded( false );
	scene.itemmanager_->prepareForShutdown();
	visServ().removeScene( scene.itemmanager_->sceneID() );
	sceneClosed.trigger( scene.itemmanager_->sceneID() );
    }

    scene.mdiwin_->closed().remove( mWSMCB(removeSceneCB) );
    scenes_ -= &scene;
    delete &scene;
}


void uiODSceneMgr::removeSceneCB( CallBacker* cb )
{
    mDynamicCastGet(uiGroupObj*,grp,cb)
    if ( !grp ) return;
    int idxnr = -1;
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( grp == scenes_[idx]->mdiwin_->mainObject() )
	{
	    idxnr = idx;
	    break;
	}
    }
    if ( idxnr < 0 ) return;

    uiODSceneMgr::Scene* scene = scenes_[idxnr];
    removeScene( *scene );
}


void uiODSceneMgr::setSceneName( int sceneid, const char* nm )
{
    visServ().setObjectName( sceneid, nm );
    Scene* scene = getScene( sceneid );
    if ( !scene ) return;

    scene->mdiwin_->setTitle( nm );
    scene->dw_->setDockName( nm );
    uiTreeItem* itm = scene->itemmanager_->findChild( sceneid );
    if ( itm )
	itm->updateColumnText( uiODSceneMgr::cNameColumn() );
}


const char* uiODSceneMgr::getSceneName( int sceneid ) const
{ return const_cast<uiODSceneMgr*>(this)->visServ().getObjectName( sceneid ); }


void uiODSceneMgr::getScenePars( IOPar& iopar )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	IOPar iop;
	scenes_[idx]->sovwr_->fillPar( iop );
	iopar.mergeComp( iop, toString(idx) );
    }
}


void uiODSceneMgr::useScenePars( const IOPar& sessionpar )
{
    for ( int idx=0; ; idx++ )
    {
	PtrMan<IOPar> scenepar = sessionpar.subselect( toString(idx) );
	if ( !scenepar || !scenepar->size() )
	{
	    if ( !idx ) continue;
	    break;
	}

	Scene& scn = mkNewScene();
	if ( !scn.sovwr_->usePar(*scenepar) )
	{
	    removeScene( scn );
	    continue;
	}

	visBase::DataObject* obj =
	    visBase::DM().getObject( scn.sovwr_->sceneID() );
	mDynamicCastGet( visSurvey::Scene*,visscene,obj );

	if ( visscene )
	{
	    if ( scn.sovwr_->getPolygonSelector() )
		visscene->setPolygonSelector(scn.sovwr_->getPolygonSelector());
	    if ( scn.sovwr_->getSceneColTab() )
		visscene->setSceneColTab( scn.sovwr_->getSceneColTab() );
	}

	visServ().displaySceneColorbar( visServ().sceneColorbarDisplayed() );
	visServ().turnSelectionModeOn( false );

	BufferString title( scenestr );
	title += vwridx_;
	scn.mdiwin_->setTitle( title );
	visServ().setObjectName( scn.sovwr_->sceneID(), title );

	scn.sovwr_->display( true );
	scn.sovwr_->showRotAxis( true );
	scn.sovwr_->viewmodechanged.notify( mWSMCB(viewModeChg) );
	scn.sovwr_->pageupdown.notify(mCB(this,uiODSceneMgr,pageUpDownPressed));
	scn.mdiwin_->display( true, false );

	treeToBeAdded.trigger( scn.sovwr_->sceneID() );
	initTree( scn, vwridx_ );
    }

    ObjectSet<ui3DViewer> vwrs;
    getSoViewers( vwrs );
    if ( !vwrs.isEmpty() && vwrs[0] )
    {
	const bool isperspective = vwrs[0]->isCameraPerspective();
	menuMgr().setCameraPixmap( isperspective );
	menuMgr().updateAxisMode( true );
    }

    rebuildTrees();

}


void uiODSceneMgr::setSceneProperties()
{
    ObjectSet<ui3DViewer> vwrs;
    getSoViewers( vwrs );
    if ( vwrs.isEmpty() )
    {
	uiMSG().error( "No scenes available" );
	return;
    }

    int curvwridx = 0;
    if ( vwrs.size() > 1 )
    {
	const int sceneid = askSelectScene();
	const ui3DViewer* vwr = getSoViewer( sceneid );
	if ( !vwr ) return;

	curvwridx = vwrs.indexOf( vwr );
    }

    uiScenePropertyDlg dlg( &appl_, vwrs, curvwridx );
    dlg.go();
}


void uiODSceneMgr::viewModeChg( CallBacker* cb )
{
    if ( scenes_.isEmpty() ) return;

    mDynamicCastGet(ui3DViewer*,sovwr_,cb)
    if ( sovwr_ ) setToViewMode( sovwr_->isViewMode() );
}


void uiODSceneMgr::setToViewMode( bool yn )
{
    mDoAllScenes(sovwr_,setViewMode,yn);
    visServ().setViewMode( yn , false );
    menuMgr().updateViewMode( yn );
    updateStatusBar();
    viewModeChanged.trigger();
}


void uiODSceneMgr::setToWorkMode(uiVisPartServer::WorkMode wm)
{
    bool yn = ( wm == uiVisPartServer::View ) ? true : false;

    mDoAllScenes(sovwr_,setViewMode,yn);
    visServ().setWorkMode( wm , false );
    menuMgr().updateViewMode( yn );
    updateStatusBar();
}


void uiODSceneMgr::actMode( CallBacker* )
{
    setToWorkMode( uiVisPartServer::Interactive );
}


void uiODSceneMgr::viewMode( CallBacker* )
{
    setToWorkMode( uiVisPartServer::View );
}


void uiODSceneMgr::pageUpDownPressed( CallBacker* cb )
{
    mCBCapsuleUnpack(bool,up,cb);
    applMgr().pageUpDownPressed( up );
}


void uiODSceneMgr::updateStatusBar()
{
    if ( visServ().isViewMode() )
    {
	appl_.statusBar()->message( "", mPosField );
	appl_.statusBar()->message( "", mValueField );
	appl_.statusBar()->message( "", mNameField );
	appl_.statusBar()->message( "", mStatusField );
	appl_.statusBar()->setBGColor( mStatusField,
				   appl_.statusBar()->getBGColor(mPosField) );
    }

    const Coord3 xytpos = visServ().getMousePos( true );
    const bool haspos = !mIsUdf( xytpos.x );

    BufferString msg;
    if ( haspos  )
    {
	const BinID bid( SI().transform( Coord(xytpos.x,xytpos.y) ) );
	msg = bid.toString();
	msg += "   (";
	msg += mNINT32(xytpos.x); msg += ", ";
	msg += mNINT32(xytpos.y); msg += ", ";

	const float zfact = mCast(float,visServ().zFactor());
	float zval = (float) (zfact * xytpos.z);
	if ( zfact>100 || zval>10 ) zval = mCast( float, mNINT32(zval) );
	msg += zval; msg += ")";
    }

    appl_.statusBar()->message( msg, mPosField );

    const BufferString valstr = visServ().getMousePosVal();
    if ( haspos )
    {
	msg = valstr.isEmpty() ? "" : "Value = ";
	msg += valstr;
    }
    else
	msg = "";

    appl_.statusBar()->message( msg, mValueField );

    msg = haspos ? visServ().getMousePosString() : "";
    if ( msg.isEmpty() )
    {
	const int selid = visServ().getSelObjectId();
	msg = visServ().getInteractionMsg( selid );
    }
    appl_.statusBar()->message( msg, mNameField );

    visServ().getPickingMessage( msg );
    appl_.statusBar()->message( msg, mStatusField );

    appl_.statusBar()->setBGColor( mStatusField, visServ().isPicking() ?
	    Color(255,0,0) : appl_.statusBar()->getBGColor(mPosField) );
}


void uiODSceneMgr::setKeyBindings()
{
    if ( scenes_.isEmpty() ) return;

    BufferStringSet keyset;
    scenes_[0]->sovwr_->getAllKeyBindings( keyset );

    StringListInpSpec* inpspec = new StringListInpSpec( keyset );
    inpspec->setText( scenes_[0]->sovwr_->getCurrentKeyBindings(), 0 );
    uiGenInputDlg dlg( &appl_, "Select Mouse Controls", "Select", inpspec );
    dlg.setHelpKey("0.2.7");
    if ( dlg.go() )
	mDoAllScenes(sovwr_,setKeyBindings,dlg.text());
}


int uiODSceneMgr::getStereoType() const
{
    return scenes_.size() ? (int)scenes_[0]->sovwr_->getStereoType() : 0;
}


void uiODSceneMgr::setStereoType( int type )
{
    if ( scenes_.isEmpty() ) return;

    ui3DViewer::StereoType stereotype = (ui3DViewer::StereoType)type;
    const float stereooffset = scenes_[0]->sovwr_->getStereoOffset();
    for ( int ids=0; ids<scenes_.size(); ids++ )
    {
	ui3DViewer& sovwr_ = *scenes_[ids]->sovwr_;
	if ( !sovwr_.setStereoType(stereotype) )
	{
	    uiMSG().error( "No support for this type of stereo rendering" );
	    return;
	}
	if ( type )
	    sovwr_.setStereoOffset( stereooffset );
    }

    if ( type>0 )
	applMgr().setStereoOffset();
}


void uiODSceneMgr::tile()		{ mdiarea_->tile(); }
void uiODSceneMgr::tileHorizontal()	{ mdiarea_->tileHorizontal(); }
void uiODSceneMgr::tileVertical()	{ mdiarea_->tileVertical(); }
void uiODSceneMgr::cascade()		{ mdiarea_->cascade(); }


void uiODSceneMgr::layoutScenes()
{
    const int nrgrps = scenes_.size();
    if ( nrgrps == 1 && scenes_[0] )
	scenes_[0]->mdiwin_->display( true, false, true );
    else if ( nrgrps>1 && scenes_[0] )
	tile();
}


void uiODSceneMgr::toHomePos( CallBacker* )
{ mDoAllScenes(sovwr_,toHomePos,); }
void uiODSceneMgr::saveHomePos( CallBacker* )
{ mDoAllScenes(sovwr_,saveHomePos,); }
void uiODSceneMgr::viewAll( CallBacker* )
{ mDoAllScenes(sovwr_,viewAll,); }
void uiODSceneMgr::align( CallBacker* )
{ mDoAllScenes(sovwr_,align,); }

void uiODSceneMgr::viewX( CallBacker* )
{ mDoAllScenes(sovwr_,viewPlane,ui3DViewer::X); }
void uiODSceneMgr::viewY( CallBacker* )
{ mDoAllScenes(sovwr_,viewPlane,ui3DViewer::Y); }
void uiODSceneMgr::viewZ( CallBacker* )
{ mDoAllScenes(sovwr_,viewPlane,ui3DViewer::Z); }
void uiODSceneMgr::viewInl( CallBacker* )
{ mDoAllScenes(sovwr_,viewPlane,ui3DViewer::Inl); }
void uiODSceneMgr::viewCrl( CallBacker* )
{ mDoAllScenes(sovwr_,viewPlane,ui3DViewer::Crl); }

void uiODSceneMgr::setViewSelectMode( int md )
{
    mDoAllScenes(sovwr_,viewPlane,(ui3DViewer::PlaneType)md);
}


void uiODSceneMgr::showRotAxis( CallBacker* cb )
{
    mDynamicCastGet(const uiAction*,act,cb)
    mDoAllScenes(sovwr_,showRotAxis,act?act->isChecked():false);
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	const Color& col = applMgr().visServer()->getSceneAnnotCol( idx );
	scenes_[idx]->sovwr_->setAxisAnnotColor( col );
    }
}


class uiSnapshotDlg : public uiDialog
{
public:
			uiSnapshotDlg(uiParent*);

    enum		SnapshotType { Scene=0, Window, Desktop };
    SnapshotType	getSnapshotType() const;

protected:
    uiButtonGroup*	butgrp_;
};


uiSnapshotDlg::uiSnapshotDlg( uiParent* p )
    : uiDialog( p, uiDialog::Setup("Specify snapshot",
				   "Select area to take snapshot","50.0.1") )
{
    butgrp_ = new uiButtonGroup( this, "Area type", uiObject::Vertical );
    butgrp_->setExclusive( true );
    new uiRadioButton( butgrp_, "Scene" );
    new uiRadioButton( butgrp_, "Window" );
    new uiRadioButton( butgrp_, "Desktop" );
    butgrp_->selectButton( 0 );
}


uiSnapshotDlg::SnapshotType uiSnapshotDlg::getSnapshotType() const
{ return (uiSnapshotDlg::SnapshotType) butgrp_->selectedId(); }


void uiODSceneMgr::mkSnapshot( CallBacker* )
{
    uiSnapshotDlg snapdlg( &appl_ );
    if ( !snapdlg.go() )
	return;

    if ( snapdlg.getSnapshotType() == uiSnapshotDlg::Scene )
    {
	/*
	ObjectSet<ui3DViewer> viewers;
	getSoViewers( viewers );
	if ( viewers.size() == 0 ) return;

	uiPrintSceneDlg printdlg( &appl_, viewers );
	printdlg.go();
	// TODO: save settings in iopar
	 */
	pErrMsg("Print screen not implemented with osg.");
    }
    else
    {
	const bool desktop = snapdlg.getSnapshotType()==uiSnapshotDlg::Desktop;
	wingrabber_->grabDesktop( desktop );
	wingrabber_->go();
    }
}


void uiODSceneMgr::soloMode( CallBacker* )
{
    TypeSet< TypeSet<int> > dispids;
    int selectedid = -1;
    const bool issolomodeon = menuMgr().isSoloModeOn();
    for ( int idx=0; idx<scenes_.size(); idx++ )
	dispids += scenes_[idx]->itemmanager_->getDisplayIds( selectedid,
							      !issolomodeon );

    visServ().setSoloMode( issolomodeon, dispids, selectedid );
    updateSelectedTreeItem();
}


void uiODSceneMgr::switchCameraType( CallBacker* )
{
    ObjectSet<ui3DViewer> vwrs;
    getSoViewers( vwrs );
    if ( vwrs.isEmpty() ) return;
    mDoAllScenes(sovwr_,toggleCameraType,);
    const bool isperspective = vwrs[0]->isCameraPerspective();
    menuMgr().setCameraPixmap( isperspective );
}


int uiODSceneMgr::askSelectScene() const
{
    BufferStringSet scenenms; TypeSet<int> sceneids;
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	int sceneid = scenes_[idx]->itemmanager_->sceneID();
	sceneids += sceneid;
	scenenms.add( getSceneName(sceneid) );
    }

    if ( sceneids.size() < 2 )
	return sceneids.isEmpty() ? -1 : sceneids[0];

    StringListInpSpec* inpspec = new StringListInpSpec( scenenms );
    uiGenInputDlg dlg( &appl_, "Choose scene", "", inpspec );
    const int selidx = dlg.go() ? dlg.getIntValue() : -1;
    return sceneids.validIdx(selidx) ? sceneids[selidx] : -1;
}


void uiODSceneMgr::getSoViewers( ObjectSet<ui3DViewer>& vwrs )
{
    vwrs.erase();
    for ( int idx=0; idx<scenes_.size(); idx++ )
	vwrs += scenes_[idx]->sovwr_;
}


const ui3DViewer* uiODSceneMgr::getSoViewer( int sceneid ) const
{
    const Scene* scene = getScene( sceneid );
    return scene ? scene->sovwr_ : 0;
}


ui3DViewer* uiODSceneMgr::getSoViewer( int sceneid )
{
    const Scene* scene = getScene( sceneid );
    return scene ? scene->sovwr_ : 0;
}


uiODTreeTop* uiODSceneMgr::getTreeItemMgr( const uiTreeView* lv ) const
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( scenes_[idx]->lv_ == lv )
	    return scenes_[idx]->itemmanager_;
    }
    return 0;
}


void uiODSceneMgr::getSceneNames( BufferStringSet& nms, int& active ) const
{
    TypeSet<uiString> windownames;
    mdiarea_->getWindowNames( windownames );

    nms.setEmpty();
    for ( int idx=0; idx<windownames.size(); idx++ )
    {
	nms.add( windownames[idx].getFullString() );
    }

    const char* activenm = mdiarea_->getActiveWin();
    active = nms.indexOf( activenm );
}


void uiODSceneMgr::getActiveSceneName( BufferString& nm ) const
{ nm = mdiarea_->getActiveWin(); }


int uiODSceneMgr::getActiveSceneID() const
{
    const BufferString scenenm = mdiarea_->getActiveWin();
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( !scenes_[idx] || !scenes_[idx]->itemmanager_ )
	    continue;

	if ( scenenm == getSceneName(scenes_[idx]->itemmanager_->sceneID()) )
	    return scenes_[idx]->itemmanager_->sceneID();
    }

    return -1;
}


void uiODSceneMgr::mdiAreaChanged( CallBacker* )
{
//    const bool wasparalysed = mdiarea_->paralyse( true );
    if ( &menuMgr() ) menuMgr().updateSceneMenu();
//    mdiarea_->paralyse( wasparalysed );
    activeSceneChanged.trigger();
}


void uiODSceneMgr::setActiveScene( const char* scenenm )
{
    mdiarea_->setActiveWin( scenenm );
    activeSceneChanged.trigger();
}


void uiODSceneMgr::initTree( Scene& scn, int vwridx )
{
    BufferString capt( "Tree scene " ); capt += vwridx;
    scn.dw_ = new uiDockWin( &appl_, capt );
    scn.dw_->setMinimumWidth( 200 );
    scn.lv_ = new uiTreeView( scn.dw_, capt );
    scn.dw_->setObject( scn.lv_ );
    BufferStringSet labels;
    labels.add( "Elements" );
    labels.add( "Color" );
    scn.lv_->addColumns( labels );
    scn.lv_->setFixedColumnWidth( cColorColumn(), 40 );

    scn.itemmanager_ = new uiODTreeTop( scn.sovwr_, scn.lv_, &applMgr(), tifs_);
    uiODSceneTreeItem* sceneitm =
	new uiODSceneTreeItem( scn.mdiwin_->getTitle().getFullString(),
			       scn.sovwr_->sceneID() );
    scn.itemmanager_->addChild( sceneitm, false );

    TypeSet<int> idxs;
    TypeSet<int> placeidxs;

    for ( int idx=0; idx<tifs_->nrFactories(); idx++ )
    {
	SurveyInfo::Pol2D pol2d = (SurveyInfo::Pol2D)tifs_->getPol2D( idx );
	if ( SI().survDataType() == SurveyInfo::Both2DAnd3D ||
	     pol2d == SurveyInfo::Both2DAnd3D ||
	     pol2d == SI().survDataType() )
	{
	    idxs += idx;
	    placeidxs += tifs_->getPlacementIdx( idx );
	}
    }

    sort_coupled( placeidxs.arr(), idxs.arr(), idxs.size() );

    for ( int idx=0; idx<idxs.size(); idx++ )
    {
	const int fidx = idxs[idx];
	scn.itemmanager_->addChild(
		tifs_->getFactory(fidx)->create(), true );
    }

    scn.lv_->display( true );
    appl_.addDockWindow( *scn.dw_, uiMainWin::Left );
    scn.dw_->display( true );
}


void uiODSceneMgr::updateTrees()
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	scene.itemmanager_->updateColumnText( cNameColumn() );
	scene.itemmanager_->updateColumnText( cColorColumn() );
	scene.itemmanager_->updateCheckStatus();
    }
}


void uiODSceneMgr::rebuildTrees()
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	const int sceneid = scene.sovwr_->sceneID();
	TypeSet<int> visids; visServ().getChildIds( sceneid, visids );

	for ( int idy=0; idy<visids.size(); idy++ )
	{
	    mDynamicCastGet( const visSurvey::SurveyObject*, surobj,
		visServ().getObject(visids[idy]) );

	    if ( surobj && surobj->getSaveInSessionsFlag() == false )
		continue;

	    uiODDisplayTreeItem::create( scene.itemmanager_, &applMgr(),
					 visids[idy] );
	}
    }
    updateSelectedTreeItem();
}


uiTreeView* uiODSceneMgr::getTree( int sceneid )
{
    Scene* scene = getScene( sceneid );
    return scene ? scene->lv_ : 0;
}


void uiODSceneMgr::setItemInfo( int id )
{
    mDoAllScenes(itemmanager_,updateColumnText,cColorColumn());
    appl_.statusBar()->message( "", mPosField );
    appl_.statusBar()->message( "", mValueField );
    appl_.statusBar()->message( visServ().getInteractionMsg(id), mNameField );
    appl_.statusBar()->message( "", mStatusField );
    appl_.statusBar()->setBGColor( mStatusField,
				   appl_.statusBar()->getBGColor(mPosField) );
}


void uiODSceneMgr::updateItemToolbar( int id )
{
    visServ().getToolBarHandler()->setMenuID( id );
    visServ().getToolBarHandler()->executeMenu(); // addButtons
}


void uiODSceneMgr::updateSelectedTreeItem()
{
    const int id = visServ().getSelObjectId();
    updateItemToolbar( id );

    if ( id != -1 )
    {
	setItemInfo( id );
	//applMgr().modifyColorTable( id );
	if ( !visServ().isOn(id) ) visServ().turnOn(id, true, true);
	else if ( scenes_.size() != 1 && visServ().isSoloMode() )
	    visServ().updateDisplay( true, id );
    }

    if ( !applMgr().attrServer() )
	return;

    bool found = applMgr().attrServer()->attrSetEditorActive();
    bool gotoact = false;
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	if ( !found )
	{
	    mDynamicCastGet( const uiODDisplayTreeItem*, treeitem,
			     scene.itemmanager_->findChild(id) );
	    if ( treeitem )
	    {
		gotoact = treeitem->actModeWhenSelected();
		found = true;
	    }
	}

	scene.itemmanager_->updateSelection( id );
	scene.itemmanager_->updateColumnText( cNameColumn() );
	scene.itemmanager_->updateColumnText( cColorColumn() );
    }

    if ( gotoact && !applMgr().attrServer()->attrSetEditorActive() )
	actMode( 0 );
}


int uiODSceneMgr::getIDFromName( const char* str ) const
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	const uiTreeItem* itm = scenes_[idx]->itemmanager_->findChild( str );
	if ( itm ) return itm->selectionKey();
    }

    return -1;
}


void uiODSceneMgr::disabRightClick( bool yn )
{
    mDoAllScenes(itemmanager_,disabRightClick,yn);
}


void uiODSceneMgr::disabTrees( bool yn )
{
    const bool wasparalysed = mdiarea_->paralyse( true );

    for ( int idx=0; idx<scenes_.size(); idx++ )
	scenes_[idx]->lv_->setSensitive( !yn );

    mdiarea_->paralyse( wasparalysed );
}


#define mGetOrAskForScene \
    Scene* scene = getScene( sceneid ); \
    if ( !scene ) \
    { \
	sceneid = askSelectScene(); \
	scene = getScene( sceneid ); \
    } \
    if ( !scene ) return -1;

int uiODSceneMgr::addWellItem( const MultiID& mid, int sceneid )
{
    mGetOrAskForScene

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !scene || !ioobj ) return -1;

    if ( ioobj->group() != mTranslGroupName(Well) )
	return -1;

    uiODDisplayTreeItem* itm = new uiODWellTreeItem( mid );
    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


int uiODSceneMgr::addEMItem( const EM::ObjectID& emid, int sceneid )
{
    mGetOrAskForScene

    FixedString type = applMgr().EMServer()->getType( emid );
    uiODDisplayTreeItem* itm;
    if ( type=="Horizon" )
	itm = new uiODHorizonTreeItem(emid,false);
    else if ( type=="2D Horizon" )
	itm = new uiODHorizon2DTreeItem(emid);
    else if ( type=="Fault" )
	itm = new uiODFaultTreeItem(emid);
    else if ( type=="FaultStickSet" )
	itm = new uiODFaultStickSetTreeItem(emid);
    else if ( type=="RandomPosBody" )
	itm = new uiODBodyDisplayTreeItem(emid);
    else if ( type=="MCBody" )
	itm = new uiODBodyDisplayTreeItem(emid);
    else
	return -1;

    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


int uiODSceneMgr::addPickSetItem( Pick::Set& ps, int sceneid )
{
    mGetOrAskForScene

    uiODPickSetTreeItem* itm = new uiODPickSetTreeItem( -1, ps );
    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


int uiODSceneMgr::addRandomLineItem( int visid, int sceneid )
{
    mGetOrAskForScene

    uiODRandomLineTreeItem* itm = new uiODRandomLineTreeItem( visid );
    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


int uiODSceneMgr::add2DLineSetItem( const MultiID& mid, const char* name,
				    int displayid, int sceneid )
{
    mGetOrAskForScene

    uiOD2DLineSetTreeItem* itm = new uiOD2DLineSetTreeItem( mid );
    scene->itemmanager_->addChild( itm, false );

    uiOD2DLineTreeItem* subitm = new uiOD2DLineTreeItem( name, displayid );
    itm->addChild( subitm, false );
    return subitm->displayID();
}


void uiODSceneMgr::removeTreeItem( int displayid )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	uiTreeItem* itm = scene.itemmanager_->findChild( displayid );
	if ( itm ) scene.itemmanager_->removeChild( itm );
    }
}


uiTreeItem* uiODSceneMgr::findItem( int displayid )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	uiTreeItem* itm = scene.itemmanager_->findChild( displayid );
	if ( itm ) return itm;
    }

    return 0;
}


void uiODSceneMgr::findItems( const char* nm, ObjectSet<uiTreeItem>& items )
{
    deepErase( items );
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	scene.itemmanager_->findChildren( nm, items );
    }
}


void uiODSceneMgr::displayIn2DViewer( int visid, int attribid, bool dowva )
{
    appl_.viewer2DMgr().displayIn2DViewer( visid, attribid, dowva );
}


void uiODSceneMgr::remove2DViewer( int visid )
{
    appl_.viewer2DMgr().remove2DViewer( visid );
}


void uiODSceneMgr::doDirectionalLight(CallBacker*)
{
    visServ().setDirectionalLight();
}


float uiODSceneMgr::getHeadOnLightIntensity( int sceneid ) const
{
    const Scene* scene = getScene( sceneid );
    return scene && scene->sovwr_
	? scene->sovwr_->getHeadOnLightIntensity() : 0;
}


void uiODSceneMgr::setHeadOnLightIntensity( int sceneid, float value )
{
    Scene* scene = getScene( sceneid );
    if ( scene && scene->sovwr_ )
	scene->sovwr_->setHeadOnLightIntensity( value );
}


uiODSceneMgr::Scene* uiODSceneMgr::getScene( int sceneid )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	uiODSceneMgr::Scene* scn = scenes_[idx];
	if ( scn && scn->itemmanager_ &&
		scn->itemmanager_->sceneID() == sceneid )
	    return scenes_[idx];
    }

    return 0;
}


const uiODSceneMgr::Scene* uiODSceneMgr::getScene( int sceneid ) const
{ return const_cast<uiODSceneMgr*>(this)->getScene( sceneid ); }


// uiODSceneMgr::Scene
uiODSceneMgr::Scene::Scene( uiMdiArea* mdiarea )
        : lv_(0)
	, dw_(0)
	, mdiwin_(0)
        , sovwr_(0)
	, itemmanager_(0)
{
    if ( !mdiarea ) return;

    mdiwin_ = new uiMdiAreaWindow();
    mdiwin_->setIcon( scene_xpm_data );
    sovwr_ = new ui3DViewer( mdiwin_, true );
    sovwr_->setPrefWidth( 400 );
    sovwr_->setPrefHeight( 400 );
    mdiarea->addWindow( mdiwin_ );
}


uiODSceneMgr::Scene::~Scene()
{
    delete sovwr_;
    delete itemmanager_;
    delete dw_;
}
