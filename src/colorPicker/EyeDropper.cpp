#include "EyeDropper.h"

#if 0

#include <QApplication>
#include <QCursor>
#include <QDialog>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QScreen>
#include <QTimer>
#include <QWindow>

namespace Fluent::ColorPicker {

static QCursor pipetteCursor()
{
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.translate(4.0, 6.0);
    QPen pen(QColor(0, 0, 0, 190), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
#include <limits>
    p.setBrush(QColor(255, 255, 255, 230));

    QPainterPath path;
    path.moveTo(18, 2);
    path.lineTo(22, 6);
    path.lineTo(14, 14);
struct ScreenGrab {
    QPointer<QScreen> screen;
    QRect geomDip;
    QPixmap pix;
    QImage img;
    qreal dpr = 1.0;
};

class EyeDropperOverlay final : public QDialog
    path.closeSubpath();
    p.drawPath(path);

    p.drawLine(QPointF(10, 10), QPointF(3, 17));
    p.setPen(QPen(QColor(0, 0, 0, 170), 3.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPoint(QPointF(3, 17));

    p.setPen(QPen(QColor(255, 255, 255, 220), 1.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(QPointF(10, 10), QPointF(3, 17));

    return QCursor(pm, 1, 30);
}

static QRect screenGeometryDip(QScreen *screen)
{
    if (!screen) {
        return {};
    }
    // QScreen::geometry is in device-independent pixels (DIPs).
    return screen->geometry();
}

static QColor sampleAtGlobalDip(const QImage &img, qreal dpr, const QPoint &globalPosDip, const QRect &screenGeomDip)
{
    if (img.isNull()) {
        return QColor();
    }

    const QPoint localDip = globalPosDip - screenGeomDip.topLeft();
    const int x = qRound(localDip.x() * dpr);
    const int y = qRound(localDip.y() * dpr);
    if (x < 0 || y < 0 || x >= img.width() || y >= img.height()) {
        return QColor();
    }

    const QRgb px = img.pixel(x, y);
    return QColor::fromRgb(px);
}

class EyeDropperWindow final : public QDialog
{
    Q_OBJECT
public:
    explicit EyeDropperWindow(QScreen *screen)
        : QDialog(nullptr)
        , m_screen(screen)
    {
        setWindowFlag(Qt::FramelessWindowHint, true);
        setWindowFlag(Qt::Tool, true);
        setWindowFlag(Qt::WindowStaysOnTopHint, true);

        // Use an opaque painted surface (screen snapshot) so the window reliably receives input
        // on Windows (transparent layered windows can become click-through depending on alpha).
        setAttribute(Qt::WA_TranslucentBackground, false);
        setAttribute(Qt::WA_NoSystemBackground, false);
        // Need activation/focus so ESC and mouse clicks reliably reach us.
        setAttribute(Qt::WA_ShowWithoutActivating, false);

        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);

        // Capture current screen snapshot (designer reference does this once).
        refreshGrab();

        // Stretch to cover the screen.
        const QRect geom = screenGeometryDip(m_screen);
        setGeometry(geom);

        setCursor(pipetteCursor());

        // A tiny timer to keep hover updates even if some systems skip move events.
        m_poll = new QTimer(this);
        m_poll->setInterval(16);
        connect(m_poll, &QTimer::timeout, this, [this]() {
            if (!isVisible()) {
                return;
            }
            const QPoint gp = QCursor::pos();
            updateHover(gp);
        });
        m_poll->start();
    }

    ~EyeDropperWindow() override
    {
        releaseMouse();
        releaseKeyboard();
    }

signals:
    void hovered(const QColor &color);
    void picked(const QColor &color);
    void canceled();

public:
    bool containsGlobal(const QPoint &globalPos) const
    {
        return m_screen && screenGeometryDip(m_screen).contains(globalPos);
    }

    QColor sampleColorAtGlobal(const QPoint &globalPos) const
    {
        if (!m_screen || m_grabImg.isNull()) {
            return QColor();
        }
        const QRect sg = screenGeometryDip(m_screen);
        return sampleAtGlobalDip(m_grabImg, m_grabDpr, globalPos, sg);
    }

protected:
    void showEvent(QShowEvent *e) override
    {
        QDialog::showEvent(e);
        raise();
        activateWindow();
        setFocus(Qt::ActiveWindowFocusReason);
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        updateHover(e->globalPos());
        QDialog::mouseMoveEvent(e);
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton) {
            const QColor c = currentColor();
            if (c.isValid()) {
                emit picked(c);
            } else {
                emit canceled();
            }
            close();
            return;
        }

        if (e->button() == Qt::RightButton) {
            emit canceled();
            close();
            return;
        }

        QDialog::mousePressEvent(e);
    }

    void keyPressEvent(QKeyEvent *e) override
    {
        if (e->key() == Qt::Key_Escape) {
            emit canceled();
            close();
            return;
        }
        QDialog::keyPressEvent(e);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Paint captured screen as background (opaque, stable for hit testing).
        if (!m_grab.isNull()) {
            p.setRenderHint(QPainter::SmoothPixmapTransform, true);
            p.drawPixmap(rect(), m_grab);
        } else {
            p.fillRect(rect(), Qt::black);
        }

        if (!m_active) {
            return;
        }

        // Draw magnifier HUD near cursor (simple, Fluent-like)
        const QPoint gp = QCursor::pos();
        const QPoint local = mapFromGlobal(gp);

        const int radius = 48;
        const int border = 2;
        const int pad = 10;

        QPoint hudTopLeft = local + QPoint(24, 24);
        QRect hud(hudTopLeft, QSize(radius * 2 + pad * 2, radius * 2 + pad * 2 + 28));

        // keep within window
        hud.moveLeft(qBound(8, hud.left(), width() - hud.width() - 8));
        hud.moveTop(qBound(8, hud.top(), height() - hud.height() - 8));

        QPainterPath path;
        path.addRoundedRect(hud, 12, 12);

        p.fillPath(path, QColor(20, 20, 20, 200));
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        p.drawPath(path);

        // magnified pixels
        if (!m_grabImg.isNull()) {
            QRect magArea(hud.left() + pad, hud.top() + pad, radius * 2, radius * 2);

            const QRect sg = screenGeometryDip(m_screen);
            const QPoint localDip = gp - sg.topLeft();
            const int cx = qRound(localDip.x() * m_grabDpr);
            const int cy = qRound(localDip.y() * m_grabDpr);

            const int sampleRadiusPx = 8;
            QRect src(cx - sampleRadiusPx, cy - sampleRadiusPx, sampleRadiusPx * 2 + 1, sampleRadiusPx * 2 + 1);
            src = src.intersected(QRect(QPoint(0, 0), m_grabImg.size()));

            QImage img = m_grabImg.copy(src);
            if (!img.isNull()) {
                p.setRenderHint(QPainter::SmoothPixmapTransform, false);
                p.drawImage(magArea, img);

                // crosshair
                p.setPen(QPen(QColor(255, 255, 255, 180), 1));
                p.drawRect(magArea.adjusted(0, 0, -1, -1));
                p.drawLine(magArea.center().x(), magArea.top(), magArea.center().x(), magArea.bottom());
                p.drawLine(magArea.left(), magArea.center().y(), magArea.right(), magArea.center().y());
            }
        }

        // color text
        const QColor c = currentColor();
        QString text = c.isValid() ? c.name(QColor::HexArgb).toUpper() : QStringLiteral("(invalid)");
        QRect textRect(hud.left() + pad, hud.bottom() - 24, hud.width() - pad * 2, 18);
        p.setPen(QColor(255, 255, 255, 220));
        p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
    }

    void closeEvent(QCloseEvent *e) override
    {
        releaseMouse();
        releaseKeyboard();
        QDialog::closeEvent(e);
    }

private:
    void refreshGrab()
    {
        if (!m_screen) {
            return;
        }

        // Grab whole screen into a pixmap.
        // Note: On Windows, grabWindow(0) should capture the specified screen.
        m_grab = m_screen->grabWindow(0);
        m_grabDpr = m_grab.isNull() ? 1.0 : m_grab.devicePixelRatio();
        m_grabImg = m_grab.toImage();
    }

    QColor currentColor() const
    {
        if (!m_screen || m_grabImg.isNull()) {
            return QColor();
        }
        const QRect sg = screenGeometryDip(m_screen);
        const QPoint gp = QCursor::pos();
        return sampleAtGlobalDip(m_grabImg, m_grabDpr, gp, sg);
    }

    void updateHover(const QPoint &globalPos)
    {
        if (!m_screen) {
            return;
        }

        // Only active when cursor is on this screen
        const QRect sg = screenGeometryDip(m_screen);
        const bool contains = sg.contains(globalPos);
        if (!contains) {
            if (m_active) {
                m_active = false;
                m_lastHover = QColor();
                releaseMouse();
                releaseKeyboard();
                update();
            }
            return;
        }

        if (!m_active) {
            m_active = true;
            // Ensure this window becomes the active receiver.
            raise();
            activateWindow();
            setFocus(Qt::ActiveWindowFocusReason);
            grabMouse();
            grabKeyboard();
            update();
        }

        const QColor c = currentColor();
        if (c.isValid() && c != m_lastHover) {
            m_lastHover = c;
            emit hovered(c);
        }
        update();
    }

    QPointer<QScreen> m_screen;
    QPixmap m_grab;
    QImage m_grabImg;
    qreal m_grabDpr = 1.0;
    QColor m_lastHover;
    QTimer *m_poll = nullptr;
    bool m_active = false;
};

EyeDropperController::EyeDropperController(QObject *parent)
    : QObject(parent)
    auto *overlay = new EyeDropperOverlay();
    m_overlay = overlay;

    connect(overlay, &EyeDropperOverlay::hovered, this, &EyeDropperController::hovered);
    connect(overlay, &EyeDropperOverlay::picked, this, [this](const QColor &c) {
        if (m_finished) {
            return;
        }
        m_finished = true;
        emit picked(c);
        closeAll();
    });
    connect(overlay, &EyeDropperOverlay::canceled, this, [this]() {
        if (m_finished) {
            return;
        }
        m_finished = true;
        emit canceled();
        closeAll();
    });

    overlay->show();
    overlay->raise();
    overlay->activateWindow();
{
    QApplication::setOverrideCursor(pipetteCursor());

    // Fallback: catch mouse/keyboard at application level.
    qApp->installEventFilter(this);

    const auto screens = QGuiApplication::screens();

    for (QScreen *screen : screens) {
        auto *w = new EyeDropperWindow(screen);
        m_windows.push_back(w);

        // Force native handle and bind to the intended screen.
        (void)w->winId();
        if (auto *wh = w->windowHandle()) {
            wh->setScreen(screen);
        }

        connect(w, &EyeDropperWindow::hovered, this, &EyeDropperController::hovered);
        connect(w, &EyeDropperWindow::picked, this, [this](const QColor &c) {
            if (m_finished) {
                return;
            }
            m_finished = true;
            emit picked(c);
            closeAll();
        });
        connect(w, &EyeDropperWindow::canceled, this, [this]() {
            if (m_finished) {
                return;
            }
            m_finished = true;
            emit canceled();
            closeAll();
        });

        w->showFullScreen();
    }

    // Active window will grab mouse/keyboard based on cursor position.
}

bool EyeDropperController::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);

    if (m_finished) {
        return false;
    }

    switch (event->type()) {
    case QEvent::KeyPress: {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape) {
            m_finished = true;
            emit canceled();
            closeAll();
            return true;
        }
        break;
    }
    case QEvent::MouseButtonPress: {
        auto *me = static_cast<QMouseEvent *>(event);
        const QPoint gp = me->globalPos();

        if (me->button() == Qt::RightButton) {
            m_finished = true;
            emit canceled();
            closeAll();
            return true;
        }

        if (me->button() == Qt::LeftButton) {
            QColor c;
            for (EyeDropperWindow *w : m_windows) {
                if (!w) {
                    continue;
                }
                if (w->containsGlobal(gp)) {
                    c = w->sampleColorAtGlobal(gp);
                    break;
    if (m_overlay) {
        m_overlay->close();
        m_overlay->deleteLater();
        m_overlay.clear();
    }
                }
            }

            m_finished = true;
            if (c.isValid()) {
                emit picked(c);
            } else {
                emit canceled();
            }
            closeAll();
            return true;
        }
        break;
    }
    default:
        break;
    }

    return false;
}

EyeDropperController::~EyeDropperController()
{
    closeAll();
}

void EyeDropperController::closeAll()
{
    qApp->removeEventFilter(this);
    if (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }
    for (EyeDropperWindow *w : m_windows) {
        if (!w) {
            continue;
        }
        w->close();
        w->deleteLater();
    }
    m_windows.clear();
}

} // namespace Fluent::ColorPicker

#else

#include <QApplication>
#include <QCursor>
#include <QDialog>
#include <QGuiApplication>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QScreen>
#include <QTimer>

#include <limits>

namespace Fluent::ColorPicker {

static QCursor pipetteCursor()
{
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.translate(4.0, 6.0);
    QPen pen(QColor(0, 0, 0, 190), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(QColor(255, 255, 255, 230));

    QPainterPath path;
    path.moveTo(18, 2);
    path.lineTo(22, 6);
    path.lineTo(14, 14);
    path.lineTo(10, 10);
    path.closeSubpath();
    p.drawPath(path);

    p.drawLine(QPointF(10, 10), QPointF(3, 17));
    p.setPen(QPen(QColor(0, 0, 0, 170), 3.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPoint(QPointF(3, 17));

    p.setPen(QPen(QColor(255, 255, 255, 220), 1.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(QPointF(10, 10), QPointF(3, 17));

    return QCursor(pm, 1, 30);
}

static QRect screenGeometryDip(QScreen *screen)
{
    return screen ? screen->geometry() : QRect();
}

static QColor sampleAtGlobalDip(const QImage &img, qreal dpr, const QPoint &globalPosDip, const QRect &screenGeomDip)
{
    if (img.isNull()) {
        return QColor();
    }

    const QPoint localDip = globalPosDip - screenGeomDip.topLeft();
    const int x = qRound(localDip.x() * dpr);
    const int y = qRound(localDip.y() * dpr);
    if (x < 0 || y < 0 || x >= img.width() || y >= img.height()) {
        return QColor();
    }

    return QColor::fromRgb(img.pixel(x, y));
}

class EyeDropperOverlay final : public QDialog
{
    Q_OBJECT
public:
    explicit EyeDropperOverlay(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowFlag(Qt::FramelessWindowHint, true);
        setWindowFlag(Qt::Tool, false);
        setWindowFlag(Qt::Dialog, true);
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
        setWindowModality(Qt::ApplicationModal);

        setAttribute(Qt::WA_ShowWithoutActivating, false);
        setAttribute(Qt::WA_DeleteOnClose, true);
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        setCursor(pipetteCursor());

        // Initialize to current screen (designer style).
        updateScreenForGlobal(QCursor::pos(), true);

        m_poll = new QTimer(this);
        m_poll->setInterval(16);
        connect(m_poll, &QTimer::timeout, this, [this]() {
            if (!isVisible()) {
                return;
            }
            const QPoint gp = QCursor::pos();
            updateScreenForGlobal(gp, false);
            updateHover(gp);
        });
        m_poll->start();
    }

signals:
    void hovered(const QColor &color);
    void picked(const QColor &color);
    void canceled();

protected:
    void showEvent(QShowEvent *e) override
    {
        QDialog::showEvent(e);
        raise();
        activateWindow();
        setFocus(Qt::ActiveWindowFocusReason);
        grabMouse();
        grabKeyboard();
    }

    void closeEvent(QCloseEvent *e) override
    {
        releaseMouse();
        releaseKeyboard();
        QDialog::closeEvent(e);
    }

    void keyPressEvent(QKeyEvent *e) override
    {
        if (e->key() == Qt::Key_Escape) {
            emit canceled();
            close();
            return;
        }
        QDialog::keyPressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        updateScreenForGlobal(e->globalPos(), false);
        updateHover(e->globalPos());
        QDialog::mouseMoveEvent(e);
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::RightButton) {
            emit canceled();
            close();
            return;
        }

        if (e->button() == Qt::LeftButton) {
            updateScreenForGlobal(e->globalPos(), false);
            const QColor c = sampleAtGlobal(e->globalPos());
            if (c.isValid()) {
                emit picked(c);
            } else {
                emit canceled();
            }
            close();
            return;
        }

        QDialog::mousePressEvent(e);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        // Background: captured current screen snapshot.
        if (!m_pix.isNull()) {
            p.drawPixmap(rect(), m_pix);
        } else {
            p.fillRect(rect(), Qt::black);
        }

        // HUD
        const QPoint gp = QCursor::pos();
        if (!m_screen) {
            return;
        }

        const QRect sg = screenGeometryDip(m_screen);
        if (!sg.contains(gp)) {
            return;
        }

        const QPoint local = gp - sg.topLeft();
        const int radius = 48;
        const int pad = 10;

        QRect hud(local + QPoint(24, 24), QSize(radius * 2 + pad * 2, radius * 2 + pad * 2 + 28));
        hud.moveLeft(qBound(8, hud.left(), width() - hud.width() - 8));
        hud.moveTop(qBound(8, hud.top(), height() - hud.height() - 8));

        p.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;
        path.addRoundedRect(hud, 12, 12);
        p.fillPath(path, QColor(20, 20, 20, 200));
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        p.drawPath(path);

        QRect magArea(hud.left() + pad, hud.top() + pad, radius * 2, radius * 2);
        if (!m_img.isNull()) {
            const QPoint localDip = local;
            const int cx = qRound(localDip.x() * m_dpr);
            const int cy = qRound(localDip.y() * m_dpr);
            const int sampleRadiusPx = 8;
            QRect src(cx - sampleRadiusPx, cy - sampleRadiusPx, sampleRadiusPx * 2 + 1, sampleRadiusPx * 2 + 1);
            src = src.intersected(QRect(QPoint(0, 0), m_img.size()));
            const QImage img = m_img.copy(src);
            if (!img.isNull()) {
                p.setRenderHint(QPainter::SmoothPixmapTransform, false);
                p.drawImage(magArea, img);
                p.setPen(QPen(QColor(255, 255, 255, 180), 1));
                p.drawRect(magArea.adjusted(0, 0, -1, -1));
                p.drawLine(magArea.center().x(), magArea.top(), magArea.center().x(), magArea.bottom());
                p.drawLine(magArea.left(), magArea.center().y(), magArea.right(), magArea.center().y());
            }
        }

        const QColor c = sampleAtGlobal(gp);
        const QString text = c.isValid() ? c.name(QColor::HexArgb).toUpper() : QStringLiteral("(invalid)");
        QRect textRect(hud.left() + pad, hud.bottom() - 24, hud.width() - pad * 2, 18);
        p.setPen(QColor(255, 255, 255, 220));
        p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
    }

private:
    QColor sampleAtGlobal(const QPoint &globalPos) const
    {
        if (!m_screen || m_img.isNull()) {
            return QColor();
        }
        const QRect sg = screenGeometryDip(m_screen);
        return sampleAtGlobalDip(m_img, m_dpr, globalPos, sg);
    }

    void updateHover(const QPoint &globalPos)
    {
        const QColor c = sampleAtGlobal(globalPos);
        if (c.isValid() && c != m_lastHover) {
            m_lastHover = c;
            emit hovered(c);
        }
        update();
    }

    void updateScreenForGlobal(const QPoint &globalPos, bool force)
    {
        QScreen *s = nullptr;
        const auto screens = QGuiApplication::screens();
        for (QScreen *cand : screens) {
            if (cand && screenGeometryDip(cand).contains(globalPos)) {
                s = cand;
                break;
            }
        }
        if (!s) {
            s = QGuiApplication::screenAt(globalPos);
        }
        if (!s) {
            s = QGuiApplication::primaryScreen();
        }
        if (!s) {
            return;
        }
        if (!force && m_screen == s) {
            return;
        }

        // Important: grab BEFORE moving this overlay onto the target screen,
        // otherwise the screenshot will capture this overlay itself.
        QPixmap grabbed = s->grabWindow(0);

        m_screen = s;
        const QRect sg = screenGeometryDip(m_screen);
        setGeometry(sg);

        m_pix = grabbed;
        m_dpr = m_pix.isNull() ? 1.0 : m_pix.devicePixelRatio();
        m_img = m_pix.toImage();
        m_lastHover = QColor();
        update();
    }

    QPointer<QScreen> m_screen;
    QPixmap m_pix;
    QImage m_img;
    qreal m_dpr = 1.0;
    QColor m_lastHover;
    QTimer *m_poll = nullptr;
};

EyeDropperController::EyeDropperController(QObject *parent)
    : QObject(parent)
{
    QApplication::setOverrideCursor(pipetteCursor());

    auto *overlay = new EyeDropperOverlay();
    m_overlay = overlay;

    connect(overlay, &EyeDropperOverlay::hovered, this, &EyeDropperController::hovered);
    connect(overlay, &EyeDropperOverlay::picked, this, [this](const QColor &c) {
        if (m_finished) {
            return;
        }
        m_finished = true;
        emit picked(c);
        closeAll();
    });
    connect(overlay, &EyeDropperOverlay::canceled, this, [this]() {
        if (m_finished) {
            return;
        }
        m_finished = true;
        emit canceled();
        closeAll();
    });

    overlay->show();
    overlay->raise();
    overlay->activateWindow();
}

EyeDropperController::~EyeDropperController()
{
    closeAll();
}

void EyeDropperController::closeAll()
{
    if (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }

    if (m_overlay) {
        m_overlay->close();
        m_overlay->deleteLater();
        m_overlay.clear();
    }
}

} // namespace Fluent::ColorPicker

#endif

#include "EyeDropper.moc"
