/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
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

#include "Socket.h"

#include "MessageHeader.h"
#include "NetworkProtocol.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QLoggingCategory>
#include <QTcpSocket>
#include <iostream>

#define INVALID_NETWORK_PROTOCOL_VERSION   -1
#define RECEIVE_TIMEOUT_MS                 1000
#define WAIT_FOR_BYTES_WRITTEN_TIMEOUT_MS  1000

namespace deflect
{

const unsigned short Socket::defaultPortNumber = DEFAULT_PORT_NUMBER;

Socket::Socket( const std::string& hostname, const unsigned short port )
    : _socket( new QTcpSocket( ))
    , _remoteProtocolVersion( INVALID_NETWORK_PROTOCOL_VERSION )
{
    // disable warnings which occur if no QCoreApplication is present during
    // _connect(): QObject::connect: Cannot connect (null)::destroyed() to
    // QHostInfoLookupManager::waitForThreadPoolDone()
    if( !qApp )
    {
        QLoggingCategory* log = QLoggingCategory::defaultCategory();
        log->setEnabled( QtWarningMsg, false );
    }

    _connect( hostname, port );

    QObject::connect( _socket, &QTcpSocket::disconnected,
                      this, &Socket::disconnected );
}

Socket::~Socket()
{
    delete _socket;
}

bool Socket::isConnected() const
{
    return _socket->state() == QTcpSocket::ConnectedState;
}

int Socket::getFileDescriptor() const
{
    return _socket->socketDescriptor();
}

bool Socket::hasMessage( const size_t messageSize ) const
{
    QMutexLocker locker( &_socketMutex );

    // needed to 'wakeup' socket when no data was streamed for a while
    _socket->waitForReadyRead( 0 );
    return _socket->bytesAvailable() >=
            (int)(MessageHeader::serializedSize + messageSize);
}

bool Socket::send( const MessageHeader& messageHeader,
                   const QByteArray& message )
{
    QMutexLocker locker( &_socketMutex );

    // send header
    QDataStream stream( _socket );
    stream << messageHeader;
    if( stream.status() != QDataStream::Ok )
        return false;

    if( message.isEmpty( ))
        return true;

    // Send message data
    const char* data = message.constData();
    const int size = message.size();

    int sent = _socket->write( data, size );

    while( sent < size && isConnected( ))
        sent += _socket->write( data + sent, size - sent );

    // Needed in the absence of event loop, otherwise the reception is frozen.
    while( _socket->bytesToWrite() > 0 && isConnected( ))
        _socket->waitForBytesWritten();

    return sent == size;
}

bool Socket::receive( MessageHeader& messageHeader, QByteArray& message )
{
    QMutexLocker locker( &_socketMutex );

    if ( !_receiveHeader( messageHeader ))
        return false;

    // get the message
    if( messageHeader.size > 0 )
    {
        message = _socket->read( messageHeader.size );

        while( message.size() < int(messageHeader.size) )
        {
            if ( !_socket->waitForReadyRead( RECEIVE_TIMEOUT_MS ))
                return false;

            message.append( _socket->read( messageHeader.size -
                                           message.size( )));
        }
    }

    if( messageHeader.type == MESSAGE_TYPE_QUIT )
    {
        _socket->disconnectFromHost();
        return false;
    }

    return true;
}

int32_t Socket::getRemoteProtocolVersion() const
{
    return _remoteProtocolVersion;
}

bool Socket::_receiveHeader( MessageHeader& messageHeader )
{
    while( _socket->bytesAvailable() < qint64(MessageHeader::serializedSize) )
    {
        if( !_socket->waitForReadyRead( RECEIVE_TIMEOUT_MS ))
            return false;
    }

    QDataStream stream( _socket );
    stream >> messageHeader;

    return stream.status() == QDataStream::Ok;
}

bool Socket::_connect( const std::string& hostname, const unsigned short port )
{
    // make sure we're disconnected
    _socket->disconnectFromHost();

    // open connection
    _socket->connectToHost( hostname.c_str(), port );

    if( !_socket->waitForConnected( RECEIVE_TIMEOUT_MS ))
    {
        std::cerr << "could not connect to host " << hostname << ":" << port
                  << std::endl;
        return false;
    }

    // handshake
    if( _checkProtocolVersion( ))
        return true;

    std::cerr << "Protocol version check failed for host: " << hostname << ":"
              << port << std::endl;
    _socket->disconnectFromHost();
    return false;
}

bool Socket::_checkProtocolVersion()
{
    while( _socket->bytesAvailable() < qint64(sizeof(int32_t)) )
    {
        if( !_socket->waitForReadyRead( RECEIVE_TIMEOUT_MS ))
            return false;
    }

    _socket->read((char *)&_remoteProtocolVersion, sizeof(int32_t));

    if( _remoteProtocolVersion == NETWORK_PROTOCOL_VERSION )
        return true;

    std::cerr << "unsupported protocol version " << _remoteProtocolVersion
              << " != " << NETWORK_PROTOCOL_VERSION << std::endl;
    return false;
}

}
