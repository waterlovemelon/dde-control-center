#include "updatemodesettingitem.h"
#include "window/gsettingwatcher.h"
#include "window/utils.h"
#include "widgets/settingsgroup.h"

#include <DFontSizeManager>

#include <QVBoxLayout>

using namespace dcc;
using namespace dcc::widgets;
using namespace dcc::update;
using namespace DCC_NAMESPACE::update;
using namespace DCC_NAMESPACE;
DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

UpdateModeSettingItem::UpdateModeSettingItem(UpdateModel *model, QWidget *parent)
    : QWidget{parent}
    , m_model(model)
    , m_title(new DLabel(this))
    , m_secureUpdateSwitch(nullptr)
    , m_systemUpdateSwitch(nullptr)
{
    initUI();
    initConnections();
}

UpdateModeSettingItem::~UpdateModeSettingItem()
{
    GSettingWatcher::instance()->erase("updateSystemUpdate", m_systemUpdateSwitch);
    GSettingWatcher::instance()->erase("updateSecureUpdate", m_secureUpdateSwitch);
}

void UpdateModeSettingItem::initUI()
{
    TranslucentFrame *contentWidget = new TranslucentFrame(this); // 添加一层半透明框架
    //~ contents_path /update/Check for Updates
    //~ child_page Check for Updates
    m_systemUpdateSwitch = new SwitchWidget(tr("System"), this);
    m_systemUpdateSwitch->setChecked(m_model->updateMode() & SystemUpdate);
    //~ contents_path /update/Check for Updates
    //~ child_page Check for Updates
    m_secureUpdateSwitch = new SwitchWidget(tr("Security"), this);
    m_secureUpdateSwitch->setChecked(m_model->updateMode() & SecurityUpdate);

    m_title->setText(tr("Update content"));  // TODO 翻译
    m_title->setAlignment(Qt::AlignLeft);
    m_title->setContentsMargins(10, 0, 10, 0);
    DFontSizeManager::instance()->bind(m_title, DFontSizeManager::T5, QFont::DemiBold);

    SettingsGroup *updatesGrp = new SettingsGroup(contentWidget, SettingsGroup::GroupBackground);
    updatesGrp->appendItem(m_systemUpdateSwitch);
    updatesGrp->appendItem(m_secureUpdateSwitch);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->addWidget(updatesGrp);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_title);
    mainLayout->addWidget(contentWidget);

    if (IsCommunitySystem) {
        m_secureUpdateSwitch->hide();
    }
}

void UpdateModeSettingItem::initConnections()
{
    connect(m_systemUpdateSwitch, &SwitchWidget::checkedChanged, this, [this] (bool checked) {
        Q_EMIT updateModeEnableStateChanged(SystemUpdate, checked);
    });
    connect(m_secureUpdateSwitch, &SwitchWidget::checkedChanged, this, [this] (bool checked) {
        Q_EMIT updateModeEnableStateChanged(SecurityUpdate, checked);
    });
    connect(m_model, &UpdateModel::updateModeChanged, this, [this] (quint64 updateMode) {
        m_systemUpdateSwitch->setChecked(updateMode & SystemUpdate);
        m_secureUpdateSwitch->setChecked(updateMode & SecurityUpdate);
    });
}

void UpdateModeSettingItem::setButtonsEnabled(bool enabled)
{
    m_systemUpdateSwitch->switchButton()->setEnabled(enabled);
    m_secureUpdateSwitch->switchButton()->setEnabled(enabled);
}
