#ifndef COMBOBOXWIDGET_H
#define COMBOBOXWIDGET_H

#include "nextpagewidget.h"

#include <QPointer>

namespace dcc {

class ContentWidget;

namespace widgets {

class SettingsGroup;
class OptionItem;
class ComboBoxWidget : public NextPageWidget
{
    Q_OBJECT

public:
    explicit ComboBoxWidget(QFrame *parent = 0);
    ~ComboBoxWidget();

    void appendOption(const QString &name, const QVariant &value);
    void setCurrent(const QVariant &value);

signals:
    void requestPage(ContentWidget * const page) const;
    void dataChanged(const QVariant &data) const;

private:
    // block parent's signal
    void clicked() const;
    void setValue(const QString &);

private slots:
    void onNextPageClicked();
    void onContentDesktory();
    void onItemClicked();

private:
    SettingsGroup *m_optionsGroup;
    QPointer<ContentWidget> m_contentPage;

    OptionItem *m_lastSelectedItem;
    QMap<OptionItem *, QVariant> m_options;
};

}

}

#endif // COMBOBOXWIDGET_H
