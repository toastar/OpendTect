#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2016
-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class uiFileInput;

mExpClass(uiEarthModel) uiBulkFaultExport : public uiDialog
{ mODTextTranslationClass(uiBulkFaultExport)
public:
			uiBulkFaultExport(uiParent*);
			~uiBulkFaultExport();

protected:

    bool		acceptOK();

    uiFileInput*	outfld_;
};
