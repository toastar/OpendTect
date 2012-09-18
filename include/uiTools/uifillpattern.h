#ifndef uifillpattern_h
#define uifillpattern_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2012
 RCS:           $Id: uifillpattern.h,v 1.1 2012/09/17 14:40:52 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "draw.h"
class uiComboBox;
class uiRectItem;



/*! \brief one-line element for fill pattern selection. No text label. */


mClass uiFillPattern : public uiGroup
{
public:

    				uiFillPattern(uiParent*);

    FillPattern			get() const;
    void			set(const FillPattern&);

    Notifier<uiFillPattern> 	patternChanged;

protected:

    uiComboBox*			typefld_;
    uiComboBox*			optfld_;
    uiRectItem*			patrect_;

    void			reDrawPattern();
    void			setOptNms();

    void			selChg(CallBacker*);

};


#endif
