#ifndef uifingerprintattrib_h
#define uifingerprintattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id: uifingerprintattrib.h,v 1.7 2006-05-19 14:34:02 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "uidialog.h"
#include "position.h"
#include "ranges.h"
#include "runstat.h"

class CtxtIOObj;
class uiAttrSel;
class uiTable;
class uiLabel;
class uiStepOutSel;
class uiIOObjSel;
class uiGenInput;
class uiRadioButton;
class uiToolButton;
class uiButtonGroup;
class uiSpinBox;
class BinIDValueSet;
class BufferStringSet;
namespace Attrib { class EngineMan; }

class uiFPAdvancedDlg;
class calcFingParsObject;

/*! \brief FingerPrint Attribute description editor */

class uiFingerPrintAttrib : public uiAttrDescEd
{
public:

			uiFingerPrintAttrib(uiParent*);
			~uiFingerPrintAttrib();

    void		set2D(bool);

protected:

    uiTable*            table_;
    uiButtonGroup*      refgrp_;
    uiRadioButton*	refposbut_;
    uiRadioButton*	picksetbut_;
    uiToolButton*	getposbut_;
    uiGenInput*		statsfld_;
    uiGenInput*		refposzfld_;
    uiStepOutSel*	refposfld_;
    uiIOObjSel*		picksetfld_;
    uiLabel*		manlbl_;
   
    CtxtIOObj&		ctio_;
    ObjectSet<uiAttrSel> attribflds_;

    uiFPAdvancedDlg*	advanceddlg_;
    calcFingParsObject*	calcobj_;

    void		insertRowCB(CallBacker*);
    void		deleteRowCB(CallBacker*);
    void		initTable(int);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    BinIDValueSet*	createValuesBinIDSet(BufferString&) const;
    
    void                getPosPush(CallBacker*);
    void                calcPush(CallBacker*);
    void                getAdvancedPush(CallBacker*);
    void		refSel(CallBacker*);
};


class calcFingParsObject
{
public:
    			calcFingParsObject(uiParent*);
			~calcFingParsObject();
    void		setUserRefList( BufferStringSet* refset )
    							{ reflist_ = refset; }
    void		setDescSet( DescSet* ds )	{ attrset_ = ds; }
    void                setWeights( TypeSet<int> wgs )  { weights_ = wgs; }
    void		setRanges(TypeSet<Interval<float> > rg) {ranges_ = rg;}
    void		setValues( TypeSet<float> vals ){ values_ = vals; }
    void		setRgRefPick(const char* pickid){ rgpickset_ = pickid; }
    void		setRgRefType( int type )	{ rgreftype_ = type; }
    
    TypeSet<int>        getWeights() const              { return weights_; }
    TypeSet<float>	getValues() const		{ return values_; }
    TypeSet< Interval<float> >	getRanges() const	{ return ranges_; }
    BufferString	getRgRefPick() const		{ return rgpickset_; }
    int			getRgRefType() const		{ return rgreftype_; }

    void		clearValues()			{ values_.erase(); }
    void		clearRanges()			{ ranges_.erase(); }
    void		clearWeights()			{ weights_.erase(); }
    
    BinIDValueSet*      createRangesBinIDSet() const;
    void		setValRgSet(BinIDValueSet*,bool);
    void                computeValsAndRanges();
protected:
    
    EngineMan*          createEngineMan();
    void                extractAndSaveValsAndRanges();
    void		fillInStats(BinIDValueSet*,
	    			  ObjectSet< RunningStatistics<float> >&) const;

    BufferStringSet*	reflist_;
    DescSet*		attrset_;
    int			statstype_;
    TypeSet<float>	values_;
    TypeSet<int>	weights_;
    TypeSet< Interval<float> > ranges_;
    ObjectSet<BinIDValueSet> posset_;
    uiParent*		parent_;
    BufferString	rgpickset_;
    int			rgreftype_;

};

#endif
