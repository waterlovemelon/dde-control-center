#ifndef DOWNLOADCONTROLPANEL_H
#define DOWNLOADCONTROLPANEL_H

#include "common.h"
#include "updatemodel.h"

#include <QWidget>
#include <QLabel>
#include <QPushButton>

#include <DFloatingButton>
#include <DCommandLinkButton>
#include <DLabel>
#include <DLineEdit>
#include <DTextEdit>
#include <DIconButton>
#include <DTipLabel>
#include <DProgressBar>
#include <DSysInfo>
#include <DConfig>

namespace dcc {
namespace update {

enum ButtonStatus {
    invalid,
    start,
    pause,
    retry
};

enum UpdateDProgressType {
    InvalidType,
    Download,
    Paused,
    Install,
    Backup
};

class DownloadControlPanel : public QWidget
{
    Q_OBJECT
public:
    explicit DownloadControlPanel(QWidget *parent = nullptr);

    void initUi();
    void initConnect();
    void setButtonStatus(const ButtonStatus &value);
    void setModel(UpdateModel *model);
    void setUpdateStatus(UpdatesStatus status);
    void setDownloadSize(qlonglong downloadSize);
    const QString getElidedText(QWidget *widget, QString data, Qt::TextElideMode mode = Qt::ElideRight, int width = 100, int flags = 0, int line = 0);
    UpdateDProgressType getProgressType() const;
    void setProgressType(const UpdateDProgressType &progressType);

Q_SIGNALS:
    void startUpdate();
    void StartDownload();
    void PauseDownload();
    void RetryUpdate();
    void requestDownload();
    void requestUpdate();

public Q_SLOTS:
    void onCtrlButtonClicked();
    void setProgressValue(double progress);
    void setButtonIcon(ButtonStatus status);

private:
    void setProgressText(const QString &text, const QString &toolTip = "");
    void updateWidgets();

private:
    Dtk::Widget::DLabel *m_progressLabel;
    Dtk::Widget::DIconButton *m_startButton;
    Dtk::Widget::DProgressBar *m_progress;
    QWidget *m_downloadingWidget;
    QPushButton *m_downloadAllBtn;
    QPushButton *m_updateBtn;
    ButtonStatus m_buttonStatus;
    UpdateDProgressType m_progressType;
    UpdatesStatus m_updateStatus;
    qlonglong m_downloadSize;
    UpdateModel *m_model;
    Dtk::Core::DConfig *laStoreDconfig;
};

}
}
#endif // DOWNLOADCONTROLPANEL_H
