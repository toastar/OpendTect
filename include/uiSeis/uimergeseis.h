#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "multiid.h"

class CtxtIOObj;
class IOObj;
class uiGenInput;
class uiListBox;
class uiSeisFmtScale;
class uiSeisSel;


mExpClass(uiSeis) uiMergeSeis : public uiDialog
{
public:
                        uiMergeSeis(uiParent*);
			~uiMergeSeis();

protected:

    uiListBox*		inpfld_;
    uiGenInput*		stackfld_;
    uiSeisFmtScale*	scfmtfld_;
    uiSeisSel*		outfld_;

    ObjectSet<MultiID>	ioobjids_;
    CtxtIOObj&		ctio_;

    virtual bool	acceptOK(CallBacker*);
    void		selChangeCB(CallBacker*);
    bool		getInput(ObjectSet<IOPar>&,IOPar&);
};

#endif

