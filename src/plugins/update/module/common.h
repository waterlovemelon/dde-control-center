// SPDX-FileCopyrightText: 2016 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef COMMON_H
#define COMMON_H

#include <QString>

#include <vector>
#include <qregexp.h>

using namespace std;

namespace dcc {
namespace update {

const double Epsion = 1e-6;
const QString SystemUpdateType = "system_upgrade";
const QString AppStoreUpdateType = "appstore_upgrade";
const QString SecurityUpdateType = "security_upgrade";
const QString UnknownUpdateType = "unknown_upgrade";

enum UpdatesStatus {
    Default,
    Checking,
    CheckingFailed,
    Updated,
    UpdatesAvailable,
    Updating,
    Downloading,
    DownloadPaused,
    Downloaded,
    DownloadFailed,
    AutoDownloaded,
    Installing,
    UpdateSucceeded,
    UpdateFailed,
    NeedRestart,
    WaitRecoveryBackup,
    RecoveryBackingUp,
    RecoveryBackupSuccessfully,
    RecoveryBackupFailed,
    SystemIsNotActive,
    UpdateIsDisabled
};

enum UpdateErrorType {
    NoError,
    NoNetwork,
    NoSpace,
    DependenciesBrokenError,
    DpkgInterrupted,
    UnKnown
};

enum ClassifyUpdateType {
    Invalid             = 0,        // 无效
    SystemUpdate        = 1 << 0,   // 系统
    AppStoreUpdate      = 1 << 1,   // 应用商店（1050版本弃用）
    SecurityUpdate      = 1 << 2,   // 安全
    UnknownUpdate       = 1 << 3,   // 未知来源
    OnlySecurityUpdate  = 1 << 4    // 仅安全更新（1060版本弃用）
};

enum UpdateCtrlType {
    Start = 0,
    Pause
};

enum BackupStatus {
    NoBackup,
    BackingUp,
    BackedUp,
    BackupFailed
};


static inline ClassifyUpdateType uintToClassifyUpdateType(uint type)
{
    ClassifyUpdateType value = ClassifyUpdateType::Invalid;
    switch (type) {
    case ClassifyUpdateType::SystemUpdate:
        value = ClassifyUpdateType::SystemUpdate;
        break;
    case ClassifyUpdateType::SecurityUpdate:
        value = ClassifyUpdateType::SecurityUpdate;
        break;
    case ClassifyUpdateType::UnknownUpdate:
        value = ClassifyUpdateType::UnknownUpdate;
        break;
    default:
        value = ClassifyUpdateType::Invalid;
        break;
    }

    return value;
}

//equal : false
static inline bool compareDouble(const double value1, const double value2)
{
    return !((value1 - value2 >= -Epsion) && (value1 - value2 <= Epsion));
}

static inline QString formatCap(qulonglong cap, const int size = 1024)
{
    static QString type[] = {"B", "KB", "MB", "GB", "TB"};

    if (cap < qulonglong(size)) {
        return QString::number(cap) + type[0];
    }
    if (cap < qulonglong(size) * size) {
        return QString::number(double(cap) / size, 'f', 2) + type[1];
    }
    if (cap < qulonglong(size) * size * size) {
        return QString::number(double(cap) / size / size, 'f', 2) + type[2];
    }
    if (cap < qulonglong(size) * size * size * size) {
        return QString::number(double(cap) / size / size / size, 'f', 2) + type[3];
    }

    return QString::number(double(cap) / size / size / size / size, 'f', 2) + type[4];
}

static inline vector<double> getNumListFromStr(const QString &str)
{
    //筛选出字符串中的数字
    QRegExp rx("-?[1-9]\\d*\\.\\d*|0+.[0-9]+|-?0\\.\\d*[1-9]\\d*|-?\\d+");
    int pos = 0;
    vector<double> v;
    while ((pos = rx.indexIn(str, pos)) != -1) {
        pos += rx.matchedLength();
        v.push_back(rx.cap(0).toDouble());
    }
    return v;
}

}
}

#endif // COMMON_H
