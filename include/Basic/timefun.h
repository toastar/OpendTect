#pragma once

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	Time functions
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"

mFDQtclass(QTime)

namespace Time
{

    mExpClass(Basic) Counter
    {
    public:

			Counter();
			~Counter();
	void		start();
	int		restart();		//!< Returns elapsed time in ms
	int		elapsed() const;	//!< Returns elapsed time in ms

    protected:

	mQtclass(QTime*)	qtime_;

    };


    mGlobal(Basic) int getMilliSeconds();
				//!< From day start.
    mGlobal(Basic) int passedSince(int);
				//!< ... since the getMilliSeconds() before.
    mGlobal(Basic) od_int64 getFileTimeInSeconds();
				//!< Can be compared to File::getTimeInSeconds()


    mGlobal(Basic) const char*	defDateTimeFmt();
    mGlobal(Basic) const char*	defDateFmt();
    mGlobal(Basic) const char*	defTimeFmt();

    mGlobal(Basic) const char*	getDateTimeString(const char* fmt
					    =defDateTimeFmt(),bool local=true);
    mGlobal(Basic) const char*	getDateString(const char* fmt=defDateFmt(),
					      bool local=true);
    mGlobal(Basic) const char*	getTimeString(const char* fmt=defTimeFmt(),
					      bool local=true);

    mGlobal(Basic) bool isEarlier(const char* first, const char* second,
			   const char* fmt=defDateTimeFmt());
			/*! returns true if the first DateTime string is
			  earlier than the second */


} // namespace Time
