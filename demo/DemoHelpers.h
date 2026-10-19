#pragma once

#include <QByteArray>
#include <functional>
#include <QString>
#include <QList>
#include <QStringList>

class QWidget;
class QGridLayout;
class QVBoxLayout;
class QString;
class QColor;

namespace Fluent {
class FluentAnimatedButton;
class FluentCard;
class FluentLottieWidget;
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

QString annotatedRailTitleForWidget(QWidget *widget);

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

// State matrix: a caption column plus equal-width state columns on one grid, so
// every row lines up and cells shrink with the card instead of overflowing it.
QGridLayout *makeStateMatrix(const QString &captionHeader, const QStringList &stateHeaders);
// Appends a row; a cell spanning several state columns can be passed with span > 1.
void addStateMatrixRow(QGridLayout *matrix, const QString &caption, const QList<QWidget *> &cells, int span = 1);

void applyAccent(const QColor &accent);
void applyBackground(const QColor &bg);
void applySurface(const QColor &surface);

QByteArray animatedSearchIconJson();
QString rlottieResourcePath(const QString &fileName);
bool loadRlottieResource(Fluent::FluentLottieWidget *widget, const QString &fileName);
QString demoLottieResourcePath(const QString &fileName);
bool loadDemoLottieResource(Fluent::FluentLottieWidget *widget, const QString &fileName);
Fluent::FluentAnimatedButton *makeAnimatedSearchButton(const QString &text, QWidget *parent = nullptr);

// 页眉：eyebrow（等宽小字+字距）/ 大标题 / 描述。颜色随主题，仅排版用 stylesheet。
QWidget *makePageHeader(const QString &eyebrow, const QString &title, const QString &desc);
QWidget *makePage(const std::function<void(QVBoxLayout *)> &fill);
QWidget *makeSidebarCard(QWidget *inner);

} // namespace Demo

#define DEMO_TEXT(zh, en) ::Demo::text(QStringLiteral(zh), QStringLiteral(en))
