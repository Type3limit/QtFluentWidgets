#pragma once

#include "DemoHelpers.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentMenuBar.h"
#include "Fluent/FluentToast.h"

#include <QPointer>
#include <QString>

namespace Demo {

class DemoWindow final : public Fluent::FluentMainWindow
{
    Q_OBJECT
public:
    explicit DemoWindow(QWidget *parent = nullptr,
                        const QString &selectedNavigationKey = QStringLiteral("overview"),
                        Fluent::FluentToast::Position toastPosition = Fluent::FluentToast::Position::BottomRight);

private:
    void switchLanguage(DemoLanguage language);
    void performQueuedLanguageSwitch(DemoLanguage language);
    void clearUi();
    void buildUi();

    Fluent::FluentToast::Position m_toastPosition = Fluent::FluentToast::Position::BottomRight;
    QString m_selectedNavigationKey = QStringLiteral("overview");
    bool m_languageSwitchPending = false;
    QPointer<QWidget> m_rootWidget;
    QPointer<QWidget> m_titleBarLeftOwnedWidget;
    QPointer<QWidget> m_titleBarRightOwnedWidget;
    QPointer<Fluent::FluentMenuBar> m_ownedMenuBar;
};

} // namespace Demo
