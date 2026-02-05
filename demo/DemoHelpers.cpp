#include "DemoHelpers.h"

#include "DemoCodeEditorSettings.h"

#include "Fluent/FluentCard.h"
#include "Fluent/FluentCodeEditor.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentWidget.h"

#include <QVBoxLayout>

namespace Demo {

using namespace Fluent;

Section makeSection(const QString &title, const QString &subtitle)
{
    auto *card = new FluentCard();
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(10);

    auto *t = new FluentLabel(title);
    t->setStyleSheet("font-size: 16px; font-weight: 600;");
    layout->addWidget(t);

    if (!subtitle.isEmpty()) {
        auto *st = new FluentLabel(subtitle);
        st->setStyleSheet("font-size: 12px; opacity: 0.85;");
        layout->addWidget(st);
    }

    auto *body = new QVBoxLayout();
    body->setSpacing(10);
    layout->addLayout(body);

    return {card, body};
}

FluentCard *makeCollapsedCard(const QString &title,
                              const QString &subtitle,
                              const QString &features,
                              const QString &code,
                              bool collapsed)
{
    auto *card = new FluentCard();
    card->setCollapsible(true);
    card->setTitle(title);
    card->setCollapsed(collapsed);

    auto *body = card->contentLayout();
    if (!body) {
        return card;
    }

    if (!subtitle.isEmpty()) {
        auto *st = new FluentLabel(subtitle);
        st->setWordWrap(true);
        st->setStyleSheet("font-size: 12px; opacity: 0.85;");
        body->addWidget(st);
    }

    if (!features.isEmpty()) {
        auto *ft = new FluentLabel(features);
        ft->setWordWrap(true);
        ft->setStyleSheet("font-size: 12px; opacity: 0.9;");
        body->addWidget(ft);
    }

    if (!code.isEmpty()) {
        auto *edit = new FluentCodeEditor();
        edit->setReadOnly(true);
        edit->setAutoFormatEnabled(false);
        edit->setAutoBraceNewlineEnabled(false);
        edit->setClangFormatMissingHintEnabled(false);
        QString pretty = code;
        pretty.replace("\\\"", "\"");
        edit->setPlainText(pretty);
        DemoCodeEditorSettings::instance().attach(edit, true);
        edit->setFixedHeight(170);
        body->addWidget(edit);
    }

    return card;
}

FluentCard *makeCollapsedExample(const QString &title,
                                 const QString &subtitle,
                                 const QString &features,
                                 const QString &code,
                                 const std::function<void(QVBoxLayout *body)> &buildDemo,
                                 bool collapsed,
                                 int codeBlockHeight)
{
    auto *card = new FluentCard();
    card->setCollapsible(true);
    card->setTitle(title);
    card->setCollapsed(collapsed);

    auto *body = card->contentLayout();
    if (!body) {
        return card;
    }

    if (!subtitle.isEmpty()) {
        auto *st = new FluentLabel(subtitle);
        st->setWordWrap(true);
        st->setStyleSheet("font-size: 12px; opacity: 0.85;");
        body->addWidget(st);
    }

    if (!features.isEmpty()) {
        auto *ft = new FluentLabel(features);
        ft->setWordWrap(true);
        ft->setStyleSheet("font-size: 12px; opacity: 0.9;");
        body->addWidget(ft);
    }

    if (buildDemo) {
        buildDemo(body);
    }

    if (!code.isEmpty()) {
        auto *edit = new FluentCodeEditor();
        edit->setReadOnly(true);
        edit->setAutoFormatEnabled(false);
        edit->setAutoBraceNewlineEnabled(false);
        edit->setClangFormatMissingHintEnabled(false);
        QString pretty = code;
        pretty.replace("\\\"", "\"");
        edit->setPlainText(pretty);
        DemoCodeEditorSettings::instance().attach(edit, true);
        edit->setFixedHeight(codeBlockHeight);
        body->addWidget(edit);
    }

    return card;
}

void applyAccent(const QColor &accent)
{
    auto colors = ThemeManager::instance().colors();
    colors.accent = accent;
    colors.focus = accent.lighter(135);
    ThemeManager::instance().setColors(colors);
}

void applyBackground(const QColor &bg)
{
    auto colors = ThemeManager::instance().colors();
    colors.background = bg;
    ThemeManager::instance().setColors(colors);
}

void applySurface(const QColor &surface)
{
    auto colors = ThemeManager::instance().colors();
    colors.surface = surface;
    ThemeManager::instance().setColors(colors);
}

FluentScrollArea *makePage(const std::function<void(QVBoxLayout *)> &fill)
{
    auto *area = new FluentScrollArea();
    area->setOverlayScrollBarsEnabled(true);

    auto *content = new FluentWidget();
    content->setBackgroundRole(FluentWidget::BackgroundRole::WindowBackground);
    area->setWidget(content);

    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    fill(layout);
    layout->addStretch(1);

    return area;
}

QWidget *makeSidebarCard(QWidget *inner)
{
    auto *card = new FluentCard();
    auto *l = new QVBoxLayout(card);
    l->setContentsMargins(16, 16, 16, 16);
    l->setSpacing(10);
    l->addWidget(inner);
    return card;
}

} // namespace Demo
