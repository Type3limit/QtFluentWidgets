#include "Fluent/FluentMenu.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QAction>
#include <QHideEvent>
#include <QEasingCurve>
#include <QMouseEvent>
#include <QPainter>
#include <QProxyStyle>
#include <QShowEvent>
#include <QStyle>
#include <QVariantAnimation>

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
};

static QString rgbaString2(const QColor &c)
{
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
        .arg(c.alpha());
}

} // namespace

FluentMenu::FluentMenu(QWidget *parent)
    : QMenu(parent)
    , m_border(this)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlag(Qt::FramelessWindowHint, true);
    setWindowFlag(Qt::NoDropShadowWindowHint, true);
    setAttribute(Qt::WA_StyledBackground, true);

    setStyle(new FluentMenuProxyStyle2(style()));

    connect(this, &QMenu::aboutToShow, this, [this]() { ensureSubMenusFluent(); });
    connect(this, &QMenu::hovered, this, [this](QAction *a) {
        if (a == m_hoverAction) {
            return;
        }
        m_hoverAction = a;
        updateHighlightForAction(a, true);
        startHoverAnimation(a ? 1.0 : 0.0);
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentMenu::applyTheme);
    m_border.syncFromTheme();

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(120);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_highlightAnim = new QVariantAnimation(this);
    m_highlightAnim->setDuration(160);
    m_highlightAnim->setEasingCurve(QEasingCurve::OutCubic);
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
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlag(Qt::FramelessWindowHint, true);
    setWindowFlag(Qt::NoDropShadowWindowHint, true);
    setAttribute(Qt::WA_StyledBackground, true);

    setStyle(new FluentMenuProxyStyle2(style()));

    connect(this, &QMenu::aboutToShow, this, [this]() { ensureSubMenusFluent(); });
    connect(this, &QMenu::hovered, this, [this](QAction *a) {
        if (a == m_hoverAction) {
            return;
        }
        m_hoverAction = a;
        updateHighlightForAction(a, true);
        startHoverAnimation(a ? 1.0 : 0.0);
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentMenu::applyTheme);
    m_border.syncFromTheme();

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(120);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_highlightAnim = new QVariantAnimation(this);
    m_highlightAnim->setDuration(160);
    m_highlightAnim->setEasingCurve(QEasingCurve::OutCubic);
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
    if (isVisible()) {
        m_border.onThemeChanged();
    } else {
        m_border.syncFromTheme();
    }

    const auto &colors = ThemeManager::instance().colors();
    const bool dark = colors.background.lightnessF() < 0.5;

    QColor sep = colors.border;
    sep.setAlpha(dark ? 140 : 90);

    QColor disabled = colors.disabledText;
    disabled.setAlpha(dark ? 180 : 160);

    const QString next = QString(
        "QMenu { background: transparent; border: none; padding: 6px; }"
        "QMenu::item {"
        "  padding: 7px 34px 7px 40px;"
        "  min-height: 26px;"
        "  border-radius: 6px;"
        "  background: transparent;"
        "  color: %1;"
        "}"
        "QMenu::item:disabled { color: %2; } "
        "QMenu::item:selected { background: transparent; }"
        "QMenu::item:selected:disabled { color: %2; }"
        "QMenu::separator { height: 1px; background: %3; margin: 6px 10px; }"
        "QMenu::indicator { width: 0px; height: 0px; }"
        "QMenu::right-arrow { width: 0px; height: 0px; image: none; }")
        .arg(colors.text.name())
        .arg(rgbaString2(disabled))
        .arg(rgbaString2(sep));

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
        updateHighlightForAction(action, true);
        startHoverAnimation(action ? 1.0 : 0.0);
    }
    QMenu::mouseMoveEvent(event);
}

void FluentMenu::leaveEvent(QEvent *event)
{
    startHoverAnimation(0.0);
    m_hoverAction = nullptr;
    updateHighlightForAction(nullptr, true);
    QMenu::leaveEvent(event);
}

void FluentMenu::showEvent(QShowEvent *event)
{
    QMenu::showEvent(event);

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
}

void FluentMenu::startPopupAnimation()
{
    const QPoint finalPos = pos();

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
    move(finalPos + QPoint(0, 6));

    m_popupFadeAnim = new QVariantAnimation(this);
    m_popupFadeAnim->setDuration(140);
    m_popupFadeAnim->setEasingCurve(QEasingCurve::OutCubic);
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
    m_popupSlideAnim->setDuration(140);
    m_popupSlideAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_popupSlideAnim->setStartValue(finalPos + QPoint(0, 6));
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

void FluentMenu::paintEvent(QPaintEvent *event)
{
    const auto &colors = ThemeManager::instance().colors();

    constexpr int kItemInsetX = 2;
    constexpr int kItemRadius = 6;
    constexpr qreal kPanelRadius = 10.0;

    // Clear to transparent first.
    {
        QPainter clear(this);
        if (!clear.isActive()) {
            return;
        }
        clear.setCompositionMode(QPainter::CompositionMode_Source);
        clear.fillRect(rect(), Qt::transparent);
    }

    const QRectF panelRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const QPainterPath outerClip = Style::roundedRectPath(panelRect, kPanelRadius);

    QPainter panelPainter(this);
    if (!panelPainter.isActive()) {
        return;
    }
    panelPainter.setRenderHint(QPainter::Antialiasing, true);
    panelPainter.setClipPath(outerClip);

    FluentFrameSpec frame;
    frame.radius = kPanelRadius;
    frame.maximized = false;
    frame.clearToTransparent = false;
    m_border.applyToFrameSpec(frame, colors);
    paintFluentPanel(panelPainter, panelRect, colors, frame);

    if (!m_highlightRect.isNull()) {
        QColor fill = colors.hover;
        const qreal level = qBound<qreal>(0.0, m_hoverLevel, 1.0);
        int alpha = static_cast<int>(80 + 60 * level);
        fill.setAlpha(qBound(0, alpha, 200));

        QPainter itemPainter(this);
        if (!itemPainter.isActive()) {
            return;
        }
        itemPainter.setRenderHint(QPainter::Antialiasing, true);
        itemPainter.setClipPath(outerClip);
        itemPainter.setPen(Qt::NoPen);
        itemPainter.setBrush(fill);

        QRect r = m_highlightRect.adjusted(kItemInsetX, 0, -kItemInsetX, 0);
        itemPainter.drawRoundedRect(r, kItemRadius, kItemRadius);
    }

    QMenu::paintEvent(event);

    // Custom glyphs (checkmark + submenu chevron).
    QPainter glyph(this);
    if (!glyph.isActive()) {
        return;
    }
    glyph.setRenderHint(QPainter::Antialiasing, true);
    glyph.setClipPath(outerClip);

    for (QAction *a : actions()) {
        if (!a) {
            continue;
        }
        const QRect ar = actionGeometry(a);
        if (!ar.isValid()) {
            continue;
        }

        if (a->isCheckable() && a->isChecked()) {
            glyph.setPen(QPen(colors.accent, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            const QPointF center(ar.left() + 18.0, ar.center().y());
            glyph.drawLine(center + QPointF(-4.0, 0.5), center + QPointF(-1.2, 3.3));
            glyph.drawLine(center + QPointF(-1.2, 3.3), center + QPointF(5.2, -3.0));
        }

        if (a->menu()) {
            const QColor arrow = a->isEnabled() ? colors.subText : colors.disabledText;
            const QPointF center(ar.right() - 18.0, ar.center().y());
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
