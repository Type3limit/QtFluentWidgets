#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QAbstractItemView>
#include <QEvent>
#include <QFrame>
#include <QListView>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QRegion>
#include <QStyledItemDelegate>
#include <QVariantAnimation>
#include <QApplication>
#include <QPointer>
#include <QScreen>
#include <QTimer>
#include <QWindow>

namespace Fluent {

namespace {

class FluentComboPopupView final : public QListView
{
public:
    explicit FluentComboPopupView(QWidget *parent = nullptr)
        : QListView(parent)
    {
        // QComboBox owns a private popup container; the view itself should not be a top-level popup.
        setAttribute(Qt::WA_StyledBackground, true);
        setAutoFillBackground(false);
        setFrameShape(QFrame::NoFrame);

        setSpacing(2);
        // Keep a small inset for rounded corners; no shadows for performance.
        setViewportMargins(6, 6, 6, 6);
        setUniformItemSizes(true);
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        setVerticalScrollBar(new FluentScrollBar(Qt::Vertical, this));

        if (viewport()) {
            viewport()->setAutoFillBackground(false);
            viewport()->setAttribute(Qt::WA_StyledBackground, true);
        }
    }

protected:
    bool event(QEvent *event) override
    {
        if (event && event->type() == QEvent::Show) {
            QTimer::singleShot(0, this, [this]() { patchContainer(); });
        }

        if (event && (event->type() == QEvent::ParentChange || event->type() == QEvent::Show)) {
            QTimer::singleShot(0, this, [this]() { patchContainer(); });
        }
        return QListView::event(event);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        // Always let Qt paint the view + items first.
        // If we return early here (e.g. QPainter engine==0 during popup show), the popup can become blank.
        struct ScopedViewportPaintFlag {
            QWidget *vp = nullptr;
            bool prev = false;
            explicit ScopedViewportPaintFlag(QWidget *v)
                : vp(v)
            {
                if (!vp) {
                    return;
                }
                prev = vp->testAttribute(Qt::WA_WState_InPaintEvent);
                if (!prev) {
                    vp->setAttribute(Qt::WA_WState_InPaintEvent, true);
                }
            }
            ~ScopedViewportPaintFlag()
            {
                if (vp && !prev) {
                    vp->setAttribute(Qt::WA_WState_InPaintEvent, false);
                }
            }
        } scopedViewportPaint(viewport());

        QListView::paintEvent(event);

        // Optional decoration painting (rounded surface + border) on the view widget itself.
        // Guard on exposure to avoid sporadic "engine == 0" warnings during popup startup.
        if (QWidget *w = window()) {
            if (w->windowHandle() && !w->windowHandle()->isExposed()) {
                return;
            }
        }

        QPainter painter(this); // Paint on the widget to cover the margins area
        if (!painter.isActive()) {
            return;
        }
        painter.setRenderHint(QPainter::Antialiasing, true);
        const auto &colors = ThemeManager::instance().colors();

        const QRectF r = QRectF(this->rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        const qreal radius = 10.0;

        painter.setPen(QPen(colors.border, 1));
        painter.setBrush(colors.surface);
        painter.drawRoundedRect(r, radius, radius);
    }

private:
    static void applyRoundedMask(QWidget *w, qreal radius)
    {
        if (!w) {
            return;
        }

        if (!w->isWindow()) {
            return;
        }

        const QRectF r(0.0, 0.0, w->width(), w->height());
        QPainterPath path;
        path.addRoundedRect(r, radius, radius);
        w->setMask(QRegion(path.toFillPolygon().toPolygon()));
    }

    void patchContainer()
    {
        // QComboBox shows the view inside a private popup container window.
        // Only patch that popup; never touch the combobox itself.
        QWidget *popup = window();
        if (!popup) {
            return;
        }

        if (!(popup->windowFlags().testFlag(Qt::Popup))) {
            return;
        }

        // Avoid translucent popup windows on Windows (often appears as black in light mode).
        popup->setAttribute(Qt::WA_TranslucentBackground, false);
        popup->setAttribute(Qt::WA_StyledBackground, true);
        popup->setAutoFillBackground(false);

        // Remove default popup shadow + square window chrome.
        // Only set once; changing window flags during show/resize can trigger transient paint warnings.
        if (!popup->property("_fluentComboPopupPatched").toBool()) {
            popup->setProperty("_fluentComboPopupPatched", true);
            popup->setWindowFlag(Qt::FramelessWindowHint, true);
            popup->setWindowFlag(Qt::NoDropShadowWindowHint, true);
        }

        const auto &colors = ThemeManager::instance().colors();
        popup->setStyleSheet(QString(
            "background: %1;"
            "border: 1px solid %2;"
            "border-radius: 10px;"
        ).arg(colors.surface.name()).arg(colors.border.name()));

        if (auto *frame = qobject_cast<QFrame *>(popup)) {
            frame->setFrameShape(QFrame::NoFrame);
        }

        // Shape the popup itself to rounded corners so there is no square backdrop.
        applyRoundedMask(popup, 10.0);
    }
};

class FluentComboItemDelegate final : public QStyledItemDelegate
{
public:
    explicit FluentComboItemDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        // Increase item height in popup logic
        QSize sz = QStyledItemDelegate::sizeHint(option, index);
        if (sz.height() < 32) sz.setHeight(32); // Minimum 32px height for Fluent ComboBox items
        return sz;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        opt.state &= ~QStyle::State_HasFocus; // Remove dotted line

        const auto &colors = ThemeManager::instance().colors();

        const bool enabled = opt.state.testFlag(QStyle::State_Enabled);

        const QRectF itemRect = QRectF(opt.rect).adjusted(4, 2, -4, -2);
        const bool selected = opt.state.testFlag(QStyle::State_Selected);
        const bool hovered = opt.state.testFlag(QStyle::State_MouseOver);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        // Background
        QColor bgColor = Qt::transparent;
        if (selected) {
            bgColor = colors.accent;
            bgColor.setAlpha(34);
        } else if (hovered) {
            bgColor = colors.hover;
            bgColor.setAlpha(160);
        }

        if (bgColor.alpha() > 0) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(bgColor);
            painter->drawRoundedRect(itemRect, 4, 4);
        }

        // Selected indicator
        if (selected) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(colors.accent);
            const qreal indicatorHeight = 16.0;
            const QRectF indRect(itemRect.left(), itemRect.center().y() - indicatorHeight / 2.0, 3, indicatorHeight);
            painter->drawRoundedRect(indRect, 1.5, 1.5);
        }

        // Icon + text (fully custom to avoid native selection rendering)
        const int iconSize = 16;
        const int leftPadding = 10;
        qreal x = itemRect.left() + leftPadding;

        const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
        if (!icon.isNull()) {
            const QRect iconRect(static_cast<int>(x), static_cast<int>(itemRect.center().y() - iconSize / 2), iconSize, iconSize);
            icon.paint(painter, iconRect, Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
            x += iconSize + 8;
        }

        const QString text = opt.text;
        const QRectF textRect(x, itemRect.top(), itemRect.right() - x - 8, itemRect.height());
        painter->setPen(enabled ? colors.text : colors.disabledText);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

        painter->restore();
    }
};

} // namespace

FluentComboBox::FluentComboBox(QWidget *parent)
    : QComboBox(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);

    setMinimumHeight(Style::metrics().height);

    m_hoverAnim = new QPropertyAnimation(this, "hoverLevel", this);
    m_hoverAnim->setDuration(120);

    setView(new FluentComboPopupView(this));
    if (view()) {
        view()->setItemDelegate(new FluentComboItemDelegate(view()));
        view()->setMouseTracking(true);
        view()->setAttribute(Qt::WA_TranslucentBackground, false);
        if (view()->viewport()) {
            view()->viewport()->setAttribute(Qt::WA_TranslucentBackground, false);
            view()->viewport()->setAutoFillBackground(false);
        }
    }

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentComboBox::applyTheme);
}
QSize FluentComboBox::sizeHint() const
{
    QSize sz = QComboBox::sizeHint();
    if (sz.height() < 32) sz.setHeight(32);
    return sz;
}
qreal FluentComboBox::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentComboBox::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

void FluentComboBox::changeEvent(QEvent *event)
{
    QComboBox::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentComboBox::applyTheme()
{
    const auto &colors = ThemeManager::instance().colors();
    if (view()) {
        const QString viewNext = QString(
            "QAbstractItemView {"
            "  background: transparent;"
            "  color: %1;"
            "  border: none;"
            "  outline: 0;"
            "}"
            "QScrollBar:vertical {"
            "  background: transparent;"
            "  width: 10px;"
            "  margin: 2px;"
            "}"
            "QScrollBar::handle:vertical {"
            "  background-color: %2;"
            "  border: 1px solid transparent;"
            "  border-radius: 999px;"
            "  min-height: 24px;"
            "}"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
            "  height: 0px;"
            "}"
        ).arg(colors.text.name())
         .arg(colors.border.name());

        if (view()->styleSheet() != viewNext) {
            view()->setStyleSheet(viewNext);
        }

        // Ensure the private popup container uses solid Fluent surface.
        if (QWidget *container = view()->parentWidget()) {
            container->setAttribute(Qt::WA_TranslucentBackground, false);
            container->setAttribute(Qt::WA_StyledBackground, true);
            container->setAutoFillBackground(false);
            const QString containerNext = QString(
                "background: %1;"
                "border: 1px solid %2;"
                "border-radius: 10px;"
            ).arg(colors.surface.name()).arg(colors.border.name());
            if (container->styleSheet() != containerNext) {
                container->setStyleSheet(containerNext);
            }
        }
    }
    update();
}

void FluentComboBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();

    const bool enabled = isEnabled();
    const bool expanded = (view() && view()->isVisible());

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF rect = QRectF(this->rect());
    Style::paintControlSurface(painter, rect, colors, m_hoverLevel, hasFocus() ? 1.0 : 0.0, enabled, expanded);

    const auto m = Style::metrics();
    const QRect textRect = this->rect().adjusted(m.paddingX, 0, -m.iconAreaWidth, 0);
    const QColor textColor = enabled ? colors.text : colors.disabledText;

    painter.setPen(textColor);
    const QString elided = fontMetrics().elidedText(currentText(), Qt::ElideRight, textRect.width());
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elided);

    const QRect arrowRect(this->rect().right() - m.iconAreaWidth, this->rect().top(), m.iconAreaWidth, this->rect().height());
    QColor separator = colors.border;
    separator.setAlpha(80);
    painter.setPen(QPen(separator, 1));
    painter.drawLine(QPointF(arrowRect.left() + 0.5, arrowRect.top() + 8), QPointF(arrowRect.left() + 0.5, arrowRect.bottom() - 8));

    const QColor iconColor = enabled ? colors.subText : colors.disabledText;
    Style::drawChevronDown(painter, arrowRect.center(), iconColor, 8.0, 1.7);
}

void FluentComboBox::enterEvent(QEvent *event)
{
    QComboBox::enterEvent(event);
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();
}

void FluentComboBox::leaveEvent(QEvent *event)
{
    QComboBox::leaveEvent(event);
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(0.0);
    m_hoverAnim->start();
}

void FluentComboBox::showPopup()
{
    // Let Qt create/size the private popup container first.
    QComboBox::showPopup();

    if (!view()) {
        return;
    }

    QWidget *popup = view()->window();
    if (!popup || !(popup->windowFlags().testFlag(Qt::Popup))) {
        return;
    }

    const int gap = 5;

    const QPoint globalTopLeft = mapToGlobal(QPoint(0, 0));
    const int comboTopY = globalTopLeft.y();
    const int comboBottomY = globalTopLeft.y() + height();

    QRect popupGeom = popup->geometry();

    // Determine the screen to clamp within.
    QScreen *screen = QApplication::screenAt(globalTopLeft);
    if (!screen) {
        screen = QApplication::primaryScreen();
    }
    const QRect avail = screen ? screen->availableGeometry() : QRect();

    // If Qt opened below, add a downward gap; if opened above, add an upward gap.
    const bool openedBelow = popupGeom.top() >= comboBottomY - 1;
    const bool openedAbove = popupGeom.bottom() <= comboTopY + 1;

    if (openedBelow) {
        popupGeom.moveTop(comboBottomY + gap);
    } else if (openedAbove) {
        popupGeom.moveTop(comboTopY - gap - popupGeom.height());
    } else {
        // Fallback: keep direction consistent with center.
        popupGeom.moveTop(comboBottomY + gap);
    }

    // Clamp vertically to the available screen area.
    if (avail.isValid()) {
        if (popupGeom.top() < avail.top()) {
            popupGeom.moveTop(avail.top());
        }
        if (popupGeom.bottom() > avail.bottom()) {
            popupGeom.moveBottom(avail.bottom());
        }
    }

    popup->setGeometry(popupGeom);
}

} // namespace Fluent
