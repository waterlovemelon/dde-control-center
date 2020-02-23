/*
 * Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SHORTCUTITEM_H
#define SHORTCUTITEM_H

#include <widgets/settingsitem.h>
#include "shortcutkey.h"

#include <DIconButton>

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>

using namespace dcc::widgets;

static const QMap<QString, QString> DisplaykeyMap = { {"exclam", "!"}, {"at", "@"}, {"numbersign", "#"}, {"dollar", "$"}, {"percent", "%"},
    {"asciicircum", "^"}, {"ampersand", "&"}, {"asterisk", "*"}, {"parenleft", "("},
    {"parenright", ")"}, {"underscore", "_"}, {"plus", "+"}, {"braceleft", "{"}, {"braceright", "}"},
    {"bar", "|"}, {"colon", ":"}, {"quotedbl", "\""}, {"less", "<"}, {"greater", ">"}, {"question", "?"},
    {"minus", "-"}, {"equal", "="}, {"brackertleft", "["}, {"breckertright", "]"}, {"backslash", "\\"},
    {"semicolon", ";"}, {"apostrophe", "'"}, {"comma", ","}, {"period", "."}, {"slash", "/"}, {"Up", "↑"},
    {"Left", "←"}, {"Down", "↓"}, {"Right", "→"}, {"asciitilde", "~"}, {"grave", "`"}, {"Control", "Ctrl"},
    {"Super_L", "Super"}, {"Super_R", "Super"}
};

class ShortcutItem : public SettingsItem
{
    Q_OBJECT

public:
    struct ShortcutInfo {
        QString name;
        QString accels;
    };

    explicit ShortcutItem(QFrame *parent = nullptr);

    void setShortcutInfo(const ShortcutInfo &info);
    inline const ShortcutInfo &curInfo() { return m_info; }

    void setChecked(bool checked);
    void setTitle(const QString &title);
    void setShortcut(const QString &shortcut);

Q_SIGNALS:
    void shortcutEditChanged(const ShortcutInfo &info);
    void requestUpdateKey(const ShortcutInfo &info);
    void requestRemove(const ShortcutInfo &info);

public Q_SLOTS:
    void onEditMode(bool value);
    void onRemoveClick();

private:
    void onShortcutEdit();
    void updateTitleSize();

protected:
    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

private:
    QLineEdit *m_shortcutEdit;
    QLabel *m_title;
    ShortcutInfo m_info;
    DTK_WIDGET_NAMESPACE::DIconButton *m_delBtn;
    DTK_WIDGET_NAMESPACE::DIconButton *m_editBtn;
    ShortcutKey *m_key;
};

#endif // SHORTCUTITEM_H