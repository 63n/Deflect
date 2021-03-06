/*********************************************************************/
/* Copyright (c) 2015, EPFL/Blue Brain Project                       */
/*                     Daniel.Nachbaur <daniel.nachbaur@epfl.ch>     */
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

#ifndef QMLSTREAMERIMPL_H
#define QMLSTREAMERIMPL_H

#include <QTimer>
#include <QWindow>

#include "QmlStreamer.h"
#include "../SizeHints.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLContext)
QT_FORWARD_DECLARE_CLASS(QOpenGLFramebufferObject)
QT_FORWARD_DECLARE_CLASS(QOffscreenSurface)
QT_FORWARD_DECLARE_CLASS(QQuickRenderControl)
QT_FORWARD_DECLARE_CLASS(QQuickWindow)
QT_FORWARD_DECLARE_CLASS(QQmlEngine)
QT_FORWARD_DECLARE_CLASS(QQmlComponent)
QT_FORWARD_DECLARE_CLASS(QQuickItem)

namespace deflect
{

class Stream;

namespace qt
{

class EventReceiver;

class QmlStreamer::Impl : public QWindow
{
    Q_OBJECT

public:
    Impl( const QString& qmlFile, const std::string& streamHost,
          const std::string& streamName );

    ~Impl();

    QQuickItem* getRootItem() { return _rootItem; }
    QQmlEngine* getQmlEngine() { return _qmlEngine; }

protected:
    void resizeEvent( QResizeEvent* e ) final;
    void mousePressEvent( QMouseEvent* e ) final;
    void mouseReleaseEvent( QMouseEvent* e ) final;

private slots:
    bool _setupRootItem();

    void _createFbo();
    void _destroyFbo();
    void _render();
    void _requestUpdate();

    void _onPressed( double, double );
    void _onReleased( double, double );
    void _onMoved( double, double );
    void _onResized( double, double );
    void _onWheeled( double, double, double );

private:
    std::string _getDeflectStreamName() const;
    bool _setupDeflectStream();
    void _updateSizes( const QSize& size );

    QOpenGLContext* _context;
    QOffscreenSurface* _offscreenSurface;
    QQuickRenderControl* _renderControl;
    QQuickWindow* _quickWindow;
    QQmlEngine* _qmlEngine;
    QQmlComponent* _qmlComponent;
    QQuickItem* _rootItem;
    QOpenGLFramebufferObject* _fbo;
    QTimer _updateTimer;

    Stream* _stream;
    EventReceiver* _eventHandler;
    bool _streaming;
    const std::string _streamHost;
    const std::string _streamName;
    SizeHints _sizeHints;
};

}
}

#endif
