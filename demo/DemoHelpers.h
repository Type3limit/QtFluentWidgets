#pragma once

#include <functional>
#include <QString>
class QWidget;
class QVBoxLayout;
class QString;
class QColor;

namespace Fluent {
class FluentCard;
class FluentScrollArea;
}

namespace Demo {

enum class DemoLanguage {
    Chinese,
    English,
};

struct Section {
    Fluent::FluentCard *card = nullptr;
    QVBoxLayout *body = nullptr;
};

DemoLanguage systemLanguage();
void initializeLanguage();
DemoLanguage currentLanguage();
bool setLanguage(DemoLanguage language);

QString text(const QString &zh, const QString &en);

Section makeSection(const QString &title, const QString &subtitle = QString());

Fluent::FluentCard *makeCollapsedCard(const QString &title,
                                      const QString &subtitle = QString(),
                                      const QString &features = QString(),
                                      const QString &code = QString(),
                                      bool collapsed = true);

Fluent::FluentCard *makeCollapsedExample(const QString &title,
                                         const QString &subtitle,
                                         const QString &features,
                                         const QString &code,
                                         const std::function<void(QVBoxLayout *body)> &buildDemo,
                                         bool collapsed = true,
                                         int codeBlockHeight = 190);

void applyAccent(const QColor &accent);
void applyBackground(const QColor &bg);
void applySurface(const QColor &surface);

Fluent::FluentScrollArea *makePage(const std::function<void(QVBoxLayout *)> &fill);
QWidget *makeSidebarCard(QWidget *inner);

} // namespace Demo

#define DEMO_TEXT(zh, en) ::Demo::text(QStringLiteral(zh), QStringLiteral(en))
