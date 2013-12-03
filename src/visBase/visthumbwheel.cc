/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "visthumbwheel.h"

#include <osgGeo/ThumbWheel>

mCreateFactoryEntry( visBase::ThumbWheel );

#define col2f(rgb) float(col.rgb())/255

namespace visBase
{
    
class ThumbWheelMess : public osg::NodeCallback
{
public:
		ThumbWheelMess( ThumbWheel* t )
		    : visthumbwheel_( t )
		{}
    void	operator() (osg::Node* node, osg::NodeVisitor* nv )
		{
		    if ( visthumbwheel_ && nv )
		    {
			osgGeo::ThumbWheelEventNodeVisitor* thnv =
			    (osgGeo::ThumbWheelEventNodeVisitor*) nv;
					
			visthumbwheel_->rotation.trigger(
				    thnv->getDeltaAngle(), visthumbwheel_ );
		    }
		}
    void	detach() { visthumbwheel_ = 0; }
    
protected:
		~ThumbWheelMess()
		{
		    
		}
    ThumbWheel*	visthumbwheel_;
};

ThumbWheel::ThumbWheel()
    : rotation( this )
    , thumbwheel_( new osgGeo::ThumbWheel )
{
    setOsgNode( thumbwheel_ );
    thumbwheel_->ref();
    messenger_ = new ThumbWheelMess( this );
    messenger_->ref();
    thumbwheel_->addRotateCallback( messenger_ );
}
    

ThumbWheel::~ThumbWheel()
{
    messenger_->detach();
    messenger_->unref();
    thumbwheel_->unref();
}
    
    
void ThumbWheel::setPosition(bool horizontal, float x, float y, float len,
			     float width, float zval)
{
    const osg::Vec2 min( x, y );
    const osg::Vec2 max( horizontal ? x+len : x+width,
			 horizontal ? y+width : y+len );
    
    thumbwheel_->setShape( horizontal ? 0 : 1, min, max, zval );
}


void ThumbWheel::setBackgroundColor( const Color& col )
{
    osg::Vec4 osgcol(col2f(r),col2f(g),col2f(b), 1.0);
    osg::Vec4 gray( 0.5f, 0.5f, 0.5f, 1.0f );

    thumbwheel_->setBorderColor( (gray+osgcol)/2 );
}
    

float ThumbWheel::getAngle() const
{
    return thumbwheel_->getAngle();
}
    
    
void ThumbWheel::setAngle( float a )
{
    thumbwheel_->setAngle( a );
}


}; // namespace visBase
