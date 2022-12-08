// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatesettings.h"
#include "widgets/settingsgroup.h"
#include "updatemodel.h"
#include "widgets/translucentframe.h"
#include "widgets/labels/smalllabel.h"
#include "widgets/titlelabel.h"
#include "widgets/switchwidget.h"
#include "widgets/nextpagewidget.h"
#include "dsysinfo.h"
#include "window/utils.h"
#include "window/gsettingwatcher.h"

#include <DTipLabel>
#include <DDialog>
#include <DWaterProgress>
#include <DFontSizeManager>

#include <QVBoxLayout>
#include <QGSettings>
#include <QMessageBox>
#include <QSpinBox>

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace dcc;
using namespace dcc::widgets;
using namespace dcc::update;
using namespace DCC_NAMESPACE;
using namespace DCC_NAMESPACE::update;

UpdateSettings::UpdateSettings(UpdateModel *model, QWidget *parent)
    : ContentWidget(parent)
    , m_model(nullptr)
    , m_autoDownloadUpdateTips(new DTipLabel(tr("Switch it on to automatically download the updates in wireless or wired network"), this))
    , m_testingChannelTips(new DTipLabel(tr("Join the internal testing channel to get deepin latest updates")))
    , m_testingChannelLinkLabel(new QLabel(""))
    , m_idleDownloadCheckBox(new QCheckBox(tr("Download at leisure"), this))   // TODO 翻译
    , m_startTimeLabel(new QLabel(tr("Start time"), this)) // TODO 翻译
    , m_endTimeLabel(new QLabel(tr("End time"), this)) // TODO 翻译
    , m_startTimeEdit(new Dtk::Widget::DTimeEdit(this))
    , m_endTimeEdit(new Dtk::Widget::DTimeEdit(this))
    , m_idleTimeDownloadWidget(new QWidget(this))
    , m_autoCleanCache(new SwitchWidget(this))
    , m_dconfig(nullptr)
{
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_onlySecurityUpdates = new SwitchWidget(tr("Security Updates Only"), this);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_systemUpdate = new SwitchWidget(tr("System"), this);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_autoCheckAppUpdate = new SwitchWidget(tr("App installed in App Store"), this);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_autoCheckThirdPartyUpdate = new SwitchWidget(tr("Third-party Repositories"), this);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_updateNotify = new SwitchWidget(tr("Updates Notification"), this);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_autoDownloadUpdate = new SwitchWidget(tr("Auto Download Updates"), this);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_testingChannel = new SwitchWidget(tr("Join Internal Testing Channel"), this);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_testingChannelHeadingLabel = new QLabel(tr("Updates from Internal Testing Sources"));

    initUi();
    initConnection();
    setModel(model);

    if (IsCommunitySystem) {
        m_systemUpdate->setTitle(tr("System"));
        m_onlySecurityUpdates->hide();
    }
}

UpdateSettings::~UpdateSettings()
{
    GSettingWatcher::instance()->erase("updateUpdateNotify", m_updateNotify);
    GSettingWatcher::instance()->erase("updateAutoDownlaod", m_autoDownloadUpdate);
    GSettingWatcher::instance()->erase("updateCleanCache", m_autoCleanCache);
    GSettingWatcher::instance()->erase("updateSystemUpdate", m_systemUpdate);
    GSettingWatcher::instance()->erase("updateAppUpdate", m_autoCheckAppUpdate);
    GSettingWatcher::instance()->erase("updateSecureUpdate", m_onlySecurityUpdates);
}

void UpdateSettings::initUi()
{
    setTitle(tr("Update Settings"));

    TranslucentFrame *contentWidget = new TranslucentFrame(this); // 添加一层半透明框架
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);

    contentLayout->addSpacing(20);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    QLabel *autoUpdateSettingsLabel = new QLabel(tr("Updates from Repositories"), this);
    autoUpdateSettingsLabel->hide();
    DFontSizeManager::instance()->bind(autoUpdateSettingsLabel, DFontSizeManager::T5, QFont::DemiBold);
    autoUpdateSettingsLabel->setContentsMargins(10, 0, 10, 0); // 左右边距为10
    contentLayout->addWidget(autoUpdateSettingsLabel);
    // contentLayout->addSpacing(10);

    SettingsGroup *updatesGrp = new SettingsGroup(contentWidget);
    updatesGrp->hide();
    updatesGrp->appendItem(m_systemUpdate);
    updatesGrp->appendItem(m_autoCheckAppUpdate);
    updatesGrp->appendItem(m_onlySecurityUpdates);
    contentLayout->addWidget(updatesGrp);
    // contentLayout->addSpacing(10);
    SettingsGroup *autoCheckThirdPartyGrp = new SettingsGroup(contentWidget);
    autoCheckThirdPartyGrp->appendItem(m_autoCheckThirdPartyUpdate);
    contentLayout->addWidget(autoCheckThirdPartyGrp);

    // contentLayout->addSpacing(20);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings

    // QLabel *otherSettingsLabel = new QLabel(tr("Other settings"), this);
    // DFontSizeManager::instance()->bind(otherSettingsLabel, DFontSizeManager::T5, QFont::DemiBold);
    // otherSettingsLabel->setContentsMargins(10, 0, 10, 0); // 左右边距为10
    // contentLayout->addWidget(otherSettingsLabel);
    // contentLayout->addSpacing(10);

    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    SettingsGroup *checkUpdatesGrp = new SettingsGroup(nullptr, SettingsGroup::GroupBackground);
    checkUpdatesGrp->appendItem(m_autoDownloadUpdate);

    QVBoxLayout *idleDownloadLayout = new QVBoxLayout(this);
    idleDownloadLayout->addWidget(m_idleDownloadCheckBox, 0);
    QHBoxLayout *idleTimeSpinLayout = new QHBoxLayout(this);
    m_startTimeEdit->setDisplayFormat("hh:mm");
    m_startTimeEdit->setAlignment(Qt::AlignCenter);
    m_startTimeEdit->setAccessibleName("Start_Time_Edit");
    m_startTimeEdit->setProperty("_d_dtk_spinBox", true);
    m_endTimeEdit->setDisplayFormat("hh:mm");
    m_endTimeEdit->setAlignment(Qt::AlignCenter);
    m_endTimeEdit->setAccessibleName("End_Time_Edit");
    m_endTimeEdit->setProperty("_d_dtk_spinBox", true);
    idleTimeSpinLayout->addSpacing(30);
    idleTimeSpinLayout->addWidget(m_startTimeLabel, 0);
    idleTimeSpinLayout->addWidget(m_startTimeEdit, 0);
    idleTimeSpinLayout->addSpacing(30);
    idleTimeSpinLayout->addWidget(m_endTimeLabel, 0);
    idleTimeSpinLayout->addWidget(m_endTimeEdit, 0);
    idleTimeSpinLayout->addStretch(1);
    idleDownloadLayout->addLayout(idleTimeSpinLayout, 0);
    m_idleTimeDownloadWidget->setLayout(idleDownloadLayout);
    checkUpdatesGrp->insertWidget(m_idleTimeDownloadWidget);

    contentLayout->addWidget(checkUpdatesGrp);
    m_autoDownloadUpdateTips->setWordWrap(true);
    m_autoDownloadUpdateTips->setAlignment(Qt::AlignLeft);
    m_autoDownloadUpdateTips->setContentsMargins(10, 0, 10, 0);
    m_autoDownloadUpdateTips->hide();
    // contentLayout->addWidget(m_autoDownloadUpdateTips);
    // contentLayout->addSpacing(10);

    SettingsGroup *updatesNotificationtGrp = new SettingsGroup;
    updatesNotificationtGrp->appendItem(m_updateNotify);
    //~ contents_path /update/Update Settings
    //~ child_page Update Settings
    m_autoCleanCache->setTitle(tr("Clear Package Cache"));
    updatesNotificationtGrp->appendItem(m_autoCleanCache);
    contentLayout->addWidget(updatesNotificationtGrp);

#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    if (!IsServerSystem && !IsProfessionalSystem && !IsHomeSystem && !IsEducationSystem && !IsDeepinDesktop) {
        m_sourceCheck = new SwitchWidget(tr("System Repository Detection"), this);
        m_sourceCheck->addBackground();
        contentLayout->addWidget(m_sourceCheck);
        DTipLabel *sourceCheckTips = new DTipLabel(tr("Show a notification if system update repository has been modified"), this);
        sourceCheckTips->setWordWrap(true);
        sourceCheckTips->setAlignment(Qt::AlignLeft);
        sourceCheckTips->setContentsMargins(10, 0, 10, 0);
        contentLayout->addWidget(sourceCheckTips);
    }
#endif

    if (IsCommunitySystem) {
        m_smartMirrorBtn = new SwitchWidget(tr("Smart Mirror Switch"), this);
        m_smartMirrorBtn->addBackground();
        contentLayout->addWidget(m_smartMirrorBtn);

        DTipLabel *smartTips = new DTipLabel(tr("Switch it on to connect to the quickest mirror site automatically"), this);
        smartTips->setWordWrap(true);
        smartTips->setAlignment(Qt::AlignLeft);
        smartTips->setContentsMargins(10, 0, 10, 0);
        contentLayout->addWidget(smartTips);

        m_updateMirrors = new NextPageWidget(nullptr, false);
        m_updateMirrors->setTitle(tr("Mirror List"));
        m_updateMirrors->setRightTxtWordWrap(true);
        m_updateMirrors->addBackground();
        QStyleOption opt;
        m_updateMirrors->setBtnHiden(true);
        m_updateMirrors->setIconIcon(DStyleHelper(m_updateMirrors->style()).standardIcon(DStyle::SP_ArrowEnter, &opt, nullptr).pixmap(10, 10));
        contentLayout->addWidget(m_updateMirrors);
        contentLayout->addSpacing(20);

        DFontSizeManager::instance()->bind(m_testingChannelHeadingLabel, DFontSizeManager::T5, QFont::DemiBold);
        m_testingChannelHeadingLabel->setContentsMargins(10, 0, 10, 0); // 左右边距为10
        contentLayout->addWidget(m_testingChannelHeadingLabel);
        contentLayout->addSpacing(10);
        // Add link label to switch button
        m_testingChannelLinkLabel->setOpenExternalLinks(true);
        m_testingChannelLinkLabel->setStyleSheet("font: 12px");
        auto mainLayout = m_testingChannel->getMainLayout();
        mainLayout->insertWidget(mainLayout->count()-1, m_testingChannelLinkLabel);

        m_testingChannel->addBackground();
        contentLayout->addWidget(m_testingChannel);
        m_testingChannelTips->setWordWrap(true);
        m_testingChannelTips->setAlignment(Qt::AlignLeft);
        m_testingChannelTips->setContentsMargins(10, 0, 10, 0);
        contentLayout->addWidget(m_testingChannelTips);
        m_testingChannel->setVisible(false);
        m_testingChannelTips->setVisible(false);
    } else {
        m_testingChannel->hide();
        m_testingChannelTips->hide();
        m_testingChannelLinkLabel->hide();
        m_testingChannelHeadingLabel->hide();
    }

    contentLayout->setAlignment(Qt::AlignTop);
    contentLayout->setContentsMargins(46, 10, 46, 5);

    setContent(contentWidget);
}

void UpdateSettings::initConnection()
{
    connect(m_onlySecurityUpdates, &SwitchWidget::checkedChanged, this, &UpdateSettings::onAutoSecureUpdateCheckChanged);
    connect(m_systemUpdate, &SwitchWidget::checkedChanged, this, &UpdateSettings::onAutoUpdateCheckChanged);
    connect(m_autoCheckAppUpdate, &SwitchWidget::checkedChanged, this, &UpdateSettings::onAutoUpdateCheckChanged);
    connect(m_autoCheckThirdPartyUpdate, &SwitchWidget::checkedChanged, this, &UpdateSettings::onAutoUpdateCheckChanged);
    connect(m_updateNotify, &SwitchWidget::checkedChanged, this, &UpdateSettings::requestSetUpdateNotify);
    connect(m_autoDownloadUpdate, &SwitchWidget::checkedChanged, this, &UpdateSettings::requestSetAutoDownloadUpdates);
    connect(m_autoDownloadUpdate, &SwitchWidget::checkedChanged, this, [this] (bool checked) {
        m_idleTimeDownloadWidget->setVisible(checked);
    });
    connect(m_autoCleanCache, &SwitchWidget::checkedChanged, this, &UpdateSettings::requestSetAutoCleanCache);

#if 0
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    if (!IsServerSystem && !IsProfessionalSystem && !IsHomeSystem && !IsCommunitySystem && !IsDeepinDesktop) {
        qDebug() << "connect m_sourceCheck";
        connect(m_sourceCheck, &SwitchWidget::checkedChanged, this, &UpdateSettings::requestSetSourceCheck);
    }
#endif
#endif

    if (IsCommunitySystem) {
        connect(m_updateMirrors, &NextPageWidget::clicked, this, &UpdateSettings::requestShowMirrorsView);
        connect(m_smartMirrorBtn, &SwitchWidget::checkedChanged, this, &UpdateSettings::requestEnableSmartMirror);

        connect(m_testingChannel, &SwitchWidget::checkedChanged, this, &UpdateSettings::onTestingChannelCheckChanged);
    }
    connect(m_idleDownloadCheckBox, &QCheckBox::stateChanged, this, [this] (int state) {
        auto config = m_model->idleDownloadConfig();
        const bool isIdleDownloadEnabled = state == Qt::CheckState::Checked;
        config.idleDownloadEnabled = isIdleDownloadEnabled;
        setIdleDownloadTimeWidgetsEnabled(isIdleDownloadEnabled);
        Q_EMIT requestSetIdleDownloadConfig(config);
    });
    connect(m_startTimeEdit, &DDateTimeEdit::timeChanged, this, [this] (QTime time) {
        auto config = m_model->idleDownloadConfig();
        const QString &timeStr = time.toString("hh:mm");
        if (config.endTime == timeStr) {
            m_startTimeEdit->setTime(QTime::fromString(config.beginTime, "hh:mm"));
            return;
        }
        config.beginTime = timeStr;
        Q_EMIT requestSetIdleDownloadConfig(config);
    });
    connect(m_endTimeEdit, &DDateTimeEdit::timeChanged, this, [this] (QTime time) {
        auto config = m_model->idleDownloadConfig();
        const QString &timeStr = time.toString("hh:mm");
        if (config.beginTime == timeStr) {
            m_endTimeEdit->setTime(QTime::fromString(config.endTime, "hh:mm"));
            return;
        }
        config.endTime = timeStr;
        Q_EMIT requestSetIdleDownloadConfig(config);
    });
}

QString UpdateSettings::getAutoInstallUpdateType(quint64 type)
{
    QString text = "";
    if (type & ClassifyUpdateType::SystemUpdate) {
        //~ contents_path /update/Update Settings
        //~ child_page General
        text = tr("System Updates");
    }
    if (type & ClassifyUpdateType::SecurityUpdate) {
        if (text.isEmpty()) {
            text += tr("Security Updates");
        } else {
            text = text + "," + tr("Security Updates");
        }
    }
    if (type & ClassifyUpdateType::UnknownUpdate) {
        if (text.isEmpty()) {
            text += tr("Third-party Repositories");
        } else {
            text = text + "," + tr("Third-party Repositories");
        }
    }

    if (DSysInfo::isCommunityEdition()) {
        text = tr("Install updates automatically when the download is complete");
    } else {
        text = QString(tr("Install \"%1\" automatically when the download is complete").arg(text));
    }


    return text;
}

void UpdateSettings::setAutoCheckEnable(bool enable)
{
    auto setCheckEnable = [ = ](QWidget * widget, bool state, const QString & key, bool useDconfig) {
        QString status = DConfigWatcher::instance()->getStatus(DConfigWatcher::ModuleType::update, key);
        if (!useDconfig) {
            status = GSettingWatcher::instance()->get(key).toString();
        }

        widget->setEnabled("Enabled" == status && enable);
    };

    setCheckEnable(m_autoDownloadUpdate, enable, "updateAutoDownlaod", false);
    setCheckEnable(m_autoDownloadUpdateTips, enable, "updateAutoDownlaod", false);
    setCheckEnable(m_updateNotify, enable, "updateUpdateNotify", false);
    m_idleTimeDownloadWidget->setEnabled(m_autoDownloadUpdate->isEnabled());
}

void UpdateSettings::setModel(UpdateModel *model)
{
    if (m_model == model)
        return;

    m_model = model;

    connect(model, &UpdateModel::autoCheckSecureUpdatesChanged, m_onlySecurityUpdates, [ = ](const bool status) {
        m_onlySecurityUpdates->setChecked(status);
        setAutoCheckEnable(m_onlySecurityUpdates->checked() || m_autoCheckThirdPartyUpdate->checked() || m_systemUpdate->checked());
    });
    connect(model, &UpdateModel::autoCheckSystemUpdatesChanged, m_systemUpdate, [ = ](const bool status) {
        m_systemUpdate->setChecked(status);
        setAutoCheckEnable(m_onlySecurityUpdates->checked() || m_autoCheckThirdPartyUpdate->checked() || m_systemUpdate->checked());
    });
    connect(model, &UpdateModel::autoCheckAppUpdatesChanged, m_autoCheckAppUpdate, &SwitchWidget::setChecked);
    connect(model, &UpdateModel::autoCheckThirdPartyUpdatesChanged, m_autoCheckThirdPartyUpdate, [ = ](const bool status) {
        m_autoCheckThirdPartyUpdate->setChecked(status);
        setAutoCheckEnable(m_onlySecurityUpdates->checked() || m_autoCheckThirdPartyUpdate->checked() || m_systemUpdate->checked());
    });
    connect(model, &UpdateModel::updateNotifyChanged, m_updateNotify, &SwitchWidget::setChecked);
    connect(model, &UpdateModel::autoDownloadUpdatesChanged, m_autoDownloadUpdate, &SwitchWidget::setChecked);
    connect(model, &UpdateModel::autoCleanCacheChanged, m_autoCleanCache, &SwitchWidget::setChecked);

    auto updateIdleDownloadConfig = [this] {
        UpdateModel::IdleDownloadConfig config = m_model->idleDownloadConfig();
        m_idleDownloadCheckBox->setChecked(config.idleDownloadEnabled);
        setIdleDownloadTimeWidgetsEnabled(config.idleDownloadEnabled);
        m_startTimeEdit->setTime(QTime::fromString(config.beginTime, "hh:mm"));
        m_endTimeEdit->setTime(QTime::fromString(config.endTime, "hh:mm"));
    };
    connect(model, &UpdateModel::idleDownloadConfigChanged, this, updateIdleDownloadConfig);

    m_onlySecurityUpdates->setChecked(model->autoCheckSecureUpdates());
    m_systemUpdate->setChecked(model->autoCheckSystemUpdates());
    m_autoCheckAppUpdate->setChecked(model->autoCheckAppUpdates());
    m_autoCheckThirdPartyUpdate->setChecked(model->getAutoCheckThirdPartyUpdates());
    m_updateNotify->setChecked(model->updateNotify());
    m_autoDownloadUpdate->setChecked(model->autoDownloadUpdates());
    m_idleTimeDownloadWidget->setVisible(model->autoDownloadUpdates());
    m_autoCleanCache->setChecked(m_model->autoCleanCache());
    updateIdleDownloadConfig();

    GSettingWatcher::instance()->bind("updateUpdateNotify", m_updateNotify);
    GSettingWatcher::instance()->bind("updateAutoDownlaod", m_autoDownloadUpdate);
    GSettingWatcher::instance()->bind("updateCleanCache", m_autoCleanCache);
    GSettingWatcher::instance()->bind("updateSystemUpdate", m_systemUpdate);
    m_autoCheckAppUpdate->setVisible(false);

    DConfigWatcher::instance()->bind(DConfigWatcher::update, "updateSafety", m_onlySecurityUpdates);
    if (DCC_NAMESPACE::IsProfessionalSystem) {
        DConfigWatcher::instance()->bind(DConfigWatcher::update, "updateThirdPartySource", m_autoCheckThirdPartyUpdate);
    }

    connect(GSettingWatcher::instance(), &GSettingWatcher::notifyGSettingsChanged, this, [ = ](const QString & gsetting, const QString & state) {
        bool status = GSettingWatcher::instance()->get(gsetting).toString() == "Enabled" && (m_onlySecurityUpdates->checked() || m_autoCheckThirdPartyUpdate->checked() || m_systemUpdate->checked());

        if (gsetting == "updateAutoDownlaod") {
            m_autoDownloadUpdate->setEnabled(status);
            m_idleTimeDownloadWidget->setEnabled(status);
            m_autoDownloadUpdateTips->setEnabled(status);
        }

        if (gsetting == "updateUpdateNotify") {
            m_updateNotify->setEnabled(status);
        }
    });
    setAutoCheckEnable(m_onlySecurityUpdates->checked() || m_autoCheckThirdPartyUpdate->checked() || m_systemUpdate->checked());

#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    if (!IsServerSystem && !IsProfessionalSystem && !IsHomeSystem && !IsDeepinDesktop) {
        connect(model, &UpdateModel::sourceCheckChanged, m_sourceCheck, &SwitchWidget::setChecked);
        m_sourceCheck->setChecked(model->sourceCheck());
    }
#endif

    if (IsCommunitySystem) {
        auto setDefaultMirror = [this](const MirrorInfo & mirror) {
            m_updateMirrors->setValue(mirror.m_name);
        };

        if (!model->mirrorInfos().isEmpty()) {
            setDefaultMirror(model->defaultMirror());
        }

        connect(model, &UpdateModel::defaultMirrorChanged, this, setDefaultMirror);
        connect(model, &UpdateModel::smartMirrorSwitchChanged, m_smartMirrorBtn, &SwitchWidget::setChecked);
        m_smartMirrorBtn->setChecked(m_model->smartMirrorSwitch());

        auto setMirrorListVisible = [ = ](bool visible) {
            m_updateMirrors->setVisible(!visible);
        };

        connect(model, &UpdateModel::smartMirrorSwitchChanged, this, setMirrorListVisible);
        setMirrorListVisible(model->smartMirrorSwitch());

        auto hyperLink = QString("<a href='%1'>%2</a>").arg(m_model->getTestingChannelJoinURL().toString(),tr("here"));
        m_testingChannelLinkLabel->setText(tr("Click %1 to complete the application").arg(hyperLink));
        connect(model, &UpdateModel::testingChannelStatusChanged, this, &UpdateSettings::onTestingChannelStatusChanged);
        onTestingChannelStatusChanged();
    }
}

void UpdateSettings::onTestingChannelStatusChanged()
{
    const auto channelStatus = m_model->getTestingChannelStatus();
    if (channelStatus == UpdateModel::TestingChannelStatus::Hidden) {
        m_testingChannelHeadingLabel->hide();
        m_testingChannel->hide();
        m_testingChannelTips->hide();
        m_testingChannelLinkLabel->hide();
    } else {
        m_testingChannelHeadingLabel->show();
        m_testingChannel->show();
        m_testingChannelTips->show();
        m_testingChannelLinkLabel->setVisible(channelStatus == UpdateModel::TestingChannelStatus::WaitJoined);
        m_testingChannel->setChecked(channelStatus != UpdateModel::TestingChannelStatus::NotJoined);
    }
}

void UpdateSettings::onTestingChannelCheckChanged(const bool checked)
{
    const auto status = m_model->getTestingChannelStatus();
    if (checked) {
        Q_EMIT requestSetTestingChannelEnable(checked);
        return;
    }
    if (status != UpdateModel::TestingChannelStatus::Joined) {
        Q_EMIT requestSetTestingChannelEnable(checked);
        return;
    }

    auto dialog = new DDialog(this);
    dialog->setFixedWidth(400);
    dialog->setFixedHeight(280);

    auto label = new DLabel(dialog);
    label->setWordWrap(true);
    label->setText(tr("Checking system versions, please wait..."));

    auto progress = new DWaterProgress(dialog);
    progress->setFixedSize(100, 100);
    progress->setTextVisible(false);
    progress->setValue(50);
    progress->start();

    QWidget* content = new QWidget(dialog);
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(0, 0, 0, 0);
    content->setLayout(layout);
    dialog->addContent(content);

    layout->addStretch();
    layout->addWidget(label, 0, Qt::AlignHCenter);
    layout->addSpacing(20);
    layout->addWidget(progress, 0, Qt::AlignHCenter);
    layout->addStretch();

    connect(m_model, &UpdateModel::canExitTestingChannelChanged, dialog, [ = ](const bool can) {
        progress->setVisible(false);
        if (!can)
        {
            Q_EMIT requestSetTestingChannelEnable(checked);
            dialog->deleteLater();
            return;
        }
        const auto text = tr("If you leave the internal testing channel now, you may not be able to get the latest bug fixes and updates. Please leave after the official version is released to keep your system stable!");
        label->setText(text);
        dialog->addButton(tr("Leave"), false, DDialog::ButtonWarning);
        dialog->addButton(tr("Cancel"), true, DDialog::ButtonRecommend);
    });
    // 检查有可能会很快完成，对话框一闪而过会给人一种操作出错的感觉
    // 延迟一秒后再执行检查，可以让人有时间看到对话框，提升用户体验
    QTimer::singleShot(1000, this,  [ this ] {
        Q_EMIT requestCheckCanExitTestingChannel();
    });

    connect(dialog, &DDialog::closed, this, [ = ]() {
        // clicked windows close button
        m_testingChannel->setChecked(true);
        dialog->deleteLater();
    });
    connect(dialog, &DDialog::buttonClicked, this, [ = ](int index, const QString &text) {
        if ( index == 0 ) {
            // clicked the leave button
            Q_EMIT requestSetTestingChannelEnable(checked);
        }else {
            // clicked the cancel button
            m_testingChannel->setChecked(true);
        }
        dialog->deleteLater();
    });
    dialog->exec();
}

void UpdateSettings::setUpdateMode()
{
    quint64 updateMode = 0;

    updateMode = updateMode | m_onlySecurityUpdates->checked();
    updateMode = (updateMode << 1) | m_autoCheckThirdPartyUpdate->checked();
    updateMode = (updateMode << 2) | m_autoCheckAppUpdate->checked();
    updateMode = (updateMode << 1) | m_systemUpdate->checked();

    setAutoCheckEnable(m_onlySecurityUpdates->checked() || m_autoCheckThirdPartyUpdate->checked() || m_systemUpdate->checked());
    requestSetUpdateMode(updateMode);
}

void UpdateSettings::setCheckStatus(QWidget *widget, bool state, const QString &key)
{
    //后续对应配置相关代码
    const QString status = DConfigWatcher::instance()->getStatus(DConfigWatcher::ModuleType::update, key);

    if ("Enabled" == status) {
        widget->setEnabled((m_onlySecurityUpdates->checked() || m_autoCheckThirdPartyUpdate->checked() || m_systemUpdate->checked()));
    } else if ("Disabled" == status) {
        widget->setEnabled(false);
    }

    widget->setVisible("Hidden" != status && state);
}

void UpdateSettings::onAutoUpdateCheckChanged()
{
    if (m_onlySecurityUpdates->checked()) {
        m_onlySecurityUpdates->setChecked(false);
    }

    setUpdateMode();
}

void UpdateSettings::onAutoSecureUpdateCheckChanged()
{
    if (m_onlySecurityUpdates->checked()) {
        m_autoCheckAppUpdate->setChecked(false);
        m_systemUpdate->setChecked(false);
    }

    setUpdateMode();
}

void UpdateSettings::setIdleDownloadTimeWidgetsEnabled(bool enabled)
{
    m_startTimeLabel->setEnabled(enabled);
    m_startTimeEdit->setEnabled(enabled);
    m_endTimeLabel->setEnabled(enabled);
    m_endTimeEdit->setEnabled(enabled);
}