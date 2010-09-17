/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisplaycontrol.cc,v 1.18 2010-09-17 12:26:07 cvsbruno Exp $";


#include "uiwelldisplaycontrol.h"

#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uiwelllogdisplay.h"

#include "mouseevent.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "wellmarker.h"


uiWellDisplayControl::uiWellDisplayControl( uiWellLogDisplay& l ) 
    : selmarker_(0)	
    , seldisp_(0)			
    , lastselmarker_(0)
    , ismousedown_(false)		       
    , isctrlpressed_(false)
    , time_(0)					       
    , depth_(0)					       
    , posChanged(this)
    , mousePressed(this)
    , mouseReleased(this)
    , markerSel(this)			 
{
    addLogDisplay( l );
}    


uiWellDisplayControl::~uiWellDisplayControl()
{
    clear();
}


void uiWellDisplayControl::addLogDisplay( uiWellLogDisplay& disp )
{
    logdisps_ += &disp;
    disp.scene().setMouseEventActive( true );
    MouseEventHandler& meh = mouseEventHandler( logdisps_.size()-1 );
    meh.movement.notify( mCB( this, uiWellDisplayControl, setSelLogDispCB ) );
    meh.movement.notify( mCB( this, uiWellDisplayControl, setSelMarkerCB ) );
    meh.movement.notify( mCB( this, uiWellDisplayControl, mouseMovedCB ) );
    meh.buttonReleased.notify( mCB(this,uiWellDisplayControl,mouseReleasedCB) );
    meh.buttonPressed.notify( mCB(this,uiWellDisplayControl,mousePressedCB) );
}


void uiWellDisplayControl::removeLogDisplay( uiWellLogDisplay& disp )
{
    MouseEventHandler& meh = mouseEventHandler( logdisps_.size()-1 );
    meh.movement.remove( mCB( this, uiWellDisplayControl, mouseMovedCB ) );
    meh.movement.remove( mCB( this, uiWellDisplayControl, setSelMarkerCB ) );
    meh.movement.remove( mCB( this, uiWellDisplayControl, setSelLogDispCB ) );
    meh.buttonPressed.remove( mCB(this,uiWellDisplayControl,mousePressedCB) );
    meh.buttonReleased.remove( mCB(this,uiWellDisplayControl,mouseReleasedCB) );
    logdisps_ -= &disp;
}


void uiWellDisplayControl::clear()
{
    for ( int idx=logdisps_.size()-1; idx>=0; idx-- )
	removeLogDisplay( *logdisps_[idx] );

    seldisp_ = 0; 	
    lastselmarker_ = 0;
}


MouseEventHandler& uiWellDisplayControl::mouseEventHandler( int dispidx )
    { return logdisps_[dispidx]->scene().getMouseEventHandler(); }


MouseEventHandler* uiWellDisplayControl::mouseEventHandler()
    { return seldisp_ ? &seldisp_->scene().getMouseEventHandler() : 0; }


void uiWellDisplayControl::mouseMovedCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh ) 
	return;
    if ( !mevh->hasEvent() || mevh->isHandled() ) 
	return;

    if ( seldisp_ )
    {
	const uiWellDahDisplay::Data& zdata = seldisp_->zData();
	float pos = seldisp_->logData(true).yax_.getVal(mevh->event().pos().y);
	if ( zdata.zistime_ )
	{
	    time_ = pos;
	    if ( zdata.d2tm_ && zdata.d2tm_->size() >= 1 )
		depth_ = zdata.d2tm_->getDah( pos*0.001 );
	}
	else
	{
	    depth_ = pos;
	    if ( zdata.d2tm_ )
		time_ = zdata.d2tm_->getTime( pos )*1000;
	}
    }

    BufferString info;
    getPosInfo( info );
    CBCapsule<BufferString> caps( info, this );
    posChanged.trigger( &caps );
}


void uiWellDisplayControl::getPosInfo( BufferString& info ) const
{
    info.setEmpty();
    if ( !seldisp_ ) return;
    if ( selmarker_ )
    {
	float markerpos = selmarker_->dah();
	info += " Marker: ";
	info += selmarker_->name();
	info += "  ";
    }
    info += "  MD: ";
    info += toString( mNINT(depth_) );
    info += "  Time: ";
    info += toString( mNINT(time_) );

#define mGetLogPar( ld )\
    info += "   ";\
    info += ld.wl_->name();\
    info += ":";\
    info += toString( ld.wl_->getValue( depth_ ) );\
    info += ld.wl_->unitMeasLabel();\

    const uiWellLogDisplay::LogData& ldata1 = seldisp_->logData(true);
    const uiWellLogDisplay::LogData& ldata2 = seldisp_->logData(false);
    if ( ldata1.wl_ )
    { mGetLogPar( ldata1 ) }
    if ( ldata2.wl_ )
    { mGetLogPar( ldata2 ) }
}


void uiWellDisplayControl::setPosInfo( CallBacker* cb )
{
    info_.setEmpty();
    mCBCapsuleUnpack(BufferString,mesg,cb);
    if ( mesg.isEmpty() ) return;
    info_ += mesg;
}


void uiWellDisplayControl::mousePressedCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh ) 
	return;
    if ( !mevh->hasEvent() || mevh->isHandled() ) 
	return;

    ismousedown_ = true;
    isctrlpressed_ = seldisp_ ? seldisp_->isCtrlPressed() : false;

    mousePressed.trigger();
}


void uiWellDisplayControl::mouseReleasedCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh ) 
	return;
    if ( !mevh->hasEvent() || mevh->isHandled() ) 
	return;

    ismousedown_ = false;
    isctrlpressed_ = false;
    mouseReleased.trigger();
}


void uiWellDisplayControl::setSelLogDispCB( CallBacker* cb )
{
    seldisp_ = 0;
    if ( cb )
    {
	mDynamicCastGet(MouseEventHandler*,mevh,cb)
	for ( int idx=0; idx<logdisps_.size(); idx++)
	{
	    uiWellLogDisplay* ld = logdisps_[idx];
	    if ( &ld->getMouseEventHandler() == mevh ) 
	    {
		seldisp_ = ld;
		break;
	    }
	}
    }
}


void uiWellDisplayControl::setSelMarkerCB( CallBacker* cb ) 
{
    if ( !seldisp_ ) return;
    const MouseEvent& ev = seldisp_->getMouseEventHandler().event();
    int mousepos = ev.pos().y;
    Well::Marker* selmrk = 0;
    for ( int idx=0; idx<seldisp_->markerdraws_.size(); idx++ )
    {
	uiWellLogDisplay::MarkerDraw& markerdraw = *seldisp_->markerdraws_[idx];
	const Well::Marker& mrk = markerdraw.mrk_;
	uiLineItem& li = *markerdraw.lineitm_;

	if ( abs( li.getPos().y - mousepos )<2 ) 
	{
	    selmrk = const_cast<Well::Marker*>( &mrk );
	    break;
	}
    }
    bool markerchanged = ( lastselmarker_ != selmrk );
    setSelMarker( selmrk );
    if ( markerchanged )
	markerSel.trigger();
}


void uiWellDisplayControl::setSelMarker( const Well::Marker* mrk )
{
    if ( lastselmarker_ && ( lastselmarker_ != mrk ) )
	highlightMarker( *lastselmarker_, false );

    if ( mrk )
	highlightMarker( *mrk, true );

    selmarker_ = mrk;

    if ( lastselmarker_ != mrk )
	lastselmarker_ = mrk;
}


void uiWellDisplayControl::highlightMarker( const Well::Marker& mrk, bool yn )
{
    for ( int iddisp=0; iddisp<logdisps_.size(); iddisp++ )
    {
	uiWellLogDisplay& ld = *logdisps_[iddisp];
	const LineStyle& ls = ld.setup_.markerls_;
	uiWellLogDisplay::MarkerDraw* mrkdraw = ld.getMarkerDraw( mrk );
	if ( !mrkdraw ) continue;
	uiLineItem& li = *mrkdraw->lineitm_;
	int width = yn ? ls.width_+2 : ls.width_;
	li.setPenStyle( LineStyle( ls.type_, width, mrk.color() ) ); 
    }
}


void uiWellDisplayControl::setCtrlPressed( bool yn )
{
    for ( int idx=0; idx<logdisps_.size(); idx++)
    {
	uiWellLogDisplay* ld = logdisps_[idx];
	ld->setCtrlPressed( yn );
    }
    isctrlpressed_ = yn;
}
