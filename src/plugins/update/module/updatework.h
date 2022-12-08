// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UPDATEWORK_H
#define UPDATEWORK_H

#include "updatemodel.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QLoggingCategory>

#include <com_deepin_lastore_updater.h>
#include <com_deepin_lastore_job.h>
#include <com_deepin_lastore_jobmanager.h>
#include <com_deepin_daemon_power.h>
#include <com_deepin_system_systempower.h>
#include <com_deepin_daemon_network.h>
#include <com_deepin_lastoresessionhelper.h>
#include <com_deepin_lastore_smartmirror.h>
#include <com_deepin_abrecovery.h>
#include <com_deepin_daemon_appearance.h>

#include "common.h"

using UpdateInter = com::deepin::lastore::Updater;
using JobInter = com::deepin::lastore::Job;
using ManagerInter = com::deepin::lastore::Manager;
using PowerInter = com::deepin::daemon::Power;
using PowerSystemInter = com::deepin::system::Power;
using Network = com::deepin::daemon::Network;
using LastoressionHelper = com::deepin::LastoreSessionHelper;
using SmartMirrorInter = com::deepin::lastore::Smartmirror;
using RecoveryInter = com::deepin::ABRecovery;
using Appearance = com::deepin::daemon::Appearance;

class QJsonArray;

Q_DECLARE_LOGGING_CATEGORY(DCC_UPDATE)

namespace dcc {
namespace update {

struct CheckUpdateJobRet {
    QString status;
    QString jobID;
    QString jobDescription;
};

/**
 * @brief 更新日志中一个版本的信息
 *
 * 示例数据：
 * {
        "id": 1,
        "platformType": 1,
        "cnLog": "<p>中文日志</p>",
        "enLog": "<p>英文日志</p>",
        "serverType": 0,
        "systemVersion": "1070U1",
        "createdAt": "2022-08-10T17:45:54+08:00",
        "logType": 1,
        "publishTime": "2022-08-06T00:00:00+08:00"
    }
 */
struct UpdateLogItem
{
    int id = -1;
    int platformType = 1;
    int serverType = 0;
    int logType = 1;
    QString systemVersion = "";
    QString cnLog = "";
    QString enLog = "";
    QString publishTime = "";

    bool isValid() const { return -1 != id; }
};

class UpdateWorker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateWorker(UpdateModel *model, QObject *parent = nullptr);
    ~UpdateWorker();
    void activate();
    void deactivate();
    void setOnBattery(bool onBattery);
    void setBatteryPercentage(const BatteryPercentageInfo &info);
    void setSystemBatteryPercentage(const double &value);
    void getLicenseState();

    bool hasRepositoriesUpdates();

Q_SIGNALS:
    void requestInit();
    void requestActive();
    void requestRefreshLicenseState();

#ifndef DISABLE_SYS_UPDATE_MIRRORS
    void requestRefreshMirrors();
#endif

public Q_SLOTS:
    void init();
    void checkForUpdates();
    void setUpdateMode(const quint64 updateMode);
    void setAutoCleanCache(const bool autoCleanCache);
    void setAutoDownloadUpdates(const bool &autoDownload);
    void setAutoInstallUpdates(const bool &autoInstall);
    void setMirrorSource(const MirrorInfo &mirror);
    void setTestingChannelEnable(const bool &enable);
    void checkCanExitTestingChannel();
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    void setSourceCheck(bool enable);
#endif
    void testMirrorSpeed();
    void checkNetselect();
    void setSmartMirror(bool enable);
#ifndef DISABLE_SYS_UPDATE_MIRRORS
    void refreshMirrors();
#endif

    void licenseStateChangeSlot();
    void refreshHistoryAppsInfo();
    void refreshLastTimeAndCheckCircle();
    void setUpdateNotify(const bool notify);

    void onDownloadJobCtrl(int updateCtrlType);
    void onRequestOpenAppStore();
    void onClassifiedUpdatablePackagesChanged(QMap<QString, QStringList> packages);
    void onFixError(const ClassifyUpdateType &updateType, const QString &errorType);
    void onRequestLastoreHeartBeat();
    void startDownload();
    void setIdleDownloadConfig(const UpdateModel::IdleDownloadConfig &config);
    void onDownloadProgressChanged(double value);

private Q_SLOTS:
    void setCheckUpdatesJob(const QString &jobPath);
    void onJobListChanged(const QList<QDBusObjectPath> &jobs);
    void onIconThemeChanged(const QString &theme);

    void onCheckUpdateStatusChanged(const QString &value);
    void onDownloadStatusChanged(const QString &value);

    void onSysUpdateDownloadProgressChanged(double value);
    void onSafeUpdateDownloadProgressChanged(double value);
    void onUnknownUpdateDownloadProgressChanged(double value);

    void onSysUpdateInstallProgressChanged(double value);
    void onSafeUpdateInstallProgressChanged(double value);
    void onUnkonwnUpdateInstallProgressChanged(double value);
    void checkTestingChannelStatus();
    QStringList getSourcesOfPackage(const QString pkg, const QString version);
    QString getTestingChannelSource();
    void handleUpdateLogsReply(QNetworkReply *reply);
    QString getUpdateLogAddress() const;

private:
    QMap<ClassifyUpdateType, UpdateItemInfo *> getAllUpdateInfo(const QMap<QString, QStringList> &updatePackages);
    void setUpdateInfo();
    void setUpdateItemDownloadSize(UpdateItemInfo *updateItem, QStringList packages);

    inline bool checkDbusIsValid();
    void onSmartMirrorServiceIsValid(bool isvalid);
    void resetDownloadInfo(bool state = false);
    CheckUpdateJobRet createCheckUpdateJob(const QString &jobPath);
    void setDownloadJob(const QString &jobPath);
    void setUpdateItemProgress(UpdateItemInfo *itemInfo, double value);

    QPointer<JobInter> getDownloadJob(ClassifyUpdateType updateType);
    QPointer<JobInter> getInstallJob(ClassifyUpdateType updateType);

    bool checkJobIsValid(QPointer<JobInter> dbusJob);
    void deleteJob(QPointer<JobInter> dbusJob);
    void cleanLaStoreJob(QPointer<JobInter> dbusJob);
    UpdateErrorType analyzeJobErrorMessage(const QString &jobDescription);
    void checkUpdatablePackages(const QMap<QString, QStringList> &updatablePackages);
    void requestUpdateLog();
    void updateItemInfo(const UpdateLogItem &logItem, UpdateItemInfo *itemInfo);
    void setUpdateLogs(const QJsonArray &array);
    int isUnstableResource() const;

private:
    UpdateModel *m_model;
    QPointer<JobInter> m_checkUpdateJob;
    QPointer<JobInter> m_fixErrorJob;

    QPointer<JobInter> m_sysUpdateDownloadJob;
    QPointer<JobInter> m_safeUpdateDownloadJob;
    QPointer<JobInter> m_unknownUpdateDownloadJob;
    QPointer<JobInter> m_downloadJob;
    QPointer<JobInter> m_distUpgradeJob;

    QPointer<JobInter> m_sysUpdateInstallJob;
    QPointer<JobInter> m_safeUpdateInstallJob;
    QPointer<JobInter> m_unknownUpdateInstallJob;

    QString m_updateJobName;

    LastoressionHelper *m_lastoreSessionHelper;
    UpdateInter *m_updateInter;
    ManagerInter *m_managerInter;
    PowerInter *m_powerInter;
    PowerSystemInter *m_powerSystemInter;
    Network *m_networkInter;
    SmartMirrorInter *m_smartMirrorInter;
    Appearance *m_iconTheme;
    bool m_onBattery;
    double m_batteryPercentage;
    double m_batterySystemPercentage;
    QString m_jobPath;
    QString m_iconThemeState;

    // 当前备份状态
    BackupStatus m_backupStatus;
    // 当前正在备份的更新分类类型
    ClassifyUpdateType m_backingUpClassifyType;
    QList<ClassifyUpdateType> m_fixErrorUpdate;

    QMutex m_mutex;
    QMutex m_downloadMutex;
    QList<UpdateLogItem> m_updateLogs;
};

}
}
#endif // UPDATEWORK_H
