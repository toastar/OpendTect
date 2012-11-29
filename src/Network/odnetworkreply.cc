/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		October 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "odnetworkreply.h"

#include "qnetworkaccessconn.h"

#include <QEventLoop>


ODNetworkReply::ODNetworkReply( QNetworkReply* qnr )
    : downloadProgress( this )
    , finished( this )
    , metaDataChanged( this )
    , error( this )
    , uploadProgress( this )
    , aboutToClose( this )
    , bytesWritten( this )
    , readyRead( this )
    , remotefilesize_( 0 )
    , qeventloop_( new QEventLoop )
    , status_( NoReply )
{
    qnetworkreply_ = qnr;
    qnetworkreplyconn_ = new QNetworkReplyConn(qnetworkreply_, this);

    error.notify( mCB(this,ODNetworkReply,errorOccurred) );
    finished.notify( mCB(this,ODNetworkReply,finish) );
    readyRead.notify( mCB(this,ODNetworkReply,readFromObj) );
    metaDataChanged.notify( mCB(this,ODNetworkReply,setRemoteFileSize) );
}


ODNetworkReply::~ODNetworkReply()
{ 
    delete qnetworkreply_;
    delete qnetworkreplyconn_;
    delete qeventloop_;
}


bool ODNetworkReply::errorOccurred(CallBacker*)
{
    status_ = Error;
    if ( isEventLoopRunning() )
	stopEventLoop();

    return true;
}


bool ODNetworkReply::finish(CallBacker*)
{
    if ( status_ == Error )
	return true;

    status_ = Finish;
    if ( isEventLoopRunning() )
	stopEventLoop();

    return true;
}


bool ODNetworkReply::readFromObj( CallBacker* )
{
    status_ = DataReady;
    if ( isEventLoopRunning() )
	stopEventLoop();

    return true;
}


bool ODNetworkReply::setRemoteFileSize( CallBacker* )
{
    remotefilesize_ = qnetworkreply_->header
			    ( QNetworkRequest::ContentLengthHeader ).toInt();
    return true;
}


void ODNetworkReply::startEventLoop() const
{ qeventloop_->exec(); }


void ODNetworkReply::stopEventLoop() const
{ qeventloop_->exit(); }


bool ODNetworkReply::isEventLoopRunning() const
{ return qeventloop_->isRunning(); }
