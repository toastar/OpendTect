#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "bufstringset.h"

class uiAttrDescEd;
class uiParent;

/*! \brief Factory for attrib editors.  */


typedef uiAttrDescEd* (*uiAttrDescEdCreateFunc)(uiParent*,bool);

mExpClass(uiAttributes) uiAttributeFactory
{
public:
    virtual		~uiAttributeFactory();

    int			add(const char* displaynm,const char* attrnm,
			    const char* grpnm,uiAttrDescEdCreateFunc,
			    int,int,bool);
    uiAttrDescEd*	create(uiParent*,const char* nm, bool,
	    		       bool dispnm=true) const;

    int			size() const	{ return entries_.size(); }
    const char*		getAttribName( int idx ) const
					{ return entries_[idx]->attrnm_; }
    const char*		getDisplayName( int idx ) const
					{ return entries_[idx]->dispnm_; }
    const char*		getGroupName( int idx ) const
					{ return entries_[idx]->grpnm_; }
    int			domainType( int idx ) const
					{ return entries_[idx]->domtyp_; }
    				//!< Is, in fact, uiAttrDescEd::DomainType
    				//!< Not used to avoid dependency
    int			dimensionType( int idx ) const
					{ return entries_[idx]->dimtyp_; }
    				//!< Is, in fact, uiAttrDescEd::DimensionType
    				//!< Not used to avoid dependency
    bool		isSyntheticSupported(int idx) const
				    { return entries_[idx]->supportsynthetic_; }

    const char*		dispNameOf(const char*) const;
    const char*		attrNameOf(const char*) const;
    bool		isPresent(const char*,bool dispnm) const;

protected:

    struct Entry
    {
				Entry(	const char* dn, const char* an,
					const char* gn,
					uiAttrDescEdCreateFunc fn,
					int dt, int dimtyp, bool supsynth )
				    : dispnm_(dn)
				    , attrnm_(an)
				    , grpnm_(gn)
				    , domtyp_(dt)
				    , dimtyp_(dimtyp)
				    , supportsynthetic_(supsynth)
				    , crfn_(fn)		{}

	BufferString		dispnm_;
	BufferString		attrnm_;
	BufferString		grpnm_;
	int			domtyp_;
	int			dimtyp_;
	bool			supportsynthetic_;
	uiAttrDescEdCreateFunc	crfn_;
    };

    ObjectSet<Entry>	entries_;

    Entry*		getEntry(const char*,bool) const;

    friend mGlobal(uiAttributes) uiAttributeFactory&	uiAF();
    void			fillStd();
};

mGlobal(uiAttributes) uiAttributeFactory& uiAF();
