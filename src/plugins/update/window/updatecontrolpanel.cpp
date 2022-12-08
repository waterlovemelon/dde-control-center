// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatecontrolpanel.h"

#include <DFontSizeManager>
#include <widgets/basiclistdelegate.h>

using namespace dcc;
using namespace dcc::widgets;
using namespace dcc::update;

updateControlPanel::updateControlPanel(QWidget *parent)
    : SettingsItem(parent)
    , m_titleLabel(new DLabel(this))
    , m_versionLabel(new DLabel(this))
    , m_detailLabel(new DTipLabel("", this))
    , m_dateLabel(new DLabel(this))
    , m_showMoreBUtton(new DCommandLinkButton("", this))
{
    initUi();
    initConnect();
}

void updateControlPanel::setDetailEnable(bool enable)
{
    m_showMoreBUtton->setEnabled(enable);
}
void updateControlPanel::setShowMoreButtonVisible(bool visible)
{
    m_showMoreBUtton->setVisible(visible);
}

void updateControlPanel::setDetailLabelVisible(bool visible)
{
    m_detailLabel->setVisible(visible);
}

void updateControlPanel::setVersionVisible(bool visible)
{
    m_versionLabel->setVisible(visible);
}

void updateControlPanel::setTitle(QString title)
{
    m_titleLabel->setText(title);
}

void updateControlPanel::setVersion(QString version)
{
    m_versionLabel->setVisible(!version.isEmpty());
    if (!version.isEmpty()) {
        m_versionLabel->setText(version);
    }
}

void updateControlPanel::setDetail(QString detail)
{
    m_detailLabel->setVisible(!detail.isEmpty());
    if (!detail.isEmpty()) {
        m_detailLabel->setText(detail);
    }
}

void updateControlPanel::setDate(QString date)
{
    m_dateLabel->setVisible(!date.isEmpty());
    if (!date.isEmpty()) {
        m_dateLabel->setText(date);
    }
}
//used to display long string: "12345678" -> "12345..."
const QString updateControlPanel::getElidedText(QWidget *widget, QString data, Qt::TextElideMode mode, int width, int flags, int line)
{
    QString retTxt = data;
    if (retTxt == "")
        return retTxt;

    QFontMetrics fontMetrics(font());
    int fontWidth = fontMetrics.width(data);

    qInfo() << Q_FUNC_INFO << " [Enter], data, width, fontWidth : " << data << width << fontWidth << line;

    if (fontWidth > width) {
        retTxt = widget->fontMetrics().elidedText(data, mode, width, flags);
    }

    qInfo() << Q_FUNC_INFO << " [End], retTxt : " << retTxt;

    return retTxt;
}

void updateControlPanel::setShowMoreButtonText(QString text)
{
    m_showMoreBUtton->setText(text);
}

void updateControlPanel::setDatetimeVisible(bool visible)
{
    m_dateLabel->setVisible(visible);
}

void updateControlPanel::initUi()
{
    QVBoxLayout *titleLay = new QVBoxLayout();
    titleLay->setMargin(0);
    m_titleLabel->setForegroundRole(DPalette::TextTitle);
    m_titleLabel->setWordWrap(true);
    DFontSizeManager::instance()->bind(m_titleLabel, DFontSizeManager::T6, QFont::DemiBold);
    titleLay->addWidget(m_titleLabel, 0, Qt::AlignTop);

    DFontSizeManager::instance()->bind(m_versionLabel, DFontSizeManager::T8);
    m_versionLabel->setForegroundRole(DPalette::TextTitle);
    m_versionLabel->setObjectName("versionLabel");
    titleLay->addWidget(m_versionLabel);
    titleLay->addStretch();
    QHBoxLayout *hLay = new QHBoxLayout();
    hLay->addLayout(titleLay);
    hLay->addStretch(1);

    DFontSizeManager::instance()->bind(m_detailLabel, DFontSizeManager::T8);
    m_detailLabel->setForegroundRole(DPalette::TextTips);
    m_detailLabel->adjustSize();
    m_detailLabel->setTextFormat(Qt::RichText);
    m_detailLabel->setAlignment(Qt::AlignJustify | Qt::AlignLeft);
    m_detailLabel->setWordWrap(true);
    m_detailLabel->setOpenExternalLinks(true);

    QHBoxLayout *dateLay = new QHBoxLayout();
    DFontSizeManager::instance()->bind(m_dateLabel, DFontSizeManager::T8);
    m_dateLabel->setObjectName("dateLabel");
    m_dateLabel->setEnabled(false);

    auto pal = m_dateLabel->palette();
    QColor base_color = pal.text().color();
    base_color.setAlpha(255 / 10 * 6);
    pal.setColor(QPalette::Text, base_color);
    m_dateLabel->setPalette(pal);
    dateLay->addWidget(m_dateLabel, 0, Qt::AlignLeft | Qt::AlignTop);
    dateLay->setSpacing(0);

    m_showMoreBUtton->setText(tr("Learn more"));
    DFontSizeManager::instance()->bind(m_showMoreBUtton, DFontSizeManager::T8);
    m_showMoreBUtton->setForegroundRole(DPalette::Button);
    dateLay->addStretch();
    dateLay->addWidget(m_showMoreBUtton, 0, Qt::AlignTop);
    dateLay->setContentsMargins(0, 0, 8, 0);

    QVBoxLayout *main = new QVBoxLayout();
    main->setSpacing(0);
    main->addLayout(hLay);
    main->addWidget(m_detailLabel);
    m_detailLabel->setContentsMargins(0, 5, 0, 0);
    main->addLayout(dateLay);
    main->addStretch();

    setLayout(main);
}

void updateControlPanel::initConnect()
{
    connect(m_showMoreBUtton, &DCommandLinkButton::clicked, this, &updateControlPanel::showDetail);
}