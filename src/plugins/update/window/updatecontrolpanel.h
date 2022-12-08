// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UPDATECONTROLPANEL_H
#define UPDATECONTROLPANEL_H

#include <QWidget>
#include <QLabel>
#include <DFloatingButton>
#include <DCommandLinkButton>
#include <DLabel>
#include <DLineEdit>
#include <DTextEdit>
#include <DIconButton>
#include <DTipLabel>
#include <DProgressBar>
#include <DSysInfo>
#include <DSwitchButton>

#include "widgets/settingsitem.h"
#include "widgets/contentwidget.h"

namespace dcc {
namespace update {

class updateControlPanel: public dcc::widgets::SettingsItem
{
    Q_OBJECT
public:
    explicit updateControlPanel(QWidget *parent = nullptr);

    void initUi();
    void initConnect();

    void setTitle(QString title);
    void setVersion(QString version);
    void setDetail(QString detail);
    void setDate(QString date);
    void setShowMoreButtonText(QString text);

    void setDetailEnable(bool enable);
    void setShowMoreButtonVisible(bool visible);
    void setDetailLabelVisible(bool visible);
    void setVersionVisible(bool visible);
    void setDatetimeVisible(bool  visible);
    const QString getElidedText(QWidget *widget, QString data, Qt::TextElideMode mode = Qt::ElideRight, int width = 100, int flags = 0, int line = 0);

Q_SIGNALS:
    void showDetail();

private:
    DLabel *m_titleLabel;
    DLabel *m_versionLabel;
    DLabel *m_detailLabel;
    DLabel *m_dateLabel;
    DCommandLinkButton *m_showMoreBUtton;
};

}
}
#endif //UPDATECONTROLPANEL_H
