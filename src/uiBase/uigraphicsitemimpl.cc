/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2008
________________________________________________________________________

-*/

#include "uigraphicsitemimpl.h"

#include "angles.h"
#include "odgraphicsitem.h"
#include "polygon.h"

#include "uifont.h"
#include "uigeom.h"
#include "uigroup.h"
#include "uiobj.h"
#include "uipixmap.h"
#include "uirgbarray.h"

#include <QMutex>
#include <QBrush>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QGraphicsProxyWidget>
#include <QPen>
#include <QTextDocument>
#include <QWidget>

mUseQtnamespace

uiObjectItem::uiObjectItem( uiObject* obj )
    : uiGraphicsItem(mkQtObj())
    , obj_(0)
    , grp_(0)
{
    setObject( obj );
}


uiObjectItem::uiObjectItem( uiGroup* grp )
    : uiGraphicsItem(mkQtObj())
    , obj_(0)
    , grp_(0)
{
    setGroup( grp );
}


uiObjectItem::~uiObjectItem()
{
}


uiObject* uiObjectItem::getObject()
{ return obj_; }


void uiObjectItem::setObject( uiObject* obj )
{
    obj_ = obj;
    qwidgetitem_->setWidget( obj_ ? obj_->getWidget(0) : 0 );
}


uiGroup* uiObjectItem::getGroup()
{ return grp_; }

void uiObjectItem::setGroup( uiGroup* grp )
{
    grp_ = grp;
    if ( !grp_ ) return;

    setObject( grp->attachObj() );
}


QGraphicsItem* uiObjectItem::mkQtObj()
{
    qwidgetitem_ = new QGraphicsProxyWidget();
    return qwidgetitem_;
}


void uiObjectItem::setObjectSize( int szx, int szy )
{
    if ( grp_ )
    {
	grp_->setSize( uiSize( szx, szy ) );
    }
    else if ( obj_ && obj_->getWidget(0) )
    {
	obj_->getWidget(0)->setMinimumSize( szx, szy );
	obj_->getWidget(0)->setMaximumSize( szx, szy );
    }
}


const uiSize uiObjectItem::objectSize() const
{
    return ( obj_ && obj_->getWidget(0)
	    ? uiSize(obj_->getWidget(0)->size().width(),
			   obj_->getWidget(0)->size().height() )
	    : uiSize(0,0) );
}


// uiEllipseItem
uiEllipseItem::uiEllipseItem()
    : uiGraphicsItem(mkQtObj())
{}


uiEllipseItem::uiEllipseItem( const uiPoint& center, const uiSize& sz )
    : uiGraphicsItem(mkQtObj())
{
    setPos( center );
    setSize( sz );
}


uiEllipseItem::uiEllipseItem( const uiSize& size )
    : uiGraphicsItem(mkQtObj())
{
    setSize( size );
}


uiEllipseItem::~uiEllipseItem()
{
}


QGraphicsItem* uiEllipseItem::mkQtObj()
{
    qellipseitem_ = new QGraphicsEllipseItem();
    return qellipseitem_;
}


void uiEllipseItem::setSize( const uiSize& size )
{
    const int width = size.hNrPics();
    const int height = size.vNrPics();
    qellipseitem_->setRect( -width/2, -height/2, width, height );
}


uiCircleItem::uiCircleItem()
    : uiEllipseItem()
{}


uiCircleItem::uiCircleItem( const uiPoint& center, int r )
    : uiEllipseItem(center,uiSize(2*r,2*r))
{}


uiCircleItem::uiCircleItem( int r )
    : uiEllipseItem(uiSize(2*r,2*r))
{}


void uiCircleItem::setRadius( int r )
{
    setSize( uiSize(2*r,2*r) );
}


// uiLineItem

#define mInit \
      uiGraphicsItem(mkQtObj()) \
    , qpen_(* new QPen)

uiLineItem::uiLineItem()
    : mInit
{}


uiLineItem::uiLineItem( const uiPoint& startpos, const uiPoint& endpos )
    : mInit
{
    setLine( startpos, endpos );
}


uiLineItem::uiLineItem( float x1, float y1, float x2, float y2 )
    : mInit
{
    setLine( x1, y1, x2, y2 );
}


uiLineItem::uiLineItem( int x1, int y1, int x2, int y2 )
   : mInit
{
    setLine( x1, y1, x2, y2 );
}


uiLineItem::uiLineItem( const uiPoint& pt, float angle, float len )
    : mInit
{
    Geom::Point2D<float> ptf( pt.x_, pt.y_ );
    Geom::Point2D<float> endpt( ptf );
    float delta = len * cos( angle );
    endpt.x_ += delta;
    delta = -len * sin( angle );
    endpt.y_ += delta;

    setLine( ptf, endpt );
}


uiLineItem::~uiLineItem()
{
    delete &qpen_;
}


QGraphicsItem* uiLineItem::mkQtObj()
{
    qlineitem_ = new ODGraphicsLineItem();
    return qlineitem_;
}


void uiLineItem::setLine( const uiPoint& start, const uiPoint& end )
{ setLine( start.x_, start.y_, end.x_, end.y_ ); }

void uiLineItem::setLine( const Geom::Point2D<float>& start,
			  const Geom::Point2D<float>& end )
{ setLine( start.x_, start.y_, end.x_, end.y_ ); }

void uiLineItem::setLine( const uiWorldPoint& start, const uiWorldPoint& end )
{ setLine( (float)start.x_, (float)start.y_, (float)end.x_, (float)end.y_ ); }

void uiLineItem::setLine( int x1, int y1, int x2, int y2 )
{ setLine( (float)x1, (float)y1, (float)x2, (float)y2 ); }


void uiLineItem::setLine( float x1, float y1, float x2, float y2 )
{
    qlineitem_->setLine( x1, y1, x2, y2 );
}


void uiLineItem::setLine( const Geom::Point2D<float>& centerpos,
	      float dx1, float dy1, float dx2, float dy2 )
{
    setLine( centerpos.x_ - dx1, centerpos.y_ + dy1,
	     centerpos.x_ + dx2, centerpos.y_ - dy2 );
}


void uiLineItem::setLine( const Geom::Point2D<int>& centerpos,
	      int dx1, int dy1, int dx2, int dy2 )
{
    setLine( centerpos.x_-dx1, centerpos.y_+dy1,
	     centerpos.x_+dx2, centerpos.y_-dy2 );
}


uiRect uiLineItem::lineRect() const
{
    const QRectF rect = qlineitem_->boundingRect();
    return uiRect( rect.top(), rect.left(), rect.bottom(), rect.right() );
}


void uiLineItem::setPenColor( const Color& col, bool )
{
    QPen qpen = qlineitem_->getQpen();
    qpen.setColor( QColor(col.rgb()) );
    qpen_ = qpen;
    qlineitem_->setQPen( qpen );
}


void uiLineItem::setPenStyle( const OD::LineStyle& ls, bool )
{
    QBrush qbrush( QColor(QRgb(ls.color_.rgb())) );
    QPen qpen( qbrush, ls.width_, (Qt::PenStyle)ls.type_ );
    qpen_ = qpen;
    qlineitem_->setQPen( qpen );
}


void uiLineItem::highlight()
{
    qlineitem_->highlight();
}


void uiLineItem::unHighlight()
{
    qlineitem_->unHighlight();
}


//uiDynamicImageItem
uiDynamicImageItem::uiDynamicImageItem()
    : uiGraphicsItem( mkQtObj() )
{}


uiDynamicImageItem::~uiDynamicImageItem()
{}


void uiDynamicImageItem::setImage( bool isdynamic, const uiRGBArray& image,
				   const uiWorldRect& wr )
{
    item_->setImage( isdynamic, image.qImage(),
	      QRectF( wr.left(), wr.top(), wr.width(), wr.height() ) );
}


NotifierAccess& uiDynamicImageItem::wantsData()
{ return item_->wantsData; }


bool uiDynamicImageItem::isSnapshot() const
{ return item_->isSnapshot(); }


uiWorldRect uiDynamicImageItem::wantedWorldRect() const
{
    QRectF res = item_->wantedWorldRect();
    return uiWorldRect( res.left(), res.top(), res.right(), res.bottom() );
}


uiSize uiDynamicImageItem::wantedScreenSize() const
{
    QSize sz = item_->wantedScreenSize();
    return uiSize( sz.width(), sz.height() );
}


QGraphicsItem* uiDynamicImageItem::mkQtObj()
{
    item_ = new ODGraphicsDynamicImageItem();
    return item_;
}

// uiPixmapItem
uiPixmapItem::uiPixmapItem()
    : uiGraphicsItem(mkQtObj())
{}


uiPixmapItem::uiPixmapItem( const uiPixmap& pm )
    : uiGraphicsItem(mkQtObj())
{
    setPixmap( pm );
}


uiPixmapItem::uiPixmapItem( const uiPoint& pos, const uiPixmap& pm )
    : uiGraphicsItem(mkQtObj())
{
    setPos( pos );
    setPixmap( pm );
}


uiPixmapItem::~uiPixmapItem()
{
}


QGraphicsItem* uiPixmapItem::mkQtObj()
{
    qpixmapitem_ = new ODGraphicsPixmapItem();
    return qpixmapitem_;
}


void uiPixmapItem::setOffset( int left, int top )
{
    qpixmapitem_->setOffset( QPointF(left,top) );
}


void uiPixmapItem::setPixmap( const uiPixmap& pixmap )
{
    qpixmapitem_->setPixmap( *pixmap.qpixmap());
}


void uiPixmapItem::setPaintInCenter( bool yn )
{
    qpixmapitem_->setPaintInCenter( yn );
}


// uiPolygonItem
uiPolygonItem::uiPolygonItem()
    : uiGraphicsItem(mkQtObj())
{}


#define mImplPolygonConstructor( type ) \
uiPolygonItem::uiPolygonItem( type polygon, bool dofill ) \
    : uiGraphicsItem(mkQtObj()) \
{ \
    setPolygon( polygon ); \
    if ( dofill ) \
	fill(); \
}


mImplPolygonConstructor( const ODPolygon<int>& )
mImplPolygonConstructor( const TypeSet<uiPoint>& );
mImplPolygonConstructor( const TypeSet<uiWorldPoint>& );

uiPolygonItem::~uiPolygonItem()
{
}


QGraphicsItem* uiPolygonItem::mkQtObj()
{
    qpolygonitem_ = new ODGraphicsPolyLineItem();
    return qpolygonitem_;
}


void uiPolygonItem::fill()
{
    qpolygonitem_->setFillRule( Qt::OddEvenFill );
}


#define mImplSetPolygon( type, ptaccess ) \
void uiPolygonItem::setPolygon( type ptlist ) \
{ \
    QPolygonF qpolygonf;\
    for ( unsigned int idx=0; idx<ptlist.size(); idx++ )\
    { \
	if ( ptaccess[idx].isDefined() ) \
	    qpolygonf += QPointF( (float) ptaccess[idx].x_, \
				  (float) ptaccess[idx].y_ ); \
    } \
    if ( !ptlist.isEmpty() && !qpolygonf.isClosed() ) \
    qpolygonf += qpolygonf.first(); \
    qpolygonitem_->setPolyLine( qpolygonf, true ); \
}


mImplSetPolygon( const TypeSet<uiPoint>&, ptlist )
mImplSetPolygon( const ODPolygon<int>&, ptlist.data() )
mImplSetPolygon( const TypeSet<uiWorldPoint>&, ptlist )


// uiPolyLineItem
uiPolyLineItem::uiPolyLineItem()
    : uiGraphicsItem(mkQtObj())
{}


uiPolyLineItem::uiPolyLineItem( const TypeSet<uiPoint>& ptlist )
    : uiGraphicsItem(mkQtObj())
{
    setPolyLine( ptlist );
}


uiPolyLineItem::uiPolyLineItem( const TypeSet<uiWorldPoint>& ptlist )
    : uiGraphicsItem(mkQtObj())
{
    setPolyLine( ptlist );
}


uiPolyLineItem::~uiPolyLineItem()
{}


#define mImplSetPolyline( type ) \
void uiPolyLineItem::setPolyLine( type ptlist ) \
{ \
    QPainterPath path; \
    bool newpt = true; \
    for ( int idx=0; idx<ptlist.size(); idx++ ) \
    { \
	if ( !ptlist[idx].isDefined() ) \
	{  \
	    newpt = true; \
	    continue; \
	} \
	if ( newpt ) \
	{ \
	    path.moveTo( ptlist[idx].x_, ptlist[idx].y_ ); \
	    newpt = false; \
	} \
	else \
	    path.lineTo( ptlist[idx].x_, ptlist[idx].y_ ); \
    } \
 \
    odgraphicspath_->set( path ); \
}

mImplSetPolyline( const TypeSet<uiPoint>& );
mImplSetPolyline( const TypeSet<uiWorldPoint>& );
#undef mImplSetPolyline


QGraphicsItem* uiPolyLineItem::mkQtObj()
{
    odgraphicspath_ = new ODGraphicsPathItem();
    return odgraphicspath_;
}


void uiPolyLineItem::highlight()
{
    odgraphicspath_->highlight();
}


void uiPolyLineItem::unHighlight()
{
    odgraphicspath_->unHighlight();
}


//uiMultiColorPolyLineItem
uiMultiColorPolyLineItem::uiMultiColorPolyLineItem()
    : uiGraphicsItem(mkQtObj())
{}


uiMultiColorPolyLineItem::uiMultiColorPolyLineItem(
				const TypeSet<uiPoint>& pts )
{
    setPolyLine( pts );
}


uiMultiColorPolyLineItem::uiMultiColorPolyLineItem(
				const TypeSet<uiWorldPoint>& pts )
{
    setPolyLine( pts );
}


uiMultiColorPolyLineItem::~uiMultiColorPolyLineItem()
{}


QGraphicsItem* uiMultiColorPolyLineItem::mkQtObj()
{
    odmulticoloritem_ = new ODGraphicsMultiColorPolyLineItem();
    return odmulticoloritem_;
}


#define mImplSetPolyline( type ) \
void uiMultiColorPolyLineItem::setPolyLine( type ptlist ) \
{ \
    QPolygonF qpolygonf; \
    for ( int idx=0; idx<ptlist.size(); idx++ ) \
    { \
	if ( !ptlist[idx].isDefined() ) \
	    qpolygonf += QPointF( mUdf(double), mUdf(double) ); \
	else \
	    qpolygonf += QPointF( mCast(double,ptlist[idx].x_), \
				  mCast(double,ptlist[idx].y_) ); \
    } \
    odmulticoloritem_->setPolyLine( qpolygonf ); \
}

mImplSetPolyline( const TypeSet<uiPoint>& );
mImplSetPolyline( const TypeSet<uiWorldPoint>& );
#undef mImplSetPolyline


void uiMultiColorPolyLineItem::setColors(
	const TypeSet<Color>& colors, bool usetransparency )
{
    QVector<QPen> qpens( colors.size() );
    for ( int idx=0; idx<colors.size(); idx++ )
    {
	qpens[idx] = QPen( QColor(QRgb(colors[idx].rgb())), 2 );
	if ( usetransparency )
	    qpens[idx].color().setAlpha( 255-colors[idx].t() );
    }
    odmulticoloritem_->setQPens( qpens );
}



// uiRectItem
uiRectItem::uiRectItem()
    : uiGraphicsItem(mkQtObj())
{}


uiRectItem::uiRectItem( QGraphicsRectItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qrectitem_(qtobj)
{}


uiRectItem::uiRectItem( int x, int y, int width, int height )
    : uiGraphicsItem(mkQtObj())
{
    setRect( x, y, width, height );
}


uiRectItem::~uiRectItem()
{
}


QGraphicsItem* uiRectItem::mkQtObj()
{
    qrectitem_ = new QGraphicsRectItem();
    return qrectitem_;
}


void uiRectItem::setRect( int x, int y, int width, int height )
{
    qrectitem_->setRect( 0, 0, width, height );
    qrectitem_->setPos( x, y );
}


// uiTextItem
uiTextItem::uiTextItem()
    : uiGraphicsItem( mkODObj() )
{
    setAlignment( OD::Alignment(OD::Alignment::Left,OD::Alignment::Top) );
}


uiTextItem::uiTextItem( const uiString& txt, const OD::Alignment& al )
    : uiGraphicsItem(mkODObj())
{
    setText( txt );
    setAlignment( al );
}


uiTextItem::uiTextItem( const uiPoint& pos, const uiString& txt,
			const OD::Alignment& al )
    : uiGraphicsItem(mkODObj())
{
    setText( txt );
    setPos( pos );
    setAlignment( al );
}


uiTextItem::~uiTextItem()
{
}


ODGraphicsTextItem* uiTextItem::mkODObj()
{
    qtextitem_ = new ODGraphicsTextItem();
    return qtextitem_;
}


#define mExtraSpace 10

const uiString uiTextItem::getText() const
{ return text_; }


uiSize uiTextItem::getTextSize() const
{
    QFontMetrics qfm( qtextitem_->getFont() );
    // Extra space is added to avoid clipping on some platforms and the value is
    // arbitrarily chosen.
    return uiSize( qfm.width(text_.getOriginalString())+mExtraSpace,
		   qfm.height()+mExtraSpace );
}


void uiTextItem::setText( const uiString& txt )
{
    text_ = txt;
    qtextitem_->setText( text_.getQString() );
}


void uiTextItem::translateText()
{
    uiGraphicsItem::translateText();
    qtextitem_->setText( text_.getQString() );
}


void uiTextItem::setFont( const uiFont& font )
{
    qtextitem_->setFont( font.qFont() );
}


void uiTextItem::setFontData( const FontData& fd )
{
    QFont font = qtextitem_->getFont();
    uiFont::setFontData( font, fd );
    qtextitem_->setFont( font );
}


void uiTextItem::setAlignment( const OD::Alignment& al )
{

    switch ( al.hPos() )
    {
	case OD::Alignment::Right:
	    qtextitem_->setHAlignment( Qt::AlignRight );
	    break;
	case OD::Alignment::HCenter:
	    qtextitem_->setHAlignment( Qt::AlignHCenter );
	    break;
	case OD::Alignment::Left:
	    qtextitem_->setHAlignment( Qt::AlignLeft );
	    break;
    }

    switch ( al.vPos() )
    {
	case OD::Alignment::Bottom:
	    qtextitem_->setVAlignment( Qt::AlignBottom );
	    break;
	case OD::Alignment::VCenter:
	    qtextitem_->setVAlignment( Qt::AlignVCenter );
	    break;
	case OD::Alignment::Top:
	    qtextitem_->setVAlignment( Qt::AlignTop );
	    break;
    }
}


void uiTextItem::stPos( float x, float y )
{
    qtextitem_->setPos( x, y );
}


void uiTextItem::setTextColor( const Color& col )
{
    qtextitem_->setPen( QPen(QColor(col.r(),col.g(), col.b())) );
}


// uiAdvancedTextItem
uiAdvancedTextItem::uiAdvancedTextItem( bool centered )
    : uiGraphicsItem(0)
    , al_(OD::Alignment::Left,OD::Alignment::Top)
    , textiscentered_(centered)
{
    qgraphicsitem_ = mkQtObj();
}


uiAdvancedTextItem::uiAdvancedTextItem( const uiString& txt,
					const OD::Alignment& al,
					bool centered )
    : uiGraphicsItem(0)
    , al_(al)
    , textiscentered_(centered)
{
    qgraphicsitem_ = mkQtObj();
    setPlainText( txt );
}


uiAdvancedTextItem::~uiAdvancedTextItem()
{
}


OD::Alignment uiAdvancedTextItem::getAlignment() const
{
    return al_;
}


Color uiAdvancedTextItem::getDefaultTextColor() const
{
    QColor qcol = qtextitem_->defaultTextColor();
    return Color( qcol.red(), qcol.green(), qcol.blue(), 255-qcol.alpha() );
}


uiFont& uiAdvancedTextItem::getFont() const
{
    uiFontList fontlist;
    QFont qfont = qtextitem_->font();
    return fontlist.getFromQfnt( &qfont );
}


uiString uiAdvancedTextItem::getPlainText() const
{
    QString str = qtextitem_->toPlainText();
    uiString ret;
    ret.setFrom( str );
    return ret;
}


float uiAdvancedTextItem::getTextWidth() const
{ return qtextitem_->textWidth(); }


void uiAdvancedTextItem::setAlignment( const OD::Alignment& al )
{
    al_ = al;
}


void uiAdvancedTextItem::setDefaultTextColor( const Color& col )
{
    QColor qcol( col.r(), col.g(), col.b(), 255-col.t() );
    qtextitem_->setDefaultTextColor( qcol );
}


void uiAdvancedTextItem::setFont( const FontData& fd )
{
    QFont qfont;
    uiFont::setFontData( qfont, fd );
    qtextitem_->setFont( qfont );
}


void uiAdvancedTextItem::setPlainText( const uiString& txt )
{
    qtextitem_->setPlainText( txt.getQString() );
}


void uiAdvancedTextItem::setTextWidth( float width )
{ qtextitem_->setTextWidth( width ); }



void uiAdvancedTextItem::setTextIteraction( bool yn )
{
    if ( yn )
	qtextitem_->setTextInteractionFlags( Qt::TextEditorInteraction );
    else
	qtextitem_->setTextInteractionFlags( Qt::NoTextInteraction );
}


QGraphicsItem* uiAdvancedTextItem::mkQtObj()
{
    qtextitem_ = new ODGraphicsAdvancedTextItem( textiscentered_ );
    return qtextitem_;
}


void uiAdvancedTextItem::stPos( float x, float y )
{
    if ( !isItemIgnoresTransformationsEnabled() )
    {
	QRectF boundrec = qtextitem_->boundingRect();
	switch( al_.hPos() )
	{
	case OD::Alignment::Left:
	    boundrec.translate( 0., 0. );
	    break;
	case OD::Alignment::HCenter:
	    boundrec.translate( -boundrec.width()/2., 0. );
	    break;
	case OD::Alignment::Right:
	    boundrec.translate( -boundrec.width(), 0. );
	    break;
	}

	switch( al_.vPos() )
	{
	case OD::Alignment::Top:
	    boundrec.translate( 0., 0. );
	    break;
	case OD::Alignment::VCenter:
	    boundrec.translate( 0., -boundrec.height()/2. );
	    break;
	case OD::Alignment::Bottom:
	    boundrec.translate( 0., -boundrec.height() );
	    break;
	}

	const QPointF p00 = qtextitem_->mapToParent( QPointF(0,0) );
	const QPointF d01 = qtextitem_->mapToParent( QPointF(0,1) )-p00;
	const QPointF d10 = qtextitem_->mapToParent( QPointF(1,0) )-p00;

	const float xdist = Math::Sqrt(d10.x()*d10.x()+d10.y()*d10.y() );
	const float ydist = Math::Sqrt(d01.x()*d01.x()+d01.y()*d01.y() );

	const float xlin = x+mCast(float,boundrec.left())*xdist;
	const float ylin = y+mCast(float,boundrec.top())*ydist;
	uiGraphicsItem::stPos( xlin, ylin );
    }
    else
	uiGraphicsItem::stPos( x, y );
}


// uiMarkerItem
uiMarkerItem::uiMarkerItem( bool fill )
    : uiGraphicsItem( mkQtObj() )
{
    setFill( fill );
}


uiMarkerItem::uiMarkerItem( const uiPoint& pos, const OD::MarkerStyle2D& mstyle,
			    bool fill )
    : uiGraphicsItem( mkQtObj() )
{
    setPos( pos );
    setMarkerStyle( mstyle );
    setFill( fill );
}


uiMarkerItem::uiMarkerItem( const OD::MarkerStyle2D& mstyle, bool fill )
    : uiGraphicsItem( mkQtObj() )
{
    setMarkerStyle( mstyle );
    if ( fill )
	setFillColor( mstyle.color_ );
}


QGraphicsItem* uiMarkerItem::mkQtObj()
{
    qmarkeritem_ = new ODGraphicsMarkerItem();
    return qmarkeritem_;
}


uiMarkerItem::~uiMarkerItem()
{
}


void uiMarkerItem::setMarkerStyle( const OD::MarkerStyle2D& mstyle )
{
    qmarkeritem_->setMarkerStyle( mstyle );
}


void uiMarkerItem::setFill( bool fill )
{
    qmarkeritem_->setFill( fill );
}



void uiMarkerItem::setFillColor( const Color& col, bool )
{
    qmarkeritem_->setFill( true );
    qmarkeritem_->setFillColor( col );
}


// uiPointItem
uiPointItem::uiPointItem( const uiPoint& pos )
    : uiGraphicsItem(mkQtObj())
{
    setPos( pos );
}


uiPointItem::uiPointItem()
    : uiGraphicsItem(mkQtObj())
{}


uiPointItem::~uiPointItem()
{
}


QGraphicsItem* uiPointItem::mkQtObj()
{
    qpointitem_ = new ODGraphicsPointItem();
    return qpointitem_;
}


// uiArrowItem
uiArrowItem::uiArrowItem()
    : uiGraphicsItem(mkQtObj())
{}


uiArrowItem::uiArrowItem( const uiPoint& tail, const uiPoint& head,
			  const OD::ArrowStyle& style )
    : uiGraphicsItem(mkQtObj())
{
    tailpos_ = tail;
    headpos_ = head;
    setArrowStyle( style );
    update();
}


uiArrowItem::~uiArrowItem()
{
}


QGraphicsItem* uiArrowItem::mkQtObj()
{
    qarrowitem_ = new ODGraphicsArrowItem();
    return qarrowitem_;
}


void uiArrowItem::setHeadPos( const uiPoint& pt )
{
    headpos_ = pt;
    update();
}


void uiArrowItem::setTailPos( const uiPoint& pt )
{
    tailpos_ = pt;
    update();
}


void uiArrowItem::setTailHeadPos( const uiPoint& tail, const uiPoint& head )
{
    tailpos_ = tail;
    headpos_ = head;
    update();
}


void uiArrowItem::setArrowStyle( const OD::ArrowStyle& arrowstyle )
{
    qarrowitem_->setArrowStyle( arrowstyle );
}


void uiArrowItem::setArrowSize( int arrowsz )
{
    qarrowitem_->setArrowSize( arrowsz );
}


void uiArrowItem::update()
{
    qarrowitem_->resetTransform();

    float diffx = headpos_.x_-tailpos_.x_;
    float diffy = headpos_.y_-tailpos_.y_;
    const float arrsz = Math::Sqrt( diffx*diffx + diffy*diffy );
    setArrowSize( mNINT32(arrsz) );
    setPos( headpos_ );
    const uiPoint relvec( mNINT32(diffx), mNINT32(diffy) );
    const float ang = Math::Atan2( (float)relvec.y_, (float)relvec.x_);
    setRotation( Math::toDegrees(ang) );
}


// uiCurvedItem
uiCurvedItem::uiCurvedItem( const uiPoint& pt )
    : uiGraphicsItem(mkQtObj())
{
    qppath_->moveTo( QPointF(pt.x_,pt.y_) );
}


uiCurvedItem::uiCurvedItem( const Geom::Point2D<float>& pt )
    : uiGraphicsItem(mkQtObj())
{
    qppath_->moveTo( QPointF(pt.x_,pt.y_) );
    qpathitem_->setPath( *qppath_ );
}


uiCurvedItem::~uiCurvedItem()
{
    delete qppath_;
}


void uiCurvedItem::drawTo( const uiPoint& pt )
{
    drawTo( Geom::Point2D<float>(pt.x_,pt.y_) );
}


void uiCurvedItem::drawTo( const Geom::Point2D<float>& pt )
{
    qppath_->lineTo( QPointF(pt.x_,pt.y_) );
    qpathitem_->setPath( *qppath_ );
}


void uiCurvedItem::drawTo( const ArcSpec& as )
{
    Interval<float> angs( Math::toDegrees(as.angles_.start),
			  Math::toDegrees(as.angles_.stop) );
    QRectF qr( as.center_.x_ - as.radius_, as.center_.y_ - as.radius_,
	       2*as.radius_, 2*as.radius_ );
    qppath_->arcTo( qr, angs.start, angs.stop - angs.start );
    qpathitem_->setPath( *qppath_ );
}


void uiCurvedItem::drawTo( const SplineSpec& ss )
{
    if ( ss.cubic_ )
	qppath_->cubicTo( QPointF(ss.cp1_.x_,ss.cp1_.y_),
			  QPointF(ss.cp2_.x_,ss.cp2_.y_),
			  QPointF(ss.end_.x_,ss.end_.y_) );
    else
	qppath_->quadTo( QPointF(ss.cp1_.x_,ss.cp1_.y_),
			 QPointF(ss.end_.x_,ss.end_.y_) );
    qpathitem_->setPath( *qppath_ );
}


void uiCurvedItem::closeCurve()
{
    qppath_->closeSubpath();
    qpathitem_->setPath( *qppath_ );
}


QGraphicsItem* uiCurvedItem::mkQtObj()
{
    qppath_ = new QPainterPath();
    qpathitem_ = new QGraphicsPathItem();
    return qpathitem_;
}
