/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QBACKINGSTORE_COCOA_H
#define QBACKINGSTORE_COCOA_H

#include <QtGui/private/qrasterbackingstore_p.h>

#include <private/qcore_mac_p.h>

#include <QScopedPointer>
#include "qiosurfacegraphicsbuffer.h"

#include <unordered_map>

QT_BEGIN_NAMESPACE

class QCocoaBackingStore : public QPlatformBackingStore
{
protected:
    QCocoaBackingStore(QWindow *window);
    QCFType<CGColorSpaceRef> colorSpace() const;
};

class QCALayerBackingStore : public QObject, public QCocoaBackingStore
{
    Q_OBJECT
public:
    QCALayerBackingStore(QWindow *window);
    ~QCALayerBackingStore();

    void resize(const QSize &size, const QRegion &staticContents) override;

    void beginPaint(const QRegion &region) override;
    QPaintDevice *paintDevice() override;
    void endPaint() override;

    void flush(QWindow *, const QRegion &, const QPoint &) override;
#ifndef QT_NO_OPENGL
    void composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
        QPlatformTextureList *textures, bool translucentBackground) override;
#endif

    QImage toImage() const override;
    QPlatformGraphicsBuffer *graphicsBuffer() const override;

private:
    void observeBackingPropertiesChanges();
    bool eventFilter(QObject *watched, QEvent *event) override;

    QSize m_requestedSize;
    QRegion m_paintedRegion;

    class GraphicsBuffer : public QIOSurfaceGraphicsBuffer
    {
    public:
        GraphicsBuffer(const QSize &size, qreal devicePixelRatio,
                const QPixelFormat &format, QCFType<CGColorSpaceRef> colorSpace);

        QRegion dirtyRegion; // In unscaled coordinates
        QImage *asImage();
        qreal devicePixelRatio() const { return m_devicePixelRatio; }

    private:
        qreal m_devicePixelRatio;
        QImage m_image;
    };

    void ensureBackBuffer();
    bool recreateBackBufferIfNeeded();
    bool prepareForFlush();

    void backingPropertiesChanged();
    QMacNotificationObserver m_backingPropertiesObserver;

    std::list<std::unique_ptr<GraphicsBuffer>> m_buffers;

    void flushSubWindow(QWindow *window);
    std::unordered_map<QWindow*, std::unique_ptr<QCALayerBackingStore>> m_subWindowBackingstores;
    void windowDestroyed(QObject *object);
    bool m_clearSurfaceOnPaint = true;
};

QT_END_NAMESPACE

#endif
