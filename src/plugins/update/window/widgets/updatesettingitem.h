// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UpdateSettingItem_H
#define UpdateSettingItem_H

#include <QWidget>
#include <QLabel>
#include <DFloatingButton>
#include <DCommandLinkButton>
#include <DLabel>
#include <DLineEdit>
#include <DTextEdit>
#include <DTipLabel>
#include <DShadowLine>

#include "widgets/settingsgroup.h"
#include "widgets/contentwidget.h"
#include "widgets/settingsitem.h"
#include "widgets/labels/tipslabel.h"
#include "widgets/detailinfoitem.h"
#include "updatecontrolpanel.h"
#include "updateiteminfo.h"

namespace dcc {
namespace update {

struct Error_Info {
    UpdateErrorType ErrorType;
    QString errorMessage;
    QString errorTips;
};

class UpdateSettingItem: public dcc::widgets::SettingsItem
{
    Q_OBJECT
public:
    explicit UpdateSettingItem(QWidget *parent = nullptr);
    void initUi();
    void initConnect();

    void setIconVisible(bool show);
    void setIcon(QString path);
    void setProgress(double value);

    virtual void setData(UpdateItemInfo *updateItemInfo);

    UpdatesStatus status() const;
    void setStatus(const UpdatesStatus &status);

    ClassifyUpdateType classifyUpdateType() const;
    void setClassifyUpdateType(const ClassifyUpdateType &classifyUpdateType);

    UpdateErrorType getUpdateJobErrorMessage() const;
    void setUpdateJobErrorMessage(const UpdateErrorType &updateJobErrorMessage);

    void setUpdateFailedInfo();
    void updateStarted();

    inline qlonglong updateSize() const { return m_updateSize; }

Q_SIGNALS:
    void UpdateSuccessed();
    void recoveryBackupFailed();
    void recoveryBackupSuccessed();
    void requestRefreshSize();
    void requestRefreshWidget();
    void requestFixError(const ClassifyUpdateType &updateType, const QString &error);

    void requestUpdate(ClassifyUpdateType type);
    void requestUpdateCtrl(ClassifyUpdateType type, int ctrlType);

public Q_SLOTS:
    virtual void showMore();

    void onStartUpdate();
    void onStartDownload();
    void onPauseDownload();
    void onRetryUpdate();

    void onUpdateStatusChanged(const UpdatesStatus &status);

private:
    void setWidgetsEnabled(bool enable);

private:
    dcc::widgets::SmallLabel *m_icon;
    UpdatesStatus m_status;
    ClassifyUpdateType m_classifyUpdateType;
    qlonglong m_updateSize;
    double m_progressValue;
    UpdateErrorType m_updateJobErrorMessage;
    QMap<UpdateErrorType, Error_Info> m_UpdateErrorInfoMap;

protected:
    updateControlPanel *m_controlWidget;
    dcc::widgets::SettingsGroup *m_settingsGroup;
};

}
}


#endif //UpdateSettingItem_H
