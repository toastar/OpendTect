/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiseispartserv.cc,v 1.1 2002-02-08 22:01:28 bert Exp $
________________________________________________________________________

-*/

#include "uiseispartserv.h"
#include "uimergeseis.h"
#include "uiseisimpexp.h"
#include "uiseisidealimpexp.h"
#include "uimsg.h"
#include "idealconn.h"
#include "ptrman.h"


bool uiSeisPartServer::isAvailable( uiSeisPartServer::ExternalType t ) const
{
    return t == uiSeisPartServer::SegY ||
	   (int)t == ((int)IdealConn::guessedType() + 1);
}


bool uiSeisPartServer::ioSeis( uiSeisPartServer::ExternalType t, bool forread )
{
#ifndef sun5
    if ( !uiMSG().askGoOn( "Sorry, workstation connection not available on "
			    "this platform.\nPlease import from a "
			    "Solaris workstation or use SEG-Y.\n\n"
			    "Do you wish to see the dialog anyway?" ) )
	return false;
#endif

    PtrMan<uiDialog> dlg = t == uiSeisPartServer::SegY
	      ? new uiSeisImpExp( appserv().parent(), forread, segyid )
	      : new uiSeisIdealImpExp( appserv().parent(), forread, (int)t==1 );

    return dlg->go();
}


bool uiSeisPartServer::importSeis( uiSeisPartServer::ExternalType t )
{ return ioSeis( t, true ); }
bool uiSeisPartServer::exportSeis( uiSeisPartServer::ExternalType t )
{ return ioSeis( t, false ); }


bool uiSeisPartServer::mergeSeis()
{
    uiMergeSeis dlg( appserv().parent() );
    return dlg.go();
}
