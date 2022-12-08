// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatework.h"
#include "window/utils.h"
#include "widgets/utils.h"
#include "window/dconfigwatcher.h"

#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QApplication>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QVariant>
#include <QDBusError>

#include <vector>

Q_LOGGING_CATEGORY(DCC_UPDATE, "dcc.update")

#define MIN_NM_ACTIVE 50
#define UPDATE_PACKAGE_SIZE 0
using namespace DCC_NAMESPACE;

const QString TestingChannelPackage = "deepin-unstable-source";
const QString ChangeLogFile = "/usr/share/deepin/release-note/UpdateInfo.json";
const QString ChangeLogDic = "/usr/share/deepin/";
const QString UpdateLogTmpFile = "/tmp/deepin-update-log.json";

const int LogTypeSystem = 1;    // 系统更新
const int LogTypeSecurity = 2;  // 安全更新
const int DesktopProfessionalPlatform = 1;  // 桌面专业版
const int DesktopCommunityPlatform = 3;     // 桌面社区版
const int ServerPlatform = 6;               // 服务器版

// 系统补丁标识
const QString DDEId = "dde";

namespace dcc {
namespace update {
static int TestMirrorSpeedInternal(const QString &url, QPointer<QObject> baseObject)
{
    if (!baseObject || QCoreApplication::closingDown()) {
        return -1;
    }

    QStringList args;
    args << url << "-s" << "1";

    QProcess process;
    process.start("netselect", args);

    if (!process.waitForStarted()) {
        return 10000;
    }

    do {
        if (!baseObject || QCoreApplication::closingDown()) {
            process.kill();
            process.terminate();
            process.waitForFinished(1000);

            return -1;
        }

        if (process.waitForFinished(500))
            break;
    } while (process.state() == QProcess::Running);

    const QString output = process.readAllStandardOutput().trimmed();
    const QStringList result = output.split(' ');

    qDebug() << "speed of url" << url << "is" << result.first();

    if (!result.first().isEmpty()) {
        return result.first().toInt();
    }

    return 10000;
}

static int getPlatform()
{
    if (DCC_NAMESPACE::IsServerSystem) {
        return ServerPlatform;
    }

    if (DCC_NAMESPACE::IsCommunitySystem) {
        return DesktopCommunityPlatform;
    }

    return DesktopProfessionalPlatform;
}

UpdateWorker::UpdateWorker(UpdateModel *model, QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_checkUpdateJob(nullptr)
    , m_fixErrorJob(nullptr)
    , m_sysUpdateDownloadJob(nullptr)
    , m_safeUpdateDownloadJob(nullptr)
    , m_unknownUpdateDownloadJob(nullptr)
    , m_downloadJob(nullptr)
    , m_distUpgradeJob(nullptr)
    , m_sysUpdateInstallJob(nullptr)
    , m_safeUpdateInstallJob(nullptr)
    , m_unknownUpdateInstallJob(nullptr)
    , m_lastoreSessionHelper(nullptr)
    , m_updateInter(nullptr)
    , m_managerInter(nullptr)
    , m_powerInter(nullptr)
    , m_powerSystemInter(nullptr)
    , m_networkInter(nullptr)
    , m_smartMirrorInter(nullptr)
    , m_iconTheme(nullptr)
    , m_onBattery(true)
    , m_batteryPercentage(0.0)
    , m_batterySystemPercentage(0.0)
    , m_jobPath("")
    , m_iconThemeState("")
    , m_backupStatus(BackupStatus::NoBackup)
    , m_backingUpClassifyType(ClassifyUpdateType::Invalid)
{

}

UpdateWorker::~UpdateWorker()
{
    deleteJob(m_sysUpdateDownloadJob);
    deleteJob(m_sysUpdateInstallJob);
    deleteJob(m_safeUpdateDownloadJob);
    deleteJob(m_safeUpdateInstallJob);
    deleteJob(m_unknownUpdateDownloadJob);
    deleteJob(m_unknownUpdateInstallJob);
    deleteJob(m_checkUpdateJob);
    deleteJob(m_fixErrorJob);
}

void UpdateWorker::init()
{
    qRegisterMetaType<UpdatesStatus>("UpdatesStatus");
    qRegisterMetaType<UiActiveState>("UiActiveState");

    m_lastoreSessionHelper = new LastoressionHelper("com.deepin.LastoreSessionHelper", "/com/deepin/LastoreSessionHelper", QDBusConnection::sessionBus(), this);
    m_updateInter = new UpdateInter("com.deepin.lastore", "/com/deepin/lastore", QDBusConnection::systemBus(), this);
    m_managerInter = new ManagerInter("com.deepin.lastore", "/com/deepin/lastore", QDBusConnection::systemBus(), this);
    m_powerInter = new PowerInter("com.deepin.daemon.Power", "/com/deepin/daemon/Power", QDBusConnection::sessionBus(), this);
    m_powerSystemInter = new PowerSystemInter("com.deepin.system.Power", "/com/deepin/system/Power", QDBusConnection::systemBus(), this);
    m_networkInter = new Network("com.deepin.daemon.Network", "/com/deepin/daemon/Network", QDBusConnection::sessionBus(), this);
    m_smartMirrorInter = new SmartMirrorInter("com.deepin.lastore.Smartmirror", "/com/deepin/lastore/Smartmirror", QDBusConnection::systemBus(), this);
    m_iconTheme = new Appearance("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", QDBusConnection::sessionBus(), this);

    m_managerInter->setSync(false);
    m_updateInter->setSync(false);
    m_powerInter->setSync(false);
    m_powerSystemInter->setSync(false);
    m_lastoreSessionHelper->setSync(false);
    m_smartMirrorInter->setSync(false, false);
    m_iconTheme->setSync(false);

    QString sVersion = QString("%1 %2").arg(DSysInfo::uosProductTypeName()).arg(DSysInfo::majorVersion());
    if (!IsServerSystem)
        sVersion.append(" " + DSysInfo::uosEditionName());
    m_model->setSystemVersionInfo(sVersion);

    const auto server = valueByQSettings<QString>(DCC_CONFIG_FILES, "Testing", "Server", "");
    if (!server.isEmpty()) {
        m_model->setTestingChannelServer(server);
        if (m_managerInter->PackageExists(TestingChannelPackage)) {
            m_model->setTestingChannelStatus(UpdateModel::TestingChannelStatus::Joined);
        } else {
            m_model->setTestingChannelStatus(UpdateModel::TestingChannelStatus::NotJoined);
        }
    }

    connect(m_managerInter, &ManagerInter::JobListChanged, this, &UpdateWorker::onJobListChanged);
    connect(m_managerInter, &ManagerInter::AutoCleanChanged, m_model, &UpdateModel::setAutoCleanCache);

    connect(m_updateInter, &__Updater::AutoDownloadUpdatesChanged, m_model, &UpdateModel::setAutoDownloadUpdates);
    connect(m_updateInter, &__Updater::AutoInstallUpdatesChanged, m_model, &UpdateModel::setAutoInstallUpdates);
    connect(m_updateInter, &__Updater::AutoInstallUpdateTypeChanged, m_model, &UpdateModel::setAutoInstallUpdateType);
    connect(m_updateInter, &__Updater::MirrorSourceChanged, m_model, &UpdateModel::setDefaultMirror);
    QDBusConnection::systemBus().connect("com.deepin.lastore",
                                          "/com/deepin/lastore",
                                          "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged",
                                          m_model, SLOT(onUpdatePropertiesChanged(QString,QVariantMap,QStringList)));
    connect(m_managerInter, &ManagerInter::UpdateModeChanged, m_model, [ = ](qulonglong value) {
        m_model->setUpdateMode(value);
        m_updateInter->setSync(true);
        QMap<QString, QStringList> updatablePackages = m_updateInter->classifiedUpdatablePackages();
        m_updateInter->setSync(false);
        checkUpdatablePackages(updatablePackages);
    });
    connect(m_updateInter, &UpdateInter::UpdateNotifyChanged, m_model, &UpdateModel::setUpdateNotify);
    connect(m_updateInter, &UpdateInter::ClassifiedUpdatablePackagesChanged, this, &UpdateWorker::onClassifiedUpdatablePackagesChanged);

    connect(m_powerInter, &__Power::OnBatteryChanged, this, &UpdateWorker::setOnBattery);
    connect(m_powerInter, &__Power::BatteryPercentageChanged, this, &UpdateWorker::setBatteryPercentage);

    // connect(m_powerSystemInter, &__SystemPower::BatteryPercentageChanged, this, &UpdateWorker::setSystemBatteryPercentage);
    if (IsCommunitySystem) {
        connect(m_smartMirrorInter, &SmartMirrorInter::EnableChanged, m_model, &UpdateModel::setSmartMirrorSwitch);
        connect(m_smartMirrorInter, &SmartMirrorInter::serviceValidChanged, this, &UpdateWorker::onSmartMirrorServiceIsValid);
        connect(m_smartMirrorInter, &SmartMirrorInter::serviceStartFinished, this, [ = ] {
            QTimer::singleShot(100, this, [ = ] {
                m_model->setSmartMirrorSwitch(m_smartMirrorInter->enable());
            });
        }, Qt::UniqueConnection);
    }

    //图片主题
    connect(m_iconTheme, &Appearance::IconThemeChanged, this, &UpdateWorker::onIconThemeChanged);

#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    connect(m_lastoreSessionHelper, &LastoressionHelper::SourceCheckEnabledChanged, m_model, &UpdateModel::setSourceCheck);
#endif
}

void UpdateWorker::licenseStateChangeSlot()
{
    QFutureWatcher<void> *watcher = new QFutureWatcher<void>();
    connect(watcher, &QFutureWatcher<void>::finished, watcher, &QFutureWatcher<void>::deleteLater);

    QFuture<void> future = QtConcurrent::run(this, &UpdateWorker::getLicenseState);
    watcher->setFuture(future);
}

void UpdateWorker::getLicenseState()
{
    if (DSysInfo::DeepinDesktop == DSysInfo::deepinType()) {
        m_model->setSystemActivation(UiActiveState::Authorized);
        return;
    }
    QDBusInterface licenseInfo("com.deepin.license",
                               "/com/deepin/license/Info",
                               "com.deepin.license.Info",
                               QDBusConnection::systemBus());
    if (!licenseInfo.isValid()) {
        qDebug() << "com.deepin.license error ," << licenseInfo.lastError().name();
        return;
    }
    UiActiveState reply = static_cast<UiActiveState>(licenseInfo.property("AuthorizationState").toInt());
    qDebug() << "Authorization State:" << reply;
    m_model->setSystemActivation(reply);
}

void UpdateWorker::activate()
{
#ifndef DISABLE_SYS_UPDATE_MIRRORS
    refreshMirrors();
#endif
    m_managerInter->setSync(true);
    m_updateInter->setSync(true);
    QString checkTime;
    double interval = m_updateInter->GetCheckIntervalAndTime(checkTime);
    m_model->setLastCheckUpdateTime(checkTime);
    m_model->setAutoCheckUpdateCircle(static_cast<int>(interval));

    m_model->setAutoCleanCache(m_managerInter->autoClean());
    m_model->setAutoDownloadUpdates(m_updateInter->autoDownloadUpdates());
    m_model->setAutoInstallUpdates(m_updateInter->autoInstallUpdates());
    m_model->setAutoInstallUpdateType(m_updateInter->autoInstallUpdateType());
    m_model->setUpdateMode(m_managerInter->updateMode());
    m_model->setUpdateNotify(m_updateInter->updateNotify());

    // FIXME 使用m_updateInter->property 无法获取到属性
    QDBusInterface updaterInter("com.deepin.lastore",
                             "/com/deepin/lastore",
                             "com.deepin.lastore.Updater",
                             QDBusConnection::systemBus());
    QString config = updaterInter.property("IdleDownloadConfig").toString();
    m_model->setIdleDownloadConfig(UpdateModel::IdleDownloadConfig::toConfig(config.toLatin1()));

    if (IsCommunitySystem) {
        m_model->setSmartMirrorSwitch(m_smartMirrorInter->enable());
        onSmartMirrorServiceIsValid(m_smartMirrorInter->isValid());
    }
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    m_model->setSourceCheck(m_lastoreSessionHelper->sourceCheckEnabled());
#endif
    // setSystemBatteryPercentage(m_powerSystemInter->batteryPercentage());

#ifndef DISABLE_SYS_UPDATE_MIRRORS
    refreshMirrors();
#endif

    licenseStateChangeSlot();

    QDBusConnection::systemBus().connect("com.deepin.license", "/com/deepin/license/Info",
                                         "com.deepin.license.Info", "LicenseStateChange",
                                         this, SLOT(licenseStateChangeSlot()));

    QFutureWatcher<QMap<QString, QStringList>> *packagesWatcher = new QFutureWatcher<QMap<QString, QStringList>>();
    connect(packagesWatcher, &QFutureWatcher<QStringList>::finished, this, [ = ] {
        QMap<QString, QStringList> updatablePackages = packagesWatcher->result();
        checkUpdatablePackages(updatablePackages);
        packagesWatcher->deleteLater();
    });

    packagesWatcher->setFuture(QtConcurrent::run([ = ]() -> QMap<QString, QStringList> {
        m_updateInter->setSync(true);
        QMap<QString, QStringList> map;
        if (!m_updateInter->isValid()) {
            qDebug() << "com.deepin.license error ," << m_updateInter->lastError().name();
            return map;
        }
        map = m_updateInter->classifiedUpdatablePackages();
        m_updateInter->setSync(false);
        return map;
    }));

    QFutureWatcher<QString> *iconWatcher = new QFutureWatcher<QString>();
    connect(iconWatcher, &QFutureWatcher<QString>::finished, this, [ = ] {
        m_iconThemeState = iconWatcher->result();
        iconWatcher->deleteLater();
    });

    iconWatcher->setFuture(QtConcurrent::run([ = ] {
        bool isSync = m_iconTheme->sync();
        m_iconTheme->setSync(true);
        const QString &iconTheme = m_iconTheme->iconTheme();
        m_iconTheme->setSync(isSync);
        return iconTheme;
    }));

    const QList<QDBusObjectPath> jobs = m_managerInter->jobList();
    qInfo() << "Lastore manager jobs count: " << jobs.count();
    bool isDownloading = false;
    if (jobs.count() > 0) {
        for (QDBusObjectPath dBusObjectPath : jobs) {
            if (dBusObjectPath.path().contains("prepare_dist_upgrade")) {
                setUpdateInfo();
                isDownloading = true;
                break;
            }
        }
    }

    onJobListChanged(jobs);

    m_managerInter->setSync(false);
    m_updateInter->setSync(false);

    if (!isDownloading) {
        checkForUpdates();
    }
}

void UpdateWorker::deactivate()
{

}

void UpdateWorker::checkForUpdates()
{
    qCDebug(DCC_UPDATE) << "Check for updates";
    if (checkDbusIsValid()) {
        qCWarning(DCC_UPDATE) << "Check Dbus's validation failed do nothing";
        return;
    }

    QDBusPendingCall call = m_managerInter->UpdateSource();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, call] {
        if (!call.isError()) {
            QDBusReply<QDBusObjectPath> reply = call.reply();
            const QString jobPath = reply.value().path();
            qCDebug(DCC_UPDATE) << "Check update job path: " << jobPath;
            setCheckUpdatesJob(jobPath);
        } else {
            m_model->setStatus(UpdatesStatus::DownloadFailed, __LINE__);
            resetDownloadInfo();
            if (!m_checkUpdateJob.isNull()) {
                m_managerInter->CleanJob(m_checkUpdateJob->id());
            }

            qCWarning(DCC_UPDATE) << "Check update failed, error: " << call.error().message();
        }
    });

    // 每次检查更新的时候都从服务器请求一次更新日志
    requestUpdateLog();
}


void UpdateWorker::setUpdateInfo()
{
    qCDebug(DCC_UPDATE) << "Set update info";

    m_updateInter->setSync(true);
    m_managerInter->setSync(true);

    const QMap<QString, QStringList> &updatePackages = m_updateInter->classifiedUpdatablePackages();

    m_updateInter->setSync(false);
    m_managerInter->setSync(false);

    const QStringList &systemPackages = updatePackages.value(SystemUpdateType);
    const QStringList &securityPackages = updatePackages.value(SecurityUpdateType);
    const QStringList &unknownPackages = updatePackages.value(UnknownUpdateType);
    qCDebug(DCC_UPDATE) << "systemUpdate packages:" << systemPackages;
    qCDebug(DCC_UPDATE) << "safeUpdate packages:" << securityPackages;
    qCDebug(DCC_UPDATE) << "unknownUpdate packages:" << unknownPackages;

    int updateCount = systemPackages.count() + securityPackages.count() + unknownPackages.count();
    if (updateCount < 1) {
        QFile file("/tmp/.dcc-update-successd");
        if (file.exists()) {
            m_model->setStatus(UpdatesStatus::NeedRestart, __LINE__);
            return;
        }
    }

    // 如果内存中没有日志数据，那么从文件里面读取
    do {
        if (!m_updateLogs.isEmpty() || !QFile::exists(UpdateLogTmpFile))
            break;

        qCInfo(DCC_UPDATE) << "Update log is empty, read logs from temporary file";
        QFile logFile(UpdateLogTmpFile);
        if (!logFile.open(QFile::ReadOnly)) {
            qCWarning(DCC_UPDATE) << "Can not open update log file:" << UpdateLogTmpFile;
            break;
        }

        QJsonParseError err_rpt;
        QJsonDocument updateInfoDoc = QJsonDocument::fromJson(logFile.readAll(), &err_rpt);
        logFile.close();
        if (err_rpt.error != QJsonParseError::NoError) {
            qCWarning(DCC_UPDATE) << "Parse update log error: " << err_rpt.errorString();
            break;
        }
        setUpdateLogs(updateInfoDoc.array());
        qCInfo(DCC_UPDATE) << "Update logs size: " << m_updateLogs.size();
    } while(0);

    QMap<ClassifyUpdateType, UpdateItemInfo *> updateInfoMap = getAllUpdateInfo(updatePackages);
    m_model->setAllDownloadInfo(updateInfoMap);

    if (updateInfoMap.count() == 0) {
        m_model->setStatus(UpdatesStatus::Updated, __LINE__);
    } else {
        qInfo() << "Current status: " << m_model->status();
        m_model->setStatus(UpdatesStatus::UpdatesAvailable, __LINE__);
    }
}

QMap<ClassifyUpdateType, UpdateItemInfo *> UpdateWorker::getAllUpdateInfo(const QMap<QString, QStringList> &updatePackages)
{
    qDebug() << Q_FUNC_INFO;

    const QStringList &systemPackages = updatePackages.value(SystemUpdateType);
    const QStringList &securityPackages = updatePackages.value(SecurityUpdateType);
    const QStringList &unknownPackages = updatePackages.value(UnknownUpdateType);

    QMap<ClassifyUpdateType, UpdateItemInfo *> resultMap;

    QMap<ClassifyUpdateType, QString> updateDailyKeyMap;
    updateDailyKeyMap.insert(ClassifyUpdateType::SystemUpdate, "systemUpdateInfo");
    updateDailyKeyMap.insert(ClassifyUpdateType::SecurityUpdate, "safeUpdateInfo");
    updateDailyKeyMap.insert(ClassifyUpdateType::UnknownUpdate, "otherUpdateInfo");

    const quint64 updateMode = m_model->updateMode();

    if (systemPackages.count() > 0) {
        UpdateItemInfo *systemItemInfo = new UpdateItemInfo;
        systemItemInfo->setName(tr("System Updates"));
        systemItemInfo->setExplain(tr("Fixed some known bugs and security vulnerabilities"));
        systemItemInfo->setPackages(systemPackages);
        setUpdateItemDownloadSize(systemItemInfo, systemPackages);
        systemItemInfo->setUpdateModeEnabled(updateMode & ClassifyUpdateType::SystemUpdate);
        resultMap.insert(ClassifyUpdateType::SystemUpdate, systemItemInfo);
    }

    if (securityPackages.count() > 0) {
        UpdateItemInfo  *safeItemInfo = new UpdateItemInfo;
        safeItemInfo->setName(tr("Security Updates"));
        safeItemInfo->setExplain(tr("Fixed some known bugs and security vulnerabilities"));
        safeItemInfo->setPackages(securityPackages);
        setUpdateItemDownloadSize(safeItemInfo, securityPackages);
        safeItemInfo->setUpdateModeEnabled(updateMode & ClassifyUpdateType::SecurityUpdate);
        resultMap.insert(ClassifyUpdateType::SecurityUpdate, safeItemInfo);
    }

    if (unknownPackages.count() > 0) {
        UpdateItemInfo *unkownItemInfo = new UpdateItemInfo;
        unkownItemInfo->setName(tr("Third-party Repositories"));
        unkownItemInfo->setPackages(unknownPackages);
        setUpdateItemDownloadSize(unkownItemInfo, unknownPackages);
        unkownItemInfo->setUpdateModeEnabled(updateMode & ClassifyUpdateType::UnknownUpdate);
        resultMap.insert(ClassifyUpdateType::UnknownUpdate, unkownItemInfo);
    }

    // 将更新日志根据`系统更新`or`安全更新`进行分类，并保存留用
    for (ClassifyUpdateType type : resultMap.keys()) {
        int logType = -1;
        if (type == ClassifyUpdateType::SecurityUpdate) {
            logType = LogTypeSecurity;
        } else if (type == ClassifyUpdateType::SystemUpdate) {
            logType = LogTypeSystem;
        } else {
            continue;
        }

        UpdateItemInfo *itemInfo = resultMap.value(type);
        if (!itemInfo)
            continue;

        for (UpdateLogItem logItem : m_updateLogs) {
            if (!logItem.isValid() || logItem.logType != logType)
                continue;

            updateItemInfo(logItem, resultMap.value(type));
        }
    }

    return resultMap;
}

void UpdateWorker::requestUpdateLog()
{
    qCInfo(DCC_UPDATE) << "Request update info";
    // 接收并处理respond
    QNetworkAccessManager *http = new QNetworkAccessManager(this);
    connect(http, &QNetworkAccessManager::finished, this, [ this, http ] (QNetworkReply *reply) {
        reply->deleteLater();
        http->deleteLater();

        handleUpdateLogsReply(reply);
    });

    // 请求头
    QNetworkRequest request;
    QUrl url(getUpdateLogAddress());
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("rt", QByteArray::number(QDateTime::currentDateTime().toTime_t()));
    url.setQuery(urlQuery);
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 请求体
    // TODO 增加过滤参数，避免每次请求全量更新日志，这个需要web端配合
    QJsonObject requestBody;
    requestBody["platformType"] = getPlatform();
    requestBody["isUnstable"] = isUnstableResource();
    QJsonDocument doc;
    doc.setObject(requestBody);
    const QByteArray &body = doc.toJson();

    http->post(request, body);
    qCInfo(DCC_UPDATE) << "Post request to get update log, request body: " << body;
}

void UpdateWorker::handleUpdateLogsReply(QNetworkReply *reply)
{
    qCInfo(DCC_UPDATE) << "Handle reply of update log";
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Network Error" << reply->errorString();
        return;
    }
    QByteArray respondBody = reply->readAll();
    if (respondBody.isEmpty()) {
        qCWarning(DCC_UPDATE) << "Request body is empty";
        return;
    }

    const QJsonDocument &doc = QJsonDocument::fromJson(respondBody);
    const QJsonObject &obj = doc.object();
    if (obj.isEmpty()) {
        qCWarning(DCC_UPDATE) << "Request body json object is empty";
        return;
    }
    if (obj.value("code").toInt() != 0) {
        qCWarning(DCC_UPDATE) << "Request update log failed";
        return;
    }

    const QJsonArray array = obj.value("data").toArray();
    setUpdateLogs(array);
    // 保存一个临时文件，在没有获取到在线日志的时候展示文件中的内容
    QFile::remove(UpdateLogTmpFile);
    QFile file(UpdateLogTmpFile);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument dataDoc;
        dataDoc.setArray(array);
        file.write(dataDoc.toJson());
        file.close();
    }
}

QString UpdateWorker::getUpdateLogAddress() const
{
    const DConfig *dconfig = DConfigWatcher::instance()->getModulesConfig(DConfigWatcher::update);
    if (dconfig && dconfig->isValid()) {
        const QString &updateLogAddress = dconfig->value("updateLogAddress").toString();
        if (!updateLogAddress.isEmpty())
            return updateLogAddress;
    }
    return "https://update-platform.uniontech.com/api/v1/systemupdatelogs";
}

void UpdateWorker::updateItemInfo(const UpdateLogItem &logItem, UpdateItemInfo *itemInfo)
{
    if (!logItem.isValid() || !itemInfo) {
        return ;
    }

    QStringList language = QLocale::system().name().split('_');
    QString languageType = "CN";
    if (language.count() > 1) {
        languageType = language.value(1);
        if (languageType == "CN"
                || languageType == "TW"
                || languageType == "HK") {
            languageType = "CN";
        } else {
            languageType = "US";
        }
    }

    // 安全更新只会更新与当前系统版本匹配的内容，例如，105X的系统版本只会更新105X的安全更新，而不会更新106X的
    // 更新日志也需要与之匹配，只显示与当前系统版本相同的安全更新日志
    if (logItem.logType == LogTypeSecurity) {
        const QString &currentSystemVer = dccV20::IsCommunitySystem ? Dtk::Core::DSysInfo::deepinVersion() : Dtk::Core::DSysInfo::minorVersion();
        QString tmpSystemVersion = logItem.systemVersion;
        tmpSystemVersion.replace(tmpSystemVersion.length() - 1, 1, '0');
        if (currentSystemVer.compare(tmpSystemVersion) != 0) {
            return;
        }
    }

    const QString &explain = languageType == "CN" ? logItem.cnLog : logItem.enLog;
    // 写入最近的更新
    if (itemInfo->currentVersion().isEmpty()) {
        itemInfo->setCurrentVersion(logItem.systemVersion);
        itemInfo->setAvailableVersion(logItem.systemVersion);
        itemInfo->setExplain(explain);
        itemInfo->setUpdateTime(logItem.publishTime);
    } else {
        DetailInfo detailInfo;
        const QString &systemVersion = logItem.systemVersion;
        // 专业版不不在详细信息中显示维护线版本
        if (!dccV20::IsProfessionalSystem || (!systemVersion.isEmpty() && systemVersion.back() == '0')) {
            detailInfo.name = logItem.systemVersion;
            detailInfo.updateTime = logItem.publishTime;
            detailInfo.info = explain;
            itemInfo->addDetailInfo(detailInfo);
        }
    }
}

bool UpdateWorker::checkDbusIsValid()
{
    if (!checkJobIsValid(m_checkUpdateJob)
            || !checkJobIsValid(m_sysUpdateDownloadJob)
            || !checkJobIsValid(m_sysUpdateInstallJob)
            || !checkJobIsValid(m_safeUpdateDownloadJob)
            || !checkJobIsValid(m_safeUpdateInstallJob)
            || !checkJobIsValid(m_unknownUpdateDownloadJob)
            || !checkJobIsValid(m_unknownUpdateInstallJob)) {
        return false;
    }

    return  true;
}

void UpdateWorker::onSmartMirrorServiceIsValid(bool isvalid)
{
    m_smartMirrorInter->setSync(false);

    if (!isvalid) {
        m_smartMirrorInter->startServiceProcess();
    }
}

void UpdateWorker::resetDownloadInfo(bool state)
{
    qCDebug(DCC_UPDATE) << "Reset download info, state: " << state;
    if (m_model->systemDownloadInfo())
        m_model->systemDownloadInfo()->setPackages(QStringList());
    if (m_model->safeDownloadInfo())
        m_model->safeDownloadInfo()->setPackages(QStringList());
    if (m_model->unknownDownloadInfo())
        m_model->unknownDownloadInfo()->setPackages(QStringList());

    if (!state) {
        deleteJob(m_sysUpdateDownloadJob);
        deleteJob(m_sysUpdateInstallJob);
        deleteJob(m_safeUpdateDownloadJob);
        deleteJob(m_safeUpdateInstallJob);
        deleteJob(m_unknownUpdateDownloadJob);
        deleteJob(m_unknownUpdateInstallJob);
        deleteJob(m_checkUpdateJob);
    }
}

CheckUpdateJobRet UpdateWorker::createCheckUpdateJob(const QString &jobPath)
{
    CheckUpdateJobRet ret;
    ret.status = "failed";
    if (m_checkUpdateJob != nullptr) {
        return ret;
    }
    qCDebug(DCC_UPDATE) << "Create check update job";
    m_checkUpdateJob = new JobInter("com.deepin.lastore", jobPath, QDBusConnection::systemBus(), this);

    ret.jobID = m_checkUpdateJob->id();
    ret.jobDescription = m_checkUpdateJob->description();

    connect(m_checkUpdateJob, &__Job::StatusChanged, this, &UpdateWorker::onCheckUpdateStatusChanged);
    connect(m_checkUpdateJob, &__Job::ProgressChanged, m_model, &UpdateModel::setUpdateProgress, Qt::QueuedConnection);
    connect(qApp, &QApplication::aboutToQuit, this, [ this ] {
        if (m_checkUpdateJob) {
            delete m_checkUpdateJob.data();
        }
    });

    m_checkUpdateJob->ProgressChanged(m_checkUpdateJob->progress());
    m_checkUpdateJob->StatusChanged(m_checkUpdateJob->status());

    return  ret;
}

void UpdateWorker::startDownload()
{
    qInfo() << "Start download, update status: " << m_model->status();
    UpdatesStatus status = m_model->status();
    // 停止下载并删除job，以重新检查更新
    if (status == UpdatesStatus::Downloading) {
        if (m_downloadJob != nullptr) {
            m_managerInter->CleanJob(m_downloadJob->id());
            deleteJob(m_downloadJob);
        }
    }

    // 开始下载
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(m_managerInter->PrepareDistUpgrade(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, watcher] {
        if (!watcher->isError()) {
            watcher->reply().path();
            QDBusPendingReply<QDBusObjectPath> reply = watcher->reply();
            QDBusObjectPath data = reply.value();
            if (data.path().isEmpty()) {
                qWarning() << "Update failed, download updates error: " << watcher->error().message();
                return;
            }
            setDownloadJob(data.path());
        } else {
            m_model->setStatus(UpdatesStatus::DownloadFailed);
            resetDownloadInfo();
            if (!m_downloadJob.isNull()) {
                m_managerInter->CleanJob(m_downloadJob->id());
            }

            if (!m_distUpgradeJob.isNull()) {
                m_managerInter->CleanJob(m_distUpgradeJob->id());
            }
            qDebug() << "Download failed, download updates error: " << watcher->error().message();
        }
    });
}

void UpdateWorker::setIdleDownloadConfig(const UpdateModel::IdleDownloadConfig &config)
{
    qInfo() << "mirror source: " << m_updateInter->property("MirrorSource").toByteArray();
    qInfo() << "config: " << m_updateInter->property("IdleDownloadConfig").toString();
    QByteArray br = config.toJson();
    qInfo() << "br: " << br;
    QString aaa = QString(br);
    aaa = aaa.replace("\n", "");
    QDBusPendingCall call = m_updateInter->asyncCall("SetIdleDownloadConfig",aaa);
    call.waitForFinished();
    qInfo() << "reply: " << call.error().message();
}

void UpdateWorker::setUpdateMode(const quint64 updateMode)
{
    qDebug() << Q_FUNC_INFO << "set UpdateMode to dbus:" << updateMode;

    m_managerInter->setUpdateMode(updateMode);
}

void UpdateWorker::setAutoDownloadUpdates(const bool &autoDownload)
{
    m_updateInter->SetAutoDownloadUpdates(autoDownload);
    if (autoDownload == false) {
        m_updateInter->setAutoInstallUpdates(false);
    }
}

void UpdateWorker::setAutoInstallUpdates(const bool &autoInstall)
{
    m_updateInter->setAutoInstallUpdates(autoInstall);
}

void UpdateWorker::setMirrorSource(const MirrorInfo &mirror)
{
    m_updateInter->SetMirrorSource(mirror.m_id);
}

void UpdateWorker::checkTestingChannelStatus()
{
    qDebug() << "Testing:" << "check testing join status";
    const auto server = m_model->getTestingChannelServer();
    const auto machineID = m_model->getMachineID();
    auto http = new QNetworkAccessManager(this);
    QNetworkRequest request;
    request.setUrl(QUrl(server + QString("/api/v2/public/testing/machine/status/") + machineID));
    request.setRawHeader("content-type", "application/json");
    connect(http, &QNetworkAccessManager::finished, this, [ = ](QNetworkReply *reply){
        reply->deleteLater();
        http->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Testing:" << "Network Error" << reply->errorString();
            return;
        }
        auto data = reply->readAll();
        qDebug() << "Testing:" << "machine status body" << data;
        auto doc = QJsonDocument::fromJson(data);
        auto obj = doc.object();
        auto status = obj["data"].toObject()["status"].toString();
        // Exit the loop if switch status is disable
        if (m_model->getTestingChannelStatus() != UpdateModel::TestingChannelStatus::WaitJoined) {
            return;
        }
        // If user has joined then install testing source package;
        if (status == "joined") {
            m_model->setTestingChannelStatus(UpdateModel::TestingChannelStatus::Joined);
            qDebug() << "Testing:" << "Install testing channel package";
            // 安装内测源之前执行一次apt update，避免刚安装的系统没有仓库索引导致安装失败
            checkForUpdates();
            // 延迟1秒是为了把安装任务放在update之后
            QThread::sleep(1);
            // lastore会自动等待apt update完成后再执行安装。
            m_managerInter->InstallPackage("testing channel", TestingChannelPackage);
            return;
        }
        // Run again after sleep
        QTimer::singleShot(5000, this,  &UpdateWorker::checkTestingChannelStatus);
    });
    http->get(request);
}

void UpdateWorker::setTestingChannelEnable(const bool &enable)
{
    qDebug() << "Testing:" << "TestingChannelEnableChange" << enable;
    if (enable) {
        m_model->setTestingChannelStatus(UpdateModel::TestingChannelStatus::WaitJoined);
    } else {
        m_model->setTestingChannelStatus(UpdateModel::TestingChannelStatus::NotJoined);
    }

    const auto server = m_model->getTestingChannelServer();
    const auto machineID = m_model->getMachineID();

    // 无论是加入还是退出都先在服务器标记退出
    // 避免重装系统后机器码相同，没申请就自动加入
    auto http = new QNetworkAccessManager(this);
    QNetworkRequest request;
    request.setUrl(QUrl(server + QString("/api/v2/public/testing/machine/") + machineID));
    request.setRawHeader("content-type", "application/json");
    connect(http, &QNetworkAccessManager::finished, this, [ http ](QNetworkReply *reply){
        reply->deleteLater();
        http->deleteLater();
    });
    http->deleteResource(request);

    /* Disable Testing Channel */
    if (!enable) {
        // Uninstall testing source package if it is installed
        if (m_managerInter->PackageExists(TestingChannelPackage)) {
            qDebug() << "Testing:" << "Uninstall testing channel package";
            m_managerInter->RemovePackage("testing channel", TestingChannelPackage);
        }
        return;
    }

    /* Enable Testing Channel */
    auto u = m_model->getTestingChannelJoinURL();
    qDebug() << "Testing:" << "open join page" << u.toString();
    QDesktopServices::openUrl(u);

    // Loop to check if user hava joined
    QTimer::singleShot(1000, this,  &UpdateWorker::checkTestingChannelStatus);
}

QString UpdateWorker::getTestingChannelSource()
{
    auto sourceFile = QString("/etc/apt/sources.list.d/%1.list").arg(TestingChannelPackage);
    qDebug() << "sourceFile" << sourceFile;
    QFile f(sourceFile);
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        return "";
    }
    QTextStream in(&f);
    while (!in.atEnd()) {
        auto line = in.readLine();
        if (line.startsWith("deb"))
        {
            auto fields = line.split(" ", QString::SkipEmptyParts);
            if (fields.length() >= 2)
            {
                auto sourceURL = fields[1];
                if (sourceURL.endsWith("/"))
                {
                    sourceURL.truncate(sourceURL.length() - 1);
                }
                return sourceURL;
            }
        }
    }
    return "";
}
// get all sources of the package
QStringList UpdateWorker::getSourcesOfPackage(const QString pkg, const QString version)
{
    QStringList sources;
    QProcess aptCacheProcess(this);
    QStringList args;
    args.append("madison");
    args.append(pkg);
    // exec apt-cache madison $pkg
    aptCacheProcess.start("apt-cache", args);
    aptCacheProcess.waitForFinished();
    while (aptCacheProcess.canReadLine()) {
        QString line(aptCacheProcess.readLine());
        auto fields = line.split("|", QString::SkipEmptyParts);
        for (QString &field : fields) {
            field = field.trimmed();
        }
        if (fields.length() <= 2) {
            continue;
        }
        auto p = fields[0], ver = fields[1], src = fields[2];
        src.truncate(fields[2].indexOf(" "));
        if (p == pkg) {
            if (version.length() == 0 || version == ver) {
                sources.append(src);
            }
        }
    }
    return sources.toSet().toList();
}

// checkCanExitTestingChannel check if the current env can exit internal test channel
void UpdateWorker::checkCanExitTestingChannel()
{
    // 如果在执行apt-get clean之后使用apt-cache会很慢，比正常情况慢23倍
    // 执行一次apt update，可以让apt-cache恢复到正常速度
    checkForUpdates();
    auto testingChannelSource = getTestingChannelSource();
    QProcess dpkgProcess(this);
    // exec dpkg -l
    dpkgProcess.start("dpkg", QStringList("-l"));
    dpkgProcess.waitForStarted();
    // read stdout by line
    while (dpkgProcess.state() == QProcess::Running) {
        dpkgProcess.waitForReadyRead();
        while (dpkgProcess.canReadLine()) {
            QString line(dpkgProcess.readLine());
            // skip uninstalled
            if (!line.startsWith("ii")) {
                continue;
            }
            auto field = line.split(" ", QString::SkipEmptyParts);
            // skip unknown format line
            if (field.length() <= 2) {
                continue;
            }
            auto pkg = field[1], version = field[2];
            // skip non system software
            if (!pkg.contains("dde") && !pkg.contains("deepin") && !pkg.contains("dtk") && !pkg.contains("uos")) {
                continue;
            }
            // Does the package exists only in the internal test source
            auto sources = getSourcesOfPackage(pkg, version);
            if (sources.length() == 1 && sources[0].contains(testingChannelSource)) {
                m_model->setCanExitTestingChannel(false);
                return;
            }
        }
    }
    m_model->setCanExitTestingChannel(true);
    return;
}

#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
void UpdateWorker::setSourceCheck(bool enable)
{
    m_lastoreSessionHelper->SetSourceCheckEnabled(enable);
}
#endif

void UpdateWorker::testMirrorSpeed()
{
    QList<MirrorInfo> mirrors = m_model->mirrorInfos();

    QStringList urlList;
    for (MirrorInfo &info : mirrors) {
        urlList << info.m_url;
    }

    // reset the data;
    m_model->setMirrorSpeedInfo(QMap<QString, int>());

    QFutureWatcher<int> *watcher = new QFutureWatcher<int>();
    connect(watcher, &QFutureWatcher<int>::resultReadyAt, [this, urlList, watcher, mirrors](int index) {
        QMap<QString, int> speedInfo = m_model->mirrorSpeedInfo();

        int result = watcher->resultAt(index);
        QString mirrorId = mirrors.at(index).m_id;
        speedInfo[mirrorId] = result;

        m_model->setMirrorSpeedInfo(speedInfo);
    });

    QPointer<QObject> guest(this);
    QFuture<int> future = QtConcurrent::mapped(urlList, std::bind(TestMirrorSpeedInternal, std::placeholders::_1, guest));
    watcher->setFuture(future);
}

void UpdateWorker::checkNetselect()
{
    QProcess *process = new QProcess;
    process->start("netselect", QStringList() << "127.0.0.1");
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if ((error == QProcess::FailedToStart) || (error == QProcess::Crashed)) {
            m_model->setNetselectExist(false);
            process->deleteLater();
        }
    });
    connect(process, static_cast<void (QProcess::*)(int)>(&QProcess::finished), this, [this, process](int result) {
        bool isNetselectExist = 0 == result;
        if (!isNetselectExist) {
            qDebug() << "[wubw UpdateWorker] netselect 127.0.0.1 : " << isNetselectExist;
        }
        m_model->setNetselectExist(isNetselectExist);
        process->deleteLater();
    });
}

void UpdateWorker::setSmartMirror(bool enable)
{
    m_smartMirrorInter->SetEnable(enable);

    QTimer::singleShot(100, this, [ = ] {
        Q_EMIT m_smartMirrorInter->serviceValidChanged(m_smartMirrorInter->isValid());
    });
}

#ifndef DISABLE_SYS_UPDATE_MIRRORS
void UpdateWorker::refreshMirrors()
{
    qDebug() << QDir::currentPath();
    QFile file(":/update/themes/common/config/mirrors.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << file.errorString();
        return;
    }
    QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
    QList<MirrorInfo> list;
    for (auto item : array) {
        QJsonObject obj = item.toObject();
        MirrorInfo info;
        info.m_id = obj.value("id").toString();
        QString locale = QLocale::system().name();
        if (!(QLocale::system().name() == "zh_CN" || QLocale::system().name() == "zh_TW")) {
            locale = "zh_CN";
        }
        info.m_name = obj.value(QString("name_locale.%1").arg(locale)).toString();
        info.m_url = obj.value("url").toString();
        list << info;
    }
    m_model->setMirrorInfos(list);
    m_model->setDefaultMirror(list[0].m_id);
}
#endif

void UpdateWorker::setCheckUpdatesJob(const QString &jobPath)
{
    qCDebug(DCC_UPDATE) << "Set check updates job, current update status : " << m_model->status();
    UpdatesStatus state = m_model->status();
    if (UpdatesStatus::Downloading != state && UpdatesStatus::DownloadPaused != state && UpdatesStatus::Installing != state) {
        m_model->setStatus(UpdatesStatus::Checking, __LINE__);
    }
    createCheckUpdateJob(jobPath);
}

void UpdateWorker::setDownloadJob(const QString &jobPath)
{
    qInfo() << "Set download job: " << jobPath;
    QMutexLocker locker(&m_downloadMutex);
    if (m_model->status() == UpdatesStatus::Default || m_model->status() == UpdatesStatus::Checking) {
        setUpdateInfo();
    }

    m_model->setStatus(UpdatesStatus::Downloading, __LINE__);
    m_downloadJob = new JobInter("com.deepin.lastore", jobPath, QDBusConnection::systemBus(), this);

    connect(m_downloadJob, &__Job::ProgressChanged, this, &UpdateWorker::onDownloadProgressChanged);
    connect(m_downloadJob, &__Job::NameChanged, this, [this] (const QString &jobName) {
        m_updateJobName = jobName;
    });
    connect(m_downloadJob, &__Job::StatusChanged, this, [ = ](QString status) {
        onDownloadStatusChanged(status);
    });

    m_downloadJob->StatusChanged(m_downloadJob->status());
    m_downloadJob->ProgressChanged(m_downloadJob->progress());
    m_downloadJob->NameChanged(m_downloadJob->name());
}

void UpdateWorker::setUpdateItemProgress(UpdateItemInfo *itemInfo, double value)
{
    //异步加载数据,会导致下载信息还未获取就先取到了下载进度
    if (itemInfo) {
        itemInfo->setDownloadProgress(value);

    } else {
        //等待下载信息加载后,再通过 onNotifyDownloadInfoChanged() 设置"UpdatesStatus::Downloading"状态
        qWarning() << "DownloadInfo is nullptr";
    }
}

void UpdateWorker::setAutoCleanCache(const bool autoCleanCache)
{
    m_managerInter->SetAutoClean(autoCleanCache);
}

void UpdateWorker::onJobListChanged(const QList<QDBusObjectPath> &jobs)
{
    qCInfo(DCC_UPDATE) << Q_FUNC_INFO;

    if (!hasRepositoriesUpdates()) {
        return;
    }
    for (const auto &job : jobs) {
        const QString jobPath = job.path();

        JobInter jobInter("com.deepin.lastore", jobPath, QDBusConnection::systemBus());

        // id maybe scrapped
        const QString &id = jobInter.id();
        if (!jobInter.isValid() || id.isEmpty())
            continue;

        qDebug() << "Job id: " << id << ", job path: " << jobPath;
        if ((id == "update_source" || id == "custom_update") && m_checkUpdateJob == nullptr) {
            setCheckUpdatesJob(m_jobPath);
        } else if (id == "prepare_dist_upgrade" && m_downloadJob == nullptr) {
            setDownloadJob(m_jobPath);
        } else {
            qDebug() << "Nothing to do";
        }
    }
}

void UpdateWorker::onDownloadProgressChanged(double value)
{
    DownloadInfo *info = m_model->downloadInfo();
    info->setDownloadProgress(value);
}

void UpdateWorker::onSysUpdateDownloadProgressChanged(double value)
{
    UpdateItemInfo *itemInfo = m_model->systemDownloadInfo();
    setUpdateItemProgress(itemInfo, value);
}

void UpdateWorker::onSafeUpdateDownloadProgressChanged(double value)
{
    UpdateItemInfo *itemInfo = m_model->safeDownloadInfo();

    setUpdateItemProgress(itemInfo, value);
}

void UpdateWorker::onUnknownUpdateDownloadProgressChanged(double value)
{
    UpdateItemInfo *itemInfo = m_model->unknownDownloadInfo();

    setUpdateItemProgress(itemInfo, value);
}

void UpdateWorker::onSysUpdateInstallProgressChanged(double value)
{
    UpdateItemInfo *itemInfo = m_model->systemDownloadInfo();
    if (itemInfo == nullptr || qFuzzyIsNull(value)) {
        return;
    }

    setUpdateItemProgress(itemInfo, value);
}

void UpdateWorker::onSafeUpdateInstallProgressChanged(double value)
{
    UpdateItemInfo *itemInfo = m_model->safeDownloadInfo();
    if (itemInfo == nullptr || qFuzzyIsNull(value)) {
        return;
    }

    setUpdateItemProgress(itemInfo, value);
}

void UpdateWorker::onUnkonwnUpdateInstallProgressChanged(double value)
{
    UpdateItemInfo *itemInfo = m_model->unknownDownloadInfo();
    if (itemInfo == nullptr || qFuzzyIsNull(value)) {
        return;
    }

    qDebug() << "onUnkonwnUpdateInstallProgressChanged : " << value;
    setUpdateItemProgress(itemInfo, value);
}

void UpdateWorker::onIconThemeChanged(const QString &theme)
{
    m_iconThemeState = theme;
}

void UpdateWorker::onCheckUpdateStatusChanged(const QString &value)
{
    qCDebug(DCC_UPDATE) << "Check update status changed " << value;
    if (value == "failed" || value.isEmpty()) {
        qWarning() << "check for updates job failed";
        if (m_checkUpdateJob != nullptr) {
            m_managerInter->CleanJob(m_checkUpdateJob->id());
            m_model->setClassifyUpdateJonError(analyzeJobErrorMessage(m_checkUpdateJob->description()));
            m_model->setStatus(UpdatesStatus::CheckingFailed, __LINE__);
            resetDownloadInfo();
            deleteJob(m_checkUpdateJob);
        }
    } else if (value == "success" || value == "succeed") {
        setUpdateInfo();
    } else if (value == "end") {
        deleteJob(m_checkUpdateJob);
    }
}

void UpdateWorker::setBatteryPercentage(const BatteryPercentageInfo &info)
{
    m_batteryPercentage = info.value("Display", 0);
    const bool low = m_onBattery && m_batteryPercentage < 50;
    m_model->setLowBattery(low);
}

//Now D-Bus only in system power have BatteryPercentage data
void UpdateWorker::setSystemBatteryPercentage(const double &value)
{
    m_batterySystemPercentage = value;
    const bool low = m_onBattery && m_batterySystemPercentage < 50;
    m_model->setLowBattery(low);
}

void UpdateWorker::setOnBattery(bool onBattery)
{
    m_onBattery = onBattery;
    const bool low = m_onBattery && m_batteryPercentage < 50;
    // const bool low = m_onBattery ? m_batterySystemPercentage < 50 : false;
    m_model->setLowBattery(low);
}

void UpdateWorker::refreshHistoryAppsInfo()
{
    //m_model->setHistoryAppInfos(m_updateInter->getHistoryAppsInfo());
    m_model->setHistoryAppInfos(m_updateInter->ApplicationUpdateInfos(QLocale::system().name()));
}

void UpdateWorker::refreshLastTimeAndCheckCircle()
{
    QString checkTime;
    double interval = m_updateInter->GetCheckIntervalAndTime(checkTime);

    m_model->setAutoCheckUpdateCircle(static_cast<int>(interval));
    m_model->setLastCheckUpdateTime(checkTime);
}

void UpdateWorker::setUpdateNotify(const bool notify)
{
    m_updateInter->SetUpdateNotify(notify);
}

void UpdateWorker::onDownloadJobCtrl(int updateCtrlType)
{
    if (m_downloadJob == nullptr) {
        return;
    }

    switch (updateCtrlType) {
    case UpdateCtrlType::Start:
        m_managerInter->StartJob(m_downloadJob->id());
        break;
    case UpdateCtrlType::Pause:
        m_managerInter->PauseJob(m_downloadJob->id());
        break;
    }
}

QPointer<JobInter> UpdateWorker::getDownloadJob(ClassifyUpdateType updateType)
{
    QPointer<JobInter> job;
    switch (updateType) {
    case ClassifyUpdateType::SystemUpdate:
        job = m_sysUpdateDownloadJob;
        break;
    case ClassifyUpdateType::SecurityUpdate:
        job = m_safeUpdateDownloadJob;
        break;
    case ClassifyUpdateType::UnknownUpdate:
        job = m_unknownUpdateDownloadJob;
        break;
    default:
        job = nullptr;
        break;
    }

    return job;
}

QPointer<JobInter> UpdateWorker::getInstallJob(ClassifyUpdateType updateType)
{
    QPointer<JobInter> job;
    switch (updateType) {
    case ClassifyUpdateType::SystemUpdate:
        job = m_sysUpdateInstallJob;
        break;
    case ClassifyUpdateType::SecurityUpdate:
        job = m_safeUpdateInstallJob;
        break;
    case ClassifyUpdateType::UnknownUpdate:
        job = m_unknownUpdateInstallJob;
        break;
    default:
        job = nullptr;
        break;
    }

    return job;
}

bool UpdateWorker::checkJobIsValid(QPointer<JobInter> dbusJob)
{
    if (dbusJob.isNull())
        return  false;

    if (dbusJob->isValid())
        return true;

    dbusJob->deleteLater();
    return  false;
}

void UpdateWorker::deleteJob(QPointer<JobInter> dbusJob)
{
    if (!dbusJob.isNull()) {
        dbusJob->deleteLater();
        dbusJob = nullptr;
    }
}

void UpdateWorker::cleanLaStoreJob(QPointer<JobInter> dbusJob)
{
    if (dbusJob != nullptr) {
        m_managerInter->CleanJob(dbusJob->id());
        deleteJob(dbusJob);
    }
}

void UpdateWorker::onRequestOpenAppStore()
{
    QDBusInterface appStore("com.home.appstore.client",
                            "/com/home/appstore/client",
                            "com.home.appstore.client",
                            QDBusConnection::sessionBus());
    QVariant value = "tab/update";
    QDBusMessage reply = appStore.call("openBusinessUri", value);
    qDebug() << reply.errorMessage();
}

UpdateErrorType UpdateWorker::analyzeJobErrorMessage(const QString &jobDescription)
{
    qWarning() << Q_FUNC_INFO << jobDescription;

    QJsonParseError err_rpt;
    QJsonDocument jobErrorMessage = QJsonDocument::fromJson(jobDescription.toUtf8(), &err_rpt);

    if (err_rpt.error != QJsonParseError::NoError) {
        qDebug() << "Parse json failed";
        return UpdateErrorType::UnKnown;
    }
    const QJsonObject &object = jobErrorMessage.object();
    QString errorType =  object.value("ErrType").toString();
    if (errorType.contains("fetchFailed", Qt::CaseInsensitive) || errorType.contains("IndexDownloadFailed", Qt::CaseInsensitive)) {
        return UpdateErrorType::NoNetwork;
    }
    if (errorType.contains("unmetDependencies", Qt::CaseInsensitive) || errorType.contains("dependenciesBroken", Qt::CaseInsensitive)) {
        return UpdateErrorType::DependenciesBrokenError;
    }
    if (errorType.contains("insufficientSpace", Qt::CaseInsensitive)) {
        return UpdateErrorType::NoSpace;
    }
    if (errorType.contains("interrupted", Qt::CaseInsensitive)) {
        return UpdateErrorType::DpkgInterrupted;
    }

    return UpdateErrorType::UnKnown;
}

void UpdateWorker::onDownloadStatusChanged(const QString &value)
{
    qCDebug(DCC_UPDATE) << "Download status changed: " << "status :: " << value;
    if (value == "running" || value == "ready") {
        m_model->setStatus(UpdatesStatus::Downloading);
    } else if (value == "failed") {
        qCDebug(DCC_UPDATE) << "Job description :: " << m_downloadJob->description();
        m_model->setClassifyUpdateJonError(analyzeJobErrorMessage(m_downloadJob->description()));
        m_model->setStatus(UpdatesStatus::CheckingFailed);
        cleanLaStoreJob(m_downloadJob);
    } else if (value == "succeed") {
        if (m_updateJobName.contains("OnlyDownload")) {
            m_model->setStatus(UpdatesStatus::AutoDownloaded);
        } else {
            m_model->setStatus(UpdatesStatus::Downloaded);
        }
    } else if (value == "paused") {
        m_model->setStatus(UpdatesStatus::DownloadPaused);
    } else if (value == "end") {
        deleteJob(m_downloadJob);
    }
}

void UpdateWorker::checkUpdatablePackages(const QMap<QString, QStringList> &updatablePackages)
{
    qDebug() << "UpdatablePackages = " << updatablePackages.count();
    QMap<ClassifyUpdateType, QString> keyMap;
    keyMap.insert(ClassifyUpdateType::SystemUpdate, SystemUpdateType);
    keyMap.insert(ClassifyUpdateType::UnknownUpdate, UnknownUpdateType);
    keyMap.insert(ClassifyUpdateType::SecurityUpdate, SecurityUpdateType);
    bool showUpdateNotify = false;
    for (auto item : keyMap.keys()) {
        if ((m_model->updateMode() & static_cast<unsigned>(item)) && updatablePackages.value(keyMap.value(item)).count() > UPDATE_PACKAGE_SIZE) {
            showUpdateNotify = true;
            break;
        }
    }
    m_model->isUpdatablePackages(showUpdateNotify);
}

bool UpdateWorker::hasRepositoriesUpdates()
{
    quint64 mode = m_model->updateMode();
    return (mode & ClassifyUpdateType::SystemUpdate) || (mode & ClassifyUpdateType::UnknownUpdate) || (mode & ClassifyUpdateType::SecurityUpdate);
}

void UpdateWorker::onClassifiedUpdatablePackagesChanged(QMap<QString, QStringList> packages)
{
    auto systemUpdateInfo = m_model->systemDownloadInfo();
    if (systemUpdateInfo)
        systemUpdateInfo->setPackages(packages.value(SystemUpdateType));

    auto safeUpdateInfo = m_model->safeDownloadInfo();
    if (safeUpdateInfo)
        safeUpdateInfo->setPackages(packages.value(SecurityUpdateType));

    auto unknownUpdateInfo = m_model->unknownDownloadInfo();
    if (unknownUpdateInfo)
        unknownUpdateInfo->setPackages(packages.value(UnknownUpdateType));

    checkUpdatablePackages(packages);
}

void UpdateWorker::onFixError(const ClassifyUpdateType &updateType, const QString &errorType)
{
    if (errorType == "noNetwork") {
        checkForUpdates();
        return;
    }
    m_fixErrorUpdate.append(updateType);
    if (m_fixErrorJob != nullptr) {
        return;
    }
    QDBusInterface lastoreManager("com.deepin.lastore",
                                  "/com/deepin/lastore",
                                  "com.deepin.lastore.Manager",
                                  QDBusConnection::systemBus());
    if (!lastoreManager.isValid()) {
        qDebug() << "com.deepin.license error ," << lastoreManager.lastError().name();
        return;
    }

    QDBusReply<QDBusObjectPath> reply = lastoreManager.call("FixError", errorType);
    if (reply.isValid()) {
        QString path = reply.value().path();
        m_fixErrorJob = new JobInter("com.deepin.lastore", path, QDBusConnection::systemBus());
        connect(m_fixErrorJob, &JobInter::StatusChanged, this, [ = ](const QString status) {
            if (status == "succeed" || status == "failed" || status == "end") {
                qDebug() << "m_fixErrorJob ---status :" << status;
                // TODO 处理fixError
                m_fixErrorUpdate.clear();
                deleteJob(m_fixErrorJob);
            }
        });
    }
}

void UpdateWorker::setUpdateItemDownloadSize(UpdateItemInfo *updateItem,  QStringList packages)
{
    QDBusPendingCall call = m_managerInter->PackagesDownloadSize(packages);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [updateItem, call] {
        if (!call.isError()) {
            QDBusReply<qlonglong> reply = call.reply();
            qlonglong value = reply.value();
            qInfo() << "Download size: " << value << ", name: " << updateItem->name();
            updateItem->setDownloadSize(value);
        } else {
            qWarning() << "Get download size error: " << call.error().message();
        }
    });
}

void UpdateWorker::onRequestLastoreHeartBeat()
{
    QDBusInterface lastoreManager("com.deepin.lastore",
                                  "/com/deepin/lastore",
                                  "com.deepin.lastore.Updater",
                                  QDBusConnection::systemBus());
    if (!lastoreManager.isValid()) {
        qDebug() << "com.deepin.license error ," << lastoreManager.lastError().name();
        return;
    }
    lastoreManager.asyncCall("GetCheckIntervalAndTime");
}

void UpdateWorker::setUpdateLogs(const QJsonArray &array)
{
    if (array.isEmpty())
        return;

    m_updateLogs.clear();
    for(const QJsonValue &value : array) {
        QJsonObject obj = value.toObject();
        if (obj.isEmpty())
            continue;

        UpdateLogItem item;
        item.id = obj.value("id").toInt();
        item.systemVersion = obj.value("systemVersion").toString();
        item.cnLog = obj.value("cnLog").toString();
        item.enLog = obj.value("enLog").toString();
        item.publishTime = DCC_NAMESPACE::utcDateTime2LocalDate(obj.value("publishTime").toString());
        item.platformType = obj.value("platformType").toInt();
        item.serverType = obj.value("serverType").toInt();
        item.logType = obj.value("logType").toInt();
        m_updateLogs.append(std::move(item));
    }
    qInfo() << "m_updateLogs size: " << m_updateLogs.size();
    // 不依赖服务器返回来日志顺序，用systemVersion进行排序
    // 如果systemVersion版本号相同，则用发布时间排序；不考虑版本号相同且发布时间相同的情况，这种情况应该由运维人员避免
    qSort(m_updateLogs.begin(), m_updateLogs.end(), [] (const UpdateLogItem &v1, const UpdateLogItem &v2) -> bool {
        int compareRet = v1.systemVersion.compare(v2.systemVersion);
        if (compareRet == 0) {
            return v1.publishTime.compare(v2.publishTime) >= 0;
        }
        return compareRet > 0;
    });
}

/**
 * @brief 发行版or内测版
 *
 * @return 1: 发行版，2：内测版
 */
int UpdateWorker::isUnstableResource() const
{
    qInfo() << Q_FUNC_INFO;
    const int RELEASE_VERSION = 1;
    const int UNSTABLE_VERSION = 2;
    QObject raii;
    DConfig *config = DConfig::create("org.deepin.unstable", "org.deepin.unstable", QString(), &raii);
    if (!config) {
        qInfo() << "Can not find org.deepin.unstable or an error occurred in DTK";
        return RELEASE_VERSION;
    }

    if (!config->keyList().contains("updateUnstable")) {
        qInfo() << "Key(updateUnstable) was not found ";
        return RELEASE_VERSION;
    }

    const QString &value = config->value("updateUnstable", "Enabled").toString();
    qInfo() << "Config(updateUnstable) value: " << value;
    return "Enabled" == value ? UNSTABLE_VERSION : RELEASE_VERSION;
}

}
}
