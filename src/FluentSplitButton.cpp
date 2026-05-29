#include "Fluent/FluentSplitButton.h"

#include "Fluent/FluentButton.h"
#include "Fluent/FluentMenu.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QAction>
#include <QHBoxLayout>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>

namespace Fluent {

namespace {

QSize logicalPixmapSize(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        return {};
    }

    const qreal dpr = pixmap.devicePixelRatio();
    return QSize(qRound(pixmap.width() / dpr), qRound(pixmap.height() / dpr));
}

enum class SplitSegment {
    Left,
    Right
};

QPainterPath segmentFillPath(const QRectF &rect, qreal radius, SplitSegment segment)
{
    QPainterPath path;
    radius = qMax<qreal>(0.0, qMin(radius, qMin(rect.width(), rect.height()) / 2.0));

    if (segment == SplitSegment::Left) {
        path.moveTo(rect.right(), rect.top());
        path.lineTo(rect.left() + radius, rect.top());
        path.arcTo(QRectF(rect.left(), rect.top(), radius * 2.0, radius * 2.0), 90.0, 90.0);
        path.lineTo(rect.left(), rect.bottom() - radius);
        path.arcTo(QRectF(rect.left(), rect.bottom() - radius * 2.0, radius * 2.0, radius * 2.0), 180.0, 90.0);
        path.lineTo(rect.right(), rect.bottom());
        path.closeSubpath();
    } else {
        path.moveTo(rect.left(), rect.top());
        path.lineTo(rect.right() - radius, rect.top());
        path.arcTo(QRectF(rect.right() - radius * 2.0, rect.top(), radius * 2.0, radius * 2.0), 90.0, -90.0);
        path.lineTo(rect.right(), rect.bottom() - radius);
        path.arcTo(QRectF(rect.right() - radius * 2.0, rect.bottom() - radius * 2.0, radius * 2.0, radius * 2.0), 0.0, -90.0);
        path.lineTo(rect.left(), rect.bottom());
        path.closeSubpath();
    }

    return path;
}

QPainterPath segmentBorderPath(const QRectF &rect, qreal radius, SplitSegment segment)
{
    QPainterPath path;
    radius = qMax<qreal>(0.0, qMin(radius, qMin(rect.width(), rect.height()) / 2.0));

    if (segment == SplitSegment::Left) {
        path.moveTo(rect.right(), rect.top());
        path.lineTo(rect.left() + radius, rect.top());
        path.arcTo(QRectF(rect.left(), rect.top(), radius * 2.0, radius * 2.0), 90.0, 90.0);
        path.lineTo(rect.left(), rect.bottom() - radius);
        path.arcTo(QRectF(rect.left(), rect.bottom() - radius * 2.0, radius * 2.0, radius * 2.0), 180.0, 90.0);
        path.lineTo(rect.right(), rect.bottom());
    } else {
        path.moveTo(rect.left(), rect.top());
        path.lineTo(rect.right() - radius, rect.top());
        path.arcTo(QRectF(rect.right() - radius * 2.0, rect.top(), radius * 2.0, radius * 2.0), 90.0, -90.0);
        path.lineTo(rect.right(), rect.bottom() - radius);
        path.arcTo(QRectF(rect.right() - radius * 2.0, rect.bottom() - radius * 2.0, radius * 2.0, radius * 2.0), 0.0, -90.0);
        path.lineTo(rect.left(), rect.bottom());
        path.moveTo(rect.left(), rect.top());
        path.lineTo(rect.left(), rect.bottom());
    }

    return path;
}

class SplitSegmentButton final : public FluentButton
{
public:
    SplitSegmentButton(SplitSegment segment, bool arrowOnly, QWidget *parent)
        : FluentButton(parent)
        , m_segment(segment)
        , m_arrowOnly(arrowOnly)
    {
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)

        const auto &colors = ThemeManager::instance().colors();
        const bool checked = isCheckable() && isChecked();

        QColor base;
        QColor hover;
        QColor pressed;
        QColor border;
        QColor textColor;
        const auto tokens = Theme::tokens(colors);

        if (isPrimary()) {
            base = checked ? tokens.accent.dark1 : tokens.accent.base;
            hover = tokens.accent.light1;
            pressed = tokens.accent.dark1;
            border = Style::mix(tokens.accent.base, tokens.onAccent, 0.18);
            textColor = tokens.onAccent;
        } else {
            const QColor accentTint = Style::mix(colors.surface, colors.accent, 0.12);
            base = checked ? accentTint : colors.surface;
            hover = checked ? Style::mix(accentTint, colors.accent, 0.10)
                            : Style::mix(colors.surface, colors.hover, 0.88);
            pressed = checked ? Style::mix(accentTint, colors.accent, 0.18)
                              : Style::mix(colors.surface, colors.pressed, 0.92);
            border = checked ? Style::mix(colors.border, colors.accent, 0.85) : colors.border;
            textColor = checked ? Theme::contrastColor(accentTint) : colors.text;
        }

        QColor fill = Style::mix(base, hover, hoverLevel());
        fill = Style::mix(fill, pressed, pressLevel());

        if (!isEnabled()) {
            fill = Style::mix(colors.surface, colors.hover, 0.45);
            border = Style::mix(colors.border, colors.disabledText, 0.25);
            textColor = colors.disabledText;
        }

        QPainter painter(this);
        if (!painter.isActive()) {
            return;
        }
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const auto metrics = Style::metrics();
        const QRectF buttonRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        const qreal radius = metrics.radius;

        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);
        painter.drawPath(segmentFillPath(buttonRect, radius, m_segment));

        painter.setPen(QPen(border, 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(segmentBorderPath(buttonRect, radius, m_segment));

        if (hasFocus() && isEnabled()) {
            QColor focus = colors.focus;
            focus.setAlpha(230);
            painter.setPen(QPen(focus, 2.0));
            painter.setBrush(Qt::NoBrush);
            const QRectF focusRect = buttonRect.adjusted(1.0, 1.0, -1.0, -1.0);
            painter.drawPath(segmentFillPath(focusRect, qMax<qreal>(0.0, radius - 1.0), m_segment));
        }

        if (m_arrowOnly) {
            Style::drawChevronDown(painter, QRectF(rect()).center(), textColor, 7.0, 1.6);
            return;
        }

        painter.setPen(textColor);
        painter.setFont(font());
        const QRect contentRect = QRectF(rect()).adjusted(10, 0, -10, 0).toRect();

        if (!icon().isNull()) {
            const QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
            const QIcon::State state = (isCheckable() && isChecked()) ? QIcon::On : QIcon::Off;
            const QSize requestedSize = iconSize().isValid()
                                            ? iconSize()
                                            : QSize(metrics.height - 12, metrics.height - 12);
            const QSize actualSize = icon().actualSize(requestedSize, mode, state);
            const QPixmap pixmap = icon().pixmap(actualSize, mode, state);
            const QSize drawSize = logicalPixmapSize(pixmap);

            if (pixmap.isNull() || !drawSize.isValid()) {
                painter.drawText(contentRect, Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic, text());
                return;
            }

            if (text().isEmpty()) {
                const QRect iconRect(contentRect.center().x() - drawSize.width() / 2,
                                     contentRect.center().y() - drawSize.height() / 2,
                                     drawSize.width(),
                                     drawSize.height());
                painter.drawPixmap(iconRect, pixmap);
                return;
            }

            const int gap = 8;
            const int textWidth = fontMetrics().horizontalAdvance(text());
            const int totalWidth = drawSize.width() + gap + textWidth;
            const int startX = contentRect.center().x() - totalWidth / 2;
            const QRect iconRect(startX,
                                 contentRect.center().y() - drawSize.height() / 2,
                                 drawSize.width(),
                                 drawSize.height());
            painter.drawPixmap(iconRect, pixmap);

            const QRect textRect(startX + drawSize.width() + gap,
                                 contentRect.top(),
                                 contentRect.right() - startX - drawSize.width() - gap + 1,
                                 contentRect.height());
            painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, text());
        } else {
            painter.drawText(contentRect, Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic, text());
        }
    }

private:
    SplitSegment m_segment = SplitSegment::Left;
    bool m_arrowOnly = false;
};

} // namespace

FluentSplitButton::FluentSplitButton(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    m_mainButton = new SplitSegmentButton(SplitSegment::Left, false, this);
    m_dropDownButton = new SplitSegmentButton(SplitSegment::Right, true, this);
    m_dropDownButton->setFixedWidth(34);

    m_layout->addWidget(m_mainButton);
    m_layout->addWidget(m_dropDownButton);

    connect(m_mainButton, &QPushButton::clicked, this, [this]() {
        if (m_defaultAction && m_defaultAction->isEnabled()) {
            m_defaultAction->trigger();
            emit triggered(m_defaultAction);
        }
        emit clicked();
    });
    connect(m_dropDownButton, &QPushButton::clicked, this, &FluentSplitButton::showMenu);
}

FluentSplitButton::FluentSplitButton(const QString &text, QWidget *parent)
    : FluentSplitButton(parent)
{
    setText(text);
}

QString FluentSplitButton::text() const
{
    return m_mainButton->text();
}

void FluentSplitButton::setText(const QString &text)
{
    m_mainButton->setText(text);
}

bool FluentSplitButton::isPrimary() const
{
    return m_primary;
}

void FluentSplitButton::setPrimary(bool primary)
{
    if (m_primary == primary) {
        return;
    }
    m_primary = primary;
    m_mainButton->setPrimary(primary);
    m_dropDownButton->setPrimary(primary);
}

QAction *FluentSplitButton::defaultAction() const
{
    return m_defaultAction.data();
}

void FluentSplitButton::setDefaultAction(QAction *action)
{
    if (m_defaultAction == action) {
        return;
    }
    if (m_defaultAction) {
        disconnect(m_defaultAction, nullptr, this, nullptr);
    }
    m_defaultAction = action;
    if (m_defaultAction) {
        connect(m_defaultAction, &QAction::changed, this, &FluentSplitButton::syncDefaultAction);
    }
    syncDefaultAction();
}

QMenu *FluentSplitButton::menu() const
{
    return m_menu.data();
}

void FluentSplitButton::setMenu(QMenu *menu)
{
    if (menu && !menu->parent()) {
        menu->setParent(this);
    }
    m_menu = menu;
    m_dropDownButton->setEnabled(menu != nullptr);
}

FluentButton *FluentSplitButton::mainButton() const
{
    return m_mainButton;
}

FluentButton *FluentSplitButton::dropDownButton() const
{
    return m_dropDownButton;
}

void FluentSplitButton::showMenu()
{
    if (!m_menu) {
        return;
    }

    const QPoint pos = mapToGlobal(QPoint(0, height() + 4));
    if (auto *fluentMenu = qobject_cast<FluentMenu *>(m_menu.data())) {
        fluentMenu->popup(pos);
    } else {
        m_menu->popup(pos);
    }
}

void FluentSplitButton::syncDefaultAction()
{
    if (!m_defaultAction) {
        m_mainButton->setEnabled(true);
        return;
    }

    m_mainButton->setText(m_defaultAction->text());
    m_mainButton->setIcon(m_defaultAction->icon());
    m_mainButton->setToolTip(m_defaultAction->toolTip());
    m_mainButton->setEnabled(m_defaultAction->isEnabled());
}

} // namespace Fluent
