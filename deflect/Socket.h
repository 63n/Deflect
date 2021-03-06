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

#ifndef DEFLECT_SOCKET_H
#define DEFLECT_SOCKET_H

#ifdef _WIN32
typedef __int32 int32_t;
#endif

#include <deflect/api.h>
#include <deflect/types.h>

#include <string>

#include <QByteArray>
#include <QMutex>
#include <QObject>

class QTcpSocket;

namespace deflect
{

/**
 * Represent a communication Socket for the Stream Library.
 */
class Socket : public QObject
{
    Q_OBJECT

public:
    /** The default communication port */
    static const unsigned short defaultPortNumber;

    /**
     * Construct a Socket and connect to host.
     * @param hostname The target host (IP address or hostname)
     * @param port The target port
     */
    DEFLECT_API Socket( const std::string& hostname,
                        unsigned short port = defaultPortNumber );

    /** Destruct a Socket, disconnecting from host. */
    DEFLECT_API ~Socket();

    /** Is the Socket connected */
    DEFLECT_API bool isConnected() const;

    /**
     * Is there a pending message
     * @param messageSize Minimum size of the message
     */
    bool hasMessage( const size_t messageSize = 0 ) const;

    /**
     * Get the FileDescriptor for the Socket (for use by poll())
     * @return The file descriptor if available, otherwise return -1.
     */
    int getFileDescriptor() const;

    /**
     * Send a message.
     * @param messageHeader The message header
     * @param message The message data
     * @return true if the message could be sent, false otherwise
     */
    bool send( const MessageHeader& messageHeader, const QByteArray& message );

    /**
     * Receive a message.
     * @param messageHeader The received message header
     * @param message The received message data
     * @return true if a message could be received, false otherwise
     */
    bool receive( MessageHeader& messageHeader, QByteArray& message );

    /** Get the protocol version of the remote host */
    int32_t getRemoteProtocolVersion() const;

signals:
    /** Signal that the socket has been disconnected. */
    void disconnected();

private:
    QTcpSocket* _socket;
    int32_t _remoteProtocolVersion;
    mutable QMutex _socketMutex;

    bool _connect( const std::string &hostname, const unsigned short port );
    bool _checkProtocolVersion();

    bool _receiveHeader( MessageHeader& messageHeader );
};

}

#endif
