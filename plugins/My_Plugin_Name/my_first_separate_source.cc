/*+
 * (C) Your_copyright
 * AUTHOR   : Your_name_here
 * DATE     : Apr 2012
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "my_first_separate_source.h"
#include "uimsg.h"


My_Class::My_Class()
    : my_variable_(0)
{
    uiMSG().message( "Hello world!" );
}

