#ifndef uiseisbrowser_h
#define uiseisbrowser_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Sulochana/Satyaki
 Date:          Oct 2007
 RCS:           $Id: uiseisbrowser.h,v 1.18 2009-04-24 10:28:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"
#include "seistype.h"
#include "samplingdata.h"
#include "position.h"
#include "linekey.h"

class CBVSSeisTrcTranslator;
class SeisTrc;
class SeisTrcBuf;
class uiSeisTrcBufViewer;
class SeisTrcBufDataPack;
class uiTable;
class uiGenInput;
class uiSeisBrowserInfoVwr;


mClass uiSeisBrowser : public uiDialog
{
public :

    mClass Setup : public uiDialog::Setup
    {
    public:
    			Setup(const MultiID&,Seis::GeomType);
	mDefSetupMemb(MultiID,id)
	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(BinID,startpos)
	mDefSetupMemb(float,startz)
	mDefSetupMemb(LineKey,linekey)
	mDefSetupMemb(bool,readonly)	//!< def: true

    };
			uiSeisBrowser(uiParent*,const Setup&,bool);
			~uiSeisBrowser();

    bool		isOK() const			{ return tbl_; }
    void		setPos(const BinID&);
    bool		doSetPos( const BinID&, bool );
    void		setZ(float) ;
    void		setCrlWise( bool yn=true )	{ crlwise_ = yn; }
    void 		commitChanges();
    bool 		storeChgdData();
    bool		goTo(const BinID&);
	

    int			stepOut()			{ return stepout_; }
    void		setStepout(int);
    bool		is2D() const;
    bool 		is2d_;

    const BinID&	curBinID() const;
    const float		curZ() const;

protected:

    const Setup		setup_;
    uiSeisBrowserInfoVwr* infovwr_;
    uiSeisTrcBufViewer*	trcbufvwr_;

    SeisTrcBuf&		tbuf_;
    SeisTrcBuf&		tbufchgdtrcs_;
    SeisTrc&		ctrc_;
    CBVSSeisTrcTranslator* tr_;
    CBVSSeisTrcTranslator* tro_;
    CBVSSeisTrcTranslator* tri_;

    bool		crlwise_;
    int			crlwisebutidx_;
    int			showwgglbutidx_;

    int			stepout_;

    uiTable*		tbl_;
    uiToolBar*		uitb_;
    int			compnr_;
    int			nrcomps_;
    int			nrsamples_;
    SamplingData<float>	sd_;

    bool		openData(const Setup&);
    void		createMenuAndToolBar();
    void		createTable();
    void		fillTable();
    void		fillUdf(SeisTrc&);
    void		fillTableColumn(const SeisTrc&,int);
    BinID		getNextBid(const BinID&,int,bool) const;
    void		addTrc(SeisTrcBuf&,const BinID&);
    void		updateWiggleButtonStatus();
    void		setTrcBufViewTitle();
	
    void		goToPush(CallBacker*);
    void		infoPush(CallBacker*);
    void		infoClose(CallBacker*);
    void		rightArrowPush(CallBacker*);
    void		leftArrowPush(CallBacker*);
    void		switchViewTypePush(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		showWigglePush(CallBacker*);
    void		trcbufViewerClosed(CallBacker*);
    void		trcselectionChanged(CallBacker*);
    void		valChgReDraw(CallBacker*);

private:

    SeisTrcBuf&		tbufbefore_;
    SeisTrcBuf&		tbufafter_;
};


#endif
