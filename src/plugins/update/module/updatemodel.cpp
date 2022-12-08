// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <org_freedesktop_hostname1.h>

#include "updatemodel.h"
#include "window/utils.h"
#include "modules/systeminfo/systeminfomodel.h"

namespace dcc {
namespace update {

DownloadInfo::DownloadInfo(const qlonglong &downloadSize, const QList<AppUpdateInfo> &appInfos, QObject *parent)
    : QObject(parent)
    , m_downloadSize(downloadSize)
    , m_downloadProgress(0)
    , m_appInfos(appInfos)
{

}

DownloadInfo::DownloadInfo(QObject *parent)
    : QObject(parent)
    , m_downloadSize(0)
    , m_downloadProgress(0)
{

}

void DownloadInfo::setDownloadProgress(double downloadProgress)
{
    qInfo() << Q_FUNC_INFO << downloadProgress;
    if (compareDouble(m_downloadProgress, downloadProgress)) {
        m_downloadProgress = downloadProgress;
        Q_EMIT downloadProgressChanged(downloadProgress);
    }
}

void DownloadInfo::setDownloadSize(qlonglong size)
{
    qInfo() << Q_FUNC_INFO << size;
    if (size == m_downloadSize)
        return;

    m_downloadSize = size;
    Q_EMIT downloadSizeChanged(size);
}

UpdateModel::UpdateModel(QObject *parent)
    : QObject(parent)
    , m_status(UpdatesStatus::Default)
    , m_systemUpdateInfo(nullptr)
    , m_safeUpdateInfo(nullptr)
    , m_unknownUpdateInfo(nullptr)
    , m_updateProgress(0.0)
    , m_upgradeProgress(0.0)
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    , m_sourceCheck(false)
#endif
    , m_lowBattery(false)
    , m_netselectExist(false)
    , m_autoCleanCache(false)
    , m_autoDownloadUpdates(false)
    , m_autoInstallUpdates(false)
    , m_autoInstallUpdateType(0)
    , m_updateMode(0)
    , m_autoCheckSecureUpdates(false)
    , m_autoCheckSystemUpdates(false)
    , m_autoCheckAppUpdates(false)
    , m_autoCheckThirdPartyUpdates(false)
    , m_updateNotify(false)
    , m_smartMirrorSwitch(false)
    , m_mirrorId(QString())
    , m_bRecoverBackingUp(false)
    , m_bRecoverConfigValid(false)
    , m_bRecoverRestoring(false)
    , m_systemVersionInfo(QString())
    , m_bSystemActivation(UiActiveState::Unknown)
    , m_lastCheckUpdateTime(QString())
    , m_autoCheckUpdateCircle(0)
    , m_testingChannelServer(QString())
    , m_testingChannelStatus(TestingChannelStatus::Hidden)
    , m_isUpdatablePackages(false)
{
    qRegisterMetaType<TestingChannelStatus>("TestingChannelStatus");
}

UpdateModel::~UpdateModel()
{
    deleteUpdateInfo(m_systemUpdateInfo);
    deleteUpdateInfo(m_safeUpdateInfo);
    deleteUpdateInfo(m_unknownUpdateInfo);
}

void UpdateModel::setMirrorInfos(const MirrorInfoList &list)
{
    m_mirrorList = list;
}

void UpdateModel::setDefaultMirror(const QString &mirrorId)
{
    if (mirrorId == "")
        return;
    m_mirrorId = mirrorId;

    QList<MirrorInfo>::iterator it = m_mirrorList.begin();
    for (; it != m_mirrorList.end(); ++it) {
        if ((*it).m_id == mirrorId) {
            Q_EMIT defaultMirrorChanged(*it);
        }
    }
}

DownloadInfo *UpdateModel::downloadInfo()
{
    return &m_downloadInfo;
}

UpdateItemInfo *UpdateModel::systemDownloadInfo() const
{
    return m_systemUpdateInfo;
}

UpdateItemInfo *UpdateModel::safeDownloadInfo() const
{
    return m_safeUpdateInfo;
}

UpdateItemInfo *UpdateModel::unknownDownloadInfo() const
{
    return m_unknownUpdateInfo;
}

QMap<ClassifyUpdateType, UpdateItemInfo *>  UpdateModel::allDownloadInfo() const
{
    return m_allUpdateInfos;
}

void UpdateModel::setSystemDownloadInfo(UpdateItemInfo *updateItemInfo)
{
    deleteUpdateInfo(m_systemUpdateInfo);

    m_systemUpdateInfo = updateItemInfo;
    connect(m_systemUpdateInfo, &UpdateItemInfo::downloadSizeChanged, this, &UpdateModel::systemUpdateDownloadSizeChanged);

    Q_EMIT systemUpdateInfoChanged(updateItemInfo);
}

void UpdateModel::setSafeDownloadInfo(UpdateItemInfo *updateItemInfo)
{
    deleteUpdateInfo(m_safeUpdateInfo);
    m_safeUpdateInfo = updateItemInfo;
    connect(m_safeUpdateInfo, &UpdateItemInfo::downloadSizeChanged, this, &UpdateModel::safeUpdateDownloadSizeChanged);

    Q_EMIT safeUpdateInfoChanged(updateItemInfo);
}

void UpdateModel::setUnknownDownloadInfo(UpdateItemInfo *updateItemInfo)
{
    deleteUpdateInfo(m_unknownUpdateInfo);
    m_unknownUpdateInfo = updateItemInfo;
    connect(m_unknownUpdateInfo, &UpdateItemInfo::downloadSizeChanged, this, &UpdateModel::unknownUpdateDownloadSizeChanged);

    Q_EMIT unknownUpdateInfoChanged(updateItemInfo);
}

void UpdateModel::setAllDownloadInfo(QMap<ClassifyUpdateType, UpdateItemInfo *> &allUpdateInfoInfo)
{
    m_allUpdateInfos = allUpdateInfoInfo;

    setSystemDownloadInfo(allUpdateInfoInfo.value(ClassifyUpdateType::SystemUpdate));
    setSafeDownloadInfo(allUpdateInfoInfo.value(ClassifyUpdateType::SecurityUpdate));
    setUnknownDownloadInfo(allUpdateInfoInfo.value(ClassifyUpdateType::UnknownUpdate));
}

QMap<QString, int> UpdateModel::mirrorSpeedInfo() const
{
    return m_mirrorSpeedInfo;
}

void UpdateModel::setMirrorSpeedInfo(const QMap<QString, int> &mirrorSpeedInfo)
{
    m_mirrorSpeedInfo = mirrorSpeedInfo;

    if (mirrorSpeedInfo.keys().length())
        Q_EMIT mirrorSpeedInfoAvailable(mirrorSpeedInfo);
}

bool UpdateModel::lowBattery() const
{
    return m_lowBattery;
}

void UpdateModel::setLowBattery(bool lowBattery)
{
    if (lowBattery != m_lowBattery) {
        m_lowBattery = lowBattery;
        Q_EMIT lowBatteryChanged(lowBattery);
    }
}

bool UpdateModel::autoDownloadUpdates() const
{
    return m_autoDownloadUpdates;
}

void UpdateModel::setAutoDownloadUpdates(bool autoDownloadUpdates)
{
    if (m_autoDownloadUpdates != autoDownloadUpdates) {
        m_autoDownloadUpdates = autoDownloadUpdates;
        Q_EMIT autoDownloadUpdatesChanged(autoDownloadUpdates);
    }
}

MirrorInfo UpdateModel::defaultMirror() const
{
    QList<MirrorInfo>::const_iterator it = m_mirrorList.begin();
    for (; it != m_mirrorList.end(); ++it) {
        if ((*it).m_id == m_mirrorId) {
            return *it;
        }
    }

    return m_mirrorList.at(0);
}

UpdatesStatus UpdateModel::status() const
{
    return m_status;
}

void UpdateModel::setStatus(const UpdatesStatus &status)
{
    if (m_status != status) {
        m_status = status;
        Q_EMIT statusChanged(status);
    }
}

void UpdateModel::setStatus(const UpdatesStatus &status, int line)
{
    qDebug() << " from work set status : " << status << " , set place in work line : " << line;
    setStatus(status);
}

double UpdateModel::upgradeProgress() const
{
    return m_upgradeProgress;
}

void UpdateModel::setUpgradeProgress(double upgradeProgress)
{
    if (compareDouble(m_upgradeProgress, upgradeProgress)) {
        m_upgradeProgress = upgradeProgress;
        Q_EMIT upgradeProgressChanged(upgradeProgress);
    }
}

bool UpdateModel::autoCleanCache() const
{
    return m_autoCleanCache;
}

void UpdateModel::setAutoCleanCache(bool autoCleanCache)
{
    if (m_autoCleanCache == autoCleanCache)
        return;

    m_autoCleanCache = autoCleanCache;
    Q_EMIT autoCleanCacheChanged(autoCleanCache);
}

double UpdateModel::updateProgress() const
{
    return m_updateProgress;
}

void UpdateModel::setUpdateProgress(double updateProgress)
{
    if (compareDouble(m_updateProgress, updateProgress)) {
        m_updateProgress = updateProgress;
        Q_EMIT updateProgressChanged(updateProgress);
    }
}

bool UpdateModel::netselectExist() const
{
    return m_netselectExist;
}

void UpdateModel::setNetselectExist(bool netselectExist)
{
    if (m_netselectExist == netselectExist) {
        return;
    }

    m_netselectExist = netselectExist;

    Q_EMIT netselectExistChanged(netselectExist);
}

void UpdateModel::setUpdateMode(quint64 updateMode)
{
    qDebug() << Q_FUNC_INFO << "get UpdateMode from dbus:" << updateMode;

    if (m_updateMode == updateMode)
        return;

    m_updateMode = updateMode;

    setUpdateItemEnabled();
    setAutoCheckSystemUpdates(m_updateMode & ClassifyUpdateType::SystemUpdate);
    setAutoCheckAppUpdates(m_updateMode & ClassifyUpdateType::AppStoreUpdate);
    setAutoCheckSecureUpdates(m_updateMode & ClassifyUpdateType::SecurityUpdate);
    setAutoCheckThirdPartyUpdates(m_updateMode & ClassifyUpdateType::UnknownUpdate);

    Q_EMIT updateModeChanged(m_updateMode);

}

void UpdateModel::setAutoCheckSecureUpdates(bool autoCheckSecureUpdates)
{
    if (autoCheckSecureUpdates == m_autoCheckSecureUpdates)
        return;

    m_autoCheckSecureUpdates = autoCheckSecureUpdates;

    Q_EMIT autoCheckSecureUpdatesChanged(autoCheckSecureUpdates);
}

void UpdateModel::setAutoCheckSystemUpdates(bool autoCheckSystemUpdates)
{
    if (autoCheckSystemUpdates == m_autoCheckSystemUpdates)
        return;

    m_autoCheckSystemUpdates = autoCheckSystemUpdates;

    Q_EMIT autoCheckSystemUpdatesChanged(autoCheckSystemUpdates);
}

void UpdateModel::setAutoCheckAppUpdates(bool autoCheckAppUpdates)
{
    if (autoCheckAppUpdates == m_autoCheckAppUpdates)
        return;

    m_autoCheckAppUpdates = autoCheckAppUpdates;

    Q_EMIT autoCheckAppUpdatesChanged(autoCheckAppUpdates);
}

void UpdateModel::setSmartMirrorSwitch(bool smartMirrorSwitch)
{
    if (m_smartMirrorSwitch == smartMirrorSwitch) return;

    m_smartMirrorSwitch = smartMirrorSwitch;

    Q_EMIT smartMirrorSwitchChanged(smartMirrorSwitch);
}

void UpdateModel::setRecoverBackingUp(bool recoverBackingUp)
{
    if (m_bRecoverBackingUp == recoverBackingUp)
        return;

    m_bRecoverBackingUp = recoverBackingUp;

    Q_EMIT recoverBackingUpChanged(recoverBackingUp);
}

void UpdateModel::setRecoverConfigValid(bool recoverConfigValid)
{
    if (m_bRecoverConfigValid == recoverConfigValid)
        return;

    m_bRecoverConfigValid = recoverConfigValid;

    Q_EMIT recoverConfigValidChanged(recoverConfigValid);
}

void UpdateModel::setRecoverRestoring(bool recoverRestoring)
{
    if (m_bRecoverRestoring == recoverRestoring)
        return;

    m_bRecoverRestoring = recoverRestoring;

    Q_EMIT recoverRestoringChanged(recoverRestoring);
}

void UpdateModel::setSystemVersionInfo(const QString &systemVersionInfo)
{
    if (m_systemVersionInfo == systemVersionInfo)
        return;

    m_systemVersionInfo = systemVersionInfo;

    Q_EMIT systemVersionChanged(systemVersionInfo);
}

void UpdateModel::setSystemActivation(const UiActiveState &systemactivation)
{
    if (m_bSystemActivation == systemactivation) {
        return;
    }
    m_bSystemActivation = systemactivation;

    Q_EMIT systemActivationChanged(systemactivation);
}

void UpdateModel::isUpdatablePackages(bool isUpdatablePackages)
{
    if (m_isUpdatablePackages == isUpdatablePackages)
        return;

    m_isUpdatablePackages = isUpdatablePackages;
    Q_EMIT updatablePackagesChanged(isUpdatablePackages);
}

#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
bool UpdateModel::sourceCheck() const
{
    return m_sourceCheck;
}

void UpdateModel::setSourceCheck(bool sourceCheck)
{
    if (m_sourceCheck == sourceCheck)
        return;

    m_sourceCheck = sourceCheck;

    Q_EMIT sourceCheckChanged(sourceCheck);
}
#endif

void UpdateModel::setLastCheckUpdateTime(const QString &lastTime)
{
    qDebug() << "Last check time:" << lastTime;
    m_lastCheckUpdateTime = lastTime.left(QString("0000-00-00 00:00:00").size());
}

void UpdateModel::setHistoryAppInfos(const QList<AppUpdateInfo> &infos)
{
    m_historyAppInfos = infos;
}

void UpdateModel::setAutoCheckUpdateCircle(const int interval)
{
    m_autoCheckUpdateCircle = interval;
}

bool UpdateModel::enterCheckUpdate()
{
    qDebug() << "last update time:" << m_lastCheckUpdateTime << "check circle:" << m_autoCheckUpdateCircle;
    return QDateTime::fromString(m_lastCheckUpdateTime, "yyyy-MM-dd hh:mm:ss").secsTo(QDateTime::currentDateTime()) > m_autoCheckUpdateCircle * 3600;
}

void UpdateModel::setUpdateNotify(const bool notify)
{
    if (m_updateNotify == notify) {
        return;
    }

    m_updateNotify = notify;

    Q_EMIT updateNotifyChanged(notify);
}

void UpdateModel::deleteUpdateInfo(UpdateItemInfo *updateItemInfo)
{
    if (updateItemInfo != nullptr) {
        updateItemInfo->deleteLater();
    }
}

bool UpdateModel::getAutoInstallUpdates() const
{
    return m_autoInstallUpdates;
}

void UpdateModel::setAutoInstallUpdates(bool autoInstallUpdates)
{
    if (m_autoInstallUpdates != autoInstallUpdates) {
        m_autoInstallUpdates = autoInstallUpdates;
        Q_EMIT autoInstallUpdatesChanged(autoInstallUpdates);
    }
}

quint64 UpdateModel::getAutoInstallUpdateType() const
{
    return m_autoInstallUpdateType;
}

void UpdateModel::setAutoInstallUpdateType(const quint64 &autoInstallUpdateType)
{
    if (m_autoInstallUpdateType != autoInstallUpdateType) {
        m_autoInstallUpdateType = autoInstallUpdateType;
        Q_EMIT autoInstallUpdateTypeChanged(autoInstallUpdateType);
    }
}

QMap<ClassifyUpdateType, UpdateItemInfo *> UpdateModel::getAllUpdateInfos() const
{
    return m_allUpdateInfos;
}

void UpdateModel::setAllUpdateInfos(const QMap<ClassifyUpdateType, UpdateItemInfo *> &allUpdateInfos)
{
    m_allUpdateInfos = allUpdateInfos;
}

UpdateJobErrorMessage UpdateModel::getSystemUpdateJobError() const
{
    return m_systemUpdateJobError;
}

void UpdateModel::setSystemUpdateJobError(const UpdateJobErrorMessage &systemUpdateJobError)
{
    m_systemUpdateJobError = systemUpdateJobError;
}

UpdateJobErrorMessage UpdateModel::getSafeUpdateJobError() const
{
    return m_safeUpdateJobError;
}

void UpdateModel::setSafeUpdateJobError(const UpdateJobErrorMessage &safeUpdateJobError)
{
    m_safeUpdateJobError = safeUpdateJobError;
}

UpdateJobErrorMessage UpdateModel::getUnknownUpdateJobError() const
{
    return m_UnknownUpdateJobError;
}

void UpdateModel::setUnknownUpdateJobError(const UpdateJobErrorMessage &UnknownUpdateJobError)
{
    m_UnknownUpdateJobError = UnknownUpdateJobError;
}

void UpdateModel::setClassifyUpdateJonError(UpdateErrorType errorType)
{
    qInfo() << Q_FUNC_INFO << errorType;
    m_lastError = errorType;

    Q_EMIT classifyUpdateJobErrorChanged(errorType);
}

QMap<ClassifyUpdateType, UpdateErrorType> UpdateModel::getUpdateErrorTypeMap() const
{
    return m_updateErrorTypeMap;
}

bool UpdateModel::getAutoCheckThirdPartyUpdates() const
{
    return m_autoCheckThirdPartyUpdates;
}

void UpdateModel::setAutoCheckThirdPartyUpdates(bool autoCheckThirdPartyUpdates)
{
    if (m_autoCheckThirdPartyUpdates != autoCheckThirdPartyUpdates) {
        m_autoCheckThirdPartyUpdates = autoCheckThirdPartyUpdates;
        Q_EMIT autoCheckThirdPartyUpdatesChanged(m_autoCheckThirdPartyUpdates);
    }

}

QString UpdateModel::getHostName() const
{
    org::freedesktop::hostname1 hostnameInter("org.freedesktop.hostname1",
                                              "/org/freedesktop/hostname1",
                                              QDBusConnection::systemBus());

    return hostnameInter.staticHostname();
}

QString UpdateModel::getMachineID() const
{
    QProcess process;
    auto args = QStringList();
    args.append("-c");
    args.append("eval `apt-config shell Token Acquire::SmartMirrors::Token`; echo $Token");
    process.start("sh", args);
    process.waitForFinished();
    const auto token = QString(process.readAllStandardOutput());
    const auto list = token.split(";");
    for (const auto &line: list) {
        const auto key = line.section("=", 0, 0);
        if (key == "i"){
            const auto value = line.section("=", 1);
            return value;
        }
    }
    return "";
}

UpdateModel::TestingChannelStatus UpdateModel::getTestingChannelStatus() const
{
    return m_testingChannelStatus;
}
void UpdateModel::setTestingChannelStatus(const TestingChannelStatus status)
{
    m_testingChannelStatus = status;
    Q_EMIT testingChannelStatusChanged(m_testingChannelStatus);
}

QString UpdateModel::getTestingChannelServer() const
{
    return m_testingChannelServer;
}
void UpdateModel::setTestingChannelServer(const QString server)
{
    m_testingChannelServer = server;
}

QUrl UpdateModel::getTestingChannelJoinURL() const
{
    auto u = QUrl(getTestingChannelServer() + "/internal-testing");
    auto query = QUrlQuery(u.query());
    query.addQueryItem("h", getHostName());
    query.addQueryItem("m", getMachineID());
    query.addQueryItem("v", DSysInfo::minorVersion());
    u.setQuery(query);
    return u;
}

void UpdateModel::setCanExitTestingChannel(const bool can)
{
    Q_EMIT canExitTestingChannelChanged(can);
}

void UpdateModel::onUpdatePropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    Q_UNUSED(interfaceName)
    Q_UNUSED(invalidatedProperties)

    if (changedProperties.contains("IdleDownloadConfig")) {
        setIdleDownloadConfig(IdleDownloadConfig::toConfig(changedProperties.value("IdleDownloadConfig").toByteArray()));
    }
}

void UpdateModel::setIdleDownloadConfig(const IdleDownloadConfig &config)
{
    if (m_idleDownloadConfig == config)
        return;

    m_idleDownloadConfig = config;
    Q_EMIT idleDownloadConfigChanged();
}

qlonglong UpdateModel::downloadSize() const
{
    qlonglong downloadSize = 0;
    if (m_updateMode & ClassifyUpdateType::SystemUpdate && m_systemUpdateInfo)
        downloadSize += m_systemUpdateInfo->downloadSize();
    if (m_updateMode & ClassifyUpdateType::SecurityUpdate && m_safeUpdateInfo)
        downloadSize += m_safeUpdateInfo->downloadSize();
    if (m_updateMode & ClassifyUpdateType::UnknownUpdate && m_unknownUpdateInfo)
        downloadSize += m_unknownUpdateInfo->downloadSize();

    return downloadSize;
}

void UpdateModel::setUpdateItemEnabled()
{
    if (m_systemUpdateInfo)
        m_systemUpdateInfo->setUpdateModeEnabled(m_updateMode & ClassifyUpdateType::SystemUpdate);
    if (m_safeUpdateInfo)
        m_safeUpdateInfo->setUpdateModeEnabled(m_updateMode & ClassifyUpdateType::SecurityUpdate);
    if (m_unknownUpdateInfo)
        m_unknownUpdateInfo->setUpdateModeEnabled(m_updateMode & ClassifyUpdateType::UnknownUpdate);
}

}
}
