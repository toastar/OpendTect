/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "uiprestackagc.h"

#include "uiprestackprocessor.h"
#include "prestackagc.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "od_helpids.h"

namespace PreStack
{

void uiAGC::initClass()
{
    uiPSPD().addCreator( create, AGC::sFactoryKeyword() );
}


uiDialog* uiAGC::create( uiParent* p, Processor* sgp )
{
    mDynamicCastGet( AGC*, sgagc, sgp );
    if ( !sgagc ) return 0;

    return new uiAGC( p, sgagc );
}


uiAGC::uiAGC( uiParent* p, AGC* sgagc )
    : uiDialog( p, uiDialog::Setup(tr("AGC setup"),mNoDlgTitle,
                                    mODHelpKey(mPreStackAGCHelpID) ) )
    , processor_( sgagc )
{
    uiString unit;
    processor_->getWindowUnit( unit, true );
    uiString label = tr("Window width %1").arg(unit);
    windowfld_ = new uiGenInput( this, label,
			     FloatInpSpec(processor_->getWindow().width() ));
    lowenergymute_ = new uiGenInput( this, tr("Low energy mute (%)"),
	    			     FloatInpSpec() );
    lowenergymute_->attach( alignedBelow, windowfld_ );
    const float lowenergymute = processor_->getLowEnergyMute();
    lowenergymute_->setValue(
	    mIsUdf(lowenergymute) ? mUdf(float) : lowenergymute*100 );
}


bool uiAGC::acceptOK()
{
    if ( !processor_ ) return true;

    const float width = windowfld_->getFValue();
    if ( mIsUdf(width) )
    {
	uiMSG().error(tr("Window width is not set"));
	return false;
    }

    processor_->setWindow( Interval<float>( -width/2, width/2 ) );
    const float lowenergymute = lowenergymute_->getFValue();
    if ( mIsUdf(lowenergymute) ) processor_->setLowEnergyMute( mUdf(float) );
    else
    {
	if ( lowenergymute<0 || lowenergymute>99 )
	{
	    uiMSG().error(tr("Low energy mute must be between 0 and 99"));
	    return false;
	}

	processor_->setLowEnergyMute( lowenergymute/100 );
    }

    return true;
}

} // namespace PreStack
