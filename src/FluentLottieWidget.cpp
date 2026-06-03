#include "Fluent/FluentLottieWidget.h"

#include "Fluent/FluentMotion.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "FluentPaintSupport.h"

#include <rlottie.h>

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QResizeEvent>
#include <QTimer>

#include <cmath>
#include <cstdint>
#include <tuple>

namespace Fluent {

namespace {

constexpr int kDefaultFrameRate = 60;

QSize boundedIconSize(const QSize &size)
{
    if (size.isValid()) {
        return size;
    }
    return QSize(24, 24);
}

int boundedFrame(int frame, int totalFrames)
{
    if (totalFrames <= 0) {
        return 0;
    }
    return qBound(0, frame, totalFrames - 1);
}

std::string toStdString(const QString &value)
{
    const QByteArray utf8 = value.toUtf8();
    return std::string(utf8.constData(), static_cast<size_t>(utf8.size()));
}

std::string toStdString(const QByteArray &value)
{
    return std::string(value.constData(), static_cast<size_t>(value.size()));
}

void paintFallbackIcon(QPainter &painter,
                       const QIcon &icon,
                       const QRect &rect,
                       QIcon::Mode mode,
                       const QColor &tint)
{
    if (icon.isNull()) {
        return;
    }

    if (!tint.isValid()) {
        icon.paint(&painter, rect, Qt::AlignCenter, mode);
        return;
    }

    QPixmap pixmap = icon.pixmap(rect.size(), mode);
    if (pixmap.isNull()) {
        return;
    }

    QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(pixmap.devicePixelRatio());

    QPainter tintPainter(&image);
    tintPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tintPainter.fillRect(image.rect(), tint);
    tintPainter.end();

    painter.drawImage(rect.topLeft(), image);
}

bool imageHasVisiblePixels(const QImage &image)
{
    if (image.isNull()) {
        return false;
    }

    constexpr int kVisibleAlphaThreshold = 16;
    constexpr int kMinimumVisiblePixels = 12;
    int visiblePixels = 0;

    const QImage argb = image.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < argb.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(argb.constScanLine(y));
        for (int x = 0; x < argb.width(); ++x) {
            if (qAlpha(line[x]) > kVisibleAlphaThreshold && ++visiblePixels >= kMinimumVisiblePixels) {
                return true;
            }
        }
    }

    return false;
}

} // namespace

struct FluentLottieWidget::Private
{
    std::unique_ptr<rlottie::Animation> animation;
    QString source;
    QString errorString;
    QSize animationSize;
    int totalFrames = 0;
    qreal frameRate = kDefaultFrameRate;

    QHash<QString, QPair<int, int>> markers;

    QTimer *timer = nullptr;
    bool playing = false;
    bool looping = true;
    qreal speed = 1.0;
    int currentFrame = 0;
    bool segmentActive = false;
    int segmentStart = 0;
    int segmentEnd = 0;

    QIcon fallbackIcon;
    QSize fallbackIconSize = QSize(24, 24);
    QColor tintColor;

    QImage cachedFrame;
    QImage cachedTintedFrame;
    int cachedFrameNumber = -1;
    QSize cachedPixelSize;
    qreal cachedDevicePixelRatio = 1.0;
    QColor cachedTintColor;
};

FluentLottieWidget::FluentLottieWidget(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Private>())
{
    setAttribute(Qt::WA_Hover, true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    d->timer = new QTimer(this);
    d->timer->setTimerType(Qt::PreciseTimer);
    connect(d->timer, &QTimer::timeout, this, &FluentLottieWidget::advanceFrame);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        syncReducedMotionState();
        update();
    });
    syncTimerInterval();
}

FluentLottieWidget::~FluentLottieWidget() = default;

QString FluentLottieWidget::source() const
{
    return d->source;
}

void FluentLottieWidget::setSource(const QString &path)
{
    if (d->source == path && isLoaded()) {
        return;
    }
    load(path);
}

bool FluentLottieWidget::load(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        d->animation.reset();
        d->source = path;
        setError(tr("Failed to open Lottie file: %1").arg(path));
        emit sourceChanged(d->source);
        update();
        return false;
    }

    const QFileInfo info(path);
    const QByteArray data = file.readAll();
    const QString cacheKey = info.absoluteFilePath();
    const QString resourcePath = info.absolutePath();
    const bool ok = loadData(data, cacheKey, resourcePath);

    if (d->source != path) {
        d->source = path;
        emit sourceChanged(d->source);
    }

    return ok;
}

bool FluentLottieWidget::loadData(const QByteArray &json, const QString &cacheKey, const QString &resourcePath)
{
    const QString key = cacheKey.isEmpty()
                            ? QString::fromLatin1(QCryptographicHash::hash(json, QCryptographicHash::Sha1).toHex())
                            : cacheKey;

    std::unique_ptr<rlottie::Animation> animation =
        rlottie::Animation::loadFromData(toStdString(json), toStdString(key), toStdString(resourcePath), true);

    if (!animation) {
        d->animation.reset();
        d->animationSize = {};
        d->totalFrames = 0;
        d->markers.clear();
        d->currentFrame = 0;
        d->playing = false;
        d->timer->stop();
        setError(tr("Failed to parse Lottie data."));
        update();
        return false;
    }

    d->animation = std::move(animation);
    d->errorString.clear();
    d->segmentActive = false;
    d->currentFrame = 0;
    syncAnimationMetadata();
    syncTimerInterval();
    clearRenderCache();

    emit currentFrameChanged(d->currentFrame);
    emit progressChanged(progress());
    emit loaded();
    update();
    return true;
}

bool FluentLottieWidget::isLoaded() const
{
    return d->animation != nullptr;
}

QString FluentLottieWidget::errorString() const
{
    return d->errorString;
}

QSize FluentLottieWidget::animationSize() const
{
    return d->animationSize;
}

int FluentLottieWidget::totalFrames() const
{
    return d->totalFrames;
}

qreal FluentLottieWidget::frameRate() const
{
    return d->frameRate;
}

qreal FluentLottieWidget::duration() const
{
    if (!isLoaded() || d->frameRate <= 0.0) {
        return 0.0;
    }
    return static_cast<qreal>(d->totalFrames) / d->frameRate;
}

bool FluentLottieWidget::isPlaying() const
{
    return d->playing;
}

void FluentLottieWidget::setPlaying(bool playing)
{
    if (playing) {
        play();
    } else {
        pause();
    }
}

bool FluentLottieWidget::isLooping() const
{
    return d->looping;
}

void FluentLottieWidget::setLooping(bool looping)
{
    d->looping = looping;
}

qreal FluentLottieWidget::speed() const
{
    return d->speed;
}

void FluentLottieWidget::setSpeed(qreal speed)
{
    const qreal next = qBound<qreal>(0.05, std::abs(speed), 8.0);
    if (qFuzzyCompare(d->speed, next)) {
        return;
    }

    d->speed = next;
    syncTimerInterval();
}

int FluentLottieWidget::currentFrame() const
{
    return d->currentFrame;
}

void FluentLottieWidget::setCurrentFrame(int frame)
{
    const int next = boundedFrame(frame, d->totalFrames);
    if (d->currentFrame == next) {
        return;
    }

    d->currentFrame = next;
    clearRenderCache();
    emit currentFrameChanged(d->currentFrame);
    emit progressChanged(progress());
    update();
}

qreal FluentLottieWidget::progress() const
{
    if (d->totalFrames <= 1) {
        return 0.0;
    }
    return static_cast<qreal>(d->currentFrame) / static_cast<qreal>(d->totalFrames - 1);
}

void FluentLottieWidget::setProgress(qreal progress)
{
    progress = qBound<qreal>(0.0, progress, 1.0);
    setCurrentFrame(qRound(progress * qMax(0, d->totalFrames - 1)));
}

QIcon FluentLottieWidget::fallbackIcon() const
{
    return d->fallbackIcon;
}

void FluentLottieWidget::setFallbackIcon(const QIcon &icon)
{
    d->fallbackIcon = icon;
    update();
}

QSize FluentLottieWidget::fallbackIconSize() const
{
    return d->fallbackIconSize;
}

void FluentLottieWidget::setFallbackIconSize(const QSize &size)
{
    d->fallbackIconSize = boundedIconSize(size);
    updateGeometry();
    update();
}

QColor FluentLottieWidget::tintColor() const
{
    return d->tintColor;
}

void FluentLottieWidget::setTintColor(const QColor &color)
{
    const QColor next = color.isValid() ? color : QColor();
    if (d->tintColor == next) {
        return;
    }

    d->tintColor = next;
    d->cachedTintedFrame = QImage();
    d->cachedTintColor = QColor();
    emit tintColorChanged(d->tintColor);
    update();
}

void FluentLottieWidget::resetTintColor()
{
    setTintColor(QColor());
}

QStringList FluentLottieWidget::markerNames() const
{
    QStringList names = d->markers.keys();
    names.sort(Qt::CaseInsensitive);
    return names;
}

bool FluentLottieWidget::hasMarker(const QString &name) const
{
    return d->markers.contains(name);
}

int FluentLottieWidget::markerFrame(const QString &name) const
{
    const auto it = d->markers.constFind(name);
    return it == d->markers.constEnd() ? -1 : it.value().first;
}

int FluentLottieWidget::markerEndFrame(const QString &name) const
{
    const auto it = d->markers.constFind(name);
    return it == d->markers.constEnd() ? -1 : it.value().second;
}

QSize FluentLottieWidget::sizeHint() const
{
    if (d->animationSize.isValid()) {
        return d->animationSize.scaled(QSize(64, 64), Qt::KeepAspectRatio).expandedTo(QSize(24, 24));
    }
    return boundedIconSize(d->fallbackIconSize).expandedTo(QSize(24, 24));
}

QSize FluentLottieWidget::minimumSizeHint() const
{
    return QSize(16, 16);
}

void FluentLottieWidget::play()
{
    if (!isLoaded() || d->totalFrames <= 1) {
        return;
    }

    if (!FluentMotion::animationsEnabled()) {
        if (d->segmentActive) {
            finishActiveSegmentImmediately(true);
        }
        return;
    }

    if (d->playing) {
        return;
    }

    d->playing = true;
    syncTimerInterval();
    d->timer->start();
    emit playingChanged(true);
}

void FluentLottieWidget::pause()
{
    if (!d->playing) {
        return;
    }

    d->playing = false;
    d->timer->stop();
    emit playingChanged(false);
}

void FluentLottieWidget::stop()
{
    const bool wasPlaying = d->playing;
    d->playing = false;
    d->timer->stop();
    d->segmentActive = false;
    setCurrentFrame(0);
    if (wasPlaying) {
        emit playingChanged(false);
    }
}

void FluentLottieWidget::playSegment(int startFrame, int endFrame)
{
    if (!isLoaded()) {
        return;
    }

    const int start = boundedFrame(startFrame, d->totalFrames);
    const int end = boundedFrame(endFrame, d->totalFrames);
    d->segmentActive = true;
    d->segmentStart = start;
    d->segmentEnd = end;
    setCurrentFrame(start);

    if (start == end) {
        d->segmentActive = false;
        pause();
        return;
    }

    if (!FluentMotion::animationsEnabled()) {
        finishActiveSegmentImmediately(true);
        return;
    }

    play();
}

void FluentLottieWidget::playMarker(const QString &name)
{
    const int start = markerFrame(name);
    if (start < 0) {
        return;
    }

    const int end = markerEndFrame(name);
    if (end > start) {
        playSegment(start, end);
    } else {
        pause();
        setCurrentFrame(start);
    }
}

void FluentLottieWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    if (!Detail::canBeginWidgetPainter(this)) {
        return;
    }

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF bounds = QRectF(rect());
    if (bounds.isEmpty()) {
        return;
    }

    if (isLoaded()) {
        QSize logicalSize = d->animationSize.isValid() ? d->animationSize : bounds.size().toSize();
        logicalSize.scale(bounds.size().toSize(), Qt::KeepAspectRatio);
        logicalSize = logicalSize.expandedTo(QSize(1, 1));

        const qreal dpr = qMax<qreal>(1.0, devicePixelRatioF());
        const QSize pixelSize(qMax(1, qRound(logicalSize.width() * dpr)),
                              qMax(1, qRound(logicalSize.height() * dpr)));

        if (d->cachedFrameNumber != d->currentFrame
            || d->cachedPixelSize != pixelSize
            || !qFuzzyCompare(d->cachedDevicePixelRatio, dpr)) {
            QImage image(pixelSize, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);

            rlottie::Surface surface(reinterpret_cast<uint32_t *>(image.bits()),
                                     static_cast<size_t>(image.width()),
                                     static_cast<size_t>(image.height()),
                                     static_cast<size_t>(image.bytesPerLine()));
            d->animation->renderSync(static_cast<size_t>(d->currentFrame), surface, true);
            image.setDevicePixelRatio(dpr);

            d->cachedFrame = image;
            d->cachedTintedFrame = QImage();
            d->cachedFrameNumber = d->currentFrame;
            d->cachedPixelSize = pixelSize;
            d->cachedDevicePixelRatio = dpr;
            d->cachedTintColor = QColor();
        }

        const QImage *frame = &d->cachedFrame;
        if (d->tintColor.isValid()) {
            if (d->cachedTintedFrame.isNull() || d->cachedTintColor != d->tintColor) {
                d->cachedTintedFrame = QImage(d->cachedFrame.size(), QImage::Format_ARGB32_Premultiplied);
                d->cachedTintedFrame.setDevicePixelRatio(d->cachedFrame.devicePixelRatio());
                d->cachedTintedFrame.fill(Qt::transparent);

                QPainter tintPainter(&d->cachedTintedFrame);
                tintPainter.drawImage(QPoint(0, 0), d->cachedFrame);
                tintPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
                tintPainter.fillRect(d->cachedTintedFrame.rect(), d->tintColor);
                tintPainter.end();

                d->cachedTintColor = d->tintColor;
            }
            frame = &d->cachedTintedFrame;
        }

        QRectF target(QPointF(0, 0), QSizeF(logicalSize));
        target.moveCenter(bounds.center());
        if (!d->fallbackIcon.isNull() && !imageHasVisiblePixels(*frame)) {
            const QSize iconSize = boundedIconSize(d->fallbackIconSize).boundedTo(bounds.size().toSize());
            const QRect iconRect(QPoint(qRound(bounds.center().x() - iconSize.width() / 2.0),
                                        qRound(bounds.center().y() - iconSize.height() / 2.0)),
                                 iconSize);
            const QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
            paintFallbackIcon(painter, d->fallbackIcon, iconRect, mode, d->tintColor);
            return;
        }
        painter.drawImage(target.topLeft(), *frame);
        return;
    }

    const QSize iconSize = boundedIconSize(d->fallbackIconSize).boundedTo(bounds.size().toSize());
    const QRect iconRect(QPoint(qRound(bounds.center().x() - iconSize.width() / 2.0),
                                qRound(bounds.center().y() - iconSize.height() / 2.0)),
                         iconSize);

    if (!d->fallbackIcon.isNull()) {
        const QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
        paintFallbackIcon(painter, d->fallbackIcon, iconRect, mode, d->tintColor);
        return;
    }

    QColor color = d->tintColor;
    if (!color.isValid()) {
        const auto &colors = ThemeManager::instance().colors();
        color = isEnabled() ? colors.text : colors.disabledText;
        if (!color.isValid()) {
            color = palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::WindowText);
        }
    }

    painter.setPen(QPen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    const qreal s = qMin(iconRect.width(), iconRect.height());
    const QPointF center = QRectF(iconRect).center();
    const QRectF lens(center.x() - s * 0.28, center.y() - s * 0.32, s * 0.48, s * 0.48);
    painter.drawEllipse(lens);
    painter.drawLine(QPointF(lens.right() - s * 0.02, lens.bottom() - s * 0.02),
                     QPointF(center.x() + s * 0.34, center.y() + s * 0.34));
}

void FluentLottieWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    clearRenderCache();
}

void FluentLottieWidget::advanceFrame()
{
    if (!isLoaded() || !d->playing || d->totalFrames <= 1) {
        return;
    }

    const int rangeStart = d->segmentActive ? d->segmentStart : 0;
    const int rangeEnd = d->segmentActive ? d->segmentEnd : d->totalFrames - 1;
    const int direction = rangeEnd >= rangeStart ? 1 : -1;
    int next = d->currentFrame + direction;
    const bool passedEnd = direction > 0 ? next > rangeEnd : next < rangeEnd;

    if (passedEnd) {
        if (d->segmentActive) {
            d->segmentActive = false;
            setCurrentFrame(rangeEnd);
            pause();
            emit finished();
            return;
        }

        if (d->looping) {
            next = rangeStart;
        } else {
            setCurrentFrame(rangeEnd);
            pause();
            emit finished();
            return;
        }
    }

    setCurrentFrame(next);
}

void FluentLottieWidget::clearRenderCache()
{
    d->cachedFrame = QImage();
    d->cachedTintedFrame = QImage();
    d->cachedFrameNumber = -1;
    d->cachedPixelSize = QSize();
    d->cachedDevicePixelRatio = 1.0;
    d->cachedTintColor = QColor();
}

void FluentLottieWidget::syncTimerInterval()
{
    const qreal fps = qMax<qreal>(1.0, d->frameRate * d->speed);
    const int interval = qBound(1, qRound(1000.0 / fps), 1000);
    d->timer->setInterval(interval);
}

void FluentLottieWidget::syncReducedMotionState()
{
    if (FluentMotion::animationsEnabled()) {
        syncTimerInterval();
        return;
    }

    if (d->segmentActive) {
        finishActiveSegmentImmediately(true);
        return;
    }

    if (d->playing) {
        pause();
    }
}

void FluentLottieWidget::finishActiveSegmentImmediately(bool emitFinished)
{
    if (!d->segmentActive) {
        if (d->playing) {
            pause();
        }
        return;
    }

    const bool wasPlaying = d->playing;
    const int targetFrame = d->segmentEnd;
    d->segmentActive = false;
    d->playing = false;
    d->timer->stop();
    setCurrentFrame(targetFrame);
    if (wasPlaying) {
        emit playingChanged(false);
    }
    if (emitFinished) {
        emit finished();
    }
}

void FluentLottieWidget::syncAnimationMetadata()
{
    d->animationSize = {};
    d->totalFrames = 0;
    d->frameRate = kDefaultFrameRate;
    d->markers.clear();

    if (!d->animation) {
        return;
    }

    size_t width = 0;
    size_t height = 0;
    d->animation->size(width, height);
    if (width > 0 && height > 0) {
        d->animationSize = QSize(static_cast<int>(width), static_cast<int>(height));
    }

    d->totalFrames = static_cast<int>(d->animation->totalFrame());
    d->frameRate = qMax<qreal>(1.0, d->animation->frameRate());
    d->currentFrame = boundedFrame(d->currentFrame, d->totalFrames);

    const auto &markers = d->animation->markers();
    for (const auto &marker : markers) {
        const QString name = QString::fromStdString(std::get<0>(marker));
        if (name.isEmpty()) {
            continue;
        }

        const int start = boundedFrame(std::get<1>(marker), d->totalFrames);
        const int end = boundedFrame(std::get<2>(marker), d->totalFrames);
        d->markers.insert(name, qMakePair(start, end));
    }
}

void FluentLottieWidget::setError(const QString &error)
{
    d->errorString = error;
    emit loadFailed(error);
}

} // namespace Fluent
