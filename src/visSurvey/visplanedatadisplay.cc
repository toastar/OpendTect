/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visplanedatadisplay.cc,v 1.213 2009-02-11 11:37:52 cvsranojay Exp $";

#include "visplanedatadisplay.h"

#include "arrayndimpl.h"
#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribsel.h"
#include "coltabsequence.h"
#include "cubesampling.h"
#include "datapointset.h"
#include "iopar.h"
#include "keyenum.h"
#include "settings.h"
#include "simpnumer.h"
#include "survinfo.h"

#include "viscolortab.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visdepthtabplanedragger.h"
#include "visdrawstyle.h"
#include "visfaceset.h"
#include "visevent.h"
#include "visgridlines.h"
#include "vismaterial.h"
#include "vismultitexture2.h"
#include "vistexturechannels.h"
#include "vispickstyle.h"
#include "vissplittexture2rectangle.h"
#include "vistexturecoords.h"
#include "vistransform.h"
#include "zaxistransform.h"
#include "zaxistransformdatapack.h"


mCreateFactoryEntry( visSurvey::PlaneDataDisplay );

namespace visSurvey {

DefineEnumNames(PlaneDataDisplay,Orientation,1,"Orientation")
{ "Inline", "Crossline", "Timeslice", 0 };

PlaneDataDisplay::PlaneDataDisplay()
    : MultiTextureSurveyObject( true )
    , rectangle_( visBase::SplitTexture2Rectangle::create() )
    , rectanglepickstyle_( visBase::PickStyle::create() )
    , dragger_( visBase::DepthTabPlaneDragger::create() )
    , gridlines_( visBase::GridLines::create() )
    , curicstep_(SI().inlStep(),SI().crlStep())
    , curzstep_(SI().zStep())
    , datatransform_( 0 )
    , moving_(this)
    , movefinished_(this)
    , orientation_( Inline )
    , csfromsession_( true )			    
    , eventcatcher_( 0 )
{
    volumecache_.allowNull( true );
    rposcache_.allowNull( true );
    dragger_->ref();

    int channelidx = texture_
	? childIndex(texture_->getInventorNode() )
	: childIndex( channels_->getInventorNode() );

    insertChild( channelidx++, dragger_->getInventorNode() );
    dragger_->motion.notify( mCB(this,PlaneDataDisplay,draggerMotion) );
    dragger_->finished.notify( mCB(this,PlaneDataDisplay,draggerFinish) );
    dragger_->rightClicked()->notify(
	    		mCB(this,PlaneDataDisplay,draggerRightClick) );

    draggerrect_ = visBase::FaceSet::create();
    draggerrect_->ref();
    draggerrect_->removeSwitch();
    draggerrect_->setVertexOrdering(
	    visBase::VertexShape::cCounterClockWiseVertexOrdering() );
    draggerrect_->getCoordinates()->addPos( Coord3(-1,-1,0) );
    draggerrect_->getCoordinates()->addPos( Coord3(1,-1,0) );
    draggerrect_->getCoordinates()->addPos( Coord3(1,1,0) );
    draggerrect_->getCoordinates()->addPos( Coord3(-1,1,0) );
    draggerrect_->setCoordIndex( 0, 0 );
    draggerrect_->setCoordIndex( 1, 1 );
    draggerrect_->setCoordIndex( 2, 2 );
    draggerrect_->setCoordIndex( 3, 3 );
    draggerrect_->setCoordIndex( 4, -1 );

    draggermaterial_ = visBase::Material::create();
    draggermaterial_->ref();
    draggerrect_->setMaterial( draggermaterial_ );

    draggerdrawstyle_ = visBase::DrawStyle::create();
    draggerdrawstyle_->ref();
    draggerdrawstyle_->setDrawStyle( visBase::DrawStyle::Lines );
    draggerrect_->insertNode( draggerdrawstyle_->getInventorNode() );

    dragger_->setOwnShape( draggerrect_->getInventorNode() );
    dragger_->setDim( (int) 0 );
	
    if ( (int) orientation_ )
	dragger_->setDim( (int) orientation_ );

    rectanglepickstyle_->ref();
    addChild( rectanglepickstyle_->getInventorNode() );

    rectangle_->ref();
    rectangle_->removeSwitch();
    rectangle_->setMaterial( 0 );
    addChild( rectangle_->getInventorNode() );
    material_->setColor( Color::White() );
    material_->setAmbience( 0.8 );
    material_->setDiffIntensity( 0.2 );

    gridlines_->ref();
    insertChild( channelidx, gridlines_->getInventorNode() );

    updateRanges( true, true );

    int buttonkey = OD::NoButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyDepthKey(), buttonkey );
    dragger_->setTransDragKeys( true, buttonkey );
    buttonkey = OD::ShiftButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyPlaneKey(), buttonkey );
    dragger_->setTransDragKeys( false, buttonkey );
}


PlaneDataDisplay::~PlaneDataDisplay()
{
    setSceneEventCatcher( 0 );
    dragger_->motion.remove( mCB(this,PlaneDataDisplay,draggerMotion) );
    dragger_->finished.remove( mCB(this,PlaneDataDisplay,draggerFinish) );
    dragger_->rightClicked()->remove(
	    		mCB(this,PlaneDataDisplay,draggerRightClick) );

    deepErase( rposcache_ );
    setDataTransform( 0 );

    for ( int idx=volumecache_.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).release( volumecache_[idx] );

    for ( int idy=0; idy<displaycache_.size(); idy++ )
    {
	const TypeSet<DataPack::ID>& dpids = *displaycache_[idy];
	for ( int idx=dpids.size()-1; idx>=0; idx-- )
	    DPM(DataPackMgr::FlatID()).release( dpids[idx] );
    }

    deepErase( displaycache_ );

    rectangle_->unRef();
    dragger_->unRef();
    rectanglepickstyle_->unRef();
    gridlines_->unRef();
    draggerrect_->unRef();
    draggerdrawstyle_->unRef();
    draggermaterial_->unRef();
}


void PlaneDataDisplay::setOrientation( Orientation nt )
{
    if ( orientation_==nt )
	return;

    orientation_ = nt;

    dragger_->setDim( (int) nt );
    updateRanges( true, true );
}


void PlaneDataDisplay::updateRanges( bool resetic, bool resetz )
{
    CubeSampling survey( SI().sampling(true) );
    if ( datatransform_ )
    {
	if ( csfromsession_ != survey )
	    survey = csfromsession_;
	else
	{
	    survey.zrg.setFrom( datatransform_->getZInterval(false) );
	    survey.zrg.step = datatransform_->getGoodZStep();
	}
    }
	
    const Interval<float> inlrg( survey.hrg.start.inl, survey.hrg.stop.inl );
    const Interval<float> crlrg( survey.hrg.start.crl, survey.hrg.stop.crl );

    dragger_->setSpaceLimits( inlrg, crlrg, survey.zrg );
    dragger_->setWidthLimits(
	    Interval<float>( 4*survey.hrg.step.inl, mUdf(float) ),
	    Interval<float>( 4*survey.hrg.step.crl, mUdf(float) ),
	    Interval<float>( 4*survey.zrg.step, mUdf(float) ) );

    dragger_->setSize( Coord3(inlrg.width(), crlrg.width(),survey.zrg.width()));

    CubeSampling newpos = getCubeSampling(false,true);
    if ( !newpos.isEmpty() )
    {
	if ( !survey.includes( newpos ) )
	    newpos.limitTo( survey );
    }

    if ( !newpos.hrg.isEmpty() && !resetic && resetz )
	survey.hrg = newpos.hrg;

    if ( resetic || resetz || newpos.isEmpty() )
    {
	newpos = survey;
	if ( orientation_==Timeslice && datatransform_ && resetz )
	{
	    const float center = datatransform_->getZIntervalCenter(false);
	    if ( !mIsUdf(center) )
		newpos.zrg.start = newpos.zrg.stop = center;
	}
    }

    newpos = snapPosition( newpos );

    if ( newpos!=getCubeSampling(false,true) )
	setCubeSampling( newpos );
}


CubeSampling PlaneDataDisplay::snapPosition( const CubeSampling& cs ) const
{
    CubeSampling res( cs );
    const Interval<float> inlrg( res.hrg.start.inl, res.hrg.stop.inl );
    const Interval<float> crlrg( res.hrg.start.crl, res.hrg.stop.crl );
    const Interval<float> zrg( res.zrg );

    if ( datatransform_ )
	res.hrg.snapToSurvey();
    else
	res.snapToSurvey();

    if ( orientation_==Inline )
	res.hrg.start.inl = res.hrg.stop.inl =
	    SI().inlRange(true).snap( inlrg.center() );
    else if ( orientation_==Crossline )
	res.hrg.start.crl = res.hrg.stop.crl =
	    SI().crlRange(true).snap( crlrg.center() );
    else
	res.zrg.start = res.zrg.stop = SI().zRange(true).snap(zrg.center());

    return res;
}


Coord3 PlaneDataDisplay::getNormal( const Coord3& pos ) const
{
    if ( orientation_==Timeslice )
	return Coord3(0,0,1);
    
    return Coord3( orientation_==Inline ? SI().binID2Coord().rowDir() :
	    SI().binID2Coord().colDir(), 0 );
}


float PlaneDataDisplay::calcDist( const Coord3& pos ) const
{
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    const Coord3 xytpos = utm2display->transformBack( pos );
    const BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );

    const CubeSampling cs = getCubeSampling(false,true);
    
    BinID inlcrldist( 0, 0 );
    float zdiff = 0;

    inlcrldist.inl =
	binid.inl>=cs.hrg.start.inl && binid.inl<=cs.hrg.stop.inl 
	     ? 0
	     : mMIN( abs(binid.inl-cs.hrg.start.inl),
		     abs( binid.inl-cs.hrg.stop.inl) );
    inlcrldist.crl =
	binid.crl>=cs.hrg.start.crl && binid.crl<=cs.hrg.stop.crl 
	     ? 0
	     : mMIN( abs(binid.crl-cs.hrg.start.crl),
		     abs( binid.crl-cs.hrg.stop.crl) );
    const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
    zdiff = cs.zrg.includes(xytpos.z)
	? 0
	: mMIN(fabs(xytpos.z-cs.zrg.start),fabs(xytpos.z-cs.zrg.stop)) *
	  zfactor  * scene_->getZStretch();

    const float inldist = SI().inlDistance();
    const float crldist = SI().crlDistance();
    float inldiff = inlcrldist.inl * inldist;
    float crldiff = inlcrldist.crl * crldist;

    return Math::Sqrt( inldiff*inldiff + crldiff*crldiff + zdiff*zdiff );
}


float PlaneDataDisplay::maxDist() const
{
    const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
    float maxzdist = zfactor * scene_->getZStretch() * SI().zStep() / 2;
    return orientation_==Timeslice ? maxzdist : SurveyObject::sDefMaxDist();
}


bool PlaneDataDisplay::setDataTransform( ZAxisTransform* zat )
{
    const bool haddatatransform = datatransform_;
    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		    mCB(this, PlaneDataDisplay, dataTransformCB ));
	datatransform_->unRef();
	datatransform_ = 0;
    }

    datatransform_ = zat;
    if ( datatransform_ )
    {
	datatransform_->ref();
	updateRanges( false, !haddatatransform );
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		    mCB(this, PlaneDataDisplay, dataTransformCB ));
    }

    return true;
}


const ZAxisTransform* PlaneDataDisplay::getDataTransform() const
{ return datatransform_; }


void PlaneDataDisplay::setTranslationDragKeys( bool depth, int ns )
{ dragger_->setTransDragKeys( depth, ns ); }


int PlaneDataDisplay::getTranslationDragKeys(bool depth) const
{ return dragger_->getTransDragKeys( depth ); }


void PlaneDataDisplay::dataTransformCB( CallBacker* )
{
    updateRanges( false, true );
    for ( int idx=0; idx<volumecache_.size(); idx++ )
    {
	if ( volumecache_[idx] )
	{
	    setVolumeDataPackNoCache( idx, volumecache_[idx] );
	}
	else if ( rposcache_[idx] )
	{
	    BinIDValueSet set(*rposcache_[idx]);
	    setRandomPosDataNoCache( idx, &set );
	}
    }
}


void PlaneDataDisplay::draggerMotion( CallBacker* )
{
    moving_.trigger();

    const CubeSampling dragcs = getCubeSampling(true,true);
    const CubeSampling snappedcs = snapPosition( dragcs );
    const CubeSampling oldcs = getCubeSampling(false,true);

    bool showplane = false;
    if ( orientation_==Inline && dragcs.hrg.start.inl!=oldcs.hrg.start.inl )
	showplane = true;
    else if ( orientation_==Crossline &&
	      dragcs.hrg.start.crl!=oldcs.hrg.start.crl )
	showplane = true;
    else if ( orientation_==Timeslice && dragcs.zrg.start!=oldcs.zrg.start )
	showplane = true;
   
    draggerdrawstyle_->setDrawStyle( showplane
	    ? visBase::DrawStyle::Filled
	    : visBase::DrawStyle::Lines );
    draggermaterial_->setTransparency( showplane ? 0.5 : 0 );
}


void PlaneDataDisplay::draggerFinish( CallBacker* )
{
    const CubeSampling cs = getCubeSampling(true,true);
    const CubeSampling snappedcs = snapPosition( cs );

    if ( cs!=snappedcs )
	setDraggerPos( snappedcs );
}


void PlaneDataDisplay::draggerRightClick( CallBacker* cb )
{
    triggerRightClick( dragger_->rightClickedEventInfo() );
}


void PlaneDataDisplay::setDraggerPos( const CubeSampling& cs )
{
    const Coord3 center( (cs.hrg.start.inl+cs.hrg.stop.inl)/2.0,
		         (cs.hrg.start.crl+cs.hrg.stop.crl)/2.0,
		         cs.zrg.center() );
    Coord3 width( cs.hrg.stop.inl-cs.hrg.start.inl,
		  cs.hrg.stop.crl-cs.hrg.start.crl, cs.zrg.width() );

    const Coord3 oldwidth = dragger_->size();
    width[(int)orientation_] = oldwidth[(int)orientation_];

    dragger_->setCenter( center );
    dragger_->setSize( width );
}


void PlaneDataDisplay::coltabChanged( CallBacker* )
{
    // Hack for correct transparency display
    bool manipshown = isManipulatorShown();
    if ( manipshown ) return;
    showManipulator( true );
    showManipulator( false );
}


void PlaneDataDisplay::showManipulator( bool yn )
{
    dragger_->turnOn( yn );
    rectanglepickstyle_->setStyle( yn ? visBase::PickStyle::Unpickable
				      : visBase::PickStyle::Shape );
}


bool PlaneDataDisplay::isManipulatorShown() const
{
    return dragger_->isOn();
}


bool PlaneDataDisplay::isManipulated() const
{ return getCubeSampling(true,true)!=getCubeSampling(false,true); }


void PlaneDataDisplay::resetManipulation()
{
    CubeSampling cs = getCubeSampling( false, true );
    setDraggerPos( cs );
    draggerdrawstyle_->setDrawStyle( visBase::DrawStyle::Lines );
    draggermaterial_->setTransparency( 0 );
}


void PlaneDataDisplay::acceptManipulation()
{
    CubeSampling cs = getCubeSampling( true, true );
    setCubeSampling( cs );
    draggerdrawstyle_->setDrawStyle( visBase::DrawStyle::Lines );
    draggermaterial_->setTransparency( 0 );
}


BufferString PlaneDataDisplay::getManipulationString() const
{
    BufferString res;
    getObjectInfo( res );
    return res;
}


NotifierAccess* PlaneDataDisplay::getManipulationNotifier()
{ return &moving_; }


int PlaneDataDisplay::nrResolutions() const
{
    if ( !texture_ )
	return 1;

    return texture_->canUseShading() ? 1 : 3;
}


void PlaneDataDisplay::setResolution( int res )
{
    if ( !texture_ || texture_->canUseShading() )
	return;

    if ( res==resolution_ )
	return;

    resolution_ = res;
    texture_->clearAll();

    for ( int idx=0; idx<nrAttribs(); idx++ )
	updateFromDisplayIDs( idx );
}


SurveyObject::AttribFormat PlaneDataDisplay::getAttributeFormat() const
{
    return datatransform_ && orientation_==Timeslice
	? SurveyObject::RandomPos
	: SurveyObject::Cube;
}


void PlaneDataDisplay::addCache()
{
    volumecache_ += 0;
    rposcache_ += 0;
    displaycache_ += new TypeSet<DataPack::ID>;
}


void PlaneDataDisplay::removeCache( int attrib )
{
    DPM(DataPackMgr::FlatID()).release( volumecache_[attrib] );
    volumecache_.remove( attrib );

    if ( rposcache_[attrib] ) delete rposcache_[attrib];
    rposcache_.remove( attrib );

    const TypeSet<DataPack::ID>& dpids = *displaycache_[attrib];
    for ( int idx=dpids.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).release( dpids[idx] );

    delete displaycache_.remove( attrib );
    
    if ( !texture_ || texture_->splitsTexture() )
    {
	for ( int idx=0; idx<displaycache_.size(); idx++ )
    	    updateFromDisplayIDs( idx );
    }
}


void PlaneDataDisplay::swapCache( int a0, int a1 )
{
    volumecache_.swap( a0, a1 );
    rposcache_.swap( a0, a1 );
    displaycache_.swap( a0, a1 );
    if ( !texture_ || texture_->splitsTexture() )
    {
	for ( int idx=0; idx<displaycache_.size(); idx++ )
    	    updateFromDisplayIDs( idx );
    }
}


void PlaneDataDisplay::emptyCache( int attrib )
{
    DPM(DataPackMgr::FlatID()).release( volumecache_[attrib] );
    volumecache_.replace( attrib, 0 );

    if ( rposcache_[attrib] ) delete rposcache_[attrib];
    rposcache_.replace( attrib, 0 );
    
    if ( displaycache_[attrib] )
    {
	TypeSet<DataPack::ID>& dpids = *displaycache_[attrib];
    	for ( int idx=dpids.size()-1; idx>=0; idx-- )
	    DPM(DataPackMgr::FlatID()).release( dpids[idx] );

	dpids.erase();
    }

    if ( texture_ )
	texture_->setData( attrib, 0, 0 );
    else
	channels_->setUnMappedData( attrib, 0, 0,
				    visBase::TextureChannels::None );
}


bool PlaneDataDisplay::hasCache( int attrib ) const
{
    return volumecache_[attrib] || rposcache_[attrib];
}


void PlaneDataDisplay::triggerSel()
{
    updateMouseCursorCB( 0 );
    visBase::VisualObject::triggerSel();
}


void PlaneDataDisplay::triggerDeSel()
{
    updateMouseCursorCB( 0 );
    visBase::VisualObject::triggerDeSel();
}


CubeSampling PlaneDataDisplay::getCubeSampling( int attrib ) const
{
    return getCubeSampling( true, false, attrib );
}


void PlaneDataDisplay::getRandomPos( DataPointSet& pos ) const
{
    const CubeSampling cs = getCubeSampling( true, true, 0 ); //attrib?
    HorSamplingIterator iter( cs.hrg );

    BinIDValue curpos;
    curpos.value = cs.zrg.start;
    while ( iter.next(curpos.binid) )
    {
	const float depth = datatransform_->transformBack( curpos );
	if ( mIsUdf(depth) )
	    continue;

	DataPointSet::Pos newpos( curpos.binid, depth );
	DataPointSet::DataRow dtrow( newpos );
	pos.addRow( dtrow );
    }
    pos.dataChanged();
}


void PlaneDataDisplay::setRandomPosData( int attrib, const DataPointSet* data )
{
    if ( attrib>=nrAttribs() )
	return;

    setRandomPosDataNoCache( attrib, &data->bivSet() );

    if ( rposcache_[attrib] ) 
	delete rposcache_[attrib];

    rposcache_.replace( attrib, data ? new BinIDValueSet(data->bivSet()) : 0 );
}


void PlaneDataDisplay::setCubeSampling( CubeSampling cs )
{
    cs = snapPosition( cs );
    const HorSampling& hrg = cs.hrg;

    if ( orientation_==Inline || orientation_==Crossline )
    {
	rectangle_->setPosition(
		Coord3( hrg.start.inl, hrg.start.crl, cs.zrg.start ),
		Coord3( hrg.start.inl,  hrg.start.crl,  cs.zrg.stop ),
		Coord3( hrg.stop.inl, hrg.stop.crl, cs.zrg.start ),
		Coord3( hrg.stop.inl,  hrg.stop.crl,  cs.zrg.stop ) );
    }
    else 
    {
	rectangle_->setPosition(
		Coord3( hrg.start.inl, hrg.start.crl, cs.zrg.stop ),
		Coord3( hrg.start.inl, hrg.stop.crl,  cs.zrg.start ),
		Coord3( hrg.stop.inl,  hrg.start.crl, cs.zrg.start ),
		Coord3( hrg.stop.inl,  hrg.stop.crl,  cs.zrg.stop ) );
    }

    setDraggerPos( cs );
    if ( gridlines_ ) gridlines_->setPlaneCubeSampling( cs );

    curicstep_ = hrg.step;
    curzstep_ = cs.zrg.step;

    if ( texture_ ) texture_->clearAll();
    //else channels_->clearAll();
    movefinished_.trigger();
}


CubeSampling PlaneDataDisplay::getCubeSampling( bool manippos,
						bool displayspace,
       						int attrib ) const
{
    CubeSampling res(false);
    Coord3 c0, c1;

    if ( manippos )
    {
	const Coord3 center = dragger_->center();
	Coord3 halfsize = dragger_->size()/2;
	halfsize[orientation_] = 0;

	c0 = center + halfsize;
	c1 = center - halfsize;
    }
    else
    {
	c0 = rectangle_->getPosition( false, false );
	c1 = rectangle_->getPosition( true, true );
    }

    res.hrg.start = res.hrg.stop = BinID(mNINT(c0.x),mNINT(c0.y) );
    res.zrg.start = res.zrg.stop = c0.z;
    res.hrg.include( BinID(mNINT(c1.x),mNINT(c1.y)) );
    res.zrg.include( c1.z );
    res.hrg.step = BinID( SI().inlStep(), SI().crlStep() );
    res.zrg.step = datatransform_ && displayspace
	? datatransform_->getGoodZStep()
	: SI().zRange(true).step;

    const char* zdomain = attrib>=0 && attrib<nrAttribs() 
				? getSelSpec(attrib)->zDomainKey() : 0;
    const bool alreadytransformed = zdomain && *zdomain;
    if ( alreadytransformed ) return res;

    if ( datatransform_ && !displayspace )
    {
	res.zrg.setFrom( datatransform_->getZInterval(true) );
	res.zrg.step = SI().zRange(true).step;
    }

    return res;
}


bool PlaneDataDisplay::setDataPackID( int attrib, DataPack::ID dpid )
{
    if ( attrib>=nrAttribs() )
	return false;

    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    const DataPack* datapack = dpman.obtain( dpid );
    mDynamicCastGet( const Attrib::Flat3DDataPack*, f3ddp, datapack );
    if ( !f3ddp )
    {
	dpman.release( dpid );
	return false;
    }

    setVolumeDataPackNoCache( attrib, f3ddp );
    if ( volumecache_[attrib] )
	dpman.release( volumecache_[attrib] );

    volumecache_.replace( attrib, f3ddp );
    return true;
}
 

void PlaneDataDisplay::setVolumeDataPackNoCache( int attrib, 
			const Attrib::Flat3DDataPack* f3ddp )
{
    if ( !f3ddp ) return;
    TypeSet<DataPack::ID> attridpids;
    
    //set display datapack.
    ObjectSet<const FlatDataPack> displaypacks;
    for ( int idx=0; idx<f3ddp->cube().nrCubes(); idx++ )
    {
	if ( f3ddp->getCubeIdx()==idx )
	{
	    DPM( DataPackMgr::FlatID() ).obtain( f3ddp->id() );
	    displaypacks += f3ddp;
	    attridpids += f3ddp->id();
	}
	else
	{
	    mDeclareAndTryAlloc( Attrib::Flat3DDataPack*, ndp,
		 Attrib::Flat3DDataPack(f3ddp->descID(),f3ddp->cube(),idx) );
	    DPM( DataPackMgr::FlatID() ).addAndObtain( ndp );
	    displaypacks += ndp;
	    attridpids += ndp->id();
	}
    }

    //transform data if necessary.
    const char* zdomain = getSelSpec(attrib)->zDomainKey();
    const bool alreadytransformed = zdomain && *zdomain;

    if ( !alreadytransformed && datatransform_ )
    {
	attridpids.erase();
	for ( int idx=0; idx<displaypacks.size(); idx++ )
	{
	    mDeclareAndTryAlloc( ZAxisTransformDataPack*, ztransformdp,
		ZAxisTransformDataPack( *displaypacks[idx], 
		f3ddp->cube().cubeSampling(), *datatransform_ ) );

	    ztransformdp->setInterpolate( !isClassification(attrib) );
	    ztransformdp->setOutputCS( getCubeSampling(true,true) );
	    ztransformdp->transform();

	    DPM( DataPackMgr::FlatID() ).addAndObtain( ztransformdp );
	    attridpids += ztransformdp->id();
	    DPM( DataPackMgr::FlatID() ).release( displaypacks[idx] );
	}
    }

    setDisplayDataPackIDs( attrib, attridpids );
    
    for ( int idx=0; idx<attridpids.size(); idx++ )
	DPM( DataPackMgr::FlatID() ).release( attridpids[idx] );
}


DataPack::ID PlaneDataDisplay::getDataPackID( int attrib ) const
{
    return volumecache_.validIdx(attrib) &&  volumecache_[attrib] 
	? volumecache_[attrib]->id() : DataPack::cNoID();
}


void PlaneDataDisplay::setRandomPosDataNoCache( int attrib,
						const BinIDValueSet* bivset )
{
    if ( !bivset ) return;

    const CubeSampling cs = getCubeSampling( true, true, 0 );
    TypeSet<DataPack::ID> attridpids;
    for ( int idx=1; idx<bivset->nrVals(); idx++ )
    {
	mDeclareAndTryAlloc( Array2DImpl<float>*, arr,
			Array2DImpl<float> ( cs.hrg.nrInl(), cs.hrg.nrCrl() ) );
	mDeclareAndTryAlloc( FlatDataPack*, fdp,
	    FlatDataPack( Attrib::DataPackCommon::categoryStr(false), arr ) );
        DPM(DataPackMgr::FlatID()).addAndObtain( fdp );
        attridpids += fdp->id();

    	float* texturedataptr = arr->getData();    
    	for ( int idy=0; idy<arr->info().getTotalSz(); idy++ )
    	    (*texturedataptr++) = mUdf(float);
	
    	BinIDValueSet::Pos pos;
    	BinID bid;
    	while ( bivset->next(pos,true) )
    	{
    	    bivset->get( pos, bid );
    	    BinID idxs = (bid-cs.hrg.start)/cs.hrg.step;
    	    arr->set( idxs.inl, idxs.crl, bivset->getVals(pos)[idx]);
    	}
    }

    setDisplayDataPackIDs( attrib, attridpids );

    for ( int idx=0; idx<attridpids.size(); idx++ )
	DPM(DataPackMgr::FlatID()).release( attridpids[idx] );
}


void PlaneDataDisplay::setDisplayDataPackIDs( int attrib,
			const TypeSet<DataPack::ID>& newdpids )
{
    TypeSet<DataPack::ID>& dpids = *displaycache_[attrib];
    for ( int idx=dpids.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).release( dpids[idx] );

    dpids = newdpids;
    for ( int idx=dpids.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).obtain( dpids[idx] );

    updateFromDisplayIDs( attrib );
}


void PlaneDataDisplay::updateFromDisplayIDs( int attrib )
{
    const TypeSet<DataPack::ID>& dpids = *displaycache_[attrib];
    int sz = dpids.size();
    if ( sz<1 )
    {
	if ( texture_ )
	{
	    texture_->setData( attrib, 0, 0 );
	    texture_->turnOn( false );
	}
	else
	{
	    channels_->setUnMappedData( attrib, 0, 0,
		    visBase::TextureChannels::None );
	    channels_->turnOn( false );
	}
	return;
    }

    if ( texture_ )
	texture_->setNrVersions( attrib, sz );
    else
	channels_->setNrVersions( attrib, sz );

    for ( int idx=0; idx<sz; idx++ )
    {
	int dpid = dpids[idx];
	const DataPack* datapack = DPM(DataPackMgr::FlatID()).obtain( dpid );
	mDynamicCastGet( const FlatDataPack*, fdp, datapack );
	if ( !fdp )
	{
	    if ( texture_ )
		texture_->turnOn( false );
	    else
		channels_->turnOn( false );
	    DPM(DataPackMgr::FlatID()).release( dpid );
	    continue;
	}

	const Array2D<float>& dparr = fdp->data();

	if ( texture_ )
	{
	    if ( !texture_->usesShading() && resolution_ )
		texture_->setDataOversample( attrib, idx, resolution_, 
			!isClassification( attrib ), &dparr, true );
	    else
	    {
		texture_->splitTexture( true );
		texture_->setData( attrib, idx, &dparr, true );
	    }
	}
	else
	{
	    const float* arr = dparr.getData();
	    visBase::TextureChannels::CachePolicy cp =
		visBase::TextureChannels::Cache;

	    if ( !arr )
	    {
		const od_int64 totalsz =
		    dparr.info().getSize(0) * dparr.info().getSize(1);
		mDeclareAndTryAlloc( float*, tmparr, float[totalsz] );

		if ( !tmparr )
		{
		    DPM(DataPackMgr::FlatID()).release( dpid );
		    continue;
		}

		dparr.getAll( tmparr );
		arr = tmparr;
		cp = visBase::TextureChannels::TakeOver;
	    }

	    channels_->setSize( 1, dparr.info().getSize(0),
				   dparr.info().getSize(1) );
	    channels_->setUnMappedData( attrib, idx, arr, cp );
	}

	rectangle_->setOriginalTextureSize( dparr.info().getSize(0),
					    dparr.info().getSize(1) );
	
	DPM(DataPackMgr::FlatID()).release( dpid );
    }
   
    if ( texture_ )
	texture_->turnOn( true );
    else
	channels_->turnOn( true );
}


const TypeSet<DataPack::ID>* PlaneDataDisplay::getDisplayDataPackIDs(int attrib)
{ 
    return displaycache_.validIdx(attrib) ? displaycache_[attrib] : 0; 
} 


inline int getPow2Sz( int actsz, bool above=true, int minsz=1,
		      int maxsz=INT_MAX )
{
    char npow = 0; char npowextra = actsz == 1 ? 1 : 0;
    int sz = actsz;
    while ( sz>1 )
    {
	if ( above && !npowextra && sz % 2 )
	npowextra = 1;
	sz /= 2; npow++;
    }

    sz = intpow( 2, npow + npowextra );
    if ( sz < minsz ) sz = minsz;
    if ( sz > maxsz ) sz = maxsz;
    return sz;
}


const Attrib::DataCubes* PlaneDataDisplay::getCacheVolume( int attrib ) const
{
    return attrib<volumecache_.size() && volumecache_[attrib]
	? &volumecache_[attrib]->cube() : 0;
}


#define mIsValid(idx,sz) ( idx>=0 && idx<sz )

void PlaneDataDisplay::getMousePosInfo( const visBase::EventInfo&,
					const Coord3& pos,
					BufferString& val, 
					BufferString& info ) const
{
    info = getManipulationString();
    getValueString( pos, val );
    /*
    val = "undef";
    BufferString valname;

    BinIDValue bidv( SI().transform(pos), pos.z );
    if ( datatransform_ ) bidv.value = datatransform_->transformBack( bidv );
    for ( int idx=as_.size()-1; idx>=0; idx-- )
    {
	if ( !isAttribEnabled(idx) ||
		texture_->getTextureTransparency(idx)==255 )
	    continue;

	const int version = texture_->currentVersion(idx);
	float fval = mUdf(float);

	if ( idx<volumecache_.size() && volumecache_[idx] )
	{
	    const Attrib::DataCubes* vc = volumecache_[idx];

	    if ( !vc->getValue(version,bidv,&fval,false) )
		continue;
	}

	if ( idx<rposcache_.size() && rposcache_[idx] )
	{
	    const BinIDValueSet& set = *rposcache_[idx];
	    const BinIDValueSet::Pos setpos = set.findFirst( bidv.binid );
	    if ( setpos.i==-1 || setpos.j==-1 )
		continue;

	    fval = set.getVals(setpos)[version+1];
	}

	bool islowest = true;
	for ( int idy=idx-1; idy>=0; idy-- )
	{
	    if ( (!volumecache_[idy] && !rposcache_[idy]) || 
		 !isAttribEnabled(idy) ||
		 texture_->getTextureTransparency(idy)==255 )
		continue;

	    islowest = false;
	    break;
	}    

	if ( !islowest )
	{
	    const Color col = texture_->getColorTab(idx).color(fval);
	    if ( col.t()==255 )
		continue;
	}

	if ( !mIsUdf(fval) )
	    val = fval;

	if ( volumecache_.size()>1 )
	{
	    BufferString attribstr = "(";
	    attribstr += as_[idx]->userRef();
	    attribstr += ")";
	    val.replaceAt( cValNameOffset(), (const char*)attribstr);
	}

	return;
    }
    */
}


void PlaneDataDisplay::getObjectInfo( BufferString& info ) const
{
    if ( orientation_==Inline )
    {
	info = "Inline: ";
	info += getCubeSampling(true,true).hrg.start.inl;
    }
    else if ( orientation_==Crossline )
    {
	info = "Crossline: ";
	info += getCubeSampling(true,true).hrg.start.crl;
    }
    else
    {
	info = SI().zIsTime() ? "Time: " : "Depth: ";
	float val = getCubeSampling(true,true).zrg.start;
	info += SI().zIsTime() ? mNINT(val * 1000) : val;
    }
}


bool PlaneDataDisplay::getCacheValue( int attrib, int version,
				      const Coord3& pos, float& res ) const
{
    if ( attrib>=volumecache_.size() ||
	 (!volumecache_[attrib] && !rposcache_[attrib]) )
	return false;

    const BinIDValue bidv( SI().transform(pos), pos.z );
    if ( attrib<volumecache_.size() && volumecache_[attrib] )
    {
	const int ver = texture_
	    ? texture_->currentVersion(attrib)
	    : channels_->currentVersion(attrib);

	const Attrib::DataCubes& vc = volumecache_[attrib]->cube();
	return vc.getValue( ver, bidv, &res, false );
    }
    else if ( attrib<rposcache_.size() && rposcache_[attrib] )
    {
	const BinIDValueSet& set = *rposcache_[attrib];
	const BinIDValueSet::Pos setpos = set.findFirst( bidv.binid );
	if ( setpos.i==-1 || setpos.j==-1 )
	    return false;

	res = set.getVals(setpos)[version+1];
	return true;
    }

    return false;
}


bool PlaneDataDisplay::isVerticalPlane() const
{
    return orientation_ != PlaneDataDisplay::Timeslice;
}


void PlaneDataDisplay::setSceneEventCatcher( visBase::EventCatcher* ec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(
		mCB(this,PlaneDataDisplay,updateMouseCursorCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = ec;

    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	eventcatcher_->eventhappened.notify(
		mCB(this,PlaneDataDisplay,updateMouseCursorCB) );
    }
}


void PlaneDataDisplay::updateMouseCursorCB( CallBacker* cb )
{
    char newstatus = 1; // 1= zdrag, 2=pan
    if ( cb )
    {
	mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
	if ( eventinfo.pickedobjids.indexOf(id())==-1 )
	    newstatus = 0;
	else
	{
	    const unsigned int buttonstate =
		(unsigned int) eventinfo.buttonstate_;

	    if ( buttonstate==dragger_->getTransDragKeys(false) )
		newstatus = 2;
	}
    }

    if ( !isSelected() || !isOn() || isLocked() )
	newstatus = 0;

    if ( !newstatus ) mousecursor_.shape_ = MouseCursor::NotSet;
    else if ( newstatus==1 ) mousecursor_.shape_ = MouseCursor::PointingHand;
    else mousecursor_.shape_ = MouseCursor::SizeAll;
}


SurveyObject* PlaneDataDisplay::duplicate() const
{
    PlaneDataDisplay* pdd = create();
    pdd->setOrientation( orientation_ );
    pdd->setCubeSampling( getCubeSampling(false,true,0) );

    while ( nrAttribs() > pdd->nrAttribs() )
	pdd->addAttrib();

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	if ( !getSelSpec(idx) ) continue;

	pdd->setSelSpec( idx, *getSelSpec(idx) );
	pdd->setDataPackID( idx, getDataPackID(idx) );

	const int ctid = pdd->getColTabID( idx );
	visBase::DataObject* obj = ctid>=0 ? visBase::DM().getObject(ctid) : 0;
	mDynamicCastGet(visBase::VisColorTab*,vct,obj);
	if ( vct && texture_ )
	    vct->colorSeq().loadFromStorage(
		    texture_->getColorTab(idx).colorSeq().colors().name() );
    }

    return pdd;
}


void PlaneDataDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    MultiTextureSurveyObject::fillPar( par, saveids );

    par.set( sKeyOrientation(), eString(Orientation,orientation_) );
    getCubeSampling( false, true ).fillPar( par );

    const int gridlinesid = gridlines_->id();
    par.set( sKeyGridLinesID(), gridlinesid );
    if ( saveids.indexOf(gridlinesid) == -1 ) saveids += gridlinesid;
}


int PlaneDataDisplay::usePar( const IOPar& par )
{
    const int res =  MultiTextureSurveyObject::usePar( par );
    if ( res!=1 ) return res;

    const char* orires = par.find( sKeyOrientation() );
    if ( orires && *orires )
	setOrientation( eEnum(Orientation,orires) );

    CubeSampling cs;
    if ( cs.usePar( par ) )
    {
	csfromsession_ = cs;
	setCubeSampling( cs );
    }

    int gridlinesid;
    if ( par.get(sKeyGridLinesID(),gridlinesid) )
    { 
        DataObject* dataobj = visBase::DM().getObject( gridlinesid );
        if ( !dataobj ) return 0;
        mDynamicCastGet(visBase::GridLines*,gl,dataobj)
        if ( !gl ) return -1;
	removeChild( gridlines_->getInventorNode() );
	gridlines_->unRef();
	gridlines_ = gl;
	gridlines_->ref();
	gridlines_->setPlaneCubeSampling( cs );
	int childidx = texture_ 
	    ? childIndex(texture_->getInventorNode())
	    : childIndex( channels_->getInventorNode() );
	insertChild( childidx, gridlines_->getInventorNode() );
    }

    return 1;
}



} // namespace visSurvey

