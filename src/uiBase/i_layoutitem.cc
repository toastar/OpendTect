/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/02/2003
 RCS:           $Id: i_layoutitem.cc,v 1.2 2003-08-13 13:41:27 arend Exp $
________________________________________________________________________

-*/

#include "uilayout.h"
#include "errh.h"

#include "i_layout.h"
#include "i_layoutitem.h"
#include "uiobjbody.h"
#include "uimainwin.h"
/*
#include <qptrlist.h>
#include <qmenubar.h>

#include <stdio.h>
#include <iostream>
#include <limits.h>
*/
#ifdef __debug__
#define MAX_ITER	2000
#else
#define MAX_ITER	10000
#endif


//------------------------------------------------------------------------------

i_LayoutItem::i_LayoutItem( i_LayoutMngr& m, QLayoutItem& itm ) 
    : mngr_( m ), mQLayoutItem_( itm ) 
    , preferred_pos_inited( false ), minimum_pos_inited( false )
    , prefSzDone( false ), hsameas( false ), vsameas( false )
{
    constrList.setAutoDelete(true); 
}

i_LayoutItem::~i_LayoutItem()
{

    delete &mQLayoutItem_;
}


i_uiLayoutItem::~i_uiLayoutItem()
{
    uiObjBody_.loitemDeleted();
}

void i_LayoutItem::invalidate() 
{ 
    mQLayoutItem_.invalidate();
    preferred_pos_inited = false;
}


uiSize i_LayoutItem::actualsize( bool include_border ) const
{ 
    return curpos(setGeom).getsize(); 
}


int i_LayoutItem::stretch( bool hor ) const
{ 
    if ( hor && hsameas || !hor && vsameas ) return 0;

    const uiObjectBody* blo = bodyLayouted();
    return blo ? blo->stretch( hor ) : 0; 
}


void i_LayoutItem::commitGeometrySet( bool store2prefpos )
{
    uiRect mPos = curpos( setGeom );

    if ( store2prefpos ) curpos( preferred ) = mPos;

    if ( objLayouted() ) objLayouted()->triggerSetGeometry( this, mPos );
#ifdef __extensive_debug__
    if ( arend_debug )
    {
	cout << "setting Layout on: ";
	if( objLayouted() ) 
	    cout << objLayouted()->name() << endl;
	else 
	    cout << "Unknown" << endl;

	cout << "l: " << mPos.left() << " t: " << mPos.top();
	cout << " hor: " << mPos.hNrPics() << " ver: " << mPos.vNrPics() << endl;
    }
#endif

    mQLayoutItem_.setGeometry ( QRect ( mPos.left(), mPos.top(), 
                                        mPos.hNrPics(), mPos.vNrPics() )); 
}


void i_LayoutItem::initLayout( layoutMode lom, int mngrTop, int mngrLeft )
{
    uiRect& mPos = curpos( lom );
    int pref_h_nr_pics =0;
    int pref_v_nr_pics =0;


    if ( lom != minimum )
    {
	if ( bodyLayouted() )
	{
	    pref_h_nr_pics	= bodyLayouted()->prefHNrPics();
	    pref_v_nr_pics	= bodyLayouted()->prefVNrPics();

	}
	else
	{
	    pref_h_nr_pics	= prefSize().hNrPics();
	    pref_v_nr_pics	= prefSize().vNrPics();
	}
    }

#ifdef __extensive_debug__
    if ( arend_debug )
    {
	BufferString blnm = bodyLayouted() ?  bodyLayouted()->name() : "";

	cout << "Init layout on:" << blnm;
	cout << ": prf hsz: " << pref_h_nr_pics;
	cout <<",  prf vsz: " << pref_v_nr_pics;
	cout <<", mngr top: " << mngrTop;
	cout <<", mngr left: " << mngrLeft;
	cout <<",  layout mode: " << (int) lom << endl;
    }
#endif

    switch ( lom )
    {
	case minimum:
            if ( !minimum_pos_inited)
	    {
		mPos.zero();
		uiSize ms = minimumsize();
		mPos.setHNrPics( ms.hNrPics() );
		mPos.setVNrPics( ms.vNrPics() );
		minimum_pos_inited = true;
	    }
	    break;

	case setGeom:
	    {
		uiRect& pPos = curpos(preferred);
		if ( !preferred_pos_inited )
		{
		    pPos.setLeft( 0 );
		    pPos.setTop( 0 );

		    pPos.setHNrPics( pref_h_nr_pics  );
		    pPos.setVNrPics( pref_v_nr_pics );
		    preferred_pos_inited = true;
		}
		uiRect& mPos = curpos( lom );
		mPos = curpos( preferred );

		mPos.leftTo( mMAX( pPos.left(), mngrLeft ));
		mPos.topTo( mMAX( pPos.top(), mngrTop ));

		initChildLayout(lom);
	    }
	    break;

	case preferred:
	    {
		mPos.setLeft( mngrLeft );
		mPos.setTop( mngrTop );

		mPos.setHNrPics( pref_h_nr_pics  );
		mPos.setVNrPics( pref_v_nr_pics );
		preferred_pos_inited = true;
	    }
	    break;
    } 

    if ( mPos.left() < 0 ) 
	{ pErrMsg("left < 0"); }
    if ( mPos.top() < 0 ) 
	{ pErrMsg("top < 0"); }

}


#ifdef __debug__

int i_LayoutItem::isPosOk( uiConstraint* c, int i, bool chkNrIters )
{
    if ( chkNrIters )	{ if ( i < MAX_ITER )	return i; }
    else		{ if ( i <= 2000 )	return i; }

    if ( c->enabled() ) 
    {
	BufferString msg;
	if ( chkNrIters )
	    msg = "\n  Too many iterations with: \"";
	else
	    msg = "\n  Layout loop on: \"";
	msg+= objLayouted() ? (const char*)objLayouted()->name() : "UNKNOWN";
	msg+= "\"";

	switch ( c->type )
	{
	    case leftOf: 		msg+= " leftOf "; break;
	    case rightOf:		msg+= " rightOf "; break;
	    case leftTo:		msg+= " leftTo "; break;
	    case rightTo:		msg+= " rightTo "; break;
	    case leftAlignedBelow:	msg+= " leftAlignedBelow "; break;
	    case leftAlignedAbove:	msg+= " leftAlignedAbove "; break;
	    case rightAlignedBelow:	msg+= " rightAlignedBelow "; break;
	    case rightAlignedAbove:	msg+= " rightAlignedAbove "; break;
	    case alignedBelow:		msg+= " alignedBelow "; break;
	    case alignedAbove:		msg+= " alignedAbove "; break;
	    case centeredBelow:		msg+= " centeredBelow "; break;
	    case centeredAbove:		msg+= " centeredAbove "; break;
	    case ensureLeftOf:		msg+= " ensureLeftOf "; break;
	    case ensureRightOf:		msg+= " ensureRightOf "; break;
	    case ensureBelow:		msg+= " ensureBelow "; break;
	    case leftBorder:		msg+= " leftBorder "; break;
	    case rightBorder:		msg+= " rightBorder "; break;
	    case topBorder:		msg+= " topBorder "; break;
	    case bottomBorder:		msg+= " bottomBorder "; break;
	    case heightSameAs: 		msg+= " heightSameAs "; break;
	    case widthSameAs:		msg+= " widthSameAs "; break;
	    case stretchedBelow:	msg+= " stretchedBelow "; break;
	    case stretchedAbove:	msg+= " stretchedAbove "; break;
	    case stretchedLeftTo:	msg+= " stretchedLeftTo "; break;
	    case stretchedRightTo:	msg+= " stretchedRightTo "; break;
	    default:		 	msg+= " .. "; break;
	}

	msg+= "\"";
	msg+= c->other && c->other->objLayouted() ? 
		    (const char*)c->other->objLayouted()->name() : "UNKNOWN";
	msg+= "\"";
	pErrMsg( msg );

	c->disable();
    }

    return i;
}


#define mCP(val)	isPosOk(constr,(val),false)
#define mUpdated()	{ isPosOk(constr,iternr,true); updated=true; }

#else

#define mCP(val)	(val)
#define mUpdated()	{ updated=true; }

#endif


#define mHorSpacing (constr->margin >= 0 ? constr->margin : mngr_.horSpacing())
#define mVerSpacing (constr->margin >= 0 ? constr->margin : mngr_.verSpacing())

#define mFullStretch() (constr->margin < -1)
#define mInsideBorder  (constr->margin > mngr_.borderSpace() \
			 ? constr->margin - mngr_.borderSpace() : 0)

bool i_LayoutItem::layout( layoutMode lom, const int iternr, bool finalLoop )
{
    bool updated = false;
    uiRect& mPos = curpos(lom);

    for ( constraintIterator it(constrList);
				uiConstraint* constr = it.current(); ++it )
    {
	const uiRect& otherPos 
		= constr->other ? constr->other->curpos(lom) : curpos(lom);

	switch ( constr->type )
	{
	case rightOf:
	case rightTo:
	{
	    if ( mPos.leftToAtLeast(mCP(otherPos.right() + mHorSpacing)))
		mUpdated(); 

	    if ( mPos.topToAtLeast( mCP(otherPos.top()) ) ) 
		 mUpdated();

	    break;
	}
	case leftOf:  
	{
	    if ( mPos.rightToAtLeast(mCP(otherPos.left() - mHorSpacing)))
		mUpdated(); 

	    if ( mPos.topToAtLeast( mCP(otherPos.top())) ) 
		 mUpdated();

	    break;
	}
	case leftTo:  
	{
	    if ( mPos.topToAtLeast( mCP(otherPos.top())) ) 
		 mUpdated();

	    break;
	}
	case leftAlignedBelow:
	{
	    if ( mPos.topToAtLeast(mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated(); 

	    if ( mPos.leftToAtLeast( mCP(otherPos.left())) ) 
		mUpdated();

	    break;
	}
	case leftAlignedAbove: 
	{
	    if ( mPos.leftToAtLeast( mCP(otherPos.left())) ) 
		mUpdated();

	    break;
	}
	case rightAlignedBelow:
	{
	    if ( mPos.topToAtLeast(mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated();

	    if ( mPos.rightToAtLeast( mCP(otherPos.right())) )
		mUpdated();

	    break;
	}
	case rightAlignedAbove: 
	{
	    if ( mPos.rightToAtLeast( mCP(otherPos.right()) ) )
		mUpdated();

	    break;
	}

	case alignedWith:
	{ 
	    int malign = horAlign( lom );
	    int othalign = constr->other->horAlign( lom );

	    if ( malign < 0 || othalign < 0 ) break;

	    if ( mPos.leftToAtLeast( mCP(mPos.left() + othalign - malign)) ) 
		mUpdated();

	    break;
	}

	case alignedBelow:
	{
	    if ( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated();

	    int malign = horAlign( lom );
	    int othalign = constr->other->horAlign( lom );

	    if ( malign < 0 || othalign < 0 ) break;

	    if ( mPos.leftToAtLeast( mCP(mPos.left() + othalign - malign)) ) 
		mUpdated();

	    break;
	}

	case alignedAbove:
	{ 
	    int malign = horAlign( lom );
	    int othalign = constr->other->horAlign( lom );

	    if ( malign < 0 || othalign < 0 ) break;

	    if ( mPos.leftToAtLeast( mCP(mPos.left() + othalign - malign)) ) 
		mUpdated();

	    break;
	}

	case centeredBelow:
	{
	    if ( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated(); 

	    if ( centre(lom) > 0 && constr->other->centre(lom) > 0 &&
		mPos.leftToAtLeast( mCP(mPos.left() 
				    + constr->other->centre(lom) 
				    - centre(lom)) 
				  )
	      ) 
		mUpdated();
	    break;
	}
	case centeredAbove: 
	{
	    if ( centre(lom) > 0 && constr->other->centre(lom) > 0 &&
		mPos.leftToAtLeast( mCP(mPos.left() 
				    + constr->other->centre(lom) 
				    - centre(lom)) 
				  )
	      ) 
		mUpdated();
	    break;
	} 

	case centeredLeftOf:
	{
	    if ( mPos.rightToAtLeast(mCP(otherPos.left() - mHorSpacing)))
		mUpdated(); 

	    if ( centre(lom,false) > 0 &&
		 constr->other->centre(lom,false) > 0 &&
		 mPos.topToAtLeast( mCP(mPos.top() 
				    + constr->other->centre(lom,false) 
				    - centre(lom,false)) 
				  )
	      ) 
		mUpdated();
	    break;
	}

	case centeredRightOf: 
	{
	    if ( mPos.leftToAtLeast(mCP(otherPos.right() + mHorSpacing)))
		mUpdated(); 

	    if ( centre(lom,false) > 0 &&
		 constr->other->centre(lom,false) > 0 &&
		 mPos.topToAtLeast( mCP(mPos.top() 
				    + constr->other->centre(lom,false) 
				    - centre(lom,false)) 
				  )
	      ) 
		mUpdated();
	    break;
	} 


	case hCentered: 
	{
	    if ( finalLoop )
	    {
		int mngrcentre = ( mngr().curpos(lom).left()
				     + mngr().curpos(lom).right() ) / 2;

		int shift = mngrcentre >= 0 ?  mngrcentre - centre(lom) : 0;

		if ( shift > 0 )
		{
		    if ( mPos.leftToAtLeast( mCP(mPos.left() + shift) ) ) 
			mUpdated();
		}
	    }
	    break;
	} 


	case ensureRightOf:
	{
	    if ( mPos.leftToAtLeast( mCP(otherPos.right() + mHorSpacing )))  
		mUpdated(); 

	    break;
	}
	case ensureBelow:
	{
	    if ( mPos.topToAtLeast( mCP(otherPos.bottom() + mVerSpacing ))) 
		mUpdated(); 

	    break;
	}
	case leftBorder:
	{
	    if ( finalLoop )
	    {
		int nwLeft = mngr().curpos(lom).left() + mInsideBorder;
		if ( mPos.left() != nwLeft )
		{
		    mPos.leftTo( mCP(nwLeft));
		    mUpdated();
		}
	    }
	    break;
	}
	case rightBorder:
	{
	    if ( finalLoop )
	    {
		int nwRight = mngr().curpos(lom).right() - mInsideBorder;
		if ( mPos.right() != nwRight )
		{
		    mPos.rightTo( mCP(nwRight));
		    mUpdated();
		}
	    }
	    break;
	}
	case topBorder:
	{
	    if ( finalLoop )
	    {
		int nwTop = mngr().curpos(lom).top() + mInsideBorder;
		if ( mPos.top() != nwTop )
		{
		    mPos.topTo( mCP(nwTop ));
		    mUpdated();
		}
	    }
	    break;
	}
	case bottomBorder:
	{
	    if ( finalLoop )
	    {
		int nwBottom = mngr().curpos(lom).bottom()- mInsideBorder;
		if ( mPos.bottom() != nwBottom )
		{
		    mPos.bottomTo( mCP(nwBottom ));
		    mUpdated();
		}
	    }
	    break;
	}
	case heightSameAs:
	{
	    if ( mPos.vNrPics() < ( otherPos.vNrPics() ) )
	    {
		mPos.setVNrPics( otherPos.vNrPics() );
		mUpdated();
	    }
	    break;
	}
	case widthSameAs:
	{
	    if ( mPos.hNrPics() < ( otherPos.hNrPics() ) )
	    {
		mPos.setHNrPics( otherPos.hNrPics() );
		mUpdated();
	    }
	    break;
	}
	case stretchedBelow:
	{
	    int nwLeft = mFullStretch() ? mngr().winpos(lom).left() 
					: mngr().curpos(lom).left();

	    if ( finalLoop && mPos.left() != nwLeft )
	    {
		mPos.leftTo( mCP(nwLeft));
		mUpdated();
	    }

	    int nwWidth = mFullStretch() ? mngr().winpos(lom).hNrPics() 
					: mngr().curpos(lom).hNrPics();

	    if ( finalLoop &&  mPos.hNrPics() < nwWidth )
	    {
		mPos.setHNrPics( nwWidth );
		mUpdated();
	    }
	    if ( mPos.topToAtLeast(mCP(otherPos.bottom() + mVerSpacing)))
		mUpdated(); 

	    break;
	}
	case stretchedAbove:
	{
	    int nwLeft = mFullStretch() ? mngr().winpos(lom).left() 
					: mngr().curpos(lom).left();
	    if ( finalLoop && mPos.left() != nwLeft )
	    {
		mPos.leftTo( mCP(nwLeft));
		mUpdated();
	    }

	    int nwWidth = mFullStretch() ? mngr().winpos(lom).hNrPics() 
					: mngr().curpos(lom).hNrPics();

	    if ( finalLoop &&  mPos.hNrPics() < nwWidth )
	    {
		mPos.setHNrPics( nwWidth );
		mUpdated();
	    }

	    break;
	}
	case stretchedLeftTo:
	{
	    int nwTop = mFullStretch() ? mngr().winpos(lom).top() 
					: mngr().curpos(lom).top();
	    if ( finalLoop && mPos.top() != nwTop )
	    {
		mPos.topTo( mCP(nwTop));
		mUpdated();
	    }

	    int nwHeight = mFullStretch() ? mngr().winpos(lom).vNrPics() 
					  : mngr().curpos(lom).vNrPics();
	    if ( finalLoop && mPos.vNrPics() < nwHeight )
	    {
		mPos.setVNrPics( nwHeight );
		mUpdated();
	    }

	    break;
	}
	case stretchedRightTo:
	{
	    int nwTop = mFullStretch() ? mngr().winpos(lom).top() 
					: mngr().curpos(lom).top();
	    if ( finalLoop && mPos.top() != nwTop )
	    {
		mPos.topTo( mCP(nwTop));
		mUpdated();
	    }

	    int nwHeight = mFullStretch() ? mngr().winpos(lom).vNrPics() 
					  : mngr().curpos(lom).vNrPics();
	    if ( finalLoop && mPos.vNrPics() < nwHeight )
	    {
		mPos.setVNrPics( nwHeight );
		mUpdated();
	    }
	    if ( mPos.leftToAtLeast(mCP(otherPos.right() + mHorSpacing)))
		mUpdated(); 

	    break;
	}
	case ensureLeftOf:
	{
	    break;
	}
	default:
	{
	    pErrMsg("Unknown constraint type");
	    break;
	}
	}
    }

    return updated;
}


void i_LayoutItem::attach ( constraintType type, i_LayoutItem *other, 
			    int margn, bool reciprocal )
{
    if ( type != ensureLeftOf)
	constrList.append( new uiConstraint( type, other, margn ) );

    if( reciprocal && other )
    {
	switch ( type )
	{
	case leftOf:
	    other-> constrList.append(new uiConstraint( rightOf, this, margn ));
	break;

	case rightOf:
	    other-> constrList.append(new uiConstraint( leftOf, this, margn ));
	break;

	case leftTo:
	    other-> constrList.append(new uiConstraint( rightTo, this, margn));
	break;

	case rightTo:
	    other-> constrList.append(new uiConstraint( leftTo, this, margn ));
	break;

	case leftAlignedBelow:
	    other-> constrList.append( 
			    new uiConstraint( leftAlignedAbove, this, margn ));
	break;

	case leftAlignedAbove:
	    other-> constrList.append( 
			    new uiConstraint( leftAlignedBelow, this, margn ));
	break;

	case rightAlignedBelow:
	    other-> constrList.append( 
			    new uiConstraint( rightAlignedAbove, this, margn ));
	break;

	case rightAlignedAbove:
	    other-> constrList.append( 
			    new uiConstraint( rightAlignedBelow, this, margn ));
	break;

	case alignedWith:
	    other-> constrList.append( 
			    new uiConstraint( alignedWith, this, margn ));
	break;

	case alignedBelow:
	    other-> constrList.append( 
			    new uiConstraint( alignedAbove, this, margn ));
	break;

	case alignedAbove:
	    other-> constrList.append( 
			    new uiConstraint( alignedBelow, this, margn ));
	break;

	case centeredBelow:
	    other-> constrList.append( 
			    new uiConstraint( centeredAbove, this, margn ));
	break;

	case centeredAbove:
	    other-> constrList.append( 
			    new uiConstraint( centeredBelow, this, margn ));
	break;

	case centeredLeftOf:
	    other-> constrList.append( 
			    new uiConstraint( centeredRightOf, this, margn ));
	break;

	case centeredRightOf:
	    other-> constrList.append( 
			    new uiConstraint( centeredLeftOf, this, margn ));
	break;

	case heightSameAs:
	    vsameas=true;
	break;

	case widthSameAs:
	    hsameas=true;
	break;

	case ensureLeftOf:
	    other-> constrList.append( 
			    new uiConstraint( ensureRightOf, this, margn ));
	break;

	case stretchedBelow:
	break;

	case stretchedAbove:
	    other-> constrList.append( 
			    new uiConstraint( ensureBelow, this, margn ));
	break;

	case stretchedLeftTo:
	    other-> constrList.append( 
			    new uiConstraint( stretchedRightTo, this, margn ));
	break;

	case stretchedRightTo:
	    other-> constrList.append( 
			    new uiConstraint( stretchedLeftTo, this, margn ));
	break;

	case leftBorder:
	case rightBorder:
	case topBorder:
	case bottomBorder:
	case ensureRightOf:
	case ensureBelow:
	case hCentered:
	break;

	default:
	    pErrMsg("Unknown constraint type");
	break;
	}
    }
}


#ifdef __debug__
bool i_LayoutItem::isAligned() const
{
    for ( constraintIterator it(constrList); 
				uiConstraint* constr = it.current(); ++it )
    {
	if ( constr->type >= alignedWith && constr->type <= centeredAbove )
	    return true; 
    }

    return false;
}
#endif

