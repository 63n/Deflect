/*********************************************************************/
/* Copyright (c) 2013-2015, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
/*                          Daniel.Nachbaur@epfl.ch                  */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include "Server.h"

#include "CommandHandler.h"
#include "FrameDispatcher.h"
#include "NetworkProtocol.h"
#include "ServerWorker.h"

#ifdef DEFLECT_USE_SERVUS
#  include <servus/servus.h>
#  include <boost/lexical_cast.hpp>
#endif

#include <QThread>
#include <stdexcept>

namespace deflect
{
const int Server::defaultPortNumber = DEFAULT_PORT_NUMBER;
const std::string Server::serviceName = SERVUS_SERVICE_NAME;

class Server::Impl
{
public:
    Impl()
#ifdef DEFLECT_USE_SERVUS
        : servus( Server::serviceName )
#endif
    {}

    FrameDispatcher pixelStreamDispatcher;
    CommandHandler commandHandler;
#ifdef DEFLECT_USE_SERVUS
    servus::Servus servus;
#endif
};

Server::Server( const int port )
    : _impl( new Impl )
{
    if( !listen( QHostAddress::Any, port ))
    {
        const QString err =
                QString( "could not listen on port: %1" ).arg( port );
        throw std::runtime_error( err.toStdString( ));
    }
#ifdef DEFLECT_USE_SERVUS
    _impl->servus.announce( serverPort(), "" );
#endif
}

Server::~Server()
{
    delete _impl;
}

CommandHandler& Server::getCommandHandler()
{
    return _impl->commandHandler;
}

FrameDispatcher& Server::getPixelStreamDispatcher()
{
    return _impl->pixelStreamDispatcher;
}

void Server::onPixelStreamerClosed( const QString uri )
{
    emit _pixelStreamerClosed( uri );
}

void Server::onEventRegistrationReply( const QString uri, const bool success )
{
    emit _eventRegistrationReply( uri, success );
}

void Server::incomingConnection( const qintptr socketHandle )
{
    QThread* workerThread = new QThread( this );
    ServerWorker* worker = new ServerWorker( socketHandle );

    worker->moveToThread( workerThread );

    connect( workerThread, &QThread::started,
             worker, &ServerWorker::initConnection );
    connect( worker, &ServerWorker::connectionClosed,
             workerThread, &QThread::quit );

    // Make sure the thread will be deleted
    connect( workerThread, &QThread::finished,
             worker, &ServerWorker::deleteLater );
    connect( workerThread, &QThread::finished,
             workerThread, &QThread::deleteLater );

    // public signals/slots, forwarding from/to worker
    connect( worker, &ServerWorker::registerToEvents,
             this, &Server::registerToEvents );
    connect( worker, &ServerWorker::receivedSizeHints,
             this, &Server::receivedSizeHints );
    connect( this, &Server::_pixelStreamerClosed,
             worker, &ServerWorker::closeConnection );
    connect( this, &Server::_eventRegistrationReply,
             worker, &ServerWorker::replyToEventRegistration );

    // Commands
    connect( worker, &ServerWorker::receivedCommand,
             &_impl->commandHandler, &CommandHandler::process );

    // PixelStreamDispatcher
    connect( worker, &ServerWorker::addStreamSource,
             &_impl->pixelStreamDispatcher, &FrameDispatcher::addSource );
    connect( worker,
             &ServerWorker::receivedSegment,
             &_impl->pixelStreamDispatcher,
             &FrameDispatcher::processSegment );
    connect( worker,
             &ServerWorker::receivedFrameFinished,
             &_impl->pixelStreamDispatcher,
             &FrameDispatcher::processFrameFinished );
    connect( worker,
             &ServerWorker::removeStreamSource,
             &_impl->pixelStreamDispatcher,
             &FrameDispatcher::removeSource );

    workerThread->start();
}

}
