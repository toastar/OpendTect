/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 14-6-1996
 * FUNCTION : functions
-*/

static const char* rcsID = "$Id: executor.cc,v 1.2 2000-06-23 14:11:07 bert Exp $";

#include "executor.h"
#include "timefun.h"
#include "errh.h"
#include <iostream.h>


bool Executor::execute( ostream* strm, bool isfirst, bool islast )
{
    if ( !strm )
    {
	int rv = 1;
	while ( rv )
	{
	    rv = nextStep();
	    if ( rv < 0 )
	    {
		if ( message() ) ErrMsg( message() );
		return false;
	    }
	}
	return true;
    }

    ostream& stream = *strm;
    if ( isfirst )
	stream << GetProjectVersionName() << endl << endl;

    stream << "Process: '" << name() << "'" << endl;
    stream << "Started: " << Time_getLocalString() << endl << endl;

    UserIDString curmsg, prevmsg;
    prevmsg = message();

    if ( strm )
	stream << '\t' << prevmsg << endl;

    bool go_on = true;
    bool newmsg = true;
    bool needendl = false;
    int nrdone = 0;
    int nrdonedone = 0;
    int rv;
    while ( go_on )
    {
	rv = nextStep();
	curmsg = message();
	int newnrdone = nrDone();
	go_on = false;
	switch( rv )
	{
	case -1:
	    stream << "Error: " << curmsg << endl;
	break;
	case  0:
	    stream << "\nFinished: " << Time_getLocalString() << endl;
	break;
	default:
	    go_on = true;
	    if ( curmsg != prevmsg )
	    {
		if ( needendl ) stream << endl;
		needendl = false;
		stream << '\t' << curmsg << endl;
		newmsg = true;
	    }
	    else if ( newmsg && newnrdone )
	    {
		newmsg = false;
		if ( !nrdone )
		    stream << '\t' << nrDoneText() << ":\n\t\t";
		stream << newnrdone;
		nrdonedone = 1;
		needendl = true;
	    }
	    else if ( newnrdone && newnrdone != nrdone )
	    {
		nrdonedone++;
		needendl = nrdonedone%8 ? true : false;
		stream << (nrdonedone%8 ? " " : "\n\t\t");
		stream << newnrdone;
		stream.flush();
	    }
	break;
	}
	nrdone = newnrdone;
	prevmsg = curmsg;
    }

    if ( islast )
	stream << endl << endl << "End of processing" << endl;
    return rv < 0 ? false : true;
}
