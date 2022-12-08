// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatectrlwidget.h"
#include "widgets/translucentframe.h"
#include "updatemodel.h"
#include "loadingitem.h"
#include "widgets/settingsgroup.h"
#include "summaryitem.h"
#include "downloadprogressbar.h"
#include "resultitem.h"
#include "widgets/labels/tipslabel.h"
#include "window/dconfigwatcher.h"

#include <types/appupdateinfolist.h>
#include <QVBoxLayout>
#include <QSettings>
#include <QPushButton>
#include <QScrollArea>

#include <DFontSizeManager>
#include <DPalette>
#include <DSysInfo>
#include <DDialog>
#include <DConfig>
#include <DDBusSender>

#define UpgradeWarningSize 500
#define FullUpdateBtnWidth 120

using namespace dcc;
using namespace dcc::update;
using namespace dcc::widgets;
using namespace DCC_NAMESPACE;
using namespace DCC_NAMESPACE::update;
using namespace Dtk::Widget;

UpdateCtrlWidget::UpdateCtrlWidget(UpdateModel *model, QWidget *parent)
    : QWidget(parent)
    , m_model(nullptr)
    , m_status(UpdatesStatus::Updated)
    , m_downloadStatusWidget(new QWidget(this))
    , m_updateModeSettingItem(new UpdateModeSettingItem(model, this))
    , m_checkUpdateItem(new CheckUpdateItem(model))
    , m_resultItem(new ResultItem())
    , m_reminderTip(new TipsLabel)
    , m_noNetworkTip(new TipsLabel)
    , m_updateList(new ContentWidget(parent))
    , m_versionTip(new DLabel(parent))
    , m_updateTipsLab(new DLabel(parent))
    , m_updateSizeLab(new DLabel(parent))
    , m_updatingTipsLab(new DLabel(parent))
    , m_downloadControlPanel(new DownloadControlPanel(this))
    , m_updateSize(0)
    , m_systemUpdateItem(new SystemUpdateItem(parent))
    , m_safeUpdateItem(new SafeUpdateItem(parent))
    , m_unknownUpdateItem(new UnknownUpdateItem(parent))
    , m_updateSummaryGroup(new SettingsGroup)
{
    m_UpdateErrorInfoMap.insert(UpdateErrorType::NoError, { UpdateErrorType::NoError, "", "" });
    m_UpdateErrorInfoMap.insert(UpdateErrorType::NoSpace, { UpdateErrorType::NoSpace, tr("Update failed: insufficient disk space"), tr("") });
    m_UpdateErrorInfoMap.insert(UpdateErrorType::UnKnown, { UpdateErrorType::UnKnown, "", "" });
    m_UpdateErrorInfoMap.insert(UpdateErrorType::NoNetwork, { UpdateErrorType::NoNetwork, tr("Dependency error, failed to detect the updates"), tr("") });
    m_UpdateErrorInfoMap.insert(UpdateErrorType::DpkgInterrupted, { UpdateErrorType::DpkgInterrupted, "", "" });
    m_UpdateErrorInfoMap.insert(UpdateErrorType::DependenciesBrokenError, { UpdateErrorType::DependenciesBrokenError, "", "" });

    initUI();
    setModel(model);
    initConnect();
}

void UpdateCtrlWidget::initUI()
{
    setAccessibleName("UpdateCtrlWidget");
    m_checkUpdateItem->setAccessibleName("checkUpdateItem");

    m_updateList->setAccessibleName("updateList");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_updateModeSettingItem->setAccessibleName("UpdateModeSettingItem");
    m_updateModeSettingItem->setVisible(false);

    m_reminderTip->setText(tr("Restart the computer to use the system and the applications properly"));
    m_noNetworkTip->setText(tr("Network disconnected, please retry after connected"));

    m_reminderTip->setWordWrap(true);
    m_reminderTip->setAlignment(Qt::AlignHCenter);
    m_reminderTip->setVisible(false);

    m_noNetworkTip->setWordWrap(true);
    m_noNetworkTip->setAlignment(Qt::AlignHCenter);
    m_noNetworkTip->setVisible(false);

    QHBoxLayout *updateTitleHLay = new QHBoxLayout;
    QVBoxLayout *updateTitleFirstVLay = new QVBoxLayout;

    m_updateTipsLab->setText(tr("Updates Available"));
    DFontSizeManager::instance()->bind(m_updateTipsLab, DFontSizeManager::T5, QFont::DemiBold);
    m_updateTipsLab->setForegroundRole(DPalette::TextTitle);
    m_updateTipsLab->setVisible(false);

    DFontSizeManager::instance()->bind(m_versionTip, DFontSizeManager::T8);
    m_versionTip->setForegroundRole(DPalette::BrightText);
    m_versionTip->setEnabled(false);
    QString sVersion = QString("%1 %2").arg(Dtk::Core::DSysInfo::uosProductTypeName()).arg(Dtk::Core::DSysInfo::minorVersion());
    m_versionTip->setText(tr("Current Edition") + ": " + sVersion);

    updateTitleFirstVLay->addStretch();
    updateTitleFirstVLay->addWidget(m_updateTipsLab);
    DFontSizeManager::instance()->bind(m_updateSizeLab, DFontSizeManager::T8);
    m_updateSizeLab->setForegroundRole(DPalette::TextTips);
    updateTitleFirstVLay->addWidget(m_updateSizeLab);
    updateTitleFirstVLay->addStretch();

    updateTitleHLay->setContentsMargins(QMargins(22, 0, 20, 0));
    updateTitleHLay->addLayout(updateTitleFirstVLay);
    updateTitleHLay->addStretch(1);
    updateTitleHLay->addWidget(m_downloadControlPanel);
    m_downloadStatusWidget->setLayout(updateTitleHLay);
    m_downloadStatusWidget->setVisible(false);

    QHBoxLayout *resultItemLayout = new QHBoxLayout;
    resultItemLayout->setContentsMargins(QMargins(15, 0, 15, 0));
    resultItemLayout->addWidget(m_checkUpdateItem);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addSpacing(10);
    layout->addWidget(m_versionTip, 0, Qt::AlignHCenter);
    layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(m_updateModeSettingItem);
    layout->addWidget(m_downloadStatusWidget);
    layout->addWidget(m_updateList, 1);

    layout->addStretch();
    layout->addWidget(m_resultItem);
    layout->addLayout(resultItemLayout, 1);
    layout->addWidget(m_reminderTip);
    layout->addWidget(m_noNetworkTip);
    layout->addSpacing(10);
    layout->addStretch();

    setLayout(layout);

    QWidget *contentWidget = new QWidget;
    contentWidget->setAccessibleName("UpdateCtrlWidget_contentWidget");
    QVBoxLayout *contentLayout = new QVBoxLayout;

    m_systemUpdateItem->setVisible(false);
    m_updateSummaryGroup->appendItem(m_systemUpdateItem);

    m_safeUpdateItem->setVisible(false);
    m_updateSummaryGroup->appendItem(m_safeUpdateItem);

    m_unknownUpdateItem->setVisible(false);
    m_updateSummaryGroup->appendItem(m_unknownUpdateItem);

    m_updateSummaryGroup->setVisible(false);
    contentLayout->addWidget(m_updateSummaryGroup);
    contentLayout->addStretch();
    contentWidget->setLayout(contentLayout);
    contentWidget->setContentsMargins(10, 0, 10, 0);

    m_updateList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_updateList->setContent(contentWidget);
}

void UpdateCtrlWidget::initConnect()
{
    auto initUpdateItemConnect = [ = ](UpdateSettingItem * updateItem) {
        connect(updateItem, &UpdateSettingItem::requestRefreshSize, this, &UpdateCtrlWidget::onRequestRefreshSize);
        connect(updateItem, &UpdateSettingItem::requestFixError, this, [this] (const ClassifyUpdateType &updateType, const QString &error) {
            if (m_model->recoverConfigValid()) {
                Q_EMIT requestFixError(updateType, error);
            }
        });
        connect(m_updateModeSettingItem, &UpdateModeSettingItem::updateModeEnableStateChanged, this, [this] (ClassifyUpdateType type, bool enable) {
            const quint64 updateMode = enable ? m_model->updateMode() | type : m_model->updateMode() & (~type);
            Q_EMIT requestUpdateMode(updateMode);
        });
    };

    initUpdateItemConnect(m_systemUpdateItem);
    initUpdateItemConnect(m_safeUpdateItem);
    initUpdateItemConnect(m_unknownUpdateItem);

    connect(m_checkUpdateItem, &CheckUpdateItem::requestCheckUpdate, m_model, &UpdateModel::beginCheckUpdate);

    connect(m_downloadControlPanel, &DownloadControlPanel::PauseDownload, this, [this] {
        Q_EMIT requestUpdateCtrl(UpdateCtrlType::Pause);
    });
    connect(m_downloadControlPanel, &DownloadControlPanel::StartDownload, this, [this] {
        Q_EMIT requestUpdateCtrl(UpdateCtrlType::Start);
    });
    connect(m_downloadControlPanel, &DownloadControlPanel::requestDownload, this, &UpdateCtrlWidget::downloadAll);
    connect(m_downloadControlPanel, &DownloadControlPanel::requestUpdate, this, [] {
        qInfo() << "Request show dde-lock, interface: com.deepin.dde.shutdownFront";
        DDBusSender()
        .service("com.deepin.dde.lockFront")
        .interface("com.deepin.dde.shutdownFront")
        .path("/com/deepin/dde/shutdownFront")
        .method("UpdateAndReboot")
        .call();
    });
}

UpdateCtrlWidget::~UpdateCtrlWidget()
{

}

void UpdateCtrlWidget::setStatus(const UpdatesStatus &status)
{
    m_status = status;
    qDebug() << Q_FUNC_INFO << status;
    if (m_model->systemActivation() == UiActiveState::Unauthorized || m_model->systemActivation() == UiActiveState::TrialExpired)
        m_status = SystemIsNotActive;

    const DConfig *dconfig = DConfigWatcher::instance()->getModulesConfig(DConfigWatcher::update);
    if (dconfig && dconfig->isValid() && dconfig->value("disableUpdate", false).toBool())
        m_status = UpdateIsDisabled;

    Q_EMIT notifyUpdateState(m_status);

    m_updateModeSettingItem->setButtonsEnabled(true);

    switch (m_status) {
    case UpdatesStatus::Default:
        hideAll();
        m_updateModeSettingItem->setVisible(true);
        m_checkUpdateItem->setVisible(true);
        m_checkUpdateItem->setStatus(getCheckUpdateStatus(CheckUpdateItem::Default));
        //~ contents_path /update/Check for Updates
        //~ child_page Check for Updates
        break;
    case UpdatesStatus::SystemIsNotActive:
        hideAll();
        m_resultItem->setVisible(true);
        m_resultItem->setStatus(UpdatesStatus::SystemIsNotActive);
        break;
    case UpdatesStatus::UpdateIsDisabled:
        hideAll();
        m_resultItem->setVisible(true);
        m_resultItem->setStatus(UpdatesStatus::UpdateIsDisabled);
        break;
    case UpdatesStatus::Checking:
        hideAll();
        m_updateModeSettingItem->setButtonsEnabled(false);
        m_updateModeSettingItem->setVisible(true);
        m_checkUpdateItem->setVisible(true);
        m_checkUpdateItem->setStatus(CheckUpdateItem::Checking);
        break;
    case UpdatesStatus::CheckingFailed:
        hideAll();
        m_updateModeSettingItem->setVisible(true);
        m_checkUpdateItem->setVisible(true);
        m_checkUpdateItem->setStatus(CheckUpdateItem::CheckingFailed);
        break;
    case UpdatesStatus::UpdatesAvailable:
        hideAll();
        m_updateModeSettingItem->setVisible(true);
        m_downloadStatusWidget->setVisible(true);
        onChangeUpdatesAvailableStatus();
        break;
    case UpdatesStatus::Downloading:
        hideAll();
        m_updateModeSettingItem->setButtonsEnabled(false);
        m_updateTipsLab->setText(tr("Downloading...")); // TODO 翻译
        m_updateModeSettingItem->setVisible(true);
        m_downloadStatusWidget->setVisible(true);
        onChangeUpdatesAvailableStatus();
        break;
    case UpdatesStatus::DownloadPaused:
        m_updateTipsLab->setText(tr("Downloading...")); // TODO 翻译
        hideAll();
        m_updateModeSettingItem->setButtonsEnabled(false);
        m_updateModeSettingItem->setVisible(true);
        m_downloadStatusWidget->setVisible(true);
        onChangeUpdatesAvailableStatus();
        break;
    case UpdatesStatus::Downloaded:
        m_updateTipsLab->setText(tr("Update download completed")); // TODO 翻译
        hideAll();
        m_updateModeSettingItem->setButtonsEnabled(false);
        m_updateModeSettingItem->setVisible(true);
        m_downloadStatusWidget->setVisible(true);
        onChangeUpdatesAvailableStatus();
        break;
    case UpdatesStatus::DownloadFailed:
        hideAll();
        m_updateModeSettingItem->setVisible(true);
        m_checkUpdateItem->setVisible(true);
        m_checkUpdateItem->setStatus(CheckUpdateItem::Checking);
        setUpdateFailedInfo(updateJobErrorMessage());
        break;
    case UpdatesStatus::Updated:
        hideAll();
        m_updateModeSettingItem->setVisible(true);
        m_checkUpdateItem->setVisible(true);
        m_checkUpdateItem->setStatus(getCheckUpdateStatus(CheckUpdateItem::Updated ));
        break;
    case UpdatesStatus::NeedRestart:
        hideAll();
        m_checkUpdateItem->setVisible(true);
        m_checkUpdateItem->setStatus(CheckUpdateItem::NeedRestart);;
        break;
    default:
        qDebug() << "Unknown status!!!";
        break;
    }
}

void UpdateCtrlWidget::setSystemUpdateStatus(const UpdatesStatus &status)
{
    m_systemUpdateItem->setStatus(status);
}

void UpdateCtrlWidget::setSafeUpdateStatus(const UpdatesStatus &status)
{

    m_safeUpdateItem->setStatus(status);
}

void UpdateCtrlWidget::setUnknownUpdateStatus(const UpdatesStatus &status)
{
    m_unknownUpdateItem->setStatus(status);
}

void UpdateCtrlWidget::setUpdateProgress(const double value)
{
    m_checkUpdateItem->setProgressValue(static_cast<int>(value * 100));
}

void UpdateCtrlWidget::setActiveState(const UiActiveState &activestate)
{
    if (m_model->enterCheckUpdate()) {
        setStatus(UpdatesStatus::Checking);
    } else {
        setStatus(m_model->status());
    }
}

void UpdateCtrlWidget::setModel(UpdateModel *model)
{
    m_model = model;
    m_downloadControlPanel->setModel(model);

    qRegisterMetaType<UpdateErrorType>("UpdateErrorType");

    connect(m_model, &UpdateModel::statusChanged, this, &UpdateCtrlWidget::setStatus);
    connect(m_model, &UpdateModel::updateProgressChanged, this, &UpdateCtrlWidget::setUpdateProgress);
    connect(m_model, &UpdateModel::systemActivationChanged, this, &UpdateCtrlWidget::setActiveState);
    connect(m_model, &UpdateModel::classifyUpdateJobErrorChanged, this, &UpdateCtrlWidget::onClassifyUpdateJonErrorChanged);
    connect(m_model, &UpdateModel::systemUpdateInfoChanged, this, &UpdateCtrlWidget::setSystemUpdateInfo);
    connect(m_model, &UpdateModel::safeUpdateInfoChanged, this, &UpdateCtrlWidget::setSafeUpdateInfo);
    connect(m_model, &UpdateModel::unknownUpdateInfoChanged, this, &UpdateCtrlWidget::setUnknownUpdateInfo);
    connect(m_model, &UpdateModel::systemUpdateDownloadSizeChanged, this, &UpdateCtrlWidget::onRequestRefreshSize);
    connect(m_model, &UpdateModel::safeUpdateDownloadSizeChanged, this, &UpdateCtrlWidget::onRequestRefreshSize);
    connect(m_model, &UpdateModel::unknownUpdateDownloadSizeChanged, this, &UpdateCtrlWidget::onRequestRefreshSize);
    connect(m_model->downloadInfo(), &DownloadInfo::downloadSizeChanged, this, &UpdateCtrlWidget::onRequestRefreshSize);
    connect(m_model, &UpdateModel::updateModeChanged, this, &UpdateCtrlWidget::onUpdateModeChanged);

    // TODO 包大小 、下载（安装）进度、下载（安装）状态变化
    m_updatingItemMap.clear();

    setUpdateProgress(m_model->updateProgress());
    setSystemUpdateInfo(m_model->systemDownloadInfo());
    setSafeUpdateInfo(m_model->safeDownloadInfo());
    setUnknownUpdateInfo(m_model->unknownDownloadInfo());

    qDebug() << "setModel" << m_model->status();

    if (m_model->enterCheckUpdate()) {
        setStatus(UpdatesStatus::Checking);
    } else {
        setStatus(m_model->status());
    }
}

void UpdateCtrlWidget::setSystemVersion(const QString &version)
{
    if (m_systemVersion != version) {
        m_systemVersion = version;
    }
}

void UpdateCtrlWidget::setSystemUpdateInfo(UpdateItemInfo *updateItemInfo)
{
    qInfo() << Q_FUNC_INFO << updateItemInfo;
    m_updatingItemMap.remove(ClassifyUpdateType::SystemUpdate);
    if (nullptr == updateItemInfo) {
        m_systemUpdateItem->setVisible(false);
        return;
    }

    initUpdateItem(m_systemUpdateItem);
    m_systemUpdateItem->setData(updateItemInfo);
    m_updatingItemMap.insert(ClassifyUpdateType::SystemUpdate, m_systemUpdateItem);
}


void UpdateCtrlWidget::setSafeUpdateInfo(UpdateItemInfo *updateItemInfo)
{
    m_updatingItemMap.remove(ClassifyUpdateType::SecurityUpdate);
    if (nullptr == updateItemInfo) {
        m_safeUpdateItem->setVisible(false);
        return;
    }

    initUpdateItem(m_safeUpdateItem);
    m_safeUpdateItem->setData(updateItemInfo);
    m_updatingItemMap.insert(ClassifyUpdateType::SecurityUpdate, m_safeUpdateItem);
}

void UpdateCtrlWidget::setUnknownUpdateInfo(UpdateItemInfo *updateItemInfo)
{
    m_updatingItemMap.remove(ClassifyUpdateType::UnknownUpdate);
    if (nullptr == updateItemInfo) {
        m_unknownUpdateItem->setVisible(false);
        return;
    }

    initUpdateItem(m_unknownUpdateItem);
    m_unknownUpdateItem->setData(updateItemInfo);
    m_updatingItemMap.insert(ClassifyUpdateType::UnknownUpdate, m_unknownUpdateItem);
}

void UpdateCtrlWidget::setAllUpdateInfo(QMap<ClassifyUpdateType, UpdateItemInfo *> updateInfoMap)
{
    m_updatingItemMap.clear();
    setSystemUpdateInfo(updateInfoMap.value(ClassifyUpdateType::SystemUpdate));
    setSafeUpdateInfo(updateInfoMap.value(ClassifyUpdateType::SecurityUpdate));
    setUnknownUpdateInfo(updateInfoMap.value(ClassifyUpdateType::UnknownUpdate));
}

void UpdateCtrlWidget::showUpdateInfo()
{
    m_updateList->setVisible(true);
    m_updateTipsLab->setVisible(true);
    m_updateSizeLab->setVisible(true);
    showUpdateItem(m_model->updateMode());

    m_model->updateCheckUpdateTime();
    m_updateSummaryGroup->setVisible(true);
}

void UpdateCtrlWidget::onChangeUpdatesAvailableStatus()
{
    showUpdateInfo();
    //~ contents_path /update/Update
    setAllUpdateInfo(m_model->allDownloadInfo());

    onRequestRefreshSize();
}

void UpdateCtrlWidget::downloadAll()
{
    qInfo() << "Download all, update status: " << m_model->status();
    if (m_model->status() == UpdatesStatus::UpdatesAvailable) {
        Q_EMIT requestDownload();
    }
}

void UpdateCtrlWidget::onRequestRefreshSize()
{
    m_updateSize = m_model->downloadSize();
    m_downloadControlPanel->setDownloadSize(m_updateSize);
    QString updateSize = formatCap(m_updateSize);
    updateSize = tr("Size") + ": " + updateSize;
    m_updateSizeLab->setText(updateSize);
}

void UpdateCtrlWidget::onUpdateModeChanged(quint64 mode)
{
    onRequestRefreshSize();
    showUpdateItem(mode);

    const auto status = getCheckUpdateStatus(CheckUpdateItem::Default);
    if (status != CheckUpdateItem::Default) {
        hideAll();
        m_updateModeSettingItem->setVisible(true);
        m_checkUpdateItem->setVisible(true);
        m_checkUpdateItem->setStatus(status);
    } else {
        setStatus(m_model->status());
    }
}

CheckUpdateItem::CheckUpdateStatus UpdateCtrlWidget::getCheckUpdateStatus(CheckUpdateItem::CheckUpdateStatus defaultStatus)
{
    const quint64 updateMode = m_model->updateMode();
    if (updateMode == ClassifyUpdateType::Invalid)
        return CheckUpdateItem::NoneUpdateMode;

    if ((m_model->systemDownloadInfo() && m_model->systemDownloadInfo()->isUpdateAvailable())
        || (m_model->safeDownloadInfo() && m_model->safeDownloadInfo()->isUpdateAvailable())
        || (m_model->unknownDownloadInfo() && m_model->unknownDownloadInfo()->isUpdateAvailable()))
        return defaultStatus;

    return CheckUpdateItem::Updated;
}

void UpdateCtrlWidget::showUpdateItem(quint64 mode)
{
    m_systemUpdateItem->setVisible(m_model->systemDownloadInfo() && m_model->systemDownloadInfo()->isUpdateModeEnabled());
    m_safeUpdateItem->setVisible(m_model->safeDownloadInfo() && m_model->safeDownloadInfo()->isUpdateModeEnabled());
    m_unknownUpdateItem->setVisible(m_model->unknownDownloadInfo() && m_model->unknownDownloadInfo()->isUpdateModeEnabled());
}

UpdateErrorType UpdateCtrlWidget::updateJobErrorMessage() const
{
    return m_updateJobErrorMessage;
}

void UpdateCtrlWidget::setUpdateJobErrorMessage(const UpdateErrorType &updateJobErrorMessage)
{
    m_updateJobErrorMessage = updateJobErrorMessage;
}

void UpdateCtrlWidget::setUpdateFailedInfo(const UpdateErrorType &errorType)
{
    m_resultItem->setVisible(true);
    m_resultItem->setStatus(UpdatesStatus::UpdateFailed);
    if (errorType == UpdateErrorType::NoNetwork) {
        m_noNetworkTip->setVisible(true);
        return;
    }
    m_resultItem->setMessage(m_UpdateErrorInfoMap.contains(errorType) ? m_UpdateErrorInfoMap.value(errorType).errorMessage : tr(""));
}

void UpdateCtrlWidget::initUpdateItem(UpdateSettingItem *updateItem)
{
    updateItem->setIconVisible(true);
}

void UpdateCtrlWidget::onClassifyUpdateJonErrorChanged(const UpdateErrorType &errorType)
{
    // TODO 统一的error type
    m_systemUpdateItem->setUpdateJobErrorMessage(errorType);
}

void UpdateCtrlWidget::onShowUpdateCtrl()
{
    if (m_model->getUpdatablePackages() && m_model->status() == UpdatesStatus::Default) {
        Q_EMIT m_model->beginCheckUpdate();
    }
}

void UpdateCtrlWidget::hideAll()
{
    m_noNetworkTip->setVisible(false);
    m_resultItem->setVisible(false);
    m_updateList->setVisible(false);
    m_reminderTip->setVisible(false);
    m_checkUpdateItem->setVisible(false);
    m_updateSummaryGroup->setVisible(false);
    m_updateModeSettingItem->setVisible(false);
    m_updateTipsLab->setVisible(false);
    m_updateSizeLab->setVisible(false);
    m_updatingTipsLab->setVisible(false);
    m_downloadStatusWidget->setVisible(false);
}