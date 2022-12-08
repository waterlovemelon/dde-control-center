#ifndef UPDATEMODESETTINGITEM_H
#define UPDATEMODESETTINGITEM_H

#include "widgets/switchwidget.h"
#include "interface/namespace.h"
#include "common.h"
#include "updatemodel.h"

#include <QWidget>

#include <DLabel>

namespace DCC_NAMESPACE {
namespace update {

class UpdateModeSettingItem : public QWidget
{
    Q_OBJECT
public:
    explicit UpdateModeSettingItem(dcc::update::UpdateModel *model, QWidget *parent = nullptr);
    ~UpdateModeSettingItem();

    void setButtonsEnabled(bool enabled);

signals:
    void updateModeEnableStateChanged(dcc::update::ClassifyUpdateType type, bool enable);

private:
    void initUI();
    void initConnections();

private:
    dcc::update::UpdateModel *m_model;
    Dtk::Widget::DLabel *m_title;
    dcc::widgets::SwitchWidget *m_secureUpdateSwitch; // 检查安全更新
    dcc::widgets::SwitchWidget *m_systemUpdateSwitch; // 检查系统更新
};

}
}

#endif // UPDATEMODESETTINGITEM_H
