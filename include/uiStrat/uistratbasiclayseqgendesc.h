#ifndef uistratbasiclayseqgendesc_h
#define uistratbasiclayseqgendesc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
 RCS:           $Id: uistratbasiclayseqgendesc.h,v 1.1 2011-06-01 13:07:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uistratlayseqgendesc.h"
class uiTextItem;
class uiLineItem;
class uiCircleItem;
class uiPolygonItem;
namespace Strat { class LayerGenerator; class SingleLayerGenerator; }


mClass uiBasicLayerSequenceGenDesc : public uiLayerSequenceGenDesc
{
public:

    			uiBasicLayerSequenceGenDesc(uiParent*,
					    Strat::LayerSequenceGenDesc&);

    mDefuiLayerSequenceGenDescFns(uiBasicLayerSequenceGenDesc,"Basic");

    virtual void	getPropertyRefs( PropertyRefSelection& prs ) const
			{ prs = props_; }

protected:

    struct DispUnit
    {
			DispUnit(uiGraphicsScene&,const Strat::LayerGenerator&);
			~DispUnit();

	uiGraphicsScene& scene_;
	uiTextItem*	nm_;
	uiLineItem*	top_;
	uiCircleItem*	lithcol_;
	uiPolygonItem*	poly_;
	int		topy_;
	int		boty_;

	const Strat::SingleLayerGenerator*	gen_;
	bool genmine_;
    };

    ObjectSet<DispUnit>	disps_;
    PropertyRefSelection props_;

    void		rebuildDispUnits();
    void		fillDispUnit(int,float,float&);
    void		insertDispUnit(const Strat::LayerGenerator&,int);
    int			curUnitIdx();
    DispUnit*		curUnit();

};


#endif
