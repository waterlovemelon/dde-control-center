// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "interface/namespace.h"
#include "widgets/contentwidget.h"
#include "common.h"
#include "widgets/utils.h"
#include "updatesettingitem.h"
#include "systemupdateitem.h"
#include "downloadcontrolpanel.h"
#include "safeupdateitem.h"
#include "unknownupdateitem.h"
#include "updateiteminfo.h"
#include "updatemodesettingitem.h"
#include "loadingitem.h"

#include <QWidget>
#include <DSpinner>

class AppUpdateInfo;
class QPushButton;

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace dcc {
namespace update {
class UpdateModel;
class DownloadInfo;
class SummaryItem;
class DownloadProgressBar;
class ResultItem;
class SystemUpdateItem;
class AppstoreUpdateItem;
class SafeUpdateItem;
class UnknownUpdateItem;
class UpdateSettingItem;
}

namespace widgets {
class SettingsGroup;
class TipsLabel;
}
}

using namespace dcc::update;

namespace DCC_NAMESPACE {
namespace update {

class UpdateCtrlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UpdateCtrlWidget(UpdateModel *model, QWidget *parent = 0);
    ~UpdateCtrlWidget();

    void initConnect();
    void initUI();
    void setModel(UpdateModel *model);
    void setSystemVersion(const QString &version);

    UpdateErrorType updateJobErrorMessage() const;
    void setUpdateJobErrorMessage(const UpdateErrorType &updateJobErrorMessage);
    void setUpdateFailedInfo(const UpdateErrorType &errorType);

Q_SIGNALS:
    void notifyUpdateState(int);
    void requestUpdates(ClassifyUpdateType type);
    void requestDownload();
    void requestUpdateCtrl(int ctrlType);
    void requestOpenAppStroe();
    void requestFixError(const ClassifyUpdateType &updateType, const QString &error);
    void requestUpdateMode(quint64 updateMode);

public Q_SLOTS:
    void onShowUpdateCtrl();

private Q_SLOTS:
    void downloadAll();
    void onRequestRefreshSize();
    void onClassifyUpdateJonErrorChanged(const UpdateErrorType &errorType);
    void onUpdateModeChanged(quint64 mode);

private:
    void setStatus(const UpdatesStatus &status);

    void setSystemUpdateStatus(const UpdatesStatus &status);
    void setSafeUpdateStatus(const UpdatesStatus &status);
    void setUnknownUpdateStatus(const UpdatesStatus &status);

    void setSystemUpdateInfo(UpdateItemInfo *updateItemInfo);
    void setSafeUpdateInfo(UpdateItemInfo *updateItemInfo);
    void setUnknownUpdateInfo(UpdateItemInfo *updateItemInfo);
    void setAllUpdateInfo(QMap<ClassifyUpdateType, UpdateItemInfo *> updateInfoMap);

    void setUpdateProgress(const double value);
    void setActiveState(const UiActiveState &activestate);
    void showUpdateInfo();
    void onChangeUpdatesAvailableStatus();
    void initUpdateItem(UpdateSettingItem *updateItem);
    void showUpdateItem(quint64 mode);
    void hideAll();
    CheckUpdateItem::CheckUpdateStatus getCheckUpdateStatus(CheckUpdateItem::CheckUpdateStatus defaultStatus);

private:
    UpdateModel *m_model;
    UpdatesStatus m_status;
    QWidget *m_downloadStatusWidget;
    UpdateModeSettingItem *m_updateModeSettingItem;
    CheckUpdateItem *m_checkUpdateItem;
    ResultItem *m_resultItem;
    dcc::widgets::TipsLabel *m_reminderTip;
    dcc::widgets::TipsLabel *m_noNetworkTip;
    QString m_systemVersion;
    dcc::ContentWidget *m_updateList;
    DLabel *m_versionTip;
    DLabel *m_updateTipsLab;
    DLabel *m_updateSizeLab;
    DLabel *m_updatingTipsLab;
    DownloadControlPanel *m_downloadControlPanel;
    qlonglong m_updateSize;
    SystemUpdateItem *m_systemUpdateItem;
    SafeUpdateItem *m_safeUpdateItem;
    UnknownUpdateItem *m_unknownUpdateItem;
    QMap<ClassifyUpdateType, UpdateSettingItem *> m_updatingItemMap;
    dcc::widgets::SettingsGroup *m_updateSummaryGroup;
    UpdateErrorType m_updateJobErrorMessage;
    QMap<UpdateErrorType, Error_Info> m_UpdateErrorInfoMap;
};

}// namespace datetime
}// namespace DCC_NAMESPACE
