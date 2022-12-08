// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include "interface/namespace.h"
#include "widgets/settingsitem.h"
#include "widgets/translucentframe.h"
#include "common.h"
#include "updatemodel.h"

#include <QProgressBar>
#include <QLabel>
#include <QPushButton>

namespace dcc {
namespace widgets {
class NormalLabel;
}
}

namespace DCC_NAMESPACE {
namespace update {

class CheckUpdateItem : public dcc::widgets::TranslucentFrame
{
    Q_OBJECT
public:
    enum CheckUpdateStatus {
        Default,
        Checking,
        CheckingFailed,
        Updated,
        NeedRestart,
        NoneUpdateMode
    };
    Q_ENUM(CheckUpdateStatus)

    explicit CheckUpdateItem(dcc::update::UpdateModel *model, QFrame *parent = 0);
    void setProgressValue(int value);
    void setProgressBarVisible(bool visible);
    void setMessage(const QString &message);
    void setVersionVisible(bool state);
    void setSystemVersion(const QString &version);
    void setImageVisible(bool state);
    void setImageOrTextVisible(bool state);
    void setImageAndTextVisible(bool state);
    void setStatus(CheckUpdateStatus status);

signals:
    void requestCheckUpdate();

protected:
    void paintEvent(QPaintEvent *event) override;
    void showLastCheckingTime();
    void handleUpdateError();

private:
    dcc::update::UpdateModel *m_model;
    dcc::widgets::NormalLabel *m_messageLabel;
    QProgressBar *m_progress;
    QLabel *m_labelImage;
    QLabel *m_titleLabel;
    QPushButton *m_checkUpdateBtn;
    QLabel *m_lastCheckTimeTip;
    CheckUpdateStatus m_status;
};

} // namespace update
} // namespace DCC_NAMESPACE
