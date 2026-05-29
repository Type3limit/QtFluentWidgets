#include "Fluent/FluentGroupBox.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QStyle>
#include <QStyleOptionButton>

namespace Fluent {

namespace {
constexpr int kHorizontalPadding = 14;
constexpr int kTopPadding = 38;
constexpr int kBottomPadding = 14;
constexpr int kTitleTop = 10;
constexpr int kTitleGap = 8;
constexpr int kTitleHeight = 22;

void initializeGroupBox(FluentGroupBox *box)
{
    box->setAttribute(Qt::WA_StyledBackground, false);
    box->setAutoFillBackground(false);
    box->setContentsMargins(kHorizontalPadding, kTopPadding, kHorizontalPadding, kBottomPadding);
}
} // namespace

FluentGroupBox::FluentGroupBox(QWidget *parent)
    : QGroupBox(parent)
{
    initializeGroupBox(this);
    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentGroupBox::applyTheme);
}

FluentGroupBox::FluentGroupBox(const QString &title, QWidget *parent)
    : QGroupBox(title, parent)
{
    initializeGroupBox(this);
    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentGroupBox::applyTheme);
}

void FluentGroupBox::changeEvent(QEvent *event)
{
    QGroupBox::changeEvent(event);
    if (event->type() == QEvent::EnabledChange || event->type() == QEvent::FontChange) {
        applyTheme();
    }
}

void FluentGroupBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const auto &manager = ThemeManager::instance();
    const auto &colors = manager.colors();
    const auto &tokens = manager.tokens();

    QPainter p(this);
    if (!p.isActive()) {
        return;
    }

    FluentSurfaceSpec surface;
    surface.level = FluentSurfaceLevel::Card;
    surface.radius = tokens.radius.card;
    surface.enabled = isEnabled();
    surface.borderWidth = 1.0;
    surface.borderInset = 0.5;
    paintFluentSurface(p, QRectF(rect()), colors, surface);

    const QString groupTitle = title();
    if (groupTitle.isEmpty() && !isCheckable()) {
        return;
    }

    p.setRenderHint(QPainter::Antialiasing, true);

    int textX = kHorizontalPadding;
    if (isCheckable()) {
        QStyleOptionButton check;
        check.initFrom(this);
        check.rect = checkBoxRect();
        check.state.setFlag(QStyle::State_On, isChecked());
        check.state.setFlag(QStyle::State_Off, !isChecked());
        check.state.setFlag(QStyle::State_Enabled, isEnabled());
        style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &check, &p, this);
        textX = check.rect.right() + 1 + kTitleGap;
    }

    if (!groupTitle.isEmpty()) {
        QFont titleFont = font();
        titleFont.setWeight(QFont::DemiBold);
        p.setFont(titleFont);
        QColor textColor = isEnabled() ? colors.text : colors.disabledText;
        p.setPen(textColor);
        QRect textRect = titleRect();
        textRect.setLeft(textX);
        p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine, groupTitle);
    }
}

void FluentGroupBox::applyTheme()
{
    update();
}

QRect FluentGroupBox::titleRect() const
{
    return QRect(kHorizontalPadding, kTitleTop, width() - (kHorizontalPadding * 2), kTitleHeight);
}

QRect FluentGroupBox::checkBoxRect() const
{
    const int indicatorWidth = style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, this);
    const int indicatorHeight = style()->pixelMetric(QStyle::PM_IndicatorHeight, nullptr, this);
    const int y = kTitleTop + ((kTitleHeight - indicatorHeight) / 2);
    return QRect(kHorizontalPadding, y, indicatorWidth, indicatorHeight);
}

} // namespace Fluent
