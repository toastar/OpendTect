/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		March 2008
 RCS:		$Id: uiexpattribspi.cc,v 1.1 2008-10-17 05:42:10 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uimenu.h"
#include "uiodmain.h"
#include "plugins.h"

#include "uisemblanceattrib.h"

extern "C" int GetuiExpAttribsPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiExpAttribsPluginInfo()
{
    static PluginInfo retpi = {
	"uiExpAttribs plugin",
	"Experimental Attributes (UI)",
	"dGB (Nanne)",
	"=od" };
    return &retpi;
}


extern "C" const char* InituiExpAttribsPlugin( int, char** )
{
    uiSemblanceAttrib::initClass();
    return 0;
}
