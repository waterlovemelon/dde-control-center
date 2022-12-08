// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatewidget.h"
#include "updatemodel.h"
#include "updatework.h"
#include "updateitem.h"
#include "widgets/settingsgroup.h"
#include "updatesettings.h"
#include "updatehistorybutton.h"
#include "recenthistoryapplist.h"
#include "window/utils.h"

#include <types/appupdateinfolist.h>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QStackedLayout>
#include <DSysInfo>

DCORE_USE_NAMESPACE
using namespace dcc::update;
using namespace dcc::widgets;
using namespace DCC_NAMESPACE;
using namespace DCC_NAMESPACE::update;

UpdateWidget::UpdateWidget(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout)
    , m_model(nullptr)
    , m_work(nullptr)
    , m_centerLayout(new QVBoxLayout)
    , m_historyBtn(new UpdateHistoryButton)
    , m_updateState(UpdatesStatus::Default)
    , m_updateHistoryText(new QLabel)
    , m_applistGroup(new SettingsGroup)
    , m_recentHistoryApplist(new RecentHistoryApplist)
    , m_topSwitchWidgetBtn(new DButtonBox)
    , m_mainLayout(new QStackedLayout)
    , m_lastoreHeartBeatTimer(new QTimer)
{
    DButtonBoxButton *btnUpdate = new DButtonBoxButton(QIcon::fromTheme("dcc_update_topupdate"), tr("Check for Updates"));
    btnUpdate->setIconSize(QSize(24, 24));
    btnUpdate->setAccessibleName("UPDATE_CHECK");
    DButtonBoxButton *btnSetting = new DButtonBoxButton(QIcon::fromTheme("dcc_update_topsettings"), tr("Update Settings"));
    btnSetting->setIconSize(QSize(24, 24));
    btnSetting->setAccessibleName("UPDATE_SETTINGS");
    m_btnlist.append(btnUpdate);
    m_btnlist.append(btnSetting);
    m_topSwitchWidgetBtn->setAccessibleName("topSwitchWidgetBtn");
    m_topSwitchWidgetBtn->setButtonList(m_btnlist, true);
    m_btnlist.first()->setChecked(true);
    m_topSwitchWidgetBtn->setId(btnUpdate, 0);
    m_topSwitchWidgetBtn->setId(btnSetting, 1);
    m_topSwitchWidgetBtn->setMinimumSize(240, 36);

    m_updateHistoryText->setText(tr("Last Update"));

    connect(m_topSwitchWidgetBtn, &DButtonBox::buttonClicked, [this](QAbstractButton * value) {
        refreshWidget(static_cast<UpdateType>(m_topSwitchWidgetBtn->id(value)));
    });

    m_mainLayout->setMargin(0);
    m_layout->setMargin(0);
    m_layout->setAlignment(Qt::AlignTop);
    m_layout->setSpacing(0);
    m_layout->addSpacing(10);
    m_layout->addWidget(m_topSwitchWidgetBtn, 0, Qt::AlignHCenter);
    m_layout->addLayout(m_mainLayout, 0);

    QWidget *recentHistoryWidget = new QWidget;
    recentHistoryWidget->setAccessibleName("Update_Widget");
    QVBoxLayout *bottomLayout = new QVBoxLayout;
    recentHistoryWidget->setLayout(bottomLayout);

    bottomLayout->setMargin(0);
    bottomLayout->setSpacing(0);
    bottomLayout->addWidget(m_historyBtn, 0, Qt::AlignCenter);
    bottomLayout->addWidget(m_updateHistoryText, 0, Qt::AlignCenter);
    bottomLayout->addWidget(m_recentHistoryApplist);

    m_layout->addWidget(recentHistoryWidget);

    m_historyBtn->setVisible(false);
    m_updateHistoryText->setVisible(false);
    m_recentHistoryApplist->setVisible(false);
    m_recentHistoryApplist->setContentWidget(m_applistGroup);

    m_layout->addWidget(m_recentHistoryApplist);
    setLayout(m_layout);

    m_lastoreHeartBeatTimer->setInterval(60000);
    m_lastoreHeartBeatTimer->start();
    connect(m_lastoreHeartBeatTimer, &QTimer::timeout, this, &UpdateWidget::requestLastoreHeartBeat);

    setFocusPolicy(Qt::FocusPolicy::ClickFocus);
}

UpdateWidget::~UpdateWidget()
{
    delete  m_centerLayout;
    m_centerLayout = nullptr;

    if (m_lastoreHeartBeatTimer != nullptr) {
        if (m_lastoreHeartBeatTimer->isActive()) {
            m_lastoreHeartBeatTimer->stop();
        }
        delete m_lastoreHeartBeatTimer;
        m_lastoreHeartBeatTimer = nullptr;
    }
}

void UpdateWidget::initialize()
{
    connect(m_historyBtn, &UpdateHistoryButton::notifyBtnRelease, this, [ = ](bool state) {
        resetUpdateCheckState();

        if (state) {
            m_model->updateHistoryAppInfos();
            m_historyBtn->setLabelText(tr("Return"));
            m_updateHistoryText->setVisible(true);
            m_applistGroup->setVisible(true);
            m_recentHistoryApplist->setVisible(true);
            onAppendApplist(m_model->historyAppInfos());
        } else {
            m_applistGroup->setVisible(false);
        }
    });
}

void UpdateWidget::setModel(const UpdateModel *model, const UpdateWorker *work)
{
    m_model = const_cast<UpdateModel *>(model);
    m_work = const_cast<UpdateWorker *>(work);
    qRegisterMetaType<ClassifyUpdateType>("ClassifyUpdateType");
    qRegisterMetaType<dcc::update::UpdateModel::IdleDownloadConfig>("dcc::update::UpdateModel::IdleDownloadConfig");

    UpdateCtrlWidget *updateWidget = new UpdateCtrlWidget(m_model);
    updateWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(updateWidget, &UpdateCtrlWidget::notifyUpdateState, this, &UpdateWidget::onNotifyUpdateState);
    connect(updateWidget, &UpdateCtrlWidget::requestDownload, m_work, &UpdateWorker::startDownload);
    connect(updateWidget, &UpdateCtrlWidget::requestUpdateCtrl, m_work, &UpdateWorker::onDownloadJobCtrl);
    connect(updateWidget, &UpdateCtrlWidget::requestOpenAppStroe, m_work, &UpdateWorker::onRequestOpenAppStore);
    connect(this, &UpdateWidget::showUpdateCtrl, updateWidget, &UpdateCtrlWidget::onShowUpdateCtrl);
    connect(updateWidget, &UpdateCtrlWidget::requestFixError, m_work, &UpdateWorker::onFixError);
    connect(updateWidget, &UpdateCtrlWidget::requestUpdateMode, m_work, &UpdateWorker::setUpdateMode);

    UpdateSettings *updateSetting = new UpdateSettings(m_model);
    updateSetting->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(updateSetting, &UpdateSettings::requestSetAutoDownloadUpdates, m_work, &UpdateWorker::setAutoDownloadUpdates);
    connect(updateSetting, &UpdateSettings::requestSetTestingChannelEnable, m_work, &UpdateWorker::setTestingChannelEnable);
    connect(updateSetting, &UpdateSettings::requestCheckCanExitTestingChannel, m_work, &UpdateWorker::checkCanExitTestingChannel);
    connect(updateSetting, &UpdateSettings::requestSetAutoInstall, m_work, &UpdateWorker::setAutoInstallUpdates);
    connect(updateSetting, &UpdateSettings::requestShowMirrorsView, this, &UpdateWidget::pushMirrorsView);
    connect(updateSetting, &UpdateSettings::requestSetAutoCleanCache, m_work, &UpdateWorker::setAutoCleanCache);
    connect(updateSetting, &UpdateSettings::requestSetIdleDownloadConfig, m_work, &UpdateWorker::setIdleDownloadConfig);
    connect(updateSetting, &UpdateSettings::requestSetUpdateMode, m_work, &UpdateWorker::setUpdateMode);
    connect(updateSetting, &UpdateSettings::requestSetUpdateNotify, m_work, &UpdateWorker::setUpdateNotify);
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    connect(updateSetting, &UpdateSettings::requestSetSourceCheck, m_work, &UpdateWorker::setSourceCheck);
#endif
    if (IsCommunitySystem) {
        connect(updateSetting, &UpdateSettings::requestEnableSmartMirror, m_work, &UpdateWorker::setSmartMirror);
    }

    m_mainLayout->addWidget(updateWidget);
    m_mainLayout->addWidget(updateSetting);
}

void UpdateWidget::setSystemVersion(QString version)
{
    qDebug() << Q_FUNC_INFO << QString("%1 %2").arg(tr("Current Edition")).arg(version.toLatin1().data());

    if (m_systemVersion != version) {
        m_systemVersion = version;
    }
}

void UpdateWidget::resetUpdateCheckState(bool state)
{
    m_historyBtn->setVisible(state);
    m_updateHistoryText->setVisible(false);
    m_historyBtn->setLabelText(tr("Update History"));
    m_recentHistoryApplist->setVisible(false);
}

void UpdateWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
}

void UpdateWidget::refreshWidget(UpdateType type)
{
    displayUpdateContent(type);

    if (type == UpdateSettingMir) {
        QTimer::singleShot(0, this, [this] {
            Q_EMIT pushMirrorsView();
        });
    }
}

void UpdateWidget::showCheckUpdate()
{
    const UpdatesStatus &status = m_model->status();
    qDebug() << Q_FUNC_INFO << " current update status : " << status;
    m_mainLayout->setCurrentIndex(0);
    Q_EMIT showUpdateCtrl();
    // prohibit dde-offline-upgrader from showing while this page is showing.
    QDBusConnection::sessionBus().registerService(OfflineUpgraderService);
}

void UpdateWidget::showUpdateSetting()
{
    qDebug() << Q_FUNC_INFO;
    resetUpdateCheckState(false);
    m_work->checkNetselect();
#ifndef DISABLE_SYS_UPDATE_MIRRORS
    Q_EMIT m_work->requestRefreshMirrors();
#endif
    m_mainLayout->setCurrentIndex(1);
}

void UpdateWidget::displayUpdateContent(UpdateType index)
{
    QLayoutItem *item;
    while ((item = m_centerLayout->layout()->takeAt(0)) != nullptr) {
        item->widget()->deleteLater();
        delete item;
        item = nullptr;
    }

    switch (static_cast<UpdateType>(index)) {
    case UpdateCheck:
        showCheckUpdate();
        m_btnlist.at(0)->setChecked(true);
        break;
    case UpdateSetting:
    case UpdateSettingMir:
        showUpdateSetting();
        m_btnlist.at(1)->setChecked(true);
        break;
    default:
        break;
    }
}

void UpdateWidget::onNotifyUpdateState(int state)
{
    if (m_updateState == static_cast<UpdatesStatus>(state)) {
        return;
    } else {
        m_updateState = static_cast<UpdatesStatus>(state);
    }

    m_historyBtn->setVisible(false);
    this->setFocus();
}

void UpdateWidget::onAppendApplist(const QList<AppUpdateInfo> &infos)
{
    qDebug() << infos.count();
    QLayoutItem *item;
    while ((item = m_applistGroup->layout()->takeAt(0)) != nullptr) {
        item->widget()->deleteLater();
        delete item;
    }

    for (const AppUpdateInfo &info : infos) {
        UpdateItem *items = new UpdateItem();
        items->setAppInfo(info);

        m_applistGroup->appendItem(items);
    }
}