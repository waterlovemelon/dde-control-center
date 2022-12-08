#include "downloadcontrolpanel.h"

#include <DFontSizeManager>
#include <widgets/basiclistdelegate.h>

using namespace dcc;
using namespace dcc::widgets;
using namespace dcc::update;
using namespace Dtk::Widget;
DCORE_USE_NAMESPACE

#define FullUpdateBtnWidth 120
const int INSTALLATION_IS_READY = 1;

DownloadControlPanel::DownloadControlPanel(QWidget *parent)
    : QWidget{parent}
    , m_progressLabel(new DLabel(this))
    , m_startButton(new DIconButton(this))
    , m_progress(new DProgressBar(this))
    , m_downloadingWidget(new QWidget(this))
    , m_downloadAllBtn(new QPushButton(this))
    , m_updateBtn(new QPushButton(this))
    , m_buttonStatus(ButtonStatus::invalid)
    , m_progressType(UpdateDProgressType::InvalidType)
    , m_updateStatus(UpdatesStatus::Default)
    , m_model(nullptr)
    , laStoreDconfig(DConfig::create("org.deepin.lastore", "org.deepin.lastore", "", this))
{
    initUi();
    initConnect();
}

void DownloadControlPanel::initUi()
{
    m_progress->setFixedHeight(8);
    m_progress->setRange(0, 100);
    m_progress->setAlignment(Qt::AlignRight);
    m_progress->setFixedWidth(240);
    m_progress->setVisible(true);

    QHBoxLayout *buttonLay = new QHBoxLayout();
    m_startButton->setIcon(QIcon::fromTheme("dcc_start"));
    m_startButton->setIconSize(QSize(36, 36));
    m_startButton->setFlat(true);//设置背景透明
    m_startButton->setFixedSize(36, 36);
    buttonLay->setSpacing(0);
    buttonLay->addWidget(m_progress);
    buttonLay->addWidget(m_startButton);

    DFontSizeManager::instance()->bind(m_progressLabel, DFontSizeManager::T8);
    m_progressLabel->setScaledContents(true);
    m_progressLabel->setAlignment(Qt::AlignRight);
    QHBoxLayout *progressLabelLayout = new QHBoxLayout();
    progressLabelLayout->addStretch(1);
    progressLabelLayout->addWidget(m_progressLabel);
    progressLabelLayout->addSpacing(m_startButton->width() + 8);

    QVBoxLayout *downloadingLayout = new QVBoxLayout();
    downloadingLayout->addSpacing(0);
    downloadingLayout->addLayout(buttonLay);
    downloadingLayout->addLayout(progressLabelLayout);
    downloadingLayout->addStretch(1);
    m_downloadingWidget->setLayout(downloadingLayout);

    // 下载按钮
    m_downloadAllBtn->setText(getElidedText(m_downloadAllBtn,
                                        tr("Download All"),
                                        Qt::ElideRight,
                                        FullUpdateBtnWidth - 10,
                                        0,
                                        __LINE__));
    m_downloadAllBtn->setFixedSize(FullUpdateBtnWidth, 36);

    // 更新按钮
    m_updateBtn->setText(getElidedText(m_updateBtn,
                                    tr("Install now"),
                                    Qt::ElideRight,
                                    FullUpdateBtnWidth - 10,
                                    0,
                                    __LINE__));
    m_updateBtn->setFixedSize(FullUpdateBtnWidth, 36);

    QHBoxLayout *main = new QHBoxLayout();
    main->setSpacing(0);
    main->addWidget(m_downloadingWidget, 0);
    main->addWidget(m_downloadAllBtn, 0);
    main->addWidget(m_updateBtn, 0);
    main->addStretch();

    setLayout(main);
}

void DownloadControlPanel::setModel(UpdateModel *model)
{
    qInfo() << Q_FUNC_INFO;
    m_model = model;
    connect(m_model->downloadInfo(), &DownloadInfo::downloadProgressChanged, this, &DownloadControlPanel::setProgressValue);
    connect(m_model, &UpdateModel::statusChanged, this, &DownloadControlPanel::setUpdateStatus);

    setProgressValue(m_model->downloadInfo()->downloadProgress());
    setUpdateStatus(m_model->status());
    setEnabled(m_model->updateMode() >= ClassifyUpdateType::SystemUpdate);
}

void DownloadControlPanel::initConnect()
{
    connect(m_startButton, &DIconButton::clicked, this, &DownloadControlPanel::onCtrlButtonClicked);
    connect(m_downloadAllBtn, &QPushButton::clicked, this, [this] {
        setButtonStatus(ButtonStatus::pause);
        Q_EMIT requestDownload();
    });
    connect(m_updateBtn, &QPushButton::clicked, this, &DownloadControlPanel::requestUpdate);
    if (laStoreDconfig && laStoreDconfig->isValid()) {
        connect(laStoreDconfig, &DConfig::valueChanged, this, [this] (const QString &key) {
            if ("lastore-daemon-status" != key)
                return;

            updateWidgets();
        });
    } else {
        qWarning() << "Lastore dconfig is nullptr or invalid";
    }

}

void DownloadControlPanel::setProgressValue(double progress)
{
    qInfo() << Q_FUNC_INFO << progress;
    int value = progress * 100;
    if (value < 0 || value > 100)
        return;

    m_progress->setValue(value);
    setProgressText(QString("%1%").arg(value));
}

void DownloadControlPanel::setButtonIcon(ButtonStatus status)
{
    switch (status) {
    case ButtonStatus::start:
        m_startButton->setIcon(QIcon::fromTheme("dcc_start"));
        break;
    case ButtonStatus::pause:
        m_startButton->setIcon(QIcon::fromTheme("dcc_pause"));
        break;
    case ButtonStatus::retry:
        m_startButton->setIcon(QIcon::fromTheme("dcc_retry"));
        break;
    default:
        m_startButton->setIcon(static_cast<QStyle::StandardPixmap>(-1));
        break;

    }
}

UpdateDProgressType DownloadControlPanel::getProgressType() const
{
    return m_progressType;
}

void DownloadControlPanel::setProgressType(const UpdateDProgressType &progressType)
{
    m_progressType = progressType;
}

void DownloadControlPanel::setUpdateStatus(UpdatesStatus status)
{
    qInfo() << Q_FUNC_INFO << status;
    if (m_updateStatus == status)
        return;

    m_updateStatus = status;

    switch (status) {
        case UpdatesStatus::UpdatesAvailable:
        case UpdatesStatus::Downloading:
        case UpdatesStatus::DownloadPaused:
        case UpdatesStatus::Downloaded:
            updateWidgets();
            if (m_updateStatus == UpdatesStatus::DownloadPaused)
                setButtonStatus(ButtonStatus::start);
            else if (m_updateStatus == UpdatesStatus::Downloading)
                setButtonStatus(ButtonStatus::pause);

            break;
        default:
            break;
    }
}

void DownloadControlPanel::setButtonStatus(const ButtonStatus &value)
{
    m_buttonStatus = value;
    setButtonIcon(value);
    if (value == ButtonStatus::invalid) {
        m_startButton->setEnabled(false);
    }
}

void DownloadControlPanel::setProgressText(const QString &text, const QString &toolTip)
{
    m_progressLabel->setText(getElidedText(m_progressLabel, text, Qt::ElideRight, m_progressLabel->maximumWidth() - 10, 0, __LINE__));
    m_progressLabel->setToolTip(toolTip);
}

//used to display long string: "12345678" -> "12345..."
const QString DownloadControlPanel::getElidedText(QWidget *widget, QString data, Qt::TextElideMode mode, int width, int flags, int line)
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

void DownloadControlPanel::onCtrlButtonClicked()
{
    ButtonStatus status = ButtonStatus::invalid;
    switch (m_buttonStatus) {
    case ButtonStatus::start:
        status = ButtonStatus::pause;
        Q_EMIT StartDownload();
        break;
    case ButtonStatus::pause:
        status = ButtonStatus::start;
        Q_EMIT PauseDownload();
        break;
    case ButtonStatus::retry:
        status = ButtonStatus::invalid;
        setProgressText("");
        Q_EMIT RetryUpdate();
        break;
    default:
        break;
    }

    setButtonStatus(status);
}

void DownloadControlPanel::setDownloadSize(qlonglong downloadSize)
{
    m_downloadSize = downloadSize;
}

void DownloadControlPanel::updateWidgets()
{
    qInfo() << Q_FUNC_INFO;

    // 从配置文件中读取安装状态，第一位表示安装状态，1： 安装已就绪，0：安装未就绪
    bool installationIsReady = false;
    if (laStoreDconfig && laStoreDconfig->isValid()) {
        bool ok;
        int value = laStoreDconfig->value("lastore-daemon-status", 0).toInt(&ok);
        if (ok)
            installationIsReady = INSTALLATION_IS_READY & value;
    }

    m_downloadAllBtn->setVisible(m_updateStatus == UpdatesStatus::UpdatesAvailable && !installationIsReady);
    m_downloadingWidget->setVisible((m_updateStatus == UpdatesStatus::Downloading || m_updateStatus == UpdatesStatus::DownloadPaused) && !installationIsReady);
    m_updateBtn->setVisible(m_updateStatus == UpdatesStatus::Downloaded || installationIsReady);
}