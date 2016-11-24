﻿#ifndef OPTIONWIDGET_H
#define OPTIONWIDGET_H

#include "settingsitem.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QDebug>
#include <QPushButton>
#include <dimagebutton.h>
#include <QJsonObject>

namespace dcc
{

DWIDGET_USE_NAMESPACE

class OptionWidget : public SettingsItem
{
    Q_OBJECT
    Q_PROPERTY(bool checked READ checked WRITE setChecked NOTIFY checkedChanged)

public:
    explicit OptionWidget(QWidget *parent = 0);
    const QJsonObject getItem() { return m_CategoryItem;}
    void setItem(const QJsonObject &item);
    const QString id()    const;
    const QString name()  const;
    const QString displayName() const;
    const QString mime() const;
    bool checked() const;
    void setCheckedIcon(const QPixmap &icon);
    bool userCheck() const;
    QHBoxLayout *layout();

public slots:
    void setChecked(const bool checked);
    void setDelete(const bool delchecked);
    void setMime(const QString &mime);
    void setUserCheck(const bool check);

private:
    QHBoxLayout  *m_mainlayout;
    QLabel       *m_optionIcon;
    QLabel       *m_execPath;
    DImageButton *m_delete;
    QLabel       *m_checkedIconLabel;
    QLabel       *m_displayName;
    bool          m_checked           = false;
    bool          m_delchecked        = false;
    bool          m_userCheck         = false;
    QString       m_mime;
    QJsonObject m_CategoryItem;

protected:
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void removeItem(const QJsonObject &item);
    void setDefault(const QString &id);
    void checkedChanged(const bool checked) const;
    void checkedIconChanged(const QPixmap &icon) const;
};

}

#endif // OPTIONWIDGET_H
