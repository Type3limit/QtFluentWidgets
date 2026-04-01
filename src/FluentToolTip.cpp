#include "Fluent/FluentToolTip.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include <QApplication>
#include <QEvent>
#include <QHelpEvent>
#include <QLabel>
#include <QPainter>
#include <QPointer>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
namespace Fluent {
namespace {
constexpr int kPadH = 10;
constexpr int kPadV = 7;
constexpr int kMaxWidth = 420;
constexpr int kOffsetX = 14;
constexpr int kOffsetY = 22;
constexpr qreal kRadius = 8.0;
class FluentToolTipWidget final : public QWidget {
public:
    FluentToolTipWidget() : QWidget(nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint) {
        setObjectName(QStringLiteral("FluentToolTip"));
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_StyledBackground, false);
        setAutoFillBackground(false);
        auto *lyt = new QVBoxLayout(this);
        lyt->setContentsMargins(kPadH, kPadV, kPadH, kPadV);
        lyt->setSpacing(0);
        m_label = new QLabel(this);
        m_label->setWordWrap(true);
        m_label->setTextFormat(Qt::PlainText);
        m_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        lyt->addWidget(m_label);
    }
    void showText(const QPoint &globalPos, const QString &text, QWidget *anchor, int msecDisplayTime) {
        if (text.trimmed().isEmpty()) { hideText(); return; }
        ensureThemeHook();
        m_anchor = anchor;
        m_label->setText(text);
        applyTheme();
        updateGeometryForText();
        move(positionFor(globalPos));
        show();
        raise();
        const int ms = msecDisplayTime > 0 ? msecDisplayTime : qBound(1600, 900 + text.size() * 55, 8000);
        m_hideTimer.stop();
        m_hideTimer.setSingleShot(true);
        QObject::disconnect(&m_hideTimer, nullptr, this, nullptr);
        connect(&m_hideTimer, &QTimer::timeout, this, &FluentToolTipWidget::hideText);
        m_hideTimer.start(ms);
    }
    void hideText() { m_hideTimer.stop(); hide(); }
protected:
    bool event(QEvent *event) override {
        if (event && (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::ApplicationDeactivate)) hideText();
        return QWidget::event(event);
    }
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event)
        const auto &colors = ThemeManager::instance().colors();
        const bool dark = colors.background.lightnessF() < 0.5;
        const QColor surface = Style::mix(colors.surface, colors.accent, dark ? 0.18 : 0.08);
        const QColor border = Style::mix(colors.border, colors.accent, dark ? 0.78 : 0.84);
        QPainter clear(this);
        if (!clear.isActive()) return;
        clear.setCompositionMode(QPainter::CompositionMode_Source);
        clear.fillRect(rect(), Qt::transparent);
        QPainter p(this);
        if (!p.isActive()) return;
        p.setRenderHint(QPainter::Antialiasing, true);
        FluentFrameSpec frame;
        frame.radius = kRadius;
        frame.maximized = false;
        frame.clearToTransparent = false;
        frame.surfaceOverride = surface;
        frame.borderColorOverride = border;
        frame.accentBorderEnabled = false;
        frame.borderWidth = 1.0;
        paintFluentPanel(p, QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), colors, frame);
    }
private:
    void ensureThemeHook() {
        if (m_themeHooked) return;
        m_themeHooked = true;
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
            applyTheme();
            updateGeometryForText();
            update();
        });
    }
    void applyTheme() {
        const auto &colors = ThemeManager::instance().colors();
        QFont f = QApplication::font();
        f.setPointSizeF(12.0);
        m_label->setFont(f);
        const QString next = QStringLiteral("QLabel { background: transparent; color: %1; font-weight: 500; }").arg(colors.text.name());
        if (m_label->styleSheet() != next) m_label->setStyleSheet(next);
    }
    void updateGeometryForText() {
        if (!layout() || !m_label) return;
        m_label->setMaximumWidth(kMaxWidth - kPadH * 2);
        m_label->adjustSize();
        layout()->activate();
        adjustSize();
    }
    QPoint positionFor(const QPoint &globalPos) const {
        QRect screenRect;
        if (auto *screen = QApplication::screenAt(globalPos)) screenRect = screen->availableGeometry();
        else if (auto *screen = QApplication::primaryScreen()) screenRect = screen->availableGeometry();
        QPoint pos = globalPos + QPoint(kOffsetX, kOffsetY);
        if (screenRect.isValid()) {
            if (pos.x() + width() > screenRect.right()) pos.setX(screenRect.right() - width());
            if (pos.y() + height() > screenRect.bottom()) pos.setY(globalPos.y() - height() - 10);
            pos.setX(qMax(screenRect.left(), pos.x()));
            pos.setY(qMax(screenRect.top(), pos.y()));
        }
        return pos;
    }
    QLabel *m_label = nullptr;
    QPointer<QWidget> m_anchor;
    QTimer m_hideTimer;
    bool m_themeHooked = false;
};
class FluentToolTipManager final : public QObject {
public:
    static FluentToolTipManager &instance() { static FluentToolTipManager m; return m; }
    void ensureInstalled() { if (!m_installed && qApp) { qApp->installEventFilter(this); m_installed = true; } }
    void showText(const QPoint &globalPos, const QString &text, QWidget *anchor, int msecDisplayTime) {
        ensureInstalled();
        m_toolTip.showText(globalPos, text, anchor, msecDisplayTime);
    }
    void hideText() { m_toolTip.hideText(); }
protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (!event) return QObject::eventFilter(watched, event);
        auto *widget = qobject_cast<QWidget *>(watched);
        switch (event->type()) {
        case QEvent::ToolTip:
            if (widget) {
                const QString text = widget->toolTip();
                if (text.trimmed().isEmpty()) { hideText(); return false; }
                auto *helpEvent = dynamic_cast<QHelpEvent *>(event);
                if (!helpEvent) return QObject::eventFilter(watched, event);
                showText(helpEvent->globalPos(), text, widget, widget->toolTipDuration());
                return true;
            }
            break;
        case QEvent::Leave:
        case QEvent::MouseButtonPress:
        case QEvent::Wheel:
        case QEvent::KeyPress:
        case QEvent::FocusOut:
            hideText();
            break;
        default:
            break;
        }
        return QObject::eventFilter(watched, event);
    }
private:
    FluentToolTipManager() = default;
    bool m_installed = false;
    FluentToolTipWidget m_toolTip;
};
} // namespace
void FluentToolTip::ensureInstalled() { FluentToolTipManager::instance().ensureInstalled(); }
void FluentToolTip::showText(const QPoint &globalPos, const QString &text, QWidget *anchor, int msecDisplayTime) { FluentToolTipManager::instance().showText(globalPos, text, anchor, msecDisplayTime); }
void FluentToolTip::hideText() { FluentToolTipManager::instance().hideText(); }
} // namespace Fluent
