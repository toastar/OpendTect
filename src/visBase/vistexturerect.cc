/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: vistexturerect.cc,v 1.30 2003-02-27 16:43:42 nanne Exp $";

#include "vistexturerect.h"
#include "iopar.h"
#include "visrectangle.h"
#include "arrayndimpl.h"
#include "visdataman.h"
#include "viscolortab.h"
#include "vistexture2.h"
#include "ptrman.h"
#include "position.h"

#include <math.h>

mCreateFactoryEntry( visBase::TextureRect );


const char* visBase::TextureRect::rectangleidstr = "Rectangle ID";
const char* visBase::TextureRect::textureidstr = "Texture ID";

visBase::TextureRect::TextureRect()
    : texture(0)
    , rectangle( 0 )
    , manipstartnotifier( this )
    , manipchnotifier( this )
    , manipendsnotifier( this )
{
    setTexture( *visBase::Texture2::create() );
    useTexture( true );

    setRectangle( visBase::Rectangle::create() );
}


visBase::TextureRect::~TextureRect()
{
    if ( rectangle )
    {
	rectangle->manipStarts()->remove(
				mCB( this, TextureRect, triggerManipStarts ));
	rectangle->manipChanges()->remove(
				mCB( this, TextureRect, triggerManipChanges ));
	rectangle->manipEnds()->remove(
				mCB( this, TextureRect, triggerManipEnds ));
	rectangle->selection()->remove(
				mCB( this, TextureRect, triggerSel ));
	rectangle->deSelection()->remove(
				mCB( this, TextureRect, triggerDeSel ));
	rectangle->unRef();
    }

    if ( texture )
    {
	removeChild( texture->getData() );
	texture->unRef();
    }
}


void visBase::TextureRect::setTexture( visBase::Texture2& newtext )
{
    if ( texture )
    {
	removeChild( texture->getData() );
	texture->unRef();
    }

    texture = &newtext;
    texture->ref();
    insertChild( 1, texture->getData() );
}


visBase::Texture2& visBase::TextureRect::getTexture()
{
    return *texture;
}


void visBase::TextureRect::setRectangle( Rectangle* nr )
{
    if ( rectangle )
    {
	rectangle->manipStarts()->remove(
				mCB(this,TextureRect,triggerManipStarts) );
	rectangle->manipChanges()->remove(
				mCB(this,TextureRect,triggerManipChanges) );
	rectangle->manipEnds()->remove(
				mCB(this,TextureRect,triggerManipEnds) );
	rectangle->selection()->remove( mCB(this,TextureRect,triggerSel) );
	rectangle->deSelection()->remove( mCB(this,TextureRect,triggerDeSel) );

	removeChild( rectangle->getData() );
	rectangle->unRef();
    }

    rectangle = nr;
    rectangle->ref();
    addChild( rectangle->getData() );
    rectangle->setMaterial( 0 );

    rectangle->manipStarts()->notify( mCB(this,TextureRect,triggerManipStarts));
    rectangle->manipChanges()->notify( 
	    		       mCB(this,TextureRect,triggerManipChanges) );
    rectangle->manipEnds()->notify( mCB(this,TextureRect,triggerManipEnds) );
    rectangle->selection()->notify( mCB(this,TextureRect,triggerSel) );
    rectangle->deSelection()->notify( mCB(this,TextureRect,triggerDeSel) );
}


const visBase::Rectangle& visBase::TextureRect::getRectangle() const
{ return *rectangle; }
 
 
visBase::Rectangle& visBase::TextureRect::getRectangle()
{ return *rectangle; }
 

void visBase::TextureRect::setColorTab( VisColorTab& ct )
{
    texture->setColorTab( ct );
}


const visBase::VisColorTab& visBase::TextureRect::getColorTab() const
{ return texture->getColorTab(); }
 
 
visBase::VisColorTab& visBase::TextureRect::getColorTab()
{ return texture->getColorTab(); }


void visBase::TextureRect::setClipRate( float cr )
{
    texture->setClipRate( cr );
}


float visBase::TextureRect::clipRate() const 
{ 
    return texture->clipRate();
}


void  visBase::TextureRect::setAutoScale( bool yn )
{
    texture->setAutoScale( yn );
}


bool visBase::TextureRect::autoScale() const
{
    return texture->autoScale();
}


void visBase::TextureRect::useTexture( bool yn )
{
    texture->turnOn( yn );
}


bool visBase::TextureRect::usesTexture() const
{
    return texture->isOn();
}


void visBase::TextureRect::setData( const Array2D<float>& data )
{
    texture->setData( &data );
}


void visBase::TextureRect::setTextureQuality( float q )
{
    texture->setTextureQuality( q );
}


float visBase::TextureRect::getTextureQuality() const
{
    return texture->getTextureQuality();
}


int visBase::TextureRect::getNrResolutions() const
{
    return 3;
}


void visBase::TextureRect::setResolution( int res )
{
    texture->setResolution( res );
}


int visBase::TextureRect::getResolution() const
{
    return texture->getResolution();
}


void visBase::TextureRect::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    int rectid = rectangle->id();
    par.set( rectangleidstr, rectid );

    int textureid = texture->id();
    par.set( textureidstr, textureid );

    if ( saveids.indexOf(rectid) == -1 ) saveids += rectid;
    if ( saveids.indexOf(textureid) == -1 ) saveids += textureid;
}


int visBase::TextureRect::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    res = useOldPar( par );
    if ( res < 1 ) return res;

    if ( res == 2 )
    {
	int textureid;
	if ( !par.get( textureidstr, textureid ) ) return -1;
	DataObject* dataobj = DM().getObj( textureid );
	if ( !dataobj ) return 0;
	mDynamicCastGet(Texture2*,texture_,dataobj);
	if ( !texture_ ) return -1;
	setTexture( *texture_ );
    }

    int rectid;
    if ( !par.get( rectangleidstr, rectid ) ) return -1;
    DataObject* dataobj = DM().getObj( rectid );
    if ( !dataobj ) return 0;
    mDynamicCastGet( Rectangle*, rect, dataobj );
    if ( !rect ) return -1;

    setRectangle( rect );

    return 1;
}


int visBase::TextureRect::useOldPar( const IOPar& par )
{ 
    // Will be used in d-Tect 1.5 to be able to restore old sessions
    int coltabid;
    if ( !par.get( "ColorTable ID", coltabid ) ) return 2;
    // use new par;
    
    DataObject* dataobj = DM().getObj( coltabid );
    if ( !dataobj ) return 0;
    mDynamicCastGet( VisColorTab*, coltab, dataobj );
    if ( !coltab ) return -1;

    texture->setColorTab( *coltab );

    float cliprt = 0.025;
    if ( par.get( "Cliprate", cliprt ) )
	setClipRate( cliprt );

    bool autosc = true;
    if ( par.getYN( "Auto scale", autosc ) )
        setAutoScale( autosc );

    int newres = 0;
    par.get( "Resolution", newres );
    setResolution( newres );

    float texturequality = 1;
    par.get( "Texture quality", texturequality );
    setTextureQuality( texturequality );

    bool usetext = true;
    par.getYN( "Uses texture", usetext );
    useTexture( usetext );

    return 1;
}
