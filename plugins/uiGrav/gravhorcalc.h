#ifndef gravhorcalc_h
#define gravhorcalc_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2010
 RCS:		$Id$
________________________________________________________________________

*/

#include "uigravmod.h"
#include "executor.h"
#include "multiid.h"
#include "grav.h"
class ZAxisTransform;
namespace EM { class Horizon3D; }


namespace Grav
{


mClass(uiGrav) HorCalc : public ::Executor
{
public:

    mExpClass(uiGrav) Setup
    {
    public:
			Setup( const MultiID& calcmid )
			    : calcid_(calcmid)		{}

	mDefSetupMemb(MultiID,calcid)
	mDefSetupMemb(MultiID,topid)
	mDefSetupMemb(MultiID,botid)
	mDefSetupMemb(BufferString,denattr)
    };

			HorCalc(const MultiID&,const MultiID* top=0,
				const MultiID* bot=0,float ang=1);
			~HorCalc();

    void		setCutOffAngle( float a )	{ cutoffangle_ = a; }
    void		setVelModel( const MultiID& m )	{ velmid_ = m; }

    const char*		message() const			{ return msg_; }
    const char*		nrDoneText() const;
    od_int64		nrDone() const			{ return nrdone_; }
    od_int64		totalNr() const			{ return totnr_; }
    int			nextStep();

protected:

    float		cutoffangle_;
    MultiID		velmid_;

    EM::Horizon3D*	calchor_;
    EM::Horizon3D*	tophor_;
    EM::Horizon3D*	bothor_;
    ZAxisTransform*	ztransf_;

    BufferString	msg_;
    od_int64		totnr_;
    od_int64		nrdone_;

    int			doLoadStep();

};

} // namespace Grav


#endif

