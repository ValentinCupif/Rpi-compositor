/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwaylandcompositor.h"
#include "qwaylandsurface.h"
#include "qwaylandsurfaceitem.h"

#include <QGuiApplication>
#include <QTimer>
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

#include <QQmlContext>

#include <QQuickItem>
#include <QQuickView>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <iostream>
#include <sstream>

using namespace std;

#define CLEF 12345
typedef struct
{
    int posX;
    int posY;
}shmb;

class QmlCompositor : public QQuickView, public QWaylandCompositor
{
    Q_OBJECT
    Q_PROPERTY(QWaylandSurface* fullscreenSurface READ fullscreenSurface WRITE setFullscreenSurface NOTIFY fullscreenSurfaceChanged)

public:
    QmlCompositor()
        : QWaylandCompositor(this)
        , m_fullscreenSurface(0)
    {
        enableSubSurfaceExtension();
        setSource(QUrl("qrc:/qml/main.qml"));
        setResizeMode(QQuickView::SizeRootObjectToView);
        QSurfaceFormat format;
        format.setDepthBufferSize(16);
        format.setAlphaBufferSize(8);
        this->setFormat(format);
        this->setClearBeforeRendering(true);
        this->setColor(QColor(Qt::transparent));
        winId();

	connect(this, SIGNAL(frameSwapped()), this, SLOT(frameSwappedSlot()));
    }

    QWaylandSurface *fullscreenSurface() const
    {
        return m_fullscreenSurface;
    }
    QQuickItem *_rootObject;

signals:
    void windowAdded(QVariant window);
    void windowDestroyed(QVariant window);
    void windowResized(QVariant window);
    void setPos(QVariant posX,QVariant posY);

    void fullscreenSurfaceChanged();

public slots:
    void destroyWindow(QVariant window) {
        qvariant_cast<QObject *>(window)->deleteLater();
    }

    void destroyClientForWindow(QVariant window) {
        QWaylandSurface *surface = qobject_cast<QWaylandSurfaceItem *>(qvariant_cast<QObject *>(window))->surface();
        destroyClientForSurface(surface);
    }

    void setFullscreenSurface(QWaylandSurface *surface) {
        if (surface == m_fullscreenSurface)
            return;
        m_fullscreenSurface = surface;
        emit fullscreenSurfaceChanged();
    }

private slots:
    void surfaceMapped() {
        QWaylandSurface *surface = qobject_cast<QWaylandSurface *>(sender());
        if (!surface->hasShellSurface())
            return;
        int _posX=0;
        int _posY=0;
        QWaylandSurfaceItem *item = surface->surfaceItem();
        if (!item)
        {
            item = new QWaylandSurfaceItem(surface, rootObject());


            int mem_ID_B;
            void* ptr_shmb;

            shmb Data_B;

            if ((mem_ID_B = shmget(CLEF, sizeof(Data_B), 0444)) < 0)
            {
                perror("shmget");
                exit(1);
            }
            if ((ptr_shmb = shmat(mem_ID_B, NULL, 0)) == (void*) -1)
            {
                perror("shmat");
            }

                _posX = ((shmb*)ptr_shmb)->posX;
                _posY = ((shmb*)ptr_shmb)->posY;
            shmdt(ptr_shmb);
            emit setPos(QVariant::fromValue(_posX),QVariant::fromValue(_posY));


        }

        item->setTouchEventsEnabled(true);
        item->takeFocus();


        emit windowAdded(QVariant::fromValue(static_cast<QQuickItem *>(item)));
    }
    void surfaceUnmapped() {
        QWaylandSurface *surface = qobject_cast<QWaylandSurface *>(sender());
        if (surface == m_fullscreenSurface)
            m_fullscreenSurface = 0;
        QQuickItem *item = surface->surfaceItem();
        emit windowDestroyed(QVariant::fromValue(item));
    }

    void surfaceDestroyed(QObject *object) {
        QWaylandSurface *surface = static_cast<QWaylandSurface *>(object);
        if (surface == m_fullscreenSurface)
            m_fullscreenSurface = 0;
        QQuickItem *item = surface->surfaceItem();
        emit windowDestroyed(QVariant::fromValue(item));
    }

    void frameSwappedSlot() {
        frameFinished(m_fullscreenSurface);
    }

protected:
    void resizeEvent(QResizeEvent *event)
    {
        QQuickView::resizeEvent(event);
        QWaylandCompositor::setOutputGeometry(QRect(0, 0, width(), height()));
    }

    void surfaceCreated(QWaylandSurface *surface) {
        connect(surface, SIGNAL(destroyed(QObject *)), this, SLOT(surfaceDestroyed(QObject *)));
        connect(surface, SIGNAL(mapped()), this, SLOT(surfaceMapped()));
        connect(surface,SIGNAL(unmapped()), this,SLOT(surfaceUnmapped()));
    }

private:
    QWaylandSurface *m_fullscreenSurface;
};

int main(int argc, char *argv[])
{
    int height;
    int width;
    QGuiApplication app(argc, argv);
    if(argc<3)
    {
        qWarning("You need to set the size of the window.");
        qWarning("the size by default will be set to 1280x720");
        width = 1280;
        height= 720;
    }
    else
    {
        istringstream in(argv[1]);
        istringstream in2(argv[2]);
        int i;
        int j;
        if (!(in >> i && in.eof()) || !(in2 >> j && in2.eof()))
        {
            qWarning("You need to set a number.");
            qWarning("eg: compositor 1280 720");
            return 0;

        }else
        {
            width = atoi(argv[1]);
            height= atoi(argv[2]);
        }
    }



    QmlCompositor compositor;
    compositor.setTitle(QLatin1String("QML Compositor"));
    compositor.setGeometry(0, 0,width,height );
    compositor.show();

    compositor.rootContext()->setContextProperty("compositor", &compositor);


    QObject::connect(&compositor, SIGNAL(windowAdded(QVariant)), compositor.rootObject(), SLOT(windowAdded(QVariant)));
    QObject::connect(&compositor, SIGNAL(windowDestroyed(QVariant)), compositor.rootObject(), SLOT(windowDestroyed(QVariant)));
    QObject::connect(&compositor, SIGNAL(windowResized(QVariant)), compositor.rootObject(), SLOT(windowResized(QVariant)));
    QObject::connect(&compositor, SIGNAL(setPos(QVariant,QVariant)), compositor.rootObject(), SLOT(setPos(QVariant,QVariant)));


    return app.exec();
}



#include "main.moc"
