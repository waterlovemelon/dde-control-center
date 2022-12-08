// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "loadingitem.h"

#include "widgets/labels/normallabel.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QIcon>
#include <QPainter>
#include <DSysInfo>
#include <DApplicationHelper>
#include <DFontSizeManager>

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace dcc::widgets;
using namespace dcc::update;
using namespace DCC_NAMESPACE;
using namespace DCC_NAMESPACE::update;

CheckUpdateItem::CheckUpdateItem(UpdateModel *model, QFrame *parent)
    : TranslucentFrame(parent)
    , m_model(model)
    , m_messageLabel(new NormalLabel)
    , m_progress(new QProgressBar(this))
    , m_checkUpdateBtn(new QPushButton(parent))
    , m_lastCheckTimeTip(new QLabel(this))
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(10);

    m_progress->setAccessibleName("LoadingItem_progress");
    m_progress->setRange(0, 100);
    m_progress->setFixedWidth(200);
    m_progress->setFixedHeight(7);
    m_progress->setTextVisible(false);

    QVBoxLayout *imgLayout = new QVBoxLayout;
    imgLayout->setAlignment(Qt::AlignCenter);
    m_labelImage = new QLabel;
    m_labelImage->setMinimumSize(128, 128);
    imgLayout->addWidget(m_labelImage, 0, Qt::AlignTop);

    QHBoxLayout *txtLayout = new QHBoxLayout;
    txtLayout->setAlignment(Qt::AlignCenter);
    m_titleLabel = new QLabel;
    DFontSizeManager::instance()->bind(m_titleLabel, DFontSizeManager::T6, QFont::DemiBold);
    txtLayout->addWidget(m_titleLabel);

    m_checkUpdateBtn->setText(tr("Recheck for updates"));
    m_checkUpdateBtn->setFixedSize(QSize(300, 36));

    m_lastCheckTimeTip->setAlignment(Qt::AlignCenter);
    m_lastCheckTimeTip->setVisible(false);
    DFontSizeManager::instance()->bind(m_lastCheckTimeTip, DFontSizeManager::T8);

    layout->addStretch();
    layout->setAlignment(Qt::AlignHCenter);
    layout->addLayout(imgLayout);
    layout->addLayout(txtLayout);
    layout->addWidget(m_progress, 0, Qt::AlignHCenter);
    layout->addWidget(m_messageLabel, 0, Qt::AlignHCenter);
    layout->addWidget(m_checkUpdateBtn, 0, Qt::AlignHCenter);
    layout->addWidget(m_lastCheckTimeTip, 0, Qt::AlignHCenter);
    layout->addStretch();

    setLayout(layout);

    connect(m_checkUpdateBtn, &QPushButton::clicked, this, &CheckUpdateItem::requestCheckUpdate);
}

void CheckUpdateItem::setProgressValue(int value)
{
    m_progress->setValue(value);
}

void CheckUpdateItem::setProgressBarVisible(bool visible)
{
    m_progress->setVisible(visible);
}

void CheckUpdateItem::setMessage(const QString &message)
{
    m_messageLabel->setText(message);
}

void CheckUpdateItem::setVersionVisible(bool state)
{
    m_titleLabel->setVisible(state);
}

void CheckUpdateItem::setSystemVersion(const QString &version)
{
    Q_UNUSED(version);
    QString uVersion = DSysInfo::uosProductTypeName() + " " + DSysInfo::majorVersion();
    if (DSysInfo::uosType() != DSysInfo::UosServer)
        uVersion.append(" " + DSysInfo::uosEditionName());
    m_titleLabel->setText(uVersion);
}

void CheckUpdateItem::setImageVisible(bool state)
{
    m_labelImage->setVisible(state);
}

// image or text only use one
void CheckUpdateItem::setImageOrTextVisible(bool state)
{
    qDebug() << Q_FUNC_INFO << state;

    // setVersionVisible(state);
    setImageVisible(true);

    QString path = "";
    if (state) {
        m_labelImage->setPixmap(QIcon::fromTheme("icon_success").pixmap({128, 128}));
    } else {
        m_labelImage->setPixmap(QIcon(":/update/updatev20/dcc_checking_update.svg").pixmap({128, 128}));
    }
}

//image and text all use or not use
void CheckUpdateItem::setImageAndTextVisible(bool state)
{
    setVersionVisible(state);
    setImageVisible(state);
}

void CheckUpdateItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const DPalette &dp = DApplicationHelper::instance()->palette(this);
    QPainter p(this);
    p.setPen(Qt::NoPen);
    p.setBrush(dp.brush(DPalette::ItemBackground));
    p.drawRoundedRect(rect(), 8, 8);

    return QFrame::paintEvent(event);
}

void CheckUpdateItem::setStatus(CheckUpdateStatus status)
{
    m_status = status;

    setProgressBarVisible(false);
    setImageAndTextVisible(false);
    m_lastCheckTimeTip->setVisible(false);
    m_checkUpdateBtn->setVisible(false);
    m_titleLabel->setVisible(false);

    switch (status) {
        case Default:
        case Updated:
            setMessage(tr("Your system is up to date"));
            setImageOrTextVisible(true);
            showLastCheckingTime();
            break;
        case Checking:
            setProgressBarVisible(true);
            setImageOrTextVisible(false);
            setMessage(tr("Checking for updates, please wait..."));
            break;
        case CheckingFailed:
            handleUpdateError();
            break;
        case NeedRestart:
            setMessage(tr("The newest system installed, restart to take effect"));
            break;
        case NoneUpdateMode:
            setMessage(tr("Turn on the switch to get a better service experience~")); // TODO 翻译
            setImageOrTextVisible(true);
            break;
        default:
            break;
    }
}


void CheckUpdateItem::showLastCheckingTime()
{
    m_model->updateCheckUpdateTime();
    m_lastCheckTimeTip->setVisible(true);
    m_lastCheckTimeTip->setText(tr("Last checking time: ") + m_model->lastCheckUpdateTime());
}

void CheckUpdateItem::handleUpdateError()
{
    switch (m_model->lastError()) {
        case dcc::update::NoNetwork:
            setImageOrTextVisible(true);
            m_checkUpdateBtn->setVisible(true);
            m_titleLabel->setVisible(true);
            m_titleLabel->setText(tr("Check update failed")); // TODO 翻译
            setMessage(tr("Network disconnected, please retry after connected"));   // TODO 翻译
            showLastCheckingTime();
            break;
        default:
            break;
    }
}