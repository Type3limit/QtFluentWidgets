#include "Fluent/FluentMenu.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentPopupSurface.h"
#include "Fluent/FluentQtCompat.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "FluentMenuPopupHost.h"

#include <QApplication>
#include <QAction>
#include <QCursor>
#include <QEventLoop>
#include <QFontMetrics>
#include <QHideEvent>
#include <QKeyEvent>
#include <QEasingCurve>
#include <QMouseEvent>
#include <QPainter>
#include <QProxyStyle>
#include <QRegion>
#include <QResizeEvent>
#include <QScreen>
#include <QShowEvent>
#include <QStyle>
#include <QTimer>
#include <QVariantAnimation>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace Fluent {

namespace {

class FluentMenuProxyStyle2 final : public QProxyStyle
{
public:
    explicit FluentMenuProxyStyle2(QStyle *base)
        : QProxyStyle(base)
    {
    }

    int styleHint(StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override
    {
        switch (hint) {
        case QStyle::SH_Menu_SubMenuPopupDelay:
            return 120;
        case QStyle::SH_Menu_SloppySubMenus:
            return 0;
        default:
            break;
        }
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = nullptr) const override
    {
        switch (element) {
        case QStyle::PE_PanelMenu:
        case QStyle::PE_FrameMenu:
            return;
        default:
            break;
        }

        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override
    {
        if (metric == QStyle::PM_MenuPanelWidth) {
            return 0;
        }

        return QProxyStyle::pixelMetric(metric, option, widget);
    }

};

QString rgbaString2(const QColor &c)
{
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
        .arg(c.alpha());
}

static QRect popupAnchorRect(const QMenu *menu)
{
    if (!menu) {
        return QRect();
    }

    const auto *parentMenu = qobject_cast<const QMenu *>(menu->parentWidget());
    if (parentMenu) {
        const QList<QAction *> parentActions = parentMenu->actions();
        for (QAction *action : parentActions) {
            if (!action || action->menu() != menu) {
                continue;
            }

            const QRect actionRect = parentMenu->actionGeometry(action);
            if (actionRect.isValid()) {
                return QRect(parentMenu->mapToGlobal(actionRect.topLeft()), actionRect.size());
            }
            break;
        }
    }

    if (const QWidget *anchor = menu->parentWidget()) {
        return QRect(anchor->mapToGlobal(QPoint(0, 0)), anchor->size());
    }

    return QRect();
}

static int popupOpenSlideOffsetY(const QMenu *menu, const QPoint &finalPos, int popupHeight)
{
    const QRect anchorRect = popupAnchorRect(menu);
    if (anchorRect.isValid()) {
        if (finalPos.y() >= anchorRect.bottom()) {
            return -FluentMotion::popupSlideOffset();
        }

        if (finalPos.y() + popupHeight <= anchorRect.top()) {
            return FluentMotion::popupSlideOffset();
        }
    }

    QScreen *screen = QApplication::screenAt(finalPos);
    if (!screen) {
        screen = QApplication::primaryScreen();
    }

    const QRect avail = screen ? screen->availableGeometry() : QRect();
    if (avail.isValid() && finalPos.y() > avail.center().y()) {
        return FluentMotion::popupSlideOffset();
    }

    return -FluentMotion::popupSlideOffset();
}

QRegion roundedPopupRegion(const QRect &rect)
{
    if (!rect.isValid()) {
        return {};
    }

    const QPolygon polygon = PopupSurface::contentClipPath(rect).toFillPolygon().toPolygon();
    if (polygon.isEmpty()) {
        return {};
    }

    return QRegion(polygon);
}

#ifdef Q_OS_WIN
static void applyWindowsMenuPopupAttributes(QWidget *widget)
{
    if (!widget) {
        return;
    }

    const HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    if (!hwnd) {
        return;
    }

    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style &= ~(static_cast<LONG_PTR>(WS_BORDER) |
               static_cast<LONG_PTR>(WS_DLGFRAME) |
               static_cast<LONG_PTR>(WS_THICKFRAME));
    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    exStyle &= ~(static_cast<LONG_PTR>(WS_EX_WINDOWEDGE) |
                 static_cast<LONG_PTR>(WS_EX_CLIENTEDGE) |
                 static_cast<LONG_PTR>(WS_EX_STATICEDGE) |
                 static_cast<LONG_PTR>(WS_EX_DLGMODALFRAME));
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

    const ULONG_PTR classStyle = static_cast<ULONG_PTR>(GetClassLongPtr(hwnd, GCL_STYLE));
    if ((classStyle & CS_DROPSHADOW) != 0u) {
        SetClassLongPtr(hwnd, GCL_STYLE, static_cast<LONG_PTR>(classStyle & ~static_cast<ULONG_PTR>(CS_DROPSHADOW)));
    }

    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (dwm) {
        using DwmSetWindowAttributeFn = HRESULT(WINAPI *)(HWND, DWORD, LPCVOID, DWORD);
        auto fn = reinterpret_cast<DwmSetWindowAttributeFn>(GetProcAddress(dwm, "DwmSetWindowAttribute"));
        if (fn) {
#ifndef DWMWA_NCRENDERING_POLICY
#define DWMWA_NCRENDERING_POLICY 2
#endif
#ifndef DWMNCRP_DISABLED
#define DWMNCRP_DISABLED 1
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_DONOTROUND
#define DWMWCP_DONOTROUND 1
#endif
            const DWORD ncPolicy = DWMNCRP_DISABLED;
            fn(hwnd, DWMWA_NCRENDERING_POLICY, &ncPolicy, sizeof(ncPolicy));

            const int cornerPref = DWMWCP_DONOTROUND;
            fn(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));
        }
        FreeLibrary(dwm);
    }

    SetWindowPos(hwnd,
                 nullptr,
                 0,
                 0,
                 0,
                 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}
#endif

} // namespace

FluentMenu::FluentMenu(QWidget *parent)
    : QMenu(parent)
    , m_border(this)
{
    setMouseTracking(true);
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_StyledBackground, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAutoFillBackground(false);

    setStyle(new FluentMenuProxyStyle2(style()));
    m_border.setRequestUpdate([this]() { update(); });

    connect(this, &QMenu::aboutToShow, this, [this]() { ensureSubMenusFluent(); });
    connect(this, &QMenu::hovered, this, [this](QAction *a) {
        if (a == m_hoverAction) {
            return;
        }
        m_hoverAction = a;
        setActiveAction(a);
        updateHighlightForAction(a, true);
        startHoverAnimation(a ? 1.0 : 0.0);
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentMenu::applyTheme);
    m_border.syncFromTheme();

    m_hoverAnim = new QVariantAnimation(this);
    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_highlightAnim = new QVariantAnimation(this);
    FluentMotion::configure(m_highlightAnim, FluentMotionRole::Selection);
    connect(m_highlightAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_highlightRect = value.toRect();
        update();
    });
}

FluentMenu::FluentMenu(const QString &title, QWidget *parent)
    : QMenu(title, parent)
    , m_border(this)
{
    setMouseTracking(true);
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_StyledBackground, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAutoFillBackground(false);

    setStyle(new FluentMenuProxyStyle2(style()));
    m_border.setRequestUpdate([this]() { update(); });

    connect(this, &QMenu::aboutToShow, this, [this]() { ensureSubMenusFluent(); });
    connect(this, &QMenu::hovered, this, [this](QAction *a) {
        if (a == m_hoverAction) {
            return;
        }
        m_hoverAction = a;
        setActiveAction(a);
        updateHighlightForAction(a, true);
        startHoverAnimation(a ? 1.0 : 0.0);
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentMenu::applyTheme);
    m_border.syncFromTheme();

    m_hoverAnim = new QVariantAnimation(this);
    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_highlightAnim = new QVariantAnimation(this);
    FluentMotion::configure(m_highlightAnim, FluentMotionRole::Selection);
    connect(m_highlightAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_highlightRect = value.toRect();
        update();
    });
}

FluentMenu *FluentMenu::addFluentMenu(const QString &title)
{
    auto *menu = new FluentMenu(title, this);
    addMenu(menu);
    return menu;
}

void FluentMenu::closePopupHost()
{
    if (m_popupHost) {
        m_popupHost->close();
        m_popupHost = nullptr;
    }
}

void FluentMenu::popup(const QPoint &pos, QAction *atAction)
{
    ensureSubMenusFluent();
    closePopupHost();

    auto *popup = new FluentMenuPopupHost(this);
    popup->setAttribute(Qt::WA_DeleteOnClose, true);
    m_popupHost = popup;

    QObject::connect(popup, &QObject::destroyed, this, [this]() {
        m_popupHost = nullptr;
    });

    popup->popupAt(pos, atAction);
}

QAction *FluentMenu::exec(const QPoint &pos, QAction *atAction)
{
    ensureSubMenusFluent();
    closePopupHost();

    QAction *chosenAction = nullptr;
    QEventLoop loop;

    auto *popup = new FluentMenuPopupHost(this);
    popup->setAttribute(Qt::WA_DeleteOnClose, true);
    m_popupHost = popup;

    QObject::connect(popup, &QObject::destroyed, this, [this, &loop]() {
        m_popupHost = nullptr;
        if (loop.isRunning()) {
            loop.quit();
        }
    });

    popup->onActionTriggered = [&chosenAction](QAction *action) {
        chosenAction = action;
    };
    popup->popupAt(pos, atAction);
    loop.exec();
    return chosenAction;
}

void FluentMenu::ensureSubMenusFluent()
{
    const QList<QAction *> topActions = actions();
    for (QAction *a : topActions) {
        if (!a) {
            continue;
        }

        QMenu *sub = a->menu();
        if (!sub) {
            continue;
        }

        if (qobject_cast<FluentMenu *>(sub)) {
            continue;
        }

        auto *fluentSub = new FluentMenu(sub->title(), this);
        fluentSub->setIcon(sub->icon());

        const QList<QAction *> subActions = sub->actions();
        for (QAction *sa : subActions) {
            if (!sa) {
                continue;
            }
            sub->removeAction(sa);
            fluentSub->addAction(sa);
        }

        a->setMenu(fluentSub);
        sub->deleteLater();
        fluentSub->ensureSubMenusFluent();
    }
}

void FluentMenu::applyTheme()
{
    const auto &colors = ThemeManager::instance().colors();
    if (isVisible()) {
        m_border.onThemeChanged();
    } else {
        m_border.syncFromTheme();
    }

    const QString next = QStringLiteral(
        "QMenu { background: transparent; border: none; padding: 4px; }"
        "QMenu::item {"
        "  padding: 5px 20px 5px 32px;"
        "  min-height: 22px;"
        "  border-radius: 4px;"
        "  background: transparent;"
        "  color: %1;"
        "}"
        "QMenu::item:disabled { color: %2; } "
        "QMenu::item:selected { background: transparent; }"
        "QMenu::item:selected:disabled { color: %2; }"
        "QMenu::separator { height: 1px; background: %3; margin: 6px 10px; }"
        "QMenu::indicator { width: 0px; height: 0px; }"
        "QMenu::right-arrow { width: 0px; height: 0px; image: none; }")
            .arg(colors.text.name(QColor::HexArgb),
                 colors.disabledText.name(QColor::HexArgb),
                 colors.border.name(QColor::HexArgb));

    if (styleSheet() != next) {
        setStyleSheet(next);
    }

    update();
}

void FluentMenu::mouseMoveEvent(QMouseEvent *event)
{
    QAction *action = actionAt(event->pos());
    if (action != m_hoverAction) {
        m_hoverAction = action;
        setActiveAction(action);
        updateHighlightForAction(action, true);
        startHoverAnimation(action ? 1.0 : 0.0);
    }
    QMenu::mouseMoveEvent(event);
}

void FluentMenu::leaveEvent(QEvent *event)
{
    startHoverAnimation(0.0);
    m_hoverAction = nullptr;
    setActiveAction(nullptr);
    updateHighlightForAction(nullptr, true);
    QMenu::leaveEvent(event);
}

void FluentMenu::showEvent(QShowEvent *event)
{
    QMenu::showEvent(event);

    updateWindowMask();

#ifdef Q_OS_WIN
    applyWindowsMenuPopupAttributes(this);
#endif

    updateHighlightForAction(activeAction(), false);
    m_border.playInitialTraceOnce(0);
    startPopupAnimation();
}

void FluentMenu::hideEvent(QHideEvent *event)
{
    QMenu::hideEvent(event);
    if (m_popupFadeAnim) {
        m_popupFadeAnim->stop();
        m_popupFadeAnim->deleteLater();
        m_popupFadeAnim = nullptr;
    }
    if (m_popupSlideAnim) {
        m_popupSlideAnim->stop();
        m_popupSlideAnim->deleteLater();
        m_popupSlideAnim = nullptr;
    }
    setWindowOpacity(1.0);
    m_border.resetInitial();
}

void FluentMenu::resizeEvent(QResizeEvent *event)
{
    QMenu::resizeEvent(event);
    updateWindowMask();
    updateHighlightForAction(m_hoverAction ? m_hoverAction : activeAction(), false);
}

void FluentMenu::startPopupAnimation()
{
    const QPoint finalPos = pos();
    const int slideOffsetY = popupOpenSlideOffsetY(this, finalPos, height());

    if (m_popupFadeAnim) {
        m_popupFadeAnim->stop();
        m_popupFadeAnim->deleteLater();
        m_popupFadeAnim = nullptr;
    }
    if (m_popupSlideAnim) {
        m_popupSlideAnim->stop();
        m_popupSlideAnim->deleteLater();
        m_popupSlideAnim = nullptr;
    }

    setWindowOpacity(0.0);
    move(finalPos + QPoint(0, slideOffsetY));

    m_popupFadeAnim = new QVariantAnimation(this);
    FluentMotion::configure(m_popupFadeAnim, FluentMotionRole::PopupOpen);
    m_popupFadeAnim->setStartValue(0.0);
    m_popupFadeAnim->setEndValue(1.0);
    connect(m_popupFadeAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        setWindowOpacity(v.toReal());
    });
    connect(m_popupFadeAnim, &QVariantAnimation::finished, this, [this]() {
        if (m_popupFadeAnim) {
            m_popupFadeAnim->deleteLater();
            m_popupFadeAnim = nullptr;
        }
    });

    m_popupSlideAnim = new QVariantAnimation(this);
    FluentMotion::configure(m_popupSlideAnim, FluentMotionRole::PopupOpen);
    m_popupSlideAnim->setStartValue(finalPos + QPoint(0, slideOffsetY));
    m_popupSlideAnim->setEndValue(finalPos);
    connect(m_popupSlideAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        move(v.toPoint());
    });
    connect(m_popupSlideAnim, &QVariantAnimation::finished, this, [this]() {
        if (m_popupSlideAnim) {
            m_popupSlideAnim->deleteLater();
            m_popupSlideAnim = nullptr;
        }
    });

    if (m_popupFadeAnim->duration() <= 0 || m_popupSlideAnim->duration() <= 0) {
        setWindowOpacity(1.0);
        move(finalPos);
        m_popupFadeAnim->deleteLater();
        m_popupFadeAnim = nullptr;
        m_popupSlideAnim->deleteLater();
        m_popupSlideAnim = nullptr;
        return;
    }

    m_popupFadeAnim->start();
    m_popupSlideAnim->start();
}

QRect FluentMenu::highlightTargetRect(QAction *action) const
{
    if (!action || action->isSeparator() || !actionGeometry(action).isValid()) {
        return QRect();
    }
    return actionGeometry(action).adjusted(2, 2, -2, -2);
}

void FluentMenu::updateHighlightForAction(QAction *action, bool animate)
{
    const QRect target = highlightTargetRect(action);
    m_highlightAction = action;

    if (!animate || !m_highlightAnim) {
        m_highlightRect = target;
        update();
        return;
    }

    m_highlightAnim->stop();
    m_highlightAnim->setStartValue(m_highlightRect);
    m_highlightAnim->setEndValue(target);
    m_highlightAnim->start();
}

void FluentMenu::updateWindowMask()
{
    if (!isVisible() && !rect().isValid()) {
        return;
    }

    const QRegion region = roundedPopupRegion(rect());
    if (region.isEmpty()) {
        clearMask();
        return;
    }

    setMask(region);
}

void FluentMenu::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const auto &colors = ThemeManager::instance().colors();
    const bool dark = colors.background.lightnessF() < 0.5;

    constexpr int kItemInsetX = 4;
    constexpr int kItemRadius = 4;
    constexpr int kIconSize = 16;
    constexpr int kIconLeft = 10;
    constexpr int kTextLeft = 32;
    constexpr int kTextRight = 18;
    constexpr int kShortcutGap = 12;

    QColor sep = colors.border;
    sep.setAlpha(dark ? 140 : 90);

    // Clear to transparent first.
    {
        QPainter clear(this);
        if (!clear.isActive()) {
            return;
        }
        clear.setCompositionMode(QPainter::CompositionMode_Source);
        clear.fillRect(rect(), Qt::transparent);
    }

    const QPainterPath outerClip = PopupSurface::contentClipPath(rect());

    QPainter panelPainter(this);
    if (!panelPainter.isActive()) {
        return;
    }
    panelPainter.setRenderHint(QPainter::Antialiasing, true);
    PopupSurface::paintPanel(panelPainter, rect(), colors, &m_border);

    if (!m_highlightRect.isNull()) {
        QColor fill = colors.hover;
        const qreal level = qBound<qreal>(0.0, m_hoverLevel, 1.0);
        fill.setAlpha(qBound(0, static_cast<int>(std::lround(110.0 * level)), 110));

        QPainter itemPainter(this);
        if (!itemPainter.isActive()) {
            return;
        }
        itemPainter.setRenderHint(QPainter::Antialiasing, true);
        itemPainter.setClipPath(outerClip);
        itemPainter.setPen(Qt::NoPen);
        itemPainter.setBrush(fill);

        QRect r = m_highlightRect.adjusted(kItemInsetX, 2, -kItemInsetX, -2);
        itemPainter.drawRoundedRect(r, kItemRadius, kItemRadius);

        QColor indicator = colors.accent;
        indicator.setAlpha(qBound(0, static_cast<int>(std::lround(255.0 * level)), 255));
        itemPainter.setBrush(indicator);
        const QRectF indicatorRect(r.left(), r.center().y() - 8.0, 3.0, 16.0);
        itemPainter.drawRoundedRect(indicatorRect, 1.5, 1.5);
    }

    QPainter glyph(this);
    if (!glyph.isActive()) {
        return;
    }
    glyph.setRenderHint(QPainter::Antialiasing, true);
    glyph.setClipPath(outerClip);
    glyph.setFont(font());

    for (QAction *a : actions()) {
        if (!a || !a->isVisible()) {
            continue;
        }

        const QRect ar = actionGeometry(a);
        if (!ar.isValid()) {
            continue;
        }

        if (a->isSeparator()) {
            glyph.setPen(QPen(sep, 1.0));
            glyph.drawLine(QPointF(ar.left() + 10.0, ar.center().y() + 0.5),
                           QPointF(ar.right() - 10.0, ar.center().y() + 0.5));
            continue;
        }

        const bool enabled = a->isEnabled();
        QString text = a->text();
        QString shortcut;

        const int tabPos = text.indexOf(QLatin1Char('\t'));
        if (tabPos >= 0) {
            shortcut = text.mid(tabPos + 1);
            text = text.left(tabPos);
        } else if (!a->shortcut().isEmpty()) {
            shortcut = a->shortcut().toString(QKeySequence::NativeText);
        }

        const QRect textRect = ar.adjusted(kTextLeft, 0, -kTextRight, 0);
        const QFontMetrics fm(glyph.font());
        const int shortcutWidth = shortcut.isEmpty() ? 0 : fm.horizontalAdvance(shortcut);
        const QRect labelRect = shortcut.isEmpty()
            ? textRect
            : QRect(textRect.left(), textRect.top(), qMax(0, textRect.width() - shortcutWidth - kShortcutGap), textRect.height());
        const QRect shortcutRect = shortcut.isEmpty()
            ? QRect()
            : QRect(textRect.right() - shortcutWidth, textRect.top(), shortcutWidth, textRect.height());

        if (!a->icon().isNull()) {
            const QRect iconRect(ar.left() + kIconLeft,
                                 ar.center().y() - kIconSize / 2,
                                 kIconSize,
                                 kIconSize);
            a->icon().paint(&glyph, iconRect, Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
        }

        glyph.setPen(enabled ? colors.text : colors.disabledText);
        glyph.drawText(labelRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextShowMnemonic, text);

        if (!shortcut.isEmpty()) {
            glyph.setPen(enabled ? colors.subText : colors.disabledText);
            glyph.drawText(shortcutRect, Qt::AlignVCenter | Qt::AlignRight, shortcut);
        }

        if (a->isCheckable() && a->isChecked()) {
            glyph.setPen(QPen(colors.accent, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            const QPointF center(ar.left() + 16.0, ar.center().y());
            glyph.drawLine(center + QPointF(-4.0, 0.5), center + QPointF(-1.2, 3.3));
            glyph.drawLine(center + QPointF(-1.2, 3.3), center + QPointF(5.2, -3.0));
        }

        if (a->menu()) {
            const QColor arrow = a->isEnabled() ? colors.subText : colors.disabledText;
            const QPointF center(ar.right() - 14.0, ar.center().y());
            Style::drawChevronRight(glyph, center, arrow, 7.5, 1.6);
        }
    }
}

void FluentMenu::startHoverAnimation(qreal endValue)
{
    if (!m_hoverAnim) {
        return;
    }
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

} // namespace Fluent
