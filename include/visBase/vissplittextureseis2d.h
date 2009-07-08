#ifndef vissplittextureseis2d_h
#define vissplittextureseis2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		3-8-2008
 RCS:		$Id: vissplittextureseis2d.h,v 1.6 2009-07-08 19:58:57 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "posinfo.h"
#include "visobject.h"

class SoSeparator;
class SoIndexedTriangleStripSet;
class SoTextureCoordinate2;
class SoTextureComposer;

namespace visBase
{

class Coordinates;

/*!used for splitting 2D surface into smaller blocks with triangle strips.
   No matter split texture or not, we always split the shape along horizon with
   size mMaxHorSz. should set path before having the shape. To split texture,
   make sure to set z pixels and texture units. */

mClass SplitTextureSeis2D : public VisualObjectImpl
{
public:
    static SplitTextureSeis2D*	create()
				mCreateDataObj(SplitTextureSeis2D);

    void			setTextureZPixels(int);
    				//!<\note Horizontal size is trcrg.width()+1, 

    void			setPath(const TypeSet<PosInfo::Line2DPos>&);
    				//!<Is assumed to remain in memory

    void			setDisplayedGeometry(const Interval<int>& trcrg,
						    const Interval<float>& zrg);

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans* 			getDisplayTransformation();
    const Coordinates*		getCoordinates() const	{ return coords_; } 
    
protected:
    				~SplitTextureSeis2D();
    void			updateDisplay();
    void			updateHorSplit();
    void			updateSeparator(SoSeparator*,
	    				SoIndexedTriangleStripSet*&,
					SoTextureCoordinate2*&,
					SoTextureComposer*&,bool) const;
    TypeSet<Coord>		path_; 
    Interval<float>		zrg_;
    Interval<int>		trcrg_;
    int				nrzpixels_;
    ObjectSet<TypeSet<int> > 	horblocktrcindices_;

    Coordinates*		coords_;
    ObjectSet<SoSeparator>	separators_;
    TypeSet<int>		trcnrs_;
};

}; // Namespace


#endif
