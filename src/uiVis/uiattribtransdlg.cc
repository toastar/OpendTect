/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          March 2006
 RCS:           $Id: uiattribtransdlg.cc,v 1.2 2007-02-05 18:19:48 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattribtransdlg.h"

#include "uislider.h"
#include "vissurvobj.h"

uiAttribTransDlg::uiAttribTransDlg( uiParent* p, visSurvey::SurveyObject& so,
				    int attrib )
    : uiDialog( p, uiDialog::Setup("Attribute transperancy",0,0) )
    , so_( so )
    , attrib_( attrib )
    , initaltrans_( so.getAttribTransparency(attrib) )
{
    slider_ = new uiSliderExtra( this, uiSliderExtra::Setup("Transparency"));
    slider_->sldr()->setMinValue( 0 );
    slider_->sldr()->setMaxValue( 255 );
    slider_->sldr()->setStep( 1 );
    slider_->sldr()->setValue( initaltrans_ ); 

    slider_->sldr()->valueChanged.notify( mCB(this,uiAttribTransDlg,changeCB) );
}


void uiAttribTransDlg::changeCB( CallBacker* )
{
    so_.setAttribTransparency( attrib_, slider_->sldr()->getIntValue() );
}


bool uiAttribTransDlg::rejectOK( CallBacker* )
{
    so_.setAttribTransparency( attrib_, initaltrans_ );
    return true;
}

