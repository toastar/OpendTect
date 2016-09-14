#ifndef uigoogleexpwells_h
#define uigoogleexpwells_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2009
-*/

#include "uigoogleexpdlg.h"
#include "dbkey.h"
class uiListBox;


mClass(uiGoogleIO) uiGoogleExportWells : public uiDialog
{ mODTextTranslationClass(uiGoogleExportWells);
public:

			uiGoogleExportWells(uiParent*);
			~uiGoogleExportWells();

protected:

    DBKeySet		wellids_;

    uiListBox*		selfld_;

    void		initWin(CallBacker*);


			mDecluiGoogleExpStd;
};


#endif
