#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          April 2009
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uiparent.h"
#include "bufstringset.h"
#include "trckeyzsampling.h"

class uiLabel;
class uiSpinBox;
class uiToolBar;
class uiToolButton;

/*! \brief Toolbar for setting slice position _ base class */

mExpClass(uiTools) uiSlicePos : public CallBacker
{ mODTextTranslationClass(uiSlicePos);
public:
			uiSlicePos(uiParent*);
			~uiSlicePos();

    typedef OD::SliceType SliceDir;
			mDeclareEnumUtils(SliceDir);

    uiToolBar*		getToolBar() const		{ return toolbar_; }
    TrcKeyZSampling	getTrcKeyZSampling() const	{ return curcs_; }

    void		setLabels(const uiString& inl,const uiString& crl,
							    const uiString& z);

    Notifier<uiSlicePos> positionChg;

protected:

    uiToolBar*		toolbar_;
    uiLabel*		label_;
    uiSpinBox*		sliceposbox_;
    uiSpinBox*		slicestepbox_;
    uiToolButton*	prevbut_;
    uiToolButton*	nextbut_;
    int			laststeps_[3];
    int			zfactor_;
    TrcKeyZSampling	curcs_;
    uiStringSet		boxlabels_;

    void		setBoxLabel(SliceDir);
    virtual void	setBoxRanges()			=0;
    virtual void	setPosBoxValue()		=0;
    virtual void	setStepBoxValue()		=0;
    virtual void	slicePosChg(CallBacker*)	=0;
    virtual void	sliceStepChg(CallBacker*)	=0;
    void		shortcutsChg(CallBacker*);
    void		updatePos(CallBacker*);
    void		initSteps(CallBacker* cb=0);
    void		slicePosChanged(SliceDir,const TrcKeyZSampling&);
    void		sliceStepChanged(SliceDir);
    void		setBoxRg(SliceDir,const TrcKeyZSampling&);
    void		setPosBoxVal(SliceDir,const TrcKeyZSampling&);

    void		prevCB(CallBacker*);
    void		nextCB(CallBacker*);
};
