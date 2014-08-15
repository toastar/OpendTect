/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "applicationdata.h"

#include "genc.h"

#ifndef OD_NO_QT

#include <QCoreApplication>

ApplicationData::ApplicationData()
{
    int argc = GetArgC();
    application_ = new mQtclass(QCoreApplication)(argc, GetArgV() );
}


bool ApplicationData::exec()
{ return application_->exec(); }


void ApplicationData::setOrganizationName( const char* nm )
{ QCoreApplication::setOrganizationName( nm ); }


void ApplicationData::setOrganizationDomain( const char* domain )
{ QCoreApplication::setOrganizationDomain( domain ); }


void ApplicationData::setApplicationName( const char* nm )
{ QCoreApplication::setApplicationName( nm ); }

#else

ApplicationData::ApplicationData()
{ application_ = 0; }

bool ApplicationData::exec()
{ return false; }

void ApplicationData::setOrganizationName( const char* nm )
{}

void ApplicationData::setOrganizationDomain( const char* domain )
{}

void ApplicationData::setApplicationName( const char* nm )
{}

#endif // OD_NO_QT
