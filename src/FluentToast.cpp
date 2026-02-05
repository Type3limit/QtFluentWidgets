#include "Fluent/FluentToast.h"

#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QApplication>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVariantAnimation>
#include <QVector>
#include <QVBoxLayout>

namespace Fluent {

namespace {
constexpr int kToastWidth = 320;
constexpr int kToastRadius = 10;
constexpr int kToastMargin = 16;
constexpr int kToastSpacing = 10;

constexpr int kToastMinHeight = 64;

static int positionIndex(FluentToast::Position p)
{
    switch (p) {
    case FluentToast::Position::TopLeft:
        return 0;
    case FluentToast::Position::TopCenter:
        return 1;
    case FluentToast::Position::TopRight:
        return 2;
    case FluentToast::Position::BottomLeft:
        return 3;
    case FluentToast::Position::BottomCenter:
        return 4;
    case FluentToast::Position::BottomRight:
        return 5;
    }
    return 5;
}

static bool isTopPosition(FluentToast::Position p)
{
    return p == FluentToast::Position::TopLeft || p == FluentToast::Position::TopCenter || p == FluentToast::Position::TopRight;
}

static QPoint enterOffsetFor(FluentToast::Position p)
{
    // Different positions, different entrance directions.
    switch (p) {
    case FluentToast::Position::TopLeft:
        return QPoint(-20, -12);
    case FluentToast::Position::TopCenter:
        return QPoint(0, -14);
    case FluentToast::Position::TopRight:
        return QPoint(20, -12);
    case FluentToast::Position::BottomLeft:
        return QPoint(-20, 12);
    case FluentToast::Position::BottomCenter:
        return QPoint(0, 14);
    case FluentToast::Position::BottomRight:
        return QPoint(20, 12);
    }
    return QPoint(0, 12);
}

static int moveDurationFor(FluentToast::Position p)
{
    switch (p) {
    case FluentToast::Position::TopCenter:
    case FluentToast::Position::BottomCenter:
        return 180;
    case FluentToast::Position::TopLeft:
    case FluentToast::Position::TopRight:
        return 170;
    case FluentToast::Position::BottomLeft:
    case FluentToast::Position::BottomRight:
        return 150;
    }
    return 160;
}

class ToastOverlay final : public QWidget
{
public:
    explicit ToastOverlay(QWidget *window)
        : QWidget(window)
        , m_window(window)
    {
        setObjectName("FluentToastOverlay");
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_StyledBackground, false);
        // Use a dynamic mask instead of full transparent mouse, so toasts remain clickable
        // while the rest of the window still receives input.

        if (m_window) {
            m_window->installEventFilter(this);
            setGeometry(m_window->rect());
        }

        m_toastsByPos.resize(6);

        connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
            for (const auto &list : m_toastsByPos) {
                for (auto t : list) {
                    if (t) {
                        // triggers stylesheet refresh
                        QMetaObject::invokeMethod(t, [t]() { t->update(); }, Qt::QueuedConnection);
                    }
                }
            }
        });
    }

    void addToast(FluentToast *toast, FluentToast::Position position)
    {
        if (!toast) {
            return;
        }

        toast->setParent(this);
        stabilizeToastSize(toast);

        const int idx = positionIndex(position);
        m_toastsByPos[idx].prepend(toast);

        // For the newest toast, we anchor it directly (it does not depend on other toasts)
        // and apply an entrance offset to differentiate animations per position.
        const QPoint targetPos = anchorPos(position, qMax(toast->height(), kToastMinHeight));
        toast->move(targetPos + enterOffsetFor(position));
        toast->show();

        connect(toast, &QObject::destroyed, this, [this, toast]() {
            if (auto anim = m_moveAnims.take(toast)) {
                anim->stop();
                anim->deleteLater();
            }
        });

        connect(toast, &FluentToast::dismissed, this, [this, toast]() {
            for (auto &list : m_toastsByPos) {
                list.removeAll(toast);
            }
            layoutToasts(true);
        });

        layoutToasts(true);
        queueLayoutToasts();
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_window) {
            switch (event->type()) {
            case QEvent::Resize:
            case QEvent::Move:
            case QEvent::Show:
            case QEvent::WindowStateChange:
                setGeometry(m_window->rect());
                layoutToasts(true);
                break;
            default:
                break;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        layoutToasts(true);
    }

private:
    QPoint anchorPos(FluentToast::Position position, int toastHeight) const
    {
        int x = 0;
        switch (position) {
        case FluentToast::Position::TopLeft:
        case FluentToast::Position::BottomLeft:
            x = kToastMargin;
            break;
        case FluentToast::Position::TopRight:
        case FluentToast::Position::BottomRight:
            x = width() - kToastMargin - kToastWidth;
            break;
        case FluentToast::Position::TopCenter:
        case FluentToast::Position::BottomCenter:
            x = (width() - kToastWidth) / 2;
            break;
        }

        x = qMax(kToastMargin, x);

        const int y = isTopPosition(position) ? kToastMargin : (height() - kToastMargin - toastHeight);
        return QPoint(x, y);
    }

    void stabilizeToastSize(FluentToast *toast)
    {
        if (!toast) {
            return;
        }

        // Ensure stylesheet/font polish is applied before we measure.
        // When toasts are spawned rapidly, QLabel(wordWrap) can compute different heights
        // before/after polish + geometry assignment, which causes a visible "extra line" jump.
        toast->ensurePolished();

        // Fix wrap width for labels before we ask Qt to compute sizes.
        if (auto *lay = toast->layout()) {
            const QMargins m = lay->contentsMargins();
            const int textW = qMax(120, kToastWidth - m.left() - m.right());
            if (auto *title = toast->findChild<QLabel *>(QStringLiteral("FluentToastTitle"))) {
                title->setFixedWidth(textW);
                title->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            }
            if (auto *msg = toast->findChild<QLabel *>(QStringLiteral("FluentToastMessage"))) {
                msg->setFixedWidth(textW);
                msg->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            }
            lay->invalidate();
            lay->activate();
        }

        toast->adjustSize();
        const int h = qMax(toast->height(), kToastMinHeight);
        toast->resize(kToastWidth, h);
        toast->setFixedHeight(h);
    }

    void queueLayoutToasts()
    {
        if (m_layoutQueued) {
            return;
        }
        m_layoutQueued = true;
        QTimer::singleShot(0, this, [this]() {
            m_layoutQueued = false;
            // After the event loop runs, polish/stylesheet/font propagation is settled.
            // Re-stabilize sizes and relayout once to eliminate occasional extra-line jumps.
            for (auto &list : m_toastsByPos) {
                for (auto t : list) {
                    if (t) {
                        stabilizeToastSize(t);
                    }
                }
            }
            layoutToasts(true);
        });
    }

    void layoutToasts(bool animated)
    {
        bool anyToast = false;
        for (auto &list : m_toastsByPos) {
            list.removeAll(nullptr);
            anyToast = anyToast || !list.isEmpty();
        }

        if (!anyToast) {
            clearMask();
            // Do not block interaction when there are no toasts.
            setAttribute(Qt::WA_TransparentForMouseEvents, true);
            return;
        }

        // Toasts should be clickable, but the rest of the window should receive input.
        setAttribute(Qt::WA_TransparentForMouseEvents, false);

        QRegion region;

        for (int posIdx = 0; posIdx < m_toastsByPos.size(); ++posIdx) {
            auto &list = m_toastsByPos[posIdx];
            if (list.isEmpty()) {
                continue;
            }

            const auto position = static_cast<FluentToast::Position>(posIdx);
            const bool top = isTopPosition(position);

            int y = top ? kToastMargin : (height() - kToastMargin);
            const int x = anchorPos(position, kToastMinHeight).x();

            for (auto t : list) {
                if (!t) {
                    continue;
                }

                const int h = qMax(t->height(), kToastMinHeight);

                QPoint targetPos;
                if (top) {
                    targetPos = QPoint(x, y);
                    y += h + kToastSpacing;
                } else {
                    y -= h;
                    targetPos = QPoint(x, y);
                    y -= kToastSpacing;
                }

                if (!animated || t->pos() == targetPos) {
                    t->move(targetPos);
                } else {
                    // Cancel any previous queued move animation for this toast.
                    if (auto prev = m_moveAnims.value(t)) {
                        prev->stop();
                        prev->deleteLater();
                    }

                    auto *anim = new QPropertyAnimation(t, "pos", this);
                    anim->setDuration(moveDurationFor(position));
                    anim->setStartValue(t->pos());
                    anim->setEndValue(targetPos);
                    anim->setEasingCurve(QEasingCurve::OutCubic);
                    connect(anim, &QObject::destroyed, this, [this, t]() {
                        m_moveAnims.remove(t);
                    });
                    m_moveAnims.insert(t, anim);
                    anim->start(QAbstractAnimation::DeleteWhenStopped);
                }

                region |= QRegion(t->geometry().adjusted(-4, -4, 4, 4));
                region |= QRegion(QRect(targetPos, t->size()).adjusted(-4, -4, 4, 4));
            }
        }

        if (region.isEmpty()) {
            clearMask();
        } else {
            setMask(region);
        }

        // Ensure visible above central widgets.
        raise();
    }

    QPointer<QWidget> m_window;
    QVector<QList<QPointer<FluentToast>>> m_toastsByPos;
    QHash<FluentToast *, QPointer<QPropertyAnimation>> m_moveAnims;
    bool m_layoutQueued = false;
};

static ToastOverlay *ensureOverlay(QWidget *window)
{
    if (!window) {
        return nullptr;
    }

    // Avoid qobject_cast here: it requires Q_OBJECT on ToastOverlay.
    // We keep ToastOverlay as a lightweight internal widget.
    for (QObject *child : window->children()) {
        if (!child || child->objectName() != QLatin1String("FluentToastOverlay")) {
            continue;
        }
        if (auto *overlay = dynamic_cast<ToastOverlay *>(child)) {
            return overlay;
        }
    }

    auto *overlay = new ToastOverlay(window);
    overlay->setGeometry(window->rect());
    overlay->show();

    return overlay;
}

} // namespace

class FluentToast::ProgressBar final : public QWidget
{
public:
    explicit ProgressBar(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedHeight(3);
    }

    void setRatio(qreal ratio)
    {
        m_ratio = qBound<qreal>(0.0, ratio, 1.0);
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)
        const auto &c = ThemeManager::instance().colors();

        QPainter p(this);
        if (!p.isActive()) {
            return;
        }
        p.setRenderHint(QPainter::Antialiasing, true);

        const QRectF r = QRectF(rect()).adjusted(8, 0, -8, 0);
        if (r.width() <= 0) {
            return;
        }

        QColor track = c.border;
        track.setAlpha(80);

        QColor bar = c.accent;
        bar.setAlpha(200);

        p.setPen(Qt::NoPen);
        p.setBrush(track);
        p.drawRoundedRect(r, 1.5, 1.5);

        const qreal w = r.width() * m_ratio;
        const qreal cx = r.center().x();
        QRectF fill(cx - w / 2.0, r.top(), w, r.height());

        p.setBrush(bar);
        p.drawRoundedRect(fill, 1.5, 1.5);
    }

private:
    qreal m_ratio = 1.0;
};

FluentToast::FluentToast(const QString &title, const QString &message, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
    , m_message(message)
{
    setObjectName("FluentToast");
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, false);

    setFixedWidth(kToastWidth);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 12, 14, 10);
    layout->setSpacing(8);

    auto *titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("FluentToastTitle");
    titleLabel->setWordWrap(true);

    auto *msgLabel = new QLabel(message, this);
    msgLabel->setObjectName("FluentToastMessage");
    msgLabel->setWordWrap(true);

    layout->addWidget(titleLabel);
    layout->addWidget(msgLabel);

    m_progress = new ProgressBar(this);
    layout->addWidget(m_progress);

    applyTheme();

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentToast::applyTheme);

    m_border.syncFromTheme();
}

void FluentToast::showToast(QWidget *window, const QString &title, const QString &message, int durationMs)
{
    showToast(window, title, message, Position::BottomRight, durationMs);
}

void FluentToast::showToast(QWidget *window,
                           const QString &title,
                           const QString &message,
                           Position position,
                           int durationMs)
{
    if (!window) {
        return;
    }

    auto *overlay = ensureOverlay(window);
    if (!overlay) {
        return;
    }

    auto *toast = new FluentToast(title, message);

    overlay->addToast(toast, position);

    // Start animation after first layout.
    QTimer::singleShot(0, toast, [toast, durationMs]() {
        if (toast) {
            toast->start(durationMs);
        }
    });
}

void FluentToast::mousePressEvent(QMouseEvent *event)
{
    if (event && event->button() == Qt::LeftButton) {
        dismiss(true);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void FluentToast::applyTheme()
{
    const auto &c = ThemeManager::instance().colors();
    const bool dark = c.background.lightnessF() < 0.5;

    if (isVisible()) {
        m_border.onThemeChanged();
    } else {
        m_border.syncFromTheme();
    }

    QColor bg = c.surface;
    QColor text = c.text;

    QColor sub = c.subText;
    sub.setAlpha(dark ? 220 : 200);

    // Border is painted manually (trace animation). Keep QSS for text only.
    const QString next = QString(
        "#FluentToast { background: transparent; border: none; }"
        "#FluentToastTitle { color: %1; font-weight: 650; }"
        "#FluentToastMessage { color: %2; }")
        .arg(text.name())
        .arg(sub.name());
    if (styleSheet() != next) {
        setStyleSheet(next);
    }
}

void FluentToast::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &c = ThemeManager::instance().colors();
    const bool dark = c.background.lightnessF() < 0.5;

    QColor bg = c.surface;

    QColor accent = c.accent;
    accent.setAlpha(220);

    QColor normalBorder = c.border;
    normalBorder.setAlpha(dark ? 200 : 170);

    QPainter p(this);
    if (!p.isActive()) {
        return;
    }

    // Align strokes to device pixels to reduce jaggies on rounded corners.
    qreal dpr = devicePixelRatioF();
    if (dpr <= 0.0) {
        dpr = 1.0;
    }
    const qreal px = 0.5 / dpr;

    const QRectF r = QRectF(rect()).adjusted(px, px, -px, -px);

    FluentFrameSpec frame;
    frame.radius = kToastRadius;
    frame.maximized = false;
    frame.clearToTransparent = false;
    frame.surfaceOverride = bg;
    frame.borderWidth = 1.0;

    m_border.applyToFrameSpec(frame, c, normalBorder, accent);

    paintFluentPanel(p, r, c, frame);
}

void FluentToast::start(int durationMs)
{
    if (m_dismissing) {
        return;
    }

    // When the toast appears, trace the border in once.
    m_border.playInitialTraceOnce(0);

    durationMs = qMax(800, durationMs);

    auto *effect = new QGraphicsOpacityEffect(this);
    effect->setOpacity(0.0);
    setGraphicsEffect(effect);

    auto *opAnim = new QPropertyAnimation(effect, "opacity", this);
    opAnim->setDuration(180);
    opAnim->setStartValue(0.0);
    opAnim->setEndValue(1.0);
    opAnim->setEasingCurve(QEasingCurve::OutCubic);

    auto *group = new QParallelAnimationGroup(this);
    group->addAnimation(opAnim);
    group->start(QAbstractAnimation::DeleteWhenStopped);

    auto *progress = new QVariantAnimation(this);
    progress->setDuration(durationMs);
    progress->setStartValue(1.0);
    progress->setEndValue(0.0);
    progress->setEasingCurve(QEasingCurve::Linear);
    connect(progress, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        if (m_progress) {
            m_progress->setRatio(v.toReal());
        }
    });
    connect(progress, &QVariantAnimation::finished, this, [this]() {
        dismiss(true);
    });
    progress->start(QAbstractAnimation::DeleteWhenStopped);
}

void FluentToast::dismiss(bool animated)
{
    if (m_dismissing) {
        return;
    }
    m_dismissing = true;

    if (!animated) {
        emit dismissed();
        deleteLater();
        return;
    }

    auto *effect = qobject_cast<QGraphicsOpacityEffect *>(graphicsEffect());
    if (!effect) {
        effect = new QGraphicsOpacityEffect(this);
        effect->setOpacity(1.0);
        setGraphicsEffect(effect);
    }

    auto *opAnim = new QPropertyAnimation(effect, "opacity", this);
    opAnim->setDuration(160);
    opAnim->setStartValue(effect->opacity());
    opAnim->setEndValue(0.0);
    opAnim->setEasingCurve(QEasingCurve::InCubic);

    auto *group = new QParallelAnimationGroup(this);
    group->addAnimation(opAnim);

    connect(group, &QParallelAnimationGroup::finished, this, [this]() {
        emit dismissed();
        deleteLater();
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace Fluent
