/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visdata.cc,v 1.6 2002-04-26 13:09:35 kristofer Exp $";

#include "visdata.h"
#include "visdataman.h"
#include "visselman.h"

#include "Inventor/nodes/SoNode.h"


const char* visBase::DataObject::name() const
{
    if ( !name_ || !(*name_)[0]) return 0;
    return (const char*)*name_;
}


void visBase::DataObject::setName( const char* nn )
{
    if ( !name_ ) name_ = new BufferString;
    
    (*name_) = nn;
}


visBase::DataObject::DataObject()
    : id_( -1 )
    , name_( 0 )
{ }


visBase::DataObject::~DataObject()
{
    SoNode* data = getData();
    if ( data ) data->unref();

    delete name_;
}


void visBase::DataObject::ref() const
{
    DM().ref( this );
}


void visBase::DataObject::unRef() const
{
    DM().unRef( this );
}


void visBase::DataObject::select() const
{
    DM().selMan().select( id() );
}


void visBase::DataObject::deSelect() const
{
    DM().selMan().deSelect( id() );
}


bool visBase::DataObject::isSelected() const
{ return selectable() && DM().selMan().selected().indexOf( id()) != -1; }


void visBase::DataObject::init()
{
    id_ = DM().addObj( this );

    SoNode* data = getData();
    if ( data ) data->ref();
}


visBase::FactoryEntry::FactoryEntry( FactPtr funcptr_,
				     const char* name_ ) 
    : funcptr( funcptr_ )
    , name( name_ )
{
    DM().factory().entries += this;
}


visBase::DataObject* visBase::Factory::create( const char* nm )
{
    for ( int idx=0; idx<entries.size(); idx++ )
    {
	if ( !strcmp( nm, entries[idx]->name )) return entries[idx]->create();
    }

    return 0;
}

