#ifndef geometry_h
#define geometry_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: geometry.h,v 1.16 2004-04-15 13:12:24 macman Exp $
________________________________________________________________________

-*/

#include <gendefs.h>

namespace Geom
{

/*!\brief basic point class */
template <class T>
class Point
{
public:
			Point ( T xx = 0, T yy = 0 ) { x_ = xx; y_ = yy; }

    inline T		x() const	{ return x_; }
    inline T		y() const	{ return y_; }
    inline void		setX( T xx )	{ x_ = xx ; }  
    inline void		setY( T yy )	{ y_ = yy ; }  
    inline void		setXY( T xx, T yy ) { x_ = xx ; y_ = yy; }  
    inline Point<T>&	zero()		{ x_ = y_ = 0; return *this; }
    inline Point<T>&	operator -()	{ x_ = -x_; y_ = -y_; return *this; }

    inline bool		operator ==( const Point<T>& p ) const
			{ return p.x_ == x_ && p.y_ == y_; }
    inline bool		operator !=( const Point<T>& p ) const
			{ return p.x_ != x_ || p.y_ != y_; }
    inline Point<T>&	operator +=( const Point<T>& p )
			{ x_ += p.x_; y_ += p.y_; return *this; }
    inline Point<T>&	operator -=( const Point<T>& p )
			{ x_ -= p.x_; y_ -= p.y_; return *this; }
    
protected:

    T 	x_;
    T 	y_;

};


/*!\brief basic 2D sizes (width/height) class */

template <class T>
class Size2D
{
public:
			Size2D( T w = 0 , T h = 0 ) 
				{ width_ = w; height_ = h; }

    inline bool		operator ==( const Size2D<T>& s ) const
			{ return s.width_ == width_ && s.height_ == height_; }
    inline bool		operator !=( const Size2D<T>& s ) const
			{ return s.width_ != width_ || s.height_ != height_; }

    inline T		width() const		{ return width_; }
    inline T		height() const		{ return height_; }
    inline void		setWidth( T val )	{ width_ = val; }
    inline void		setHeight( T val )	{ height_ = val; }
    inline Size2D<T>&	operator +=( T val )
			{ width_ += val; height_ += val; return *this; }
    inline Size2D<T>&	operator -=( T val )
			{ width_ -= val; height_ -= val; return *this; }
    inline Size2D<T>&	operator +=( const Size2D<T>& s )
			{ width_+=s.width_; height_+=s.height_; return *this; }
    inline Size2D<T>&	operator -=( const Size2D<T>& s )
			{
			    width_ -= s.width_; height_ -= s.height_;
			    if ( width_ < 0 ) width_ = -width_;
			    if ( height_ < 0 ) height_ = -height_;
			    return *this;
			}

protected:

    T 	width_;
    T 	height_;

};


/*!\brief basic 2D rectangle class

This class is a bit more complicated than would be expected at first sight.
This is caused by the problem of coordinate system sign. For example, in
user interfaces, top is a lower number than bottom. But foor normal
coordinates, this is (of course) not the case.
*/

template <class T>
class Rect 
{
public:
			Rect ( T l = 0 , T t = 0, T r = 0 , T b = 0 ) 
			: topLeft_( Point<T>(l,t)) 
			, bottomRight_( Point<T>(r,b) ) {}
			Rect ( Point<T> tl, Point<T> br ) 
			: topLeft_( tl ) , bottomRight_( br ) {} 

    inline bool		operator ==( const Rect<T>& r ) const
			{ return   r.topLeft_ == topLeft_
				&& r.bottomRight_ == bottomRight_; }
    inline bool		operator !=( const Rect<T>& p ) const
			{ return   r.topLeft_ != topLeft_
				|| r.bottomRight_ != bottomRight_; }

    inline Point<T>	topLeft() const     { return topLeft_; }
    inline Point<T>	topRight() const    { return Point<T>(top(),right()); }
    inline Point<T>	bottomLeft() const  { return Point<T>(bottom(),left());}
    inline Point<T>	bottomRight() const { return bottomRight_; }
    inline Point<T>	centre() const 		
                        { return Point<T>( (topLeft_.x()+bottomRight_.x())/2,
					   (topLeft_.y()+bottomRight_.y())/2 ); 
                        }
    inline void		setTopLeft( Point<T> tl )	{ topLeft_ = tl; }
    inline void		setBottomRight( Point<T> br )	{ bottomRight_ = br; }
    inline void		setTopRight( Point<T> tr )
			{ topLeft_.setY(tr.y()); bottomRight_.setX(tr.x()); }
    inline void		setBottomLeft( Point<T> tr )
			{ topLeft_.setX(tr.x()); bottomRight_.setY(tr.y()); }

    inline bool		isInside(const Point<T>&) const;
    inline bool		isOutside( const Point<T>& p ) const
			{ return xOutside(p.x()) || yOutside(p.y()); }
    inline bool		isOnSide( const Point<T>& p ) const
			{ return !isInside(p) && !isOutside(p); }
    inline bool		contains( const Point<T>& p ) const
			{ return !isOutside(p); }

    inline bool		contains( const Rect<T>& other ) const
			{
			    return contains(other.topLeft())
				&& contains(other.bottomRight());
			}
    inline bool		isInside( const Rect<T>& other ) const
			{
			    return other.isInside(topLeft())
				&& other.isInside(bottomRight());
			}
    inline void		fitIn(const Rect<T>&);

    inline bool		operator >(const Rect<T>&) const;

    inline T 		width() const
			{ return revX() ? left()-right() : right() - left(); }
    inline T 		height() const
			{ return revY() ? bottom()-top() : top()-bottom(); }
    inline Rect<T>	grownBy(double sidesincreasebyfactor=1) const;

    inline T 		left() const 		{ return topLeft_.x(); }
    inline T 		top() const 		{ return topLeft_.y(); }
    inline T 		right() const 		{ return bottomRight_.x(); }
    inline T 		bottom() const		{ return bottomRight_.y(); }
    inline void 	setLeft( T val )	{ topLeft_.setX( val ); }
    inline void 	setTop( T val )		{ topLeft_.setY( val ); }
    inline void 	setRight( T val )	{ bottomRight_.setX( val ); }
    inline void 	setBottom( T val )	{ bottomRight_.setY( val ); }

    void		checkCorners( bool leftislow=true, bool topislow=true )
			{ 
			    if( leftislow == left() > right() )  swapHor(); 
			    if( topislow  == top()  > bottom() ) swapVer(); 
			}

    inline Size2D<T>	size() const { return Size2D<T>( width(), height() ); }
    inline		operator Size2D<T>() const	{ return size(); }
    inline void 	zero()	{ topLeft_.zero(); bottomRight_.zero(); }

    inline Rect<T>&	operator +=( const Point<T>& p )
			{ topLeft_ += p; bottomRight_ += p; return *this; }
    inline Rect<T>&	operator -=( const Point<T>& p )
			{ topLeft_ -= p; bottomRight_ -= p; return *this; }

protected:

    inline void		swapHor() 
			{ 
			    T t = topLeft_.x(); 
			    topLeft_.setX( bottomRight_.x() );
			    bottomRight_.setX( t );
			}
    inline void		swapVer() 
			{ 
			    T t = topLeft_.y(); 
			    topLeft_.setY( bottomRight_.y() );
			    bottomRight_.setY( t );
			}

    Point<T>		topLeft_;
    Point<T>		bottomRight_;

    inline bool		revX() const		{ return left() > right(); }
    inline bool		revY() const		{ return bottom() > top(); }

    inline bool		xOutside(T) const;
    inline bool		yOutside(T) const;
};


template <class T>
inline bool Rect<T>::isInside( const Point<T>& pt ) const
{
    return pt != topLeft_ && pt != bottomRight_
	&& ( (pt.x() - left() > 0) == (right() - pt.x() > 0) )
	&& ( (pt.y() - bottom() > 0) == (top() - pt.y() > 0) );
}


template <class T>
inline bool Rect<T>::xOutside( T x ) const
{
    return x != left() && x != right() && (x-left() > 0 == x-right() > 0);
}


template <class T>
inline bool Rect<T>::yOutside( T y ) const
{
    return y != bottom() && y != top() && (y-bottom() > 0 == y-top() > 0);
}


template <class T>
inline T widerPos( T x1, T x2, double f )
{ return (T)(x1 + f * (x1 - x2)); }

inline int widerPos( int x1, int x2, double f )
{ return mNINT(x1 + f * (x1 - x2)); }


template <class T>
inline Rect<T> Rect<T>::grownBy( double f ) const
{
    f *= .5;
    return Rect<T>( widerPos(left(),right(),f), widerPos(top(),bottom(),f),
		    widerPos(right(),left(),f), widerPos(bottom(),top(),f) );
}


template <class T>
inline bool Rect<T>::operator >( const Rect<T>& r ) const
{
    Size2D<T> diff( width()-r.width(), height()-r.height() );

    if ( diff.width() > 0 && diff.height() > 0 )
	return true;
    if ( diff.width() < 0 && diff.height() < 0 )
	return false;

    return diff.width() < 0 ? diff.height() < -diff.width()
			    : diff.width() > -diff.height();
}


template <class T>
inline void Rect<T>::fitIn( const Rect<T>& r )
{
    if ( revX() )
    {
	if ( r.left() < left() ) topLeft_.setX(r.left());
	if ( r.right() > right() ) bottomRight_.setX(r.right());
    }
    else
    {
	if ( r.left() > left() ) topLeft_.setX(r.left());
	if ( r.right() < right() ) bottomRight_.setX(r.right());
    }
    if ( revY() )
    {
	if ( r.bottom() < bottom() ) bottomRight_.setY(r.bottom());
	if ( r.top() > top() ) topLeft_.setY(r.top());
    }
    else
    {
	if ( r.bottom() > bottom() ) bottomRight_.setY(r.bottom());
	if ( r.top() < top() ) topLeft_.setY(r.top());
    }
}

}; // namespace Geom
#endif
