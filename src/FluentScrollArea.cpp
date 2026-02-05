#include "Fluent/FluentScrollArea.h"

#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentWidget.h"

#include <QEvent>
#include <QLayout>
#include <QSignalBlocker>
#include <QTimer>

namespace Fluent {

FluentScrollArea::FluentScrollArea(QWidget *parent)
    : QScrollArea(parent)
{
    // Use a Fluent viewport to avoid palette-based background fills that don't follow our theme.
    // In particular, QAbstractScrollArea's default viewport may show a fixed light gray (#F0F0F0)
    // depending on platform style.
    auto *vp = new FluentWidget();
    vp->setBackgroundRole(FluentWidget::BackgroundRole::Transparent);
    setViewport(vp);

    // Prevent style/palette from painting a fixed viewport background (e.g. #F0F0F0 on Windows).
    if (viewport()) {
        viewport()->setBackgroundRole(QPalette::NoRole);
        viewport()->setAutoFillBackground(false);
        viewport()->setAttribute(Qt::WA_NoSystemBackground, true);
        viewport()->setStyleSheet(QStringLiteral("background: transparent;"));
    }
    setBackgroundRole(QPalette::NoRole);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setStyleSheet(QStringLiteral("background: transparent;"));

    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);

    ensureScrollBars();
    ensureOverlayScrollBars();
    syncOverlayScrollBars();

    if (viewport()) {
        viewport()->setMouseTracking(true);
        viewport()->installEventFilter(this);
    }

    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(700);
    connect(m_hideTimer, &QTimer::timeout, this, [this]() {
        revealScrollBars(false);
    });

    revealScrollBars(false);
}

QWidget *FluentScrollArea::contentWidget() const
{
    return widget();
}

QLayout *FluentScrollArea::contentLayout() const
{
    auto *w = widget();
    return w ? w->layout() : nullptr;
}

void FluentScrollArea::setContentLayout(QLayout *layout)
{
    if (!layout) {
        return;
    }

    QWidget *content = widget();
    if (!content) {
        auto *w = new FluentWidget();
        w->setBackgroundRole(FluentWidget::BackgroundRole::Transparent);
        setWidget(w);
        content = w;
    }

    if (content->layout() == layout) {
        return;
    }

    if (content->layout() && content->layout() != layout) {
        delete content->layout();
    }

    if (layout->parent() != content) {
        layout->setParent(content);
    }

    content->setLayout(layout);
}

void FluentScrollArea::setOverlayScrollBarsEnabled(bool enabled)
{
    m_overlayEnabled = enabled;
    ensureScrollBars();
    ensureOverlayScrollBars();
    syncOverlayScrollBars();

    if (m_overlayEnabled) {
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    } else {
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }

    if (m_vbar) m_vbar->setOverlayMode(false);
    if (m_hbar) m_hbar->setOverlayMode(false);
    if (m_vbarOverlay) m_vbarOverlay->setOverlayMode(true);
    if (m_hbarOverlay) m_hbarOverlay->setOverlayMode(true);

    updateOverlayGeometry();
    revealScrollBars(!m_overlayEnabled);
}

bool FluentScrollArea::overlayScrollBarsEnabled() const
{
    return m_overlayEnabled;
}

void FluentScrollArea::setScrollBarsRevealed(bool revealed)
{
    if (m_hideTimer) {
        m_hideTimer->stop();
    }
    revealScrollBars(revealed);
}

bool FluentScrollArea::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == viewport()) {
        if (event->type() == QEvent::Enter) {
            revealScrollBars(true);
            if (m_hideTimer) {
                m_hideTimer->stop();
            }
        } else if (event->type() == QEvent::Leave) {
            if (m_hideTimer) {
                m_hideTimer->start();
            }
        } else if (event->type() == QEvent::Wheel) {
            revealScrollBars(true);
            if (m_hideTimer) {
                m_hideTimer->start();
            }
        } else if (event->type() == QEvent::Resize) {
            updateOverlayGeometry();
        }
    }

    return QScrollArea::eventFilter(watched, event);
}

void FluentScrollArea::ensureScrollBars()
{
    if (!m_vbar) {
        m_vbar = new FluentScrollBar(Qt::Vertical, this);
        setVerticalScrollBar(m_vbar);
    }
    if (!m_hbar) {
        m_hbar = new FluentScrollBar(Qt::Horizontal, this);
        setHorizontalScrollBar(m_hbar);
    }

    // Internal scrollbars are the real scrolling mechanics. In overlay mode we hide them and draw
    // separate overlay bars on top of the viewport.
    m_vbar->setOverlayMode(false);
    m_hbar->setOverlayMode(false);
}

void FluentScrollArea::ensureOverlayScrollBars()
{
    if (!viewport()) {
        return;
    }

    if (!m_vbarOverlay) {
        m_vbarOverlay = new FluentScrollBar(Qt::Vertical, viewport());
        m_vbarOverlay->setOverlayMode(true);
        m_vbarOverlay->hide();
    }
    if (!m_hbarOverlay) {
        m_hbarOverlay = new FluentScrollBar(Qt::Horizontal, viewport());
        m_hbarOverlay->setOverlayMode(true);
        m_hbarOverlay->hide();
    }
}

void FluentScrollArea::syncOverlayScrollBars()
{
    ensureScrollBars();
    ensureOverlayScrollBars();
    if (!m_vbar || !m_hbar || !m_vbarOverlay || !m_hbarOverlay) {
        return;
    }

    if (!m_overlayWired) {
        m_overlayWired = true;

        // Sync internal -> overlay
        QObject::connect(m_vbar, &QScrollBar::rangeChanged, m_vbarOverlay, [this](int min, int max) {
            QSignalBlocker b(m_vbarOverlay);
            m_vbarOverlay->setRange(min, max);
            m_vbarOverlay->setPageStep(m_vbar->pageStep());
            m_vbarOverlay->setSingleStep(m_vbar->singleStep());
            updateOverlayGeometry();
        });
        QObject::connect(m_hbar, &QScrollBar::rangeChanged, m_hbarOverlay, [this](int min, int max) {
            QSignalBlocker b(m_hbarOverlay);
            m_hbarOverlay->setRange(min, max);
            m_hbarOverlay->setPageStep(m_hbar->pageStep());
            m_hbarOverlay->setSingleStep(m_hbar->singleStep());
            updateOverlayGeometry();
        });

        QObject::connect(m_vbar, &QScrollBar::valueChanged, m_vbarOverlay, [this](int v) {
            QSignalBlocker b(m_vbarOverlay);
            m_vbarOverlay->setValue(v);
        });
        QObject::connect(m_hbar, &QScrollBar::valueChanged, m_hbarOverlay, [this](int v) {
            QSignalBlocker b(m_hbarOverlay);
            m_hbarOverlay->setValue(v);
        });

        // Sync overlay -> internal (user drags overlay thumb)
        // IMPORTANT: do NOT block internal scrollbar signals; QAbstractScrollArea relies on them.
        QObject::connect(m_vbarOverlay, &QScrollBar::valueChanged, m_vbar, [this](int v) {
            if (m_vbar->value() != v) {
                m_vbar->setValue(v);
            }
        });
        QObject::connect(m_hbarOverlay, &QScrollBar::valueChanged, m_hbar, [this](int v) {
            if (m_hbar->value() != v) {
                m_hbar->setValue(v);
            }
        });
    }
    // Initial sync
    {
        QSignalBlocker bv(m_vbarOverlay);
        m_vbarOverlay->setRange(m_vbar->minimum(), m_vbar->maximum());
        m_vbarOverlay->setPageStep(m_vbar->pageStep());
        m_vbarOverlay->setSingleStep(m_vbar->singleStep());
        m_vbarOverlay->setValue(m_vbar->value());
    }
    {
        QSignalBlocker bh(m_hbarOverlay);
        m_hbarOverlay->setRange(m_hbar->minimum(), m_hbar->maximum());
        m_hbarOverlay->setPageStep(m_hbar->pageStep());
        m_hbarOverlay->setSingleStep(m_hbar->singleStep());
        m_hbarOverlay->setValue(m_hbar->value());
    }
}

void FluentScrollArea::updateOverlayGeometry()
{
    if (!m_overlayEnabled || !viewport()) {
        if (m_vbarOverlay) m_vbarOverlay->hide();
        if (m_hbarOverlay) m_hbarOverlay->hide();
        return;
    }

    ensureOverlayScrollBars();
    if (!m_vbarOverlay || !m_hbarOverlay) {
        return;
    }

    const QRect vp = viewport()->rect();
    const int thickness = 10;
    const int margin = 2;

    const bool vNeeded = m_vbar && (m_vbar->minimum() < m_vbar->maximum());
    const bool hNeeded = m_hbar && (m_hbar->minimum() < m_hbar->maximum());

    if (vNeeded) {
        const int x = vp.right() - thickness - margin + 1;
        const int y = vp.top() + margin;
        const int h = vp.height() - margin * 2 - (hNeeded ? (thickness + margin) : 0);
        m_vbarOverlay->setGeometry(x, y, thickness, qMax(0, h));
        m_vbarOverlay->show();
        m_vbarOverlay->raise();
    } else {
        m_vbarOverlay->hide();
    }

    if (hNeeded) {
        const int x = vp.left() + margin;
        const int y = vp.bottom() - thickness - margin + 1;
        const int w = vp.width() - margin * 2 - (vNeeded ? (thickness + margin) : 0);
        m_hbarOverlay->setGeometry(x, y, qMax(0, w), thickness);
        m_hbarOverlay->show();
        m_hbarOverlay->raise();
    } else {
        m_hbarOverlay->hide();
    }
}

void FluentScrollArea::revealScrollBars(bool reveal)
{
    ensureScrollBars();

    if (m_overlayEnabled) {
        ensureOverlayScrollBars();
        syncOverlayScrollBars();
        updateOverlayGeometry();
        const bool force = reveal;
        if (m_vbarOverlay) m_vbarOverlay->setForceVisible(force);
        if (m_hbarOverlay) m_hbarOverlay->setForceVisible(force);
        return;
    }

    const bool force = reveal || !m_overlayEnabled;
    if (m_vbar) {
        m_vbar->setForceVisible(force);
    }
    if (m_hbar) {
        m_hbar->setForceVisible(force);
    }
}

} // namespace Fluent
