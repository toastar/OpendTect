/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimenu.cc,v 1.44 2007-09-28 03:50:42 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uimenu.h"
#include "i_qmenu.h"
#include "uiparentbody.h"
#include "uiobjbody.h"
#include "uibody.h"

#include <QApplication>
#include <QCursor>
#include <QEvent>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>

class uiMenuItemContainerBody
{
public:

    virtual			~uiMenuItemContainerBody()
    				{ deepErase( itms_ ); }

    int				nrItems() const 	{ return itms_.size(); }

    virtual int			insertItem(uiMenuItem*,int id) =0;
    virtual int			insertItem(uiPopupMenu*,int id) =0;

    virtual QMenuBar*		bar()			{ return 0; }
    virtual QMenu*		popup()			{ return 0; }

    void			setIcon( const QPixmap& pm )
				{
    			    	    if ( bar() )	bar()->setIcon( pm );
    			    	    if ( popup() )	bar()->setIcon( pm );
				}

    void			setSensitive( bool yn )
				{
				    if ( bar() ) bar()->setEnabled( yn );
				}

    ObjectSet<uiMenuItem>	itms_;
    ObjectSet<QAction>		actions_;

protected:
				uiMenuItemContainerBody()	{}
};


template <class T>
class uiMenuItemContainerBodyImpl : public uiMenuItemContainerBody
			 , public uiBodyImpl<uiMenuItemContainer,T>
{
public:
			uiMenuItemContainerBodyImpl(uiMenuItemContainer& handle, 
				       uiParent* parnt,
				       T& qThing )
			    : uiBodyImpl<uiMenuItemContainer,T>
				( handle, parnt, qThing )
			    , qmenu_( &qThing ) {}

			~uiMenuItemContainerBodyImpl()		{}


    int			insertItem( uiMenuItem* it, int id )
			{
			    QString nm( it->name() );
			    i_MenuMessenger* msgr__= it->messenger();

			    QAction* action = qmenu_->addAction( nm, msgr__,
				    SLOT(activated()) );

			    it->setId( id );
			    it->setMenu( this );
			    it->setAction( action );
			    if ( it->isChecked() )
				action->setChecked( it->isChecked() );
			    action->setEnabled( it->isEnabled() );
			    itms_ += it;
			    actions_ += action;
			    return id;
			}

    int			insertItem( uiPopupMenu* pmnu, int id )
			{
			    uiPopupItem* it = &pmnu->item();

			    QMenu* pu = pmnu->body_->popup();
			    pu->setTitle( it->name().buf() );

			    QAction* action = qmenu_->addMenu( pu );
			    it->setId( id );
			    it->setMenu( this );
			    it->setAction( action );
			    if ( it->isChecked() )
				action->setChecked( it->isChecked() );
			    action->setEnabled( it->isEnabled() );
			    itms_ += it;
			    actions_ += action;
			    return id;
			}

    void		clear()
    			{
			    qmenu_->clear();
			    deepErase(itms_);
			    actions_.erase();
			}

    QMenuBar*		bar()
    			{
			    mDynamicCastGet(QMenuBar*,qbar,qmenu_)
			    return qbar;
			}

    QMenu*		popup()
    			{
			    mDynamicCastGet(QMenu*,qpopup,qmenu_)
			    return qpopup;
			}

    virtual const QWidget* managewidg_() const 	{ return qmenu_; }

private:

    T*			qmenu_;
};


//-----------------------------------------------------------------------

uiMenuItem::uiMenuItem( const char* nm )
    : NamedObject(nm)
    , activated(this)
    , activatedone(this)
    , messenger_( *new i_MenuMessenger(this) ) 
    , id_(-1)
    , menu_(0)
    , qaction_(0)
    , enabled_(true)
    , checked_(false)
    , checkable_(false)
{}


uiMenuItem::uiMenuItem( const char* nm, const CallBack& cb )
    : NamedObject(nm )
    , activated(this)
    , activatedone(this)
    , messenger_( *new i_MenuMessenger(this) )
    , id_(-1)
    , menu_(0)
    , qaction_(0)
    , enabled_(true)
    , checked_(false)
    , checkable_(false)
{ 
    activated.notify( cb ); 
}


uiMenuItem::~uiMenuItem()
{ 
    delete &messenger_; 
}


bool uiMenuItem::isEnabled() const
{
    return qaction_ ? qaction_->isEnabled() : enabled_;
}


void uiMenuItem::setEnabled( bool yn )
{
    enabled_ = yn;
    if ( qaction_ ) qaction_->setEnabled( yn );
}


bool uiMenuItem::isCheckable() const
{ return qaction_ ? qaction_->isCheckable() : checkable_; }

void uiMenuItem::setCheckable( bool yn )
{
    checkable_ = yn;
    if ( qaction_ ) qaction_->setCheckable( yn );
}


bool uiMenuItem::isChecked() const
{
    if ( !checkable_ ) return false;
    return qaction_ ? qaction_->isChecked() : checked_;
}


void uiMenuItem::setChecked( bool yn )
{
    if ( !checkable_ ) return;
    if ( qaction_ ) qaction_->setChecked( yn );

    checked_ = yn;
}


void uiMenuItem::setText( const char* txt )
{
    if ( qaction_ ) qaction_->setText( txt );
}


const char* uiMenuItem::text() const
{
    return qaction_ ? qaction_->text() : "";
}


static const QEvent::Type sQEventActivate = (QEvent::Type) (QEvent::User + 0);

bool uiMenuItem::handleEvent( const QEvent* ev )
{
    if ( ev->type() == sQEventActivate )
    {
	activated.trigger();
	activatedone.trigger();
	return true;
    }

    return false;
}


void uiMenuItem::activate()
{
    QEvent* activateevent = new QEvent( sQEventActivate );
    QApplication::postEvent( &messenger_ , activateevent );
}


//-----------------------------------------------------------------------


uiMenuItemContainer::uiMenuItemContainer( const char* nm, uiBody* b,
					  uiMenuItemContainerBody* db )
    : uiObjHandle( nm, b )
    , body_( db )				{}


uiMenuItemContainer::~uiMenuItemContainer()	{ delete body_; }


int uiMenuItemContainer::nrItems() const
    { return body_->nrItems(); }


const ObjectSet<uiMenuItem>& uiMenuItemContainer::items() const
    { return body_->itms_; }


uiMenuItem* uiMenuItemContainer::find( const MenuItemSeparString& str )
{
    uiMenuItemContainer* parent = this;
    for ( unsigned int idx=0; idx<str.size(); idx++ )
    {
	if ( !parent ) return 0;

	uiMenuItem* itm = parent->find( str[idx] );
	if ( !itm ) return 0;
	if ( idx == str.size()-1 )
	    return itm;

	mDynamicCastGet(uiPopupItem*,popupitm,itm)
	parent = popupitm ? &popupitm->menu() : 0;
    }

    return 0;
}


uiMenuItem* uiMenuItemContainer::find( const char* itmtxt )
{
    for ( int idx=0; idx<body_->nrItems(); idx++ )
    {
	const uiMenuItem* itm = body_->itms_[idx];
	if ( !strcmp(itm->name(),itmtxt) )
	    return const_cast<uiMenuItem*>(itm);
    }

    return 0;
}


int uiMenuItemContainer::insertItem( uiMenuItem* it, int id )
    { return body_->insertItem( it, id ); }

int uiMenuItemContainer::insertItem( uiPopupMenu* it, int id )
    { return body_->insertItem( it, id ); }

void uiMenuItemContainer::insertSeparator( int idx ) 
{
    if ( body_->bar() )
	body_->bar()->insertSeparator( idx );
    else if ( body_->popup() )
	body_->popup()->insertSeparator( idx );
}


void uiMenuItemContainer::clear()
{
    if ( body_->bar() )		body_->bar()->clear();
    if ( body_->popup() )	body_->popup()->clear();

    deepErase( body_->itms_ );
    body_->actions_.erase();
}


void uiMenuItemContainer::removeItem( int id )
{
    for ( int idx=0; idx<body_->itms_.size(); idx++ )
    {
	if ( body_->itms_[idx]->id() != id )
	    continue;

	if ( body_->popup() )
	    body_->popup()->removeAction( body_->actions_[idx] );
	delete body_->itms_.remove( idx );
	body_->actions_.remove( idx );
	return;
    }
}

// ------------------------------------------------------------------------


uiMenuBar::uiMenuBar( uiParent* parnt, const char* nm )
    : uiMenuItemContainer( nm, 0, 0 )
{ 
    uiMenuItemContainerBodyImpl<QMenuBar>* bd =
		    new uiMenuItemContainerBodyImpl<QMenuBar>( *this, parnt,
				*new QMenuBar(parnt->body()->qwidget(),nm ) );
    body_ = bd;
    setBody( bd );
}


uiMenuBar::uiMenuBar( uiParent* parnt, const char* nm, QMenuBar& qThing )
    : uiMenuItemContainer( nm, 0, 0 )
{ 
    uiMenuItemContainerBodyImpl<QMenuBar>* bd =
	    new uiMenuItemContainerBodyImpl<QMenuBar>( *this, parnt, qThing );
    body_ = bd;
    setBody( bd );
}


void uiMenuBar::reDraw( bool deep )
{ if ( body_->bar() ) body_->bar()->update(); }

void uiMenuBar::setIcon( const QPixmap& pm )
{ body_->setIcon( pm ); }

void uiMenuBar::setSensitive( bool yn )
{ body_->setSensitive( yn ); }

bool uiMenuBar::isSensitive() const
{ return body_->bar() && body_->bar()->isEnabled(); }

// -----------------------------------------------------------------------

uiPopupMenu::uiPopupMenu( uiParent* parnt, const char* nm )
    : uiMenuItemContainer( nm, 0, 0 )
    , item_( *new uiPopupItem( *this, nm ) )
{
    uiMenuItemContainerBodyImpl<QMenu>* bd =
		    new uiMenuItemContainerBodyImpl<QMenu>( *this, parnt, 
			  *new QMenu(parnt->body()->qwidget()) );
    body_ = bd;
    setBody( bd );

    item_.setMenu( body_ );
}


uiPopupMenu::~uiPopupMenu() 
{ delete &item_; }

bool uiPopupMenu::isCheckable() const
{ return item_.isCheckable(); }

void uiPopupMenu::setCheckable( bool yn ) 
{ item_.setCheckable( yn ); }

bool uiPopupMenu::isChecked() const
{ return item_.isChecked(); }

void uiPopupMenu::setChecked( bool yn )
{ item_.setChecked( yn ); }

bool uiPopupMenu::isEnabled() const
{ return item_.isEnabled(); }

void uiPopupMenu::setEnabled( bool yn )
{ item_.setEnabled( yn ); }


int uiPopupMenu::findIdForAction( QAction* qaction ) const
{
    const int idx = body_->actions_.indexOf( qaction );
    if ( body_->itms_.validIdx(idx) )
	return body_->itms_[idx]->id();

    int id = -1;
    for ( int idx=0; idx<body_->itms_.size(); idx++ )
    {
	mDynamicCastGet(uiPopupItem*,itm,body_->itms_[idx])
	if ( itm ) id = itm->menu().findIdForAction( qaction );
	if ( id >= 0 ) break;
    }
    return id;
}


int uiPopupMenu::exec()
{
    QMenu* mnu = body_->popup();
    if ( !mnu ) return -1;

    QAction* qaction = body_->popup()->exec( QCursor::pos() );
    return findIdForAction( qaction );
}
