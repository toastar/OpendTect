#ifndef uiiosel_h
#define uiiosel_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiiosel.h,v 1.11 2001-07-19 22:16:23 bert Exp $
________________________________________________________________________

-*/

#include <uigroup.h>
class UserIDSet;
class uiLabeledComboBox;
class uiPushButton;
class IOPar;


/*! \brief UI element for selection of data objects */

class uiIOSelect : public uiGroup
{
public:

			uiIOSelect(uiParent*,const CallBack& do_selection,
				   const char* txt,
				   bool withclear=false);
			~uiIOSelect();

    const char*		getInput() const;
    const char*		getKey() const;
    void		setInput(const char* key);
			//!< Will fetch user name using userNameFromKey

    int			nrItems() const;
    int			getCurrentItem() const;
    void		setCurrentItem(int);
    const char*		getItem(int) const;

    void		addSpecialItem(const char* key,const char* value=0);
			//!< If value is null, add value same as key

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    void		clear()			{ setCurrentItem( 0 ); }
    virtual void	processInput()		{}

    Notifier<uiIOSelect> selectiondone;

protected:

    CallBack		doselcb_;
    ObjectSet<BufferString>	entries_;
    bool		withclear_;
    IOPar&		specialitems;

    uiLabeledComboBox*	inp_;
    uiPushButton*	selbut_;

    void		doSel(CallBacker*);
    void		selDone(CallBacker*);
			//!< Subclass must call it - base class can't
			//!< determine whether a selection was successful.

    virtual const char*	userNameFromKey( const char* s ) const	{ return s; }
			//!< If 0 returned, then if possible,
			//!< that entry is not entered in the entries_.

    void		updateFromEntries();
    bool		haveEntry(const char*) const;

    virtual void	objSel()		{}
			//!< notification when user selects from combo

    virtual void	finalise_();

};


class uiIOFileSelect : public uiIOSelect
{
public:
			uiIOFileSelect(uiParent*,const char* txt,
					bool for_read,
					const char* inp=0,
					bool withclear=false);

    void		setFilter( const char* f )	{ filter = f; }

protected:

    void		doFileSel(CallBacker*);
    bool		forread;
    BufferString	filter;

};


#endif
