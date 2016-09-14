#ifndef uifreqtaper_h
#define uifreqtaper_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Nov 2009
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "uifunctiondisplay.h"
#include "uiwindowfunctionsel.h"
#include "uibutton.h"
#include "survinfo.h"
#include "arrayndalgo.h"
#include "uistring.h"

class uiGenInput;
class uiFuncTaperDisp;
class uiSliceSelDlg;
class uiSlider;
class uiFreqTaperGrp;
class TrcKeyZSampling;


mStruct(uiSeis) FreqTaperSetup
{
		    FreqTaperSetup()
			: hasmin_(false)
			, hasmax_(true)
			, allfreqssetable_(false)
			{}

    DBKey		dbkey_;
    bool		hasmin_;
    bool		hasmax_;
    Interval<float>	minfreqrg_;
    Interval<float>	maxfreqrg_;
    bool		allfreqssetable_;
};


mStruct(uiSeis) TaperData
{
		    TaperData()
			: window_(0)
			, paramval_(1)
			{}

    Interval<float>	rg_;
    Interval<float>	refrg_;

    ArrayNDWindow*	window_;
    int			winsz_;
    float		paramval_;
    float		slope_;
};


mExpClass(uiSeis) uiFuncTaperDisp : public uiFunctionDisplay
{ mODTextTranslationClass(uiFuncTaperDisp);
public:

    struct Setup : public uiFunctionDisplay::Setup
    {
			Setup()
			    : is2sided_(false)
			    , datasz_((int)(0.5/SI().zStep()))
			    , logscale_(false)
			    {
				xaxcaption_ = uiStrings::phrJoinStrings(
					  uiStrings::sFrequency(), tr("(Hz)"));
				yaxcaption_ = tr("Gain Factor (dB)");
				noxgridline_ = true;
				noygridline_ = true;
				ywidth_ = 2;
				ycol_.set(200,0,0);
				y2col_.set(0,0,220);
			    }

	mDefSetupMemb(int,datasz);
	mDefSetupMemb(uiString,xaxcaption);
	mDefSetupMemb(uiString,yaxcaption);
	mDefSetupMemb(Interval<float>,leftrg)
	mDefSetupMemb(Interval<float>,rightrg)
	mDefSetupMemb(bool,is2sided);
	mDefSetupMemb(bool,logscale);
    };

			uiFuncTaperDisp(uiParent*,const Setup&);
			~uiFuncTaperDisp();

    void		setWindows(float,float rightvar=0);
    void		setFunction(Array1DImpl<float>&,Interval<float>);

    ArrayNDWindow*	window() const { return window_; }

    float*		getWinValues() const
			{ return window_ ? window_->getValues() : 0; }
    float*		getFuncValues() const
			{ return funcvals_ ? funcvals_->getData() : 0; }

    void		adaptFreqRangesToDataSize(bool,bool);
    void		taperChged(CallBacker*);

    TaperData&		leftTaperData() { return leftd_; }
    TaperData&		rightTaperData() { return rightd_; }

    Notifier<uiFuncTaperDisp> taperChanged;

protected:

    TaperData		leftd_;
    TaperData		rightd_;

    ArrayNDWindow*	window_;

    Array1DImpl<float>* funcvals_;
    Array1DImpl<float>* orgfuncvals_;
    Interval<float>	funcdisprg_;

    bool		is2sided_;
    bool		logscale_;
    int			datasz_;
    int			orgdatasz_;
};



mExpClass(uiSeis) uiFreqTaperGrp : public uiGroup
{ mODTextTranslationClass(uiFreqTaperGrp);

public:
			uiFreqTaperGrp(uiParent*,
				       const FreqTaperSetup&,
				       uiFuncTaperDisp*);
			~uiFreqTaperGrp(){};


    void		setFreqRange(Interval<float>);
    Interval<float>	getFreqRange() const;
    void		taperChged(CallBacker*);

protected :

    TaperData		td1_;
    TaperData		td2_;

    uiGenInput*		varinpfld_;
    uiGenInput*		freqinpfld_;
    uiGenInput*		inffreqfld_;
    uiGenInput*		supfreqfld_;
    uiSlider*		sliderfld_;
    uiFuncTaperDisp*    drawer_;

    bool		hasmin_;
    bool		hasmax_;
    bool		isminactive_;
    int			datasz_;
    bool		allfreqssetable_;

    void		setSlopeFromFreq();
    void		setPercentsFromFreq();
    void		setFreqFromSlope(float);

    void		freqChoiceChged(CallBacker*);
    void		freqChanged(CallBacker*);
    void		putToScreen(CallBacker*);
    void		sliderChanged(CallBacker*);
    void		slopeChanged(CallBacker*);
};



mExpClass(uiSeis) uiFreqTaperDlg : public uiDialog
{ mODTextTranslationClass(uiFreqTaperDlg);
public:

			uiFreqTaperDlg(uiParent*,const FreqTaperSetup&);
			~uiFreqTaperDlg();

    Interval<float>	getFreqRange() const
			{ return tapergrp_->getFreqRange(); }

protected:

    uiFreqTaperGrp*	tapergrp_;
    uiFuncTaperDisp*    drawer_;
    Array1DImpl<float>* funcvals_;

    DBKey		dbkey_;
    uiPushButton*	previewfld_;
    uiSliceSelDlg*	posdlg_;
    TrcKeyZSampling*	tkzs_;

    void		previewPushed(CallBacker*);
};



mExpClass(uiSeis) uiFreqTaperSel : public uiWindowFunctionSel
{ mODTextTranslationClass(uiFreqTaperSel);
public:
			uiFreqTaperSel(uiParent*,const Setup&,
						const FreqTaperSetup&);

    Interval<float>	freqValues() const;

    void		setDBKey( const DBKey& dbkey )
			{ freqsetup_.dbkey_ = dbkey; }
    void		setIsMinMaxFreq(bool,bool);
    void		setInputFreqValue(float,int);
    void		setRefFreqs(Interval<float>);

protected :

    uiFreqTaperDlg*	freqtaperdlg_;
    FreqTaperSetup	freqsetup_;

    void		winfuncseldlgCB(CallBacker*);
    void		windowClosed(CallBacker*);
    void		setSelFreqs(CallBacker*);
};

#endif
