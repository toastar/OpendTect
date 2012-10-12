/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "genc.h"

#include "uidialog.h"
#include "uilabel.h"

#ifdef __msvc__
#include "winmain.h"
#endif

#include <QApplication>


int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    
    QApplication app( argc, argv );
    
    uiDialog dlg( uiDialog::Setup("Hello", "World", "NoHelpID" ));
    
    uiLabel* label1 = new uiLabel("Label text 1");
    dlg.addChild( label1 );
    
    uiLabel* label2 = new uiLabel("Label text 2");
    dlg.addChild( label2 );
    label2->attach(uiLayout::AlignedBelow, label1 );
    
    dlg.go();
    
    
    return 0;
}
