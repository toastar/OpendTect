/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uimarkerstyledlg.cc,v 1.2 2007-02-05 18:19:48 cvsbert Exp $
________________________________________________________________________

-*/

#include "uimarkerstyledlg.h"

#include "uicolor.h"
#include "uigeninput.h"
#include "uislider.h"
#include "color.h"
#include "draw.h"


uiMarkerStyleDlg::uiMarkerStyleDlg( uiParent* p, const char* title )
	: uiDialog(p,
		   uiDialog::Setup(title,"Specify marker style properties",0)
		   .canceltext(""))
{
    typefld = new uiGenInput( this, "Type", 
	    		      StringListInpSpec(MarkerStyle3D::TypeNames) );
    typefld->valuechanged.notify( mCB(this,uiMarkerStyleDlg,typeSel) );

    sliderfld = new uiSliderExtra( this, 
	    			   uiSliderExtra::Setup("Size").withedit(true));
    sliderfld->sldr()->setMinValue( 1 );
    sliderfld->sldr()->setMaxValue( 15 );
    sliderfld->sldr()->valueChanged.notify(
		mCB(this,uiMarkerStyleDlg,sliderMove));
    sliderfld->attach( alignedBelow, typefld );

    colselfld = new uiColorInput( this, Color::White, "Color" );
    colselfld->attach( alignedBelow, sliderfld );
    colselfld->colorchanged.notify( mCB(this,uiMarkerStyleDlg,colSel) );

    finaliseStart.notify( mCB(this,uiMarkerStyleDlg,doFinalise) );
}


bool uiMarkerStyleDlg::acceptOK( CallBacker* )
{
    sliderfld->processInput();
    sliderMove(0);
    return true;
}
