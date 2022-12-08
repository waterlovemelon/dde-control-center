// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UPDATEMODEL_H
#define UPDATEMODEL_H

#include <com_deepin_lastore_updater.h>

#include "common.h"
#include "updateiteminfo.h"
#include "widgets/utils.h"

#include <QJsonDocument>
#include <QObject>

namespace dcc {
namespace update {

class DownloadInfo : public QObject {
  Q_OBJECT
public:
    explicit DownloadInfo(const qlonglong &downloadSize, const QList<AppUpdateInfo> &appInfos, QObject *parent = 0);
    explicit DownloadInfo(QObject *parent = 0);
    virtual ~DownloadInfo() {}

    void setDownloadSize(qlonglong size);
    inline qulonglong downloadSize() const { return m_downloadSize; }

    void setDownloadProgress(double downloadProgress);
    double downloadProgress() const { return m_downloadProgress; }

    QList<AppUpdateInfo> appInfos() const { return m_appInfos; }


Q_SIGNALS:
  void downloadProgressChanged(const double &progress);
  void downloadSizeChanged(qlonglong size);

private:
  qlonglong m_downloadSize;
  double m_downloadProgress;
  QList<AppUpdateInfo> m_appInfos;
};

struct UpdateJobErrorMessage {
  QString jobErrorType;
  QString jobErrorMessage;
};

class UpdateModel : public QObject {
  Q_OBJECT
public:
  explicit UpdateModel(QObject *parent = nullptr);
  ~UpdateModel();

public:
  enum TestingChannelStatus {
    Hidden,
    NotJoined,
    WaitJoined,
    Joined,
  };
  Q_ENUM(TestingChannelStatus);

  struct IdleDownloadConfig {
    bool idleDownloadEnabled = true;
    QString beginTime = "17:00";
    QString endTime = "20:00";

    static IdleDownloadConfig toConfig(const QByteArray &configStr) {
      IdleDownloadConfig config;
      QJsonParseError jsonParseError;
      const QJsonDocument doc =
          QJsonDocument::fromJson(configStr, &jsonParseError);
      if (jsonParseError.error != QJsonParseError::NoError || doc.isEmpty()) {
        qWarning() << "Parse idle download config failed: "
                   << jsonParseError.errorString();
        return config;
      }

      QJsonObject obj = doc.object();
      config.idleDownloadEnabled =
          obj.contains("IdleDownloadEnabled")
              ? obj.value("IdleDownloadEnabled").toBool()
              : true;
      config.beginTime = obj.contains("BeginTime")
                             ? obj.value("BeginTime").toString()
                             : "17:00";
      config.endTime =
          obj.contains("EndTime") ? obj.value("EndTime").toString() : "20:00";

      return config;
    }

    QByteArray toJson() const {
      QJsonObject obj;
      obj.insert("IdleDownloadEnabled", idleDownloadEnabled);
      obj.insert("BeginTime", beginTime);
      obj.insert("EndTime", endTime);

      QJsonDocument doc;
      doc.setObject(obj);
      return doc.toJson();
    }

    bool operator==(const IdleDownloadConfig &config) const {
      return config.idleDownloadEnabled == idleDownloadEnabled &&
             config.beginTime == beginTime && config.endTime == endTime;
    }
  };

  void setMirrorInfos(const MirrorInfoList &list);
  MirrorInfoList mirrorInfos() const { return m_mirrorList; }

  UpdatesStatus status() const;
  void setStatus(const UpdatesStatus &status);
  void setStatus(const UpdatesStatus &status, int line);

  MirrorInfo defaultMirror() const;
  void setDefaultMirror(const QString &mirrorId);

  DownloadInfo *downloadInfo();
  void setDownloadInfo(DownloadInfo *downloadInfo);

  UpdateItemInfo *systemDownloadInfo() const;
  void setSystemDownloadInfo(UpdateItemInfo *updateItemInfo);

  UpdateItemInfo *safeDownloadInfo() const;
  void setSafeDownloadInfo(UpdateItemInfo *updateItemInfo);

  UpdateItemInfo *unknownDownloadInfo() const;
  void setUnknownDownloadInfo(UpdateItemInfo *updateItemInfo);

  QMap<ClassifyUpdateType, UpdateItemInfo *> allDownloadInfo() const;
  void setAllDownloadInfo(
      QMap<ClassifyUpdateType, UpdateItemInfo *> &allUpdateItemInfo);

  QMap<QString, int> mirrorSpeedInfo() const;
  void setMirrorSpeedInfo(const QMap<QString, int> &mirrorSpeedInfo);

  bool lowBattery() const;
  void setLowBattery(bool lowBattery);

  bool autoDownloadUpdates() const;
  void setAutoDownloadUpdates(bool autoDownloadUpdates);

  double upgradeProgress() const;
  void setUpgradeProgress(double upgradeProgress);

  bool autoCleanCache() const;
  void setAutoCleanCache(bool autoCleanCache);

  double updateProgress() const;
  void setUpdateProgress(double updateProgress);

#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
  bool sourceCheck() const;
  void setSourceCheck(bool sourceCheck);
#endif

  bool netselectExist() const;
  void setNetselectExist(bool netselectExist);

  inline quint64 updateMode() const { return m_updateMode; }
  void setUpdateMode(quint64 updateMode);

  inline bool autoCheckSecureUpdates() const {
    return m_autoCheckSecureUpdates;
  }
  void setAutoCheckSecureUpdates(bool autoCheckSecureUpdates);

  inline bool autoCheckSystemUpdates() const {
    return m_autoCheckSystemUpdates;
  }
  void setAutoCheckSystemUpdates(bool autoCheckSystemUpdates);

  inline bool autoCheckAppUpdates() const { return m_autoCheckAppUpdates; }
  void setAutoCheckAppUpdates(bool autoCheckAppUpdates);

  bool smartMirrorSwitch() const { return m_smartMirrorSwitch; }
  void setSmartMirrorSwitch(bool smartMirrorSwitch);

  inline bool recoverBackingUp() const { return m_bRecoverBackingUp; }
  void setRecoverBackingUp(bool recoverBackingUp);

  inline bool recoverConfigValid() const { return m_bRecoverConfigValid; }
  void setRecoverConfigValid(bool recoverConfigValid);

  inline bool recoverRestoring() const { return m_bRecoverRestoring; }
  void setRecoverRestoring(bool recoverRestoring);

  inline QString systemVersionInfo() const { return m_systemVersionInfo; }
  void setSystemVersionInfo(const QString &systemVersionInfo);

  inline UiActiveState systemActivation() const { return m_bSystemActivation; }
  void setSystemActivation(const UiActiveState &systemactivation);

  inline bool getUpdatablePackages() const { return m_isUpdatablePackages; }
  void isUpdatablePackages(bool isUpdatablePackages);

  const QString &lastCheckUpdateTime() const { return m_lastCheckUpdateTime; }
  void setLastCheckUpdateTime(const QString &lastTime);

  const QList<AppUpdateInfo> &historyAppInfos() const {
    return m_historyAppInfos;
  }
  void setHistoryAppInfos(const QList<AppUpdateInfo> &infos);

  int autoCheckUpdateCircle() const { return m_autoCheckUpdateCircle; }
  void setAutoCheckUpdateCircle(const int interval);
  bool enterCheckUpdate();

  inline bool updateNotify() { return m_updateNotify; }
  void setUpdateNotify(const bool notify);

  void deleteUpdateInfo(UpdateItemInfo *updateItemInfo);

  bool getAutoInstallUpdates() const;
  void setAutoInstallUpdates(bool autoInstallUpdates);

  quint64 getAutoInstallUpdateType() const;
  void setAutoInstallUpdateType(const quint64 &autoInstallUpdateType);

  QMap<ClassifyUpdateType, UpdateItemInfo *> getAllUpdateInfos() const;
  void setAllUpdateInfos(const QMap<ClassifyUpdateType, UpdateItemInfo *> &allUpdateInfos);

  UpdateJobErrorMessage getSystemUpdateJobError() const;
  void setSystemUpdateJobError(const UpdateJobErrorMessage &systemUpdateJobError);

  UpdateJobErrorMessage getSafeUpdateJobError() const;
  void setSafeUpdateJobError(const UpdateJobErrorMessage &safeUpdateJobError);

  UpdateJobErrorMessage getUnknownUpdateJobError() const;
  void
  setUnknownUpdateJobError(const UpdateJobErrorMessage &UnkonwUpdateJobError);

  void setClassifyUpdateJonError(UpdateErrorType errorType);

  QMap<ClassifyUpdateType, UpdateErrorType> getUpdateErrorTypeMap() const;

  bool getAutoCheckThirdPartyUpdates() const;
  void setAutoCheckThirdPartyUpdates(bool autoCheckThirdpartyUpdates);

  void setIdleDownloadConfig(const IdleDownloadConfig &config);
  inline IdleDownloadConfig idleDownloadConfig() const {
    return m_idleDownloadConfig;
  };

  QString getHostName() const;
  QString getMachineID() const;

  void setTestingChannelStatus(const TestingChannelStatus status);
  TestingChannelStatus getTestingChannelStatus() const;
  QString getTestingChannelServer() const;
  void setTestingChannelServer(const QString server);
  QUrl getTestingChannelJoinURL() const;
  void setCanExitTestingChannel(const bool can);
  qlonglong downloadSize() const;

  UpdateErrorType lastError() { return m_lastError; }

public slots:
  void onUpdatePropertiesChanged(const QString &interfaceName,
                                 const QVariantMap &changedProperties,
                                 const QStringList &invalidatedProperties);

Q_SIGNALS:
  void autoDownloadUpdatesChanged(const bool &autoDownloadUpdates);
  void autoInstallUpdatesChanged(const bool &autoInstallUpdates);
  void autoInstallUpdateTypeChanged(const quint64 &autoInstallUpdateType);
  void defaultMirrorChanged(const MirrorInfo &mirror);

  void smartMirrorSwitchChanged(bool smartMirrorSwitch);

  void lowBatteryChanged(const bool &lowBattery);
  void statusChanged(const UpdatesStatus &status);
  void systemUpdateDownloadSizeChanged(const qlonglong updateSize);
  void safeUpdateDownloadSizeChanged(const qlonglong updateSize);
  void unknownUpdateDownloadSizeChanged(const qlonglong updateSize);

#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
  void sourceCheckChanged(bool sourceCheck);
#endif
  void mirrorSpeedInfoAvailable(const QMap<QString, int> &mirrorSpeedInfo);

  void downloadInfoChanged(DownloadInfo *downloadInfo);

  void systemUpdateInfoChanged(UpdateItemInfo *updateItemInfo);
  void safeUpdateInfoChanged(UpdateItemInfo *updateItemInfo);
  void unknownUpdateInfoChanged(UpdateItemInfo *updateItemInfo);

  void classifyUpdateJobErrorChanged(const UpdateErrorType &errorType);
  void updateProgressChanged(const double &updateProgress);
  void upgradeProgressChanged(const double &upgradeProgress);
  void autoCleanCacheChanged(const bool autoCleanCache);
  void netselectExistChanged(const bool netselectExist);
  void autoCheckSystemUpdatesChanged(const bool autoCheckSystemUpdate);
  void autoCheckAppUpdatesChanged(const bool autoCheckAppUpdate);
  void autoCheckSecureUpdatesChanged(const bool autoCheckSecureUpdate);
  void autoCheckThirdPartyUpdatesChanged(const bool autoCheckThirdpartyUpdate);
  void recoverBackingUpChanged(bool recoverBackingUp);
  void recoverConfigValidChanged(bool recoverConfigValid);
  void recoverRestoringChanged(bool recoverRestoring);
  void systemVersionChanged(QString version);
  void systemActivationChanged(UiActiveState systemactivation);
  void beginCheckUpdate();
  void updateCheckUpdateTime();
  void updateHistoryAppInfos();
  void updateNotifyChanged(const bool notify);
  void updatablePackagesChanged(const bool isUpdatablePackages);
  void testingChannelStatusChanged(const TestingChannelStatus status);
  void canExitTestingChannelChanged(const bool can);
  void idleDownloadConfigChanged();
  void updateModeChanged(quint64 updateMode);

private:
  void setUpdateItemEnabled();

private:
  UpdatesStatus m_status;
  DownloadInfo m_downloadInfo;

  QMap<ClassifyUpdateType, UpdateItemInfo *> m_allUpdateInfos;
  UpdateItemInfo *m_systemUpdateInfo;
  UpdateItemInfo *m_safeUpdateInfo;
  UpdateItemInfo *m_unknownUpdateInfo;

  double m_updateProgress;
  double m_upgradeProgress;

#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
  bool m_sourceCheck;
#endif

  bool m_lowBattery;
  bool m_netselectExist;
  bool m_autoCleanCache;
  bool m_autoDownloadUpdates;
  bool m_autoInstallUpdates;
  quint64 m_autoInstallUpdateType;
  quint64 m_updateMode;
  bool m_autoCheckSecureUpdates;
  bool m_autoCheckSystemUpdates;
  bool m_autoCheckAppUpdates;
  bool m_autoCheckThirdPartyUpdates;
  bool m_updateNotify;
  bool m_smartMirrorSwitch;

  QString m_mirrorId;
  MirrorInfoList m_mirrorList;
  QMap<QString, int> m_mirrorSpeedInfo;
  bool m_bRecoverBackingUp;
  bool m_bRecoverConfigValid;
  bool m_bRecoverRestoring;
  QString m_systemVersionInfo;
  UiActiveState m_bSystemActivation;

  QString m_lastCheckUpdateTime;          // 上次检查更新时间
  QList<AppUpdateInfo> m_historyAppInfos; // 历史更新应用列表
  int m_autoCheckUpdateCircle; // 决定进入检查更新界面是否自动检查,单位：小时

  QMap<ClassifyUpdateType, UpdateErrorType> m_updateErrorTypeMap;
  UpdateErrorType m_lastError;
  UpdateJobErrorMessage m_systemUpdateJobError;
  UpdateJobErrorMessage m_safeUpdateJobError;
  UpdateJobErrorMessage m_UnknownUpdateJobError;

  QString m_testingChannelServer;
  TestingChannelStatus m_testingChannelStatus;
  bool m_isUpdatablePackages;
  IdleDownloadConfig m_idleDownloadConfig;
};

} // namespace update
} // namespace dcc
#endif // UPDATEMODEL_H
