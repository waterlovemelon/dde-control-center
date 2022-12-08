// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatesettingitem.h"
#include "widgets/basiclistdelegate.h"
#include "window/utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <qpushbutton.h>
#include <QIcon>

using namespace dcc;
using namespace dcc::widgets;
using namespace dcc::update;

UpdateSettingItem::UpdateSettingItem(QWidget *parent)
    : SettingsItem(parent)
    , m_icon(new SmallLabel(this))
    , m_status(UpdatesStatus::Default)
    , m_updateSize(0)
    , m_progressValue(0)
    , m_updateJobErrorMessage(UpdateErrorType::NoError)
    , m_controlWidget(new updateControlPanel(this))
    , m_settingsGroup(new dcc::widgets::SettingsGroup(this, SettingsGroup::BackgroundStyle::NoneBackground))
{
    m_UpdateErrorInfoMap.insert(UpdateErrorType::NoError, { UpdateErrorType::NoError, "", "" });
    m_UpdateErrorInfoMap.insert(UpdateErrorType::NoSpace, { UpdateErrorType::NoSpace, tr("Insufficient disk space"), tr("Update failed: insufficient disk space") });
    m_UpdateErrorInfoMap.insert(UpdateErrorType::UnKnown, { UpdateErrorType::UnKnown, tr("Update failed"), "" });
    m_UpdateErrorInfoMap.insert(UpdateErrorType::NoNetwork, { UpdateErrorType::NoNetwork, tr("Network error"), tr("Network error, please check and try again") });
    m_UpdateErrorInfoMap.insert(UpdateErrorType::DpkgInterrupted, { UpdateErrorType::DpkgInterrupted, tr("Packages error"), tr("Packages error, please try again") });
    m_UpdateErrorInfoMap.insert(UpdateErrorType::DependenciesBrokenError, { UpdateErrorType::DependenciesBrokenError, tr("Dependency error"), tr("Unmet dependencies") });

    initUi();
    initConnect();
}

void UpdateSettingItem::initUi()
{
    m_icon->setFixedSize(48, 48);
    m_icon->setVisible(false);
    QWidget *widget = new QWidget();
    QVBoxLayout *vboxLay = new QVBoxLayout(widget);
    vboxLay->addWidget(m_icon);
    vboxLay->setContentsMargins(10, 6, 10, 10);
    widget->setLayout(vboxLay);

    QHBoxLayout *main = new QHBoxLayout;
    main->setMargin(0);
    main->setSpacing(0);
    main->setContentsMargins(10, 10, 0, 0);
    m_settingsGroup->appendItem(m_controlWidget);
    m_settingsGroup->setSpacing(0);
    main->addWidget(widget, 0, Qt::AlignTop);
    main->addWidget(m_settingsGroup, 0, Qt::AlignTop);
    setLayout(main);
}

void UpdateSettingItem::setIconVisible(bool show)
{
    m_icon->setVisible(show);
}

void UpdateSettingItem::setIcon(QString path)
{
    const qreal ratio = devicePixelRatioF();
    QPixmap pix = loadPixmap(path).scaled(m_icon->size() * ratio,
                                          Qt::KeepAspectRatioByExpanding,
                                          Qt::SmoothTransformation);
    m_icon->setPixmap(pix);
}

void UpdateSettingItem::showMore()
{
    return;
}

ClassifyUpdateType UpdateSettingItem::classifyUpdateType() const
{
    return m_classifyUpdateType;
}

void UpdateSettingItem::setClassifyUpdateType(const ClassifyUpdateType &classifyUpdateType)
{
    m_classifyUpdateType = classifyUpdateType;
}

UpdatesStatus UpdateSettingItem::status() const
{
    return m_status;
}

void UpdateSettingItem::setStatus(const UpdatesStatus &status)
{

}

void UpdateSettingItem::setData(UpdateItemInfo *updateItemInfo)
{
    if (updateItemInfo == nullptr) {
        setVisible(false);
        return;
    }

    QString value = updateItemInfo->updateTime().isEmpty() ? "" : tr("Release date: ") + updateItemInfo->updateTime();
    m_controlWidget->setDate(value);
    const QString &systemVersionType = DCC_NAMESPACE::IsServerSystem ? tr("Server") : tr("Desktop");
    QString version;
    if (!updateItemInfo->availableVersion().isEmpty()) {
        QString avaVersion = updateItemInfo->availableVersion();
        QString tmpVersion = avaVersion;
        if (dccV20::IsProfessionalSystem)
            tmpVersion = avaVersion.replace(avaVersion.length() - 1, 1, '0'); // 替换版本号的最后一位为‘0‘
        version = tr("Version") + ": " + systemVersionType + tmpVersion;
    }
    m_controlWidget->setVersion(version);
    m_controlWidget->setTitle(updateItemInfo->name());
    m_controlWidget->setDetail(updateItemInfo->explain());
    setWidgetsEnabled(updateItemInfo->isUpdateModeEnabled());
    m_updateSize = updateItemInfo->downloadSize();
    m_status = UpdatesStatus::UpdatesAvailable;
}

void UpdateSettingItem::onUpdateStatusChanged(const UpdatesStatus &status)
{
    if (m_status != status) {
        setStatus(status);
    }
}

UpdateErrorType UpdateSettingItem::getUpdateJobErrorMessage() const
{
    return m_updateJobErrorMessage;
}

void UpdateSettingItem::setUpdateJobErrorMessage(const UpdateErrorType &updateJobErrorMessage)
{
    m_updateJobErrorMessage = updateJobErrorMessage;
}

void UpdateSettingItem::setUpdateFailedInfo()
{
    QString failedInfo = "";
    QString failedTips = "";
    UpdateErrorType errorType = getUpdateJobErrorMessage();
    if (m_UpdateErrorInfoMap.contains(errorType)) {
        Error_Info info = m_UpdateErrorInfoMap.value(errorType);
        failedInfo = info.errorMessage;
        failedTips = info.errorTips;
    }
}

void UpdateSettingItem::initConnect()
{
    connect(m_controlWidget, &updateControlPanel::showDetail, this, &UpdateSettingItem::showMore);
}

void UpdateSettingItem::setWidgetsEnabled(bool enable)
{
    for (auto child : findChildren<QWidget*>()) {
            // 不禁用EnableSwitchButton及其父窗口
            bool isEnableSwitchButtonParent = false;
            for (auto w : child->findChildren<QWidget*>()) {
                if (w->objectName() == "EnableSwitchButton") {
                    isEnableSwitchButtonParent = true;
                    break;
                }
            }

            if (!isEnableSwitchButtonParent && child->objectName() != "EnableSwitchButton")
                child->setEnabled(enable);
        }

}

void UpdateSettingItem::onStartUpdate()
{
    Q_EMIT requestUpdate(m_classifyUpdateType);
}

void UpdateSettingItem::onStartDownload()
{
    int ctrlType = UpdateCtrlType::Start;
    Q_EMIT requestUpdateCtrl(m_classifyUpdateType, ctrlType);
}

void UpdateSettingItem::onPauseDownload()
{
    int ctrlType = UpdateCtrlType::Pause;
    Q_EMIT requestUpdateCtrl(m_classifyUpdateType, ctrlType);
}

void UpdateSettingItem::onRetryUpdate()
{
    if (m_updateJobErrorMessage == UpdateErrorType::DpkgInterrupted) {
        Q_EMIT requestFixError(m_classifyUpdateType, "dpkgInterrupted");
        return;
    }

    if (m_updateJobErrorMessage == UpdateErrorType::DependenciesBrokenError) {
        Q_EMIT requestFixError(m_classifyUpdateType, "dependenciesBroken");
        return;
    }

    if (m_updateJobErrorMessage == UpdateErrorType::NoNetwork) {
        Q_EMIT requestFixError(m_classifyUpdateType, "noNetwork");
        return;
    }
}

void UpdateSettingItem::updateStarted()
{
}
