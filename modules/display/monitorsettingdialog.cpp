#include "monitorsettingdialog.h"
#include "monitorcontrolwidget.h"

#include <QVBoxLayout>
#include <QTimer>

DWIDGET_USE_NAMESPACE

using namespace dcc;

const double BRIGHTNESS_MUL = 1000.;

MonitorSettingDialog::MonitorSettingDialog(DisplayModel *model, QWidget *parent)
    : QDialog(parent),

      m_primary(true),

      m_model(model),
      m_monitor(model->primaryMonitor())
{
    Q_ASSERT(m_monitor);

    init();
    initPrimary();
}

MonitorSettingDialog::MonitorSettingDialog(Monitor *monitor, QWidget *parent)
    : QDialog(parent),

      m_primary(false),
      m_monitor(monitor)
{
    init();
}

MonitorSettingDialog::~MonitorSettingDialog()
{
    qDeleteAll(m_otherDialogs);
}

void MonitorSettingDialog::init()
{
    m_resolutionsWidget = new SettingsListWidget;
    m_resolutionsWidget->setTitle(tr("Resolution"));

    m_rotateBtn = new DImageButton;

    m_lightSlider = new DCCSlider;
    m_lightSlider->setOrientation(Qt::Horizontal);
    m_lightSlider->setMinimum(0);
    m_lightSlider->setMaximum(1000);

    m_btnsLayout = new QHBoxLayout;
    m_btnsLayout->addWidget(m_rotateBtn);
    m_btnsLayout->addStretch();
    m_btnsLayout->setSpacing(0);
    m_btnsLayout->setMargin(0);

    m_mainLayout = new QVBoxLayout;
    m_mainLayout->addWidget(m_resolutionsWidget);
    m_mainLayout->addWidget(m_lightSlider);
    m_mainLayout->addLayout(m_btnsLayout);
    m_mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    QTimer *smallDelayTimer = new QTimer(this);
    smallDelayTimer->setSingleShot(true);
    smallDelayTimer->setInterval(1000);

    connect(m_monitor, &Monitor::currentModeChanged, this, &MonitorSettingDialog::onMonitorModeChanged);
    connect(m_monitor, &Monitor::brightnessChanged, this, &MonitorSettingDialog::onMonitorBrightnessChanegd);
    connect(m_monitor, &Monitor::xChanged, smallDelayTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_monitor, &Monitor::yChanged, smallDelayTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_monitor, &Monitor::wChanged, smallDelayTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_monitor, &Monitor::hChanged, smallDelayTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_resolutionsWidget, &SettingsListWidget::clicked, this, &MonitorSettingDialog::onMonitorModeSelected);
    connect(m_lightSlider, &DCCSlider::valueChanged, this, &MonitorSettingDialog::onBrightnessSliderChanged);
    connect(m_rotateBtn, &DImageButton::clicked, [=] { emit requestMonitorRotate(m_monitor); });
    connect(smallDelayTimer, &QTimer::timeout, this, &MonitorSettingDialog::onMonitorRectChanged);

    setWindowFlags(Qt::X11BypassWindowManagerHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setWindowTitle(m_monitor->name());
    setLayout(m_mainLayout);
    onMonitorModeListChanged(m_monitor->modeList());
    onMonitorBrightnessChanegd(m_monitor->brightness());
    QTimer::singleShot(10, this, &MonitorSettingDialog::onMonitorRectChanged);
}

void MonitorSettingDialog::initPrimary()
{
    Q_ASSERT(m_primary);

    QPushButton *cancelBtn = new QPushButton;
    cancelBtn->setText(tr("Cancel"));
    QPushButton *applyBtn = new QPushButton;
    applyBtn->setText(tr("Apply"));

    m_btnsLayout->addWidget(cancelBtn);
    m_btnsLayout->addWidget(applyBtn);

    // add primary screen settings widget
    m_primarySettingsWidget = new SettingsListWidget;
    m_primarySettingsWidget->setTitle(tr("Primary"));
    m_mainLayout->insertWidget(0, m_primarySettingsWidget);

    m_ctrlWidget = new MonitorControlWidget;
    m_ctrlWidget->setDisplayModel(m_model);
    m_mainLayout->insertWidget(0, m_ctrlWidget);

    // load other non-primary dialogs
    for (auto mon : m_model->monitorList())
    {
        // add primary settings
        m_primarySettingsWidget->appendOption(mon->name());

        if (mon == m_monitor)
            continue;

        MonitorSettingDialog *dialog = new MonitorSettingDialog(mon, this);

        connect(dialog, &MonitorSettingDialog::requestSetPrimary, this, &MonitorSettingDialog::requestSetPrimary);
        connect(dialog, &MonitorSettingDialog::requestSetMonitorMode, this, &MonitorSettingDialog::requestSetMonitorMode);
        connect(dialog, &MonitorSettingDialog::requestSetMonitorBrightness, this, &MonitorSettingDialog::requestSetMonitorBrightness);
        connect(dialog, &MonitorSettingDialog::requestMonitorRotate, this, &MonitorSettingDialog::requestMonitorRotate);

        dialog->show();
        m_otherDialogs.append(dialog);
    }

    connect(m_ctrlWidget, &MonitorControlWidget::requestRecognize, this, &MonitorSettingDialog::requestRecognize);
    connect(m_ctrlWidget, &MonitorControlWidget::requestMerge, this, &MonitorSettingDialog::requestMerge);
    connect(m_primarySettingsWidget, &SettingsListWidget::clicked, this, &MonitorSettingDialog::requestSetPrimary);
    connect(m_model, &DisplayModel::primaryScreenChanged, this, &MonitorSettingDialog::onPrimaryChanged);
    connect(m_model, &DisplayModel::screenHeightChanged, this, &MonitorSettingDialog::updateScreensRelation, Qt::QueuedConnection);
    connect(m_model, &DisplayModel::screenWidthChanged, this, &MonitorSettingDialog::updateScreensRelation, Qt::QueuedConnection);
    connect(cancelBtn, &QPushButton::clicked, [=] { reject(); });
    connect(applyBtn, &QPushButton::clicked, [=] { accept(); });

    onPrimaryChanged();
    updateScreensRelation();
}

void MonitorSettingDialog::mergeScreens()
{
    Q_ASSERT(m_model->monitorList().size() == 2);
}

void MonitorSettingDialog::splitScreens()
{
    Q_ASSERT(m_model->monitorList().size() == 2);
}

void MonitorSettingDialog::updateScreensRelation()
{
    const bool merged = m_model->monitorsIsIntersect();

    m_ctrlWidget->setScreensMerged(merged);
    m_primarySettingsWidget->setVisible(!merged);

    for (auto d : m_otherDialogs)
        d->setVisible(!merged);

    // TODO: reset screen mode list
//    onMonitorBrightnessChanegd();
}

void MonitorSettingDialog::onPrimaryChanged()
{
    Q_ASSERT(m_primary);

    const QString primary = m_model->primary();
    for (int i(0); i != m_model->monitorList().size(); ++i)
    {
        if (m_model->monitorList()[i]->name() == primary)
        {
            m_primarySettingsWidget->setSelectedIndex(i);
            return;
        }
    }
}

void MonitorSettingDialog::onMonitorRectChanged()
{
    QDialog::move(m_monitor->rect().center() - rect().center());
}

void MonitorSettingDialog::onMonitorModeChanged()
{
    m_resolutionsWidget->setSelectedIndex(m_monitor->modeList().indexOf(m_monitor->currentMode()));
}

void MonitorSettingDialog::onMonitorBrightnessChanegd(const double brightness)
{
    m_lightSlider->blockSignals(true);
    m_lightSlider->setValue(brightness * BRIGHTNESS_MUL);
    m_lightSlider->blockSignals(false);
}

void MonitorSettingDialog::onMonitorModeListChanged(const QList<Resolution> &modeList)
{
    m_resolutionsWidget->clear();

    bool first = true;
    for (auto r : modeList)
    {
        const QString option = QString::number(r.width()) + "×" + QString::number(r.height());

        if (first)
        {
            first = false;
            m_resolutionsWidget->appendOption(option + tr(" (Recommended)"));
        } else {
            m_resolutionsWidget->appendOption(option);
        }
    }

    onMonitorModeChanged();
}

void MonitorSettingDialog::onMonitorModeSelected(const int index)
{
    const auto modeList = m_monitor->modeList();
    Q_ASSERT(modeList.size() > index);

    emit requestSetMonitorMode(m_monitor, modeList[index].id());
}

void MonitorSettingDialog::onBrightnessSliderChanged(const int value)
{
    emit requestSetMonitorBrightness(m_monitor, value / BRIGHTNESS_MUL);
}
