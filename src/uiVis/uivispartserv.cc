/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.cc,v 1.98 2002-11-01 01:40:06 bert Exp $
________________________________________________________________________

-*/

#include "uivispartserv.h"

#include "visdataman.h"
#include "vissurvpickset.h"
#include "vissurvscene.h"
#include "vissurvwell.h"
#include "vissurvsurf.h"
#include "vissurvhorizon.h"
#include "visplanedatadisplay.h"
#include "visvolumedisplay.h"
#include "visselman.h"
#include "vismaterial.h"
#include "viscolortab.h"
#include "visrectangle.h"
#include "vistexturerect.h"
#include "visobject.h"
#include "uiexecutor.h"
#include "emmanager.h"
#include "emfault.h"

#include "uimsg.h"

#include "pickset.h"
#include "cubesampling.h"
#include "attribsel.h"
#include "attribslice.h"
#include "surfaceinfo.h"
#include "thread.h"
#include "survinfo.h"
#include "uidset.h"
#include "draw.h"
#include "color.h"
#include "colortab.h"
#include "settings.h"
#include "errh.h"

#include "uizscaledlg.h"
#include "uiworkareadlg.h"
#include "uimaterialdlg.h"
#include "uipickszdlg.h"
#include "uislicesel.h"
#include "uisellinest.h"

#include "uiprintscenedlg.h"

const int uiVisPartServer::evManipulatorMove   	= 0;
const int uiVisPartServer::evSelection		= 1;
const int uiVisPartServer::evDeSelection	= 2;
const int uiVisPartServer::evPicksChanged    	= 3;
const int uiVisPartServer::evGetNewData    	= 4;
const int uiVisPartServer::evSelectableStatusCh = 5;
const int uiVisPartServer::evMouseMove		= 6;

const char* uiVisPartServer::appvelstr = "AppVel";

uiVisPartServer::uiVisPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , viewmode(false)
    , eventobjid(-1)
    , eventmutex(*new Threads::Mutex)
    , mouseposval( mUndefValue )
{
    visBase::DM().selMan().selnotifer.notify( 
	mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifer.notify( 
  	mCB(this,uiVisPartServer,deselectObjCB) );
}


uiVisPartServer::~uiVisPartServer()
{
    visBase::DM().selMan().selnotifer.remove(
	    mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifer.remove(
	    mCB(this,uiVisPartServer,deselectObjCB) );
    delete &eventmutex;
}


bool uiVisPartServer::deleteAllObjects()
{
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	scenes[idx]->mouseposchange.remove(
	    			mCB( this, uiVisPartServer, mouseMoveCB ));
        scenes[idx]->unRef();
    }

    scenes.erase();
    wells.erase();
    surfaces.erase();
    picks.erase();
    volumes.erase();
    seisdisps.erase();

    return visBase::DM().reInit();
}


bool uiVisPartServer::usePar( const IOPar& par )
{
    if ( !visBase::DM().usePar( par ) )
    {	
	pErrMsg( "Could not parse session");
	return false;
    }

    TypeSet<int> sceneids;
    visBase::DM().getIds( typeid(visSurvey::Scene), sceneids );

    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visSurvey::Scene* newscene =
	    	(visSurvey::Scene*) visBase::DM().getObj( sceneids[idx] );

	newscene->mouseposchange.notify(
				mCB( this, uiVisPartServer, mouseMoveCB ));
	scenes += newscene;
	newscene->ref();
	selsceneid = newscene->id();
    }

    TypeSet<int> pickids;
    visBase::DM().getIds( typeid(visSurvey::PickSetDisplay), pickids );

    for ( int idx=0; idx<pickids.size(); idx++ )
    {
	visSurvey::PickSetDisplay* pickset =
	    (visSurvey::PickSetDisplay*) visBase::DM().getObj( pickids[idx] );	

	picks += pickset;
	pickset->changed.notify( mCB( this, uiVisPartServer, picksChangedCB ));
    }


    TypeSet<int> seisdispids;
    visBase::DM().getIds( typeid(visSurvey::PlaneDataDisplay), seisdispids );

    for ( int idx=0; idx<seisdispids.size(); idx++ )
    {
	visSurvey::PlaneDataDisplay* pdd =
	   (visSurvey::PlaneDataDisplay*)visBase::DM().getObj(seisdispids[idx]);
	seisdisps += pdd;

	pdd->getMovementNotification()->notify(
				    mCB(this,uiVisPartServer,getDataCB) );

	pdd->textureRect().manipChanges()->notify(
			    mCB(this,uiVisPartServer,manipMoveCB));

	getDataCB( pdd );
    }

// TODO: remove all HorizonDisplay stuff
    TypeSet<int> horizonids;
    visBase::DM().getIds( typeid(visSurvey::SurfaceDisplay), horizonids );
    if ( !horizonids.size() )
	visBase::DM().getIds( typeid(visSurvey::HorizonDisplay), horizonids );
    for ( int idx=0; idx<horizonids.size(); idx++ )
    {
	visSurvey::SurfaceDisplay* sd =
	   (visSurvey::SurfaceDisplay*)visBase::DM().getObj(horizonids[idx]);
	surfaces += sd;

	if ( sd->getAttribSelSpec().id() >= 0 )
	{
	    bool usetext = sd->usesTexture();
	    getDataCB( sd );
	    sd->useTexture( usetext );
	}
	else if ( sd->usesTexture() )
	    sd->setZValues();
    }

    float appvel;
    if ( par.get( appvelstr, appvel ) )
	visSurvey::SPM().setAppVel( appvel );

    return true;
}


void uiVisPartServer::fillPar( IOPar& par ) const
{
    TypeSet<int> storids;

    for ( int idx=0; idx<scenes.size(); idx++ )
	storids += scenes[idx]->id();

    par.set( appvelstr, visSurvey::SPM().getAppVel() );

    visBase::DM().fillPar(par, storids);
}


uiVisPartServer::ObjectType uiVisPartServer::getObjectType( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( pdd ) return DataDisplay;
    mDynamicCastGet(const visSurvey::PickSetDisplay*,psd,dobj);
    if ( psd ) return PickSetDisplay;
    mDynamicCastGet(const visSurvey::WellDisplay*,wd,dobj);
    if ( wd ) return WellDisplay;
    mDynamicCastGet(const visSurvey::SurfaceDisplay*,sd,dobj);
    if ( sd )
    {
	MultiID surfid = sd->surfaceId();
	mDynamicCastGet(const EarthModel::Fault*,f,
					EarthModel::EMM().getObject(surfid));
	return f ? FaultDisplay : HorizonDisplay;
    }
    mDynamicCastGet(const visSurvey::VolumeDisplay*,vd,dobj)
    if ( vd ) return VolumeDisplay;

    mDynamicCastGet(const visSurvey::HorizonDisplay*,hd,dobj);
    if ( hd ) return HorizonDisplay;


    return Unknown;
}


void uiVisPartServer::setObjectName( int id, const char* nm )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    if ( obj ) obj->setName( nm );
}


const char* uiVisPartServer::getObjectName( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    if ( !obj ) return 0;
    return obj->name();
}


bool uiVisPartServer::dumpOI( int id, const char* filename ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    if ( !obj ) return false;
    return obj->dumpOIgraph( filename );
}


void uiVisPartServer::turnOn( int id, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,so,obj)
    if ( so ) so->turnOn( yn );
}


bool uiVisPartServer::isOn( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,so,obj)
    return so ? so->isOn() : false;
}


void uiVisPartServer::setViewMode(bool yn)
{
    if ( yn==viewmode ) return;
    viewmode = yn;

    visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    if ( pdd ) pdd->showDraggers(!yn);
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( vd ) vd->showBox( !yn );
    mDynamicCastGet(visBase::Rectangle*,rect,obj)
    if ( rect ) rect->displayDraggers( !yn );
}


void uiVisPartServer::showAnnotText( int sceneid, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    if ( scene ) scene->showAnnotText( yn );
}


bool uiVisPartServer::isAnnotTextShown( int sceneid )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    return scene ? scene->isAnnotTextShown() : false;
}


void uiVisPartServer::showAnnotScale( int sceneid, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    if ( scene ) scene->showAnnotScale( yn ); 
} 


bool uiVisPartServer::isAnnotScaleShown( int sceneid )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    return scene ? scene->isAnnotScaleShown() : false;
}


void uiVisPartServer::showAnnot( int sceneid, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    if ( scene ) scene->showAnnot( yn );
}


bool uiVisPartServer::isAnnotShown( int sceneid )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    return scene ? scene->isAnnotShown() : false;
}


bool uiVisPartServer::isSelectable( int id ) const
{
    const visBase::DataObject* obj = visBase::DM().getObj( id );
    return obj->selectable();
}


void uiVisPartServer::makeSelectable(int id, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,visobj,obj);
    if ( !visobj ) return;

    if ( getSelObjectId() == id ) setSelObjectId( -1 );
    visobj->setSelectable( yn );

    Threads::MutexLocker lock( eventmutex );
    eventobjid = id;
    sendEvent( evSelectableStatusCh );
}

    
void uiVisPartServer::setSelObjectId( int id )
{
    visBase::DM().selMan().select( id );

    if ( !viewmode )
    {
	visBase::DataObject* obj = visBase::DM().getObj( getSelObjectId() );
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
	if ( pdd ) pdd->showDraggers(true);
    }
}


int uiVisPartServer::getSelObjectId() const
{
    const TypeSet<int>& sel = visBase::DM().selMan().selected();
    return sel.size() ? sel[0] : -1;
}


int uiVisPartServer::addScene()
{
    visSurvey::Scene* newscene = visSurvey::Scene::create();
    newscene->mouseposchange.notify( mCB(this,uiVisPartServer,mouseMoveCB) );
    scenes += newscene;
    newscene->ref();
    selsceneid = newscene->id();
    return selsceneid;
}


void uiVisPartServer::removeScene( int sceneid )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    scene->mouseposchange.remove( mCB(this,uiVisPartServer,mouseMoveCB) );
    scene->unRef();
    scenes -= scene;
}


void uiVisPartServer::getSceneIds( TypeSet<int>& sceneids ) const
{
    sceneids.erase();
    for ( int idx=0; idx<scenes.size(); idx++ )
	sceneids += scenes[idx]->id();
}


int uiVisPartServer::addDataDisplay( uiVisPartServer::ElementType etp )
{
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)

    visSurvey::PlaneDataDisplay::Type type =
	    etp == Inline ?	visSurvey::PlaneDataDisplay::Inline
	: ( etp == Crossline ?	visSurvey::PlaneDataDisplay::Crossline
			     :	visSurvey::PlaneDataDisplay::Timeslice );

    visSurvey::PlaneDataDisplay* pdd = visSurvey::PlaneDataDisplay::create();
    pdd->setType( type );
    pdd->getMovementNotification()->notify(
			    mCB(this,uiVisPartServer,getDataCB) );

    seisdisps += pdd; 
    visBase::VisColorTab* coltab = visBase::VisColorTab::create();
    BufferString ctname = "Red-White-Black";
    mSettUse(get,"dTect.Color table","Name",ctname);
    coltab->colorSeq().loadFromStorage( ctname );
    int resolution = 0;
    mSettUse(get,"dTect.PlaneDataDisplay","Resolution",resolution);
    pdd->textureRect().setResolution( resolution );
    pdd->setColorTable( coltab );
    pdd->textureRect().manipChanges()->notify(
	    				mCB(this,uiVisPartServer,manipMoveCB));

    scene->addInlCrlTObject( pdd );
    setSelObjectId( pdd->id() );
    return getSelObjectId();
}


int uiVisPartServer::duplicateDisplay( int id )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( !pdd ) return -1;

    int newid = addDataDisplay( uiVisPartServer::Inline );
    visBase::DataObject* newobj = visBase::DM().getObj( newid );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,newpdd,newobj);

    visSurvey::PlaneDataDisplay::Type tp = pdd->getType();
    newpdd->setType( tp );
    CubeSampling& cs = pdd->getCubeSampling();
    newpdd->setCubeSampling( cs );
    const ColorTable& ctab = pdd->getColorTable();
    newpdd->setColorTable( ctab );

    return newid;
}


void uiVisPartServer::removeDataDisplay( int id )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( !pdd ) return;

    dobj->deSelect();
    pdd->textureRect().manipChanges()->remove(
	    				mCB(this,uiVisPartServer,manipMoveCB));
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    int objidx = scene->getFirstIdx( pdd );
    scene->removeObject( objidx );
    seisdisps -= pdd;
}


void uiVisPartServer::resetManipulation( int id )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,dobj)
    if ( !pdd ) return;

    pdd->resetManip();
}


void uiVisPartServer::setPlanePos( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( !pdd && !vd ) return;
    const CubeSampling initcs = pdd ? pdd->getCubeSampling( true )
				   : vd->getCubeSampling();
    uiSliceSel dlg( appserv().parent(), initcs,
		    mCB(this,uiVisPartServer,updatePlanePos), (bool)vd );
    if ( dlg.go() )
    {
	CubeSampling cs = dlg.getCubeSampling();
	if ( pdd )
	{
	    pdd->setCubeSampling( cs );
	    pdd->updateAtNewPos();
	    sendEvent( evManipulatorMove );
	    sendEvent( evSelection );
	}
	else if ( vd )
	{
	    vd->setCubeSampling( cs );
	    vd->updateAtNewPos();
	    sendEvent( evSelection );
	}
    }
}


void uiVisPartServer::updatePlanePos( CallBacker* cb )
{
    int id = getSelObjectId();
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( !pdd ) return;
    mDynamicCastGet(uiSliceSel*,dlg,cb);
    if ( !dlg ) return;

    CubeSampling cs = dlg->getCubeSampling();
    pdd->setCubeSampling( cs );
    pdd->updateAtNewPos();
    sendEvent( evManipulatorMove );
    sendEvent( evSelection );
}


float uiVisPartServer::getPlanePos( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd )
    {	
        Coord3 geompos = pdd->textureRect().getRectangle().manipOrigo();
        return pdd->getType()==visSurvey::PlaneDataDisplay::Inline ? geompos.x :
	    pdd->getType()==visSurvey::PlaneDataDisplay::Crossline ? geompos.y
	    							   : geompos.z;
    }
    mDynamicCastGet(visBase::Rectangle*,rect,obj)
    if ( rect )
    {
	Coord3 geompos = rect->manipOrigo();
	if ( rect->orientation() == visBase::Rectangle::XY )
	    return (float)(int(1000*geompos.z)) / 1000;
	else if ( rect->orientation() == visBase::Rectangle::XZ )
	    return (float)(int(geompos.y));
	else if ( rect->orientation() == visBase::Rectangle::YZ )
	    return (float)(int(geompos.x));
    }

    return 0;
}


bool uiVisPartServer::isPlaneManipulated( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    if ( !pdd ) return false;

    Coord3 manippos = pdd->textureRect().getRectangle().manipOrigo();
    Coord3 rectpos = pdd->textureRect().getRectangle().origo();
    return !(manippos == rectpos);
}


void uiVisPartServer::setAttribSelSpec( int id, AttribSelSpec& as )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) pdd->setAttribSelSpec( as );

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj)
    if ( sd ) sd->setAttribSelSpec(as);

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( vd ) vd->setAttribSelSpec(as);
}


CubeSampling& uiVisPartServer::getCubeSampling( int id, bool manippos )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    if ( pdd ) return pdd->getCubeSampling( manippos );
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    return vd->getCubeSampling();
}


CubeSampling& uiVisPartServer::getPrevCubeSampling( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    if ( pdd ) return pdd->getPrevCubeSampling();
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    return vd->getPrevCubeSampling();
}


AttribSelSpec& uiVisPartServer::getAttribSelSpec(int id)
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    if ( pdd ) return pdd->getAttribSelSpec();

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj);
    if ( sd ) return sd->getAttribSelSpec();

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    return vd->getAttribSelSpec();
}


void uiVisPartServer::putNewData( int id, AttribSlice* slice )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) pdd->operationSucceeded( pdd->putNewData(slice) );
}


AttribSlice* uiVisPartServer::getPrevData( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) return pdd->getPrevData();
    else return 0;
}


void uiVisPartServer::getDataDisplayIds( int sceneid, 
	uiVisPartServer::ElementType etp, TypeSet<int>& ids )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    if ( !scene ) return;

    visSurvey::PlaneDataDisplay::Type type =
	    etp == Inline ?	visSurvey::PlaneDataDisplay::Inline
	: ( etp == Crossline ?	visSurvey::PlaneDataDisplay::Crossline
			     :	visSurvey::PlaneDataDisplay::Timeslice );

    for ( int idx=0; idx<scene->size(); idx++ )
    {
	visBase::SceneObject* obj = scene->getObject( idx );
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
	if ( !pdd ) continue;
	if ( type == pdd->getType() )
	    ids += pdd->id();
    }
}


int uiVisPartServer::addVolumeDisplay()
{
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj);

    visSurvey::VolumeDisplay* vd = visSurvey::VolumeDisplay::create();
    vd->moved.notify( mCB(this,uiVisPartServer,getDataCB));
    vd->rectmoving.notify( mCB(this,uiVisPartServer,manipMoveCB) );
    volumes += vd;
    scene->addInlCrlTObject( vd );
    setSelObjectId( vd->id() );

    return vd->id();
}


void uiVisPartServer::removeVolumeDisplay( int id )
{   
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj)
    if ( !vd ) return;

    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    int objidx = scene->getFirstIdx( vd );
    scene->removeObject( objidx );
    volumes -= vd;
}


void uiVisPartServer::putNewVolData( int id, AttribSliceSet* sliceset )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj)
    if ( vd ) vd->putNewData( sliceset );
}


AttribSliceSet* uiVisPartServer::getPrevVolData( int id )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj)
    if ( vd ) return vd->getPrevData();
    else return 0;
}


void uiVisPartServer::getVolumeDisplayIds( int sceneid, TypeSet<int>& ids )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    if ( !scene ) return;

    for ( int idx=0; idx<scene->size(); idx++ )
    {
	visBase::SceneObject* obj = scene->getObject( idx );
	mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
	if ( vd )
	    ids += vd->id();
    }
}


void uiVisPartServer::getVolumePlaneIds( int id, int& inl, int& crl, int& tsl )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj)
    if ( vd ) vd->getPlaneIds( inl, crl, tsl );
    
}


float uiVisPartServer::getVolumePlanePos( int id, int dim ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,dobj)
    if ( vd ) return vd->getPlanePos( dim );
    return 0;
}


bool uiVisPartServer::isVolumeManipulated( int id ) const
{
    return false;
}


int uiVisPartServer::addPickSetDisplay()
{
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj);

    visSurvey::PickSetDisplay* pickset = visSurvey::PickSetDisplay::create();
    picks += pickset;
    scene->addXYTObject( pickset );
    pickset->changed.notify( mCB( this, uiVisPartServer, picksChangedCB ));
    if ( picks.size() > 1 && picks[0] )
	pickset->setSize( picks[0]->getPickSize() ); 

    return pickset->id();
}


void uiVisPartServer::removePickSetDisplay(int id)
{
    visBase::DataObject* psobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,psobj)
    if ( !ps ) return;

    ps->changed.remove( mCB( this, uiVisPartServer, picksChangedCB ));
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    int objidx = scene->getFirstIdx( ps );
    scene->removeObject( objidx );
    picks -= ps;
}


bool uiVisPartServer::setPicks(int id, const PickSet& pickset )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,obj)
    if ( !ps ) return false;

    if ( ps->nrPicks() )
	ps->removeAll();

    ps->setColor( pickset.color );
    ps->setName( pickset.name() );
    int nrpicks = pickset.size();
    for ( int idx=0; idx<nrpicks; idx++ )
    {
	Coord crd( pickset[idx].pos );
	ps->addPick( Coord3(crd.x,crd.y,pickset[idx].z) );
    }

    return true;
}


void uiVisPartServer::getAllPickSets( UserIDSet& pset )
{
    for ( int idx=0; idx<picks.size(); idx++ )
    {
	if ( !picks[idx]->nrPicks() ) continue;
	pset.add( picks[idx]->name() );
    }
}


void uiVisPartServer::getPickSetData( const char* nm, PickSet& pickset )
{
    visSurvey::PickSetDisplay* visps = 0;
    for ( int idx=0; idx<picks.size(); idx++ )
    {
	if ( !strcmp(nm,picks[idx]->name()) )
	{
	    visps = picks[idx];
	    break;
	}
    }

    if ( !visps )
    {
	BufferString msg( "Cannot find PickSet " ); msg += nm;
	uiMSG().error( msg );
	return;
    }

    pickset.color = visps->getMaterial()->getColor();
    pickset.color.setTransparency( 0 );
    for ( int idx=0; idx<visps->nrPicks(); idx++ )
    {
	Coord3 pos = visps->getPick( idx );
	pickset += PickLocation( pos.x, pos.y, pos.z );
    }
}


void uiVisPartServer::getPickSetIds( int sceneid, TypeSet<int>& ids )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    if ( !scene ) return;

    for ( int idx=0; idx<scene->size(); idx++ )
    {
	visBase::SceneObject* obj = scene->getObject( idx );
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,obj)
	if ( psd )
	    ids += psd->id();
    }
}


int uiVisPartServer::nrPicks( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,obj)
    return ps ? ps->nrPicks() : 0;
}


bool uiVisPartServer::picksetIsPresent( const char* psname )
{
    for ( int idx=0; idx<picks.size(); idx++ )
    {
        if ( !strcmp(psname,picks[idx]->name()) )
        {
	    BufferString msg( "Pickset: "); msg += psname;
	    msg += "\nis already present.";
	    uiMSG().error( msg );
	    return true;
        }
    }

    return false;
}


void uiVisPartServer::showAllPicks( int id, bool showall )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,obj)
    if ( ps )
        ps->showAll( showall );

    if ( !showall )
    {
	for ( int idx=0; idx<scenes.size(); idx++ )
	    scenes[idx]->filterPicks();
    }
}


bool uiVisPartServer::allPicksShown( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,obj)
    return ps ? ps->allShown() : false;
}


int uiVisPartServer::getPickType( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,obj)
    return ps ? ps->getType() : -1;
}


void uiVisPartServer::setPickType( int id, int type )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,obj)
    if ( ps ) ps->setType( type );
}


int uiVisPartServer::addWellDisplay( const MultiID& emwellid )
{
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)

    visSurvey::WellDisplay* wd = visSurvey::WellDisplay::create();
    if ( !wd->setWellId( emwellid ) )
    {
	wd->ref(); wd->unRef();
	pErrMsg( "EarthModel em does not exist" );
	return -1;
    }

    wells += wd; 

    if ( wd->depthIsT() )
	scene->addXYTObject( wd );
    else
	scene->addXYZObject( wd );

    setSelObjectId( wd->id() );
    return wd->id();
}


void uiVisPartServer::removeWellDisplay( int id )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,wd,dobj)
    if ( !wd ) return;

    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    int objidx = scene->getFirstIdx( wd );
    if ( objidx>-1 )
    {
	scene->removeObject( objidx );
	wells -= wd;
    }
}


int uiVisPartServer::getNrWellAttribs( int id ) const 
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,wd,dobj)
    if ( !wd ) return -1;

    return wd->nrAttribs();
}


const char* uiVisPartServer::getWellAttribName( int id, int attr ) const 
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,wd,dobj)
    if ( !wd ) return 0;

    return wd->getAttribName( attr );
}


void uiVisPartServer::displayWellAttrib( int id, int attr )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,wd,dobj)
    if ( !wd ) return;

    wd->displayAttrib( attr );
}


int uiVisPartServer::displayedWellAttrib( int id ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,wd,dobj)
    if ( !wd ) return -1;

    return wd->displayedAttrib( );
}


void uiVisPartServer::modifyLineStyle( int id )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,wd,dobj)
    if ( !wd ) return;

    LineStyleDlg dlg( appserv().parent(), wd->lineStyle(), 0, false, true );
    if ( dlg.go() )
    {
	wd->setLineStyle( dlg.getLineStyle() );
    }
}


void uiVisPartServer::showWellText( int id, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,wd,obj)
    if ( wd ) wd->showWellText( yn );
}


bool uiVisPartServer::isWellTextShown( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::WellDisplay*,wd,obj)
    return wd ? wd->isWellTextShown() : false;
}


void uiVisPartServer::getWellIds( int sceneid, TypeSet<int>& ids )
{
    visBase::DataObject* obj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    if ( !scene ) return;

    for ( int idx=0; idx<scene->size(); idx++ )
    {
	visBase::SceneObject* obj = scene->getObject( idx );
	mDynamicCastGet(visSurvey::WellDisplay*,wd,obj)
	if ( wd )
	    ids += wd->id();
    }
}


int uiVisPartServer::addSurfaceDisplay( const MultiID& emhorid )
{
    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)

    visSurvey::SurfaceDisplay* horizon = visSurvey::SurfaceDisplay::create();

    PtrMan<Executor> exec = horizon->setSurfaceId( emhorid );

    if ( !exec )
    {
	horizon->ref(); horizon->unRef();
	pErrMsg( "EarthModel em does not exist" );
	return -1;
    }

    uiExecutor uiexec (appserv().parent(), *exec );
    if ( !uiexec.execute() )
    {
	horizon->ref(); horizon->unRef();
	return -1;
    }

    horizon->setZValues();
    surfaces += horizon; 

    scene->addDisplayObject( horizon );

    setSelObjectId( horizon->id() );
    return horizon->id();
}


void uiVisPartServer::removeSurfaceDisplay( int id )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( !sd ) return;

    visBase::DataObject* obj = visBase::DM().getObj( selsceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,obj)
    int objidx = scene->getFirstIdx( sd );
    if ( objidx>-1 )
    {
	scene->removeObject( objidx );
	surfaces -= sd;
    }
}


void uiVisPartServer::getSurfAttribData( int id,
				   ObjectSet< TypeSet<BinIDZValue> >& bidzvset,
				   bool posonly,
				   const BinIDRange* br ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd )
	sd->getAttribData( bidzvset, !posonly, br );
}


void uiVisPartServer::getSurfAttribValues( int id, TypeSet<float>& vals ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd )
	sd->getDataValues( vals );
}


void uiVisPartServer::putNewSurfData( int id,
			     const ObjectSet< TypeSet<BinIDZValue> >& nd )
{
    visBase::DataObject* dobj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,dobj)
    if ( sd ) sd->putNewData( nd );
}


void uiVisPartServer::getSurfaceIds( int sceneid, ObjectType type, 
				     TypeSet<int>& ids ) const
{
    visBase::DataObject* dobj = visBase::DM().getObj( sceneid );
    mDynamicCastGet(visSurvey::Scene*,scene,dobj)
    if ( !scene ) return;

    for ( int idx=0; idx<scene->size(); idx++ )
    {
	visBase::SceneObject* obj = scene->getObject( idx );
	mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj)
	if ( sd && type == getObjectType( sd->id() ) )
	    ids += sd->id();

	mDynamicCastGet(visSurvey::HorizonDisplay*,hd,obj)
	if ( hd ) ids += hd->id();
    }
}


void uiVisPartServer::getSurfaceInfo( ObjectSet<SurfaceInfo>& hinfos,
				      ObjectType type ) const
{
    TypeSet<int> sceneids; getSceneIds( sceneids );
    for ( int ids=0; ids<sceneids.size(); ids++ )
    {
	const int sceneid = sceneids[ids];
	TypeSet<int> horids; getSurfaceIds( sceneid, type, horids );
	for ( int idh=0; idh<horids.size(); idh++ )
	{
	    const int horid = horids[idh];
	    hinfos += new SurfaceInfo( horid, getObjectName(horid) );
	}
    }
}


void uiVisPartServer::getSurfaceNames( ObjectSet<BufferString>& nms, 
				       ObjectType type ) const
{
    deepErase( nms );
    ObjectSet<SurfaceInfo> hinfos;
    getSurfaceInfo( hinfos, type );
    for ( int idx=0; idx<hinfos.size(); idx++ )
	nms += new BufferString( hinfos[idx]->name );
}


void uiVisPartServer::setSurfaceNrTriPerPixel( int id, float res )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj);
    if ( !sd ) return;

    sd->setNrTriPerPixel(res);
}


float uiVisPartServer::getSurfaceNrTriPerPixel( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj);
    if ( !sd ) return mUndefValue;

    return sd->getNrTriPerPixel();
}


void uiVisPartServer::setResolution( int id, int res )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) pdd->setResolution(res);

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj);
    if ( sd ) sd->setResolution(res);
}


int uiVisPartServer::getResolution( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) return pdd->getResolution();

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj);
    if ( sd ) return sd->getResolution();

    return -1;
}


int uiVisPartServer::getNrResolutions( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) return pdd->getNrResolutions();

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj)
    if ( sd ) return sd->getNrResolutions();

    return -1;
}


BufferString uiVisPartServer::getResolutionText( int id, int res ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) return pdd->getResName( res );

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj)
    if ( sd ) return sd->getResName( res );

    return "";
}


bool uiVisPartServer::canSetColorSeq( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    return pdd;
}


void uiVisPartServer::modifyColorSeq(int id, const ColorTable& ctab )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    if ( pdd ) pdd->setColorTable( ctab );
  
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj)
    if ( sd ) sd->setColorTable( ctab );

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( vd ) vd->setColorTable( ctab );
}


const ColorTable& uiVisPartServer::getColorSeq(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    if ( pdd ) return pdd->getColorTable();

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj)
    if ( sd ) return sd->getColorTable();

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( vd ) return vd->getColorTable();

    
    return *new ColorTable("Red-White-Blue");
}


void uiVisPartServer::shareColorSeq( int toid, int fromid )
{
    visBase::DataObject* obj = visBase::DM().getObj( toid );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,tosd,obj);
    if ( !tosd ) return;

    obj = visBase::DM().getObj( fromid );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,fromsd,obj);
    if ( !fromsd ) return;

    visBase::ColorSequence* colseq =
				&fromsd->textureRect().getColorTab().colorSeq();
    tosd->textureRect().getColorTab().setColorSeq( colseq );
}


bool uiVisPartServer::canSetRange(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    return pdd;
}


void uiVisPartServer::setClipRate( int id, float cr )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) pdd->setClipRate( cr );

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj)
    if ( sd ) sd->setClipRate( cr );
    
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( vd ) vd->setClipRate( cr );
}


float uiVisPartServer::getClipRate(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) return pdd->clipRate();

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj)
    if ( sd ) return sd->getClipRate();

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( vd ) return vd->clipRate();

    return 0;
}


void uiVisPartServer::setAutoscale( int id, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) pdd->setAutoscale( yn );

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj)
    if ( sd ) sd->setAutoscale( yn );

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( vd ) vd->setAutoscale( yn );
}


bool uiVisPartServer::getAutoscale(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) return pdd->autoScale();

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj)
    if ( sd ) return sd->getAutoscale();

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( vd ) return vd->autoScale();

    return false;
}


void uiVisPartServer::setDataRange( int id, const Interval<float>& intv )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) pdd->setDataRange( intv );

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj)
    if ( sd ) sd->setDataRange( intv );

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( vd ) vd->setDataRange( intv );
}


Interval<float> uiVisPartServer::getDataRange(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj)
    if ( pdd ) return pdd->getDataRange();

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj)
    if ( sd ) return sd->getDataRange();

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    if ( vd ) return vd->getDataRange();

    return Interval<float>(0,1);
}


void uiVisPartServer::shareRange( int toid, int fromid )
{
    visBase::DataObject* obj = visBase::DM().getObj( toid );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,tosd,obj);
    if ( !tosd ) return;

    obj = visBase::DM().getObj( fromid );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,fromsd,obj);
    if ( !fromsd ) return;

    const ColorTable* coltab = &fromsd->getColorTable();
    tosd->setColorTable(*coltab);
}


bool uiVisPartServer::canSetColor(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,vo,obj);
    return vo;
}


void uiVisPartServer::modifyColor( int id, const Color& col )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,vo,obj);
    if ( vo ) vo->getMaterial()->setColor(col);
}


Color uiVisPartServer::getColor(int id) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,vo,obj);
    return vo->getMaterial()->getColor();
}


void uiVisPartServer::shareColor(int toid, int fromid )
{
    visBase::DataObject* obj = visBase::DM().getObj( toid );
    mDynamicCastGet(visBase::VisualObject*,tovo,obj);
    if ( !tovo ) return;

    obj = visBase::DM().getObj( fromid );
    mDynamicCastGet(visBase::VisualObject*,fromvo,obj);
    if ( !fromvo ) return;

    visBase::Material* material = fromvo->getMaterial();
    tovo->setMaterial(material);
}


void uiVisPartServer::useTexture( int id, bool yn )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    if ( pdd ) pdd->textureRect().useTexture( yn );

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd, obj );
    if ( sd ) sd->useTexture( yn );
}


bool uiVisPartServer::usesTexture( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    if ( pdd ) return pdd->textureRect().usesTexture();

    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd, obj );
    if ( sd ) return sd->usesTexture();

    return false;
}


bool uiVisPartServer::setWorkingArea()
{
    uiWorkAreaDlg dlg( appserv().parent() );
    if ( !dlg.go() ) return false;

    TypeSet<int> sceneids;
    getSceneIds( sceneids );

    for ( int ids=0; ids<sceneids.size(); ids++ )
    {
        int sceneid = sceneids[ids];
	visBase::DataObject* obj = visBase::DM().getObj( sceneid );
	mDynamicCastGet(visSurvey::Scene*,scene,obj)
	if ( scene ) scene->updateRange();
    }

    return true;
}


bool uiVisPartServer::setZScale()
{
    uiZScaleDlg dlg( appserv().parent() );
    dlg.go();
    return dlg.valueChanged();
}


void uiVisPartServer::setMaterial( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visBase::VisualObject*,vo,obj);
    if ( !vo ) return;

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,obj);
    if ( pdd || vd || (sd && sd->usesTexture()) )
    {
	uiMaterialDlg dlg( appserv().parent(), vo->getMaterial(), true,
	       		   true, false, false, false, true );
	dlg.go();
    }
    else
    {
	uiMaterialDlg dlg( appserv().parent(), vo->getMaterial(), true, 
			   true, false, false, false, true );
	dlg.go();
    }
}


void uiVisPartServer::setPickProps( int id )
{
    visBase::DataObject* obj = visBase::DM().getObj( id );
    mDynamicCastGet(visSurvey::PickSetDisplay*,ps,obj)
    if ( !ps ) return;

    uiPickSizeDlg dlg( appserv().parent(), ps );
    dlg.go();
}


void uiVisPartServer::selectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    if ( !viewmode )
    {
	visBase::DataObject* obj = visBase::DM().getObj( sel );
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
	if ( pdd ) pdd->showDraggers( true );
	mDynamicCastGet(visBase::Rectangle*,rect,obj)
	if ( rect ) rect->displayDraggers( true );
	mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
	if ( vd ) vd->showBox( true );
    }

    Threads::MutexLocker lock( eventmutex );
    eventobjid = sel;
    sendEvent( evSelection );
}


void uiVisPartServer::deselectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,oldsel,cb);
    if ( !viewmode )
    {
	visBase::DataObject* obj = visBase::DM().getObj( oldsel );
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,obj);
	if ( pdd ) pdd->showDraggers( false );
	mDynamicCastGet(visBase::Rectangle*,rect,obj)
	if ( rect ) rect->displayDraggers( false );
	mDynamicCastGet(visSurvey::VolumeDisplay*,vd,obj)
	if ( vd ) vd->showBox( false );
    }

    Threads::MutexLocker lock( eventmutex );
    eventobjid = oldsel;
    sendEvent( evDeSelection );
}


void uiVisPartServer::picksChangedCB(CallBacker*)
{
    Threads::MutexLocker lock( eventmutex );
    eventobjid = getSelObjectId();
    sendEvent( evPicksChanged );
}


void uiVisPartServer::manipMoveCB( CallBacker* )
{
    int id = getSelObjectId();
    if ( id<0 ) return;

    Threads::MutexLocker lock( eventmutex );
    eventobjid = id;
    sendEvent( evManipulatorMove );
}


void uiVisPartServer::getDataCB( CallBacker* cb )
{
    mDynamicCastGet(visBase::VisualObject*,vo,cb);
    if ( !vo ) return;
    
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,cb);
    mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,cb);
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,cb);
    if ( pdd || sd || vd )
    {
	Threads::MutexLocker lock( eventmutex );
	eventobjid = vo->id();
	sendEvent( evGetNewData );
	return;
    }
}


void uiVisPartServer::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::Scene*,scene,cb);
    if ( !cb ) return;

    int selid = getSelObjectId();
    if ( selid==-1 || getObjectType(selid)==PickSetDisplay)
    {
	Threads::MutexLocker lock( eventmutex );
	xytmousepos = scene->getMousePos(true);
	inlcrlmousepos = scene->getMousePos(false);
	mouseposval = scene->getMousePosValue();
	sendEvent( evMouseMove );
    }
}
