/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include <QtNetwork/private/qnetworkinformation_p.h>

#include "wrapper/androidconnectivitymanager.h"

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(lcNetInfoAndroid)
Q_LOGGING_CATEGORY(lcNetInfoAndroid, "qt.network.info.android");

static QString backendName() {
    return QString::fromUtf16(QNetworkInformationBackend::PluginNames
                                      [QNetworkInformationBackend::PluginNamesAndroidIndex]);
}

class QAndroidNetworkInformationBackend : public QNetworkInformationBackend
{
    Q_OBJECT
public:
    QAndroidNetworkInformationBackend();
    ~QAndroidNetworkInformationBackend() { m_valid = false; }

    QString name() const override { return backendName(); }
    QNetworkInformation::Features featuresSupported() const override
    {
        return featuresSupportedStatic();
    }

    static QNetworkInformation::Features featuresSupportedStatic()
    {
        using Feature = QNetworkInformation::Feature;
        return QNetworkInformation::Features(Feature::Reachability | Feature::CaptivePortal
                                             | Feature::TransportMedium);
    }

    bool isValid() { return m_valid; }

private:
    Q_DISABLE_COPY_MOVE(QAndroidNetworkInformationBackend);

    void updateConnectivity(AndroidConnectivityManager::AndroidConnectivity connectivity);
    void updateTransportMedium(AndroidConnectivityManager::AndroidTransport transport);

    bool m_valid = false;
};

class QAndroidNetworkInformationBackendFactory : public QNetworkInformationBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QNetworkInformationBackendFactory_iid)
    Q_INTERFACES(QNetworkInformationBackendFactory)
public:
    QAndroidNetworkInformationBackendFactory() = default;
    ~QAndroidNetworkInformationBackendFactory() = default;
    QString name() const override { return backendName(); }
    QNetworkInformation::Features featuresSupported() const override
    {
        return QAndroidNetworkInformationBackend::featuresSupportedStatic();
    }

    QNetworkInformationBackend *
    create(QNetworkInformation::Features requiredFeatures) const override
    {
        if ((requiredFeatures & featuresSupported()) != requiredFeatures)
            return nullptr;
        auto backend = new QAndroidNetworkInformationBackend();
        if (!backend->isValid())
            delete std::exchange(backend, nullptr);
        return backend;
    }

private:
    Q_DISABLE_COPY_MOVE(QAndroidNetworkInformationBackendFactory);
};

QAndroidNetworkInformationBackend::QAndroidNetworkInformationBackend()
{
    auto conman = AndroidConnectivityManager::getInstance();
    if (!conman)
        return;
    m_valid = true;
    setReachability(QNetworkInformation::Reachability::Unknown);
    connect(conman, &AndroidConnectivityManager::connectivityChanged, this,
            &QAndroidNetworkInformationBackend::updateConnectivity);

    connect(conman, &AndroidConnectivityManager::captivePortalChanged, this,
            &QAndroidNetworkInformationBackend::setBehindCaptivePortal);

    connect(conman, &AndroidConnectivityManager::transportMediumChanged, this,
            &QAndroidNetworkInformationBackend::updateTransportMedium);
}

void QAndroidNetworkInformationBackend::updateConnectivity(
        AndroidConnectivityManager::AndroidConnectivity connectivity)
{
    using AndroidConnectivity = AndroidConnectivityManager::AndroidConnectivity;
    static const auto mapState = [](AndroidConnectivity state) {
        switch (state) {
        case AndroidConnectivity::Connected:
            return QNetworkInformation::Reachability::Online;
        case AndroidConnectivity::Disconnected:
            return QNetworkInformation::Reachability::Disconnected;
        case AndroidConnectivity::Unknown:
        default:
            return QNetworkInformation::Reachability::Unknown;
        }
    };

    setReachability(mapState(connectivity));
}

void QAndroidNetworkInformationBackend::updateTransportMedium(
        AndroidConnectivityManager::AndroidTransport transport)
{
    using AndroidTransport = AndroidConnectivityManager::AndroidTransport;
    using TransportMedium = QNetworkInformation::TransportMedium;
    static const auto mapTransport = [](AndroidTransport state) -> TransportMedium {
        switch (state) {
        case AndroidTransport::Cellular:
            return TransportMedium::Cellular;
        case AndroidTransport::WiFi:
            return TransportMedium::WiFi;
        case AndroidTransport::Bluetooth:
            return TransportMedium::Bluetooth;
        case AndroidTransport::Ethernet:
            return TransportMedium::Ethernet;
        // These are not covered yet (but may be in the future)
        case AndroidTransport::Usb:
        case AndroidTransport::LoWPAN:
        case AndroidTransport::WiFiAware:
        case AndroidTransport::Unknown:
            return TransportMedium::Unknown;
        }
    };

    setTransportMedium(mapTransport(transport));
}

QT_END_NAMESPACE

#include "qandroidnetworkinformationbackend.moc"
