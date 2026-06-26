#pragma once

#include "Fluent/FluentExport.h"

#include <QByteArray>
#include <QColor>
#include <QIcon>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <memory>

namespace Fluent {

class FLUENT_EXPORT FluentLottieWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool looping READ isLooping WRITE setLooping)
    Q_PROPERTY(qreal speed READ speed WRITE setSpeed)
    Q_PROPERTY(int currentFrame READ currentFrame WRITE setCurrentFrame NOTIFY currentFrameChanged)
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged)
    Q_PROPERTY(QColor tintColor READ tintColor WRITE setTintColor RESET resetTintColor NOTIFY tintColorChanged)
public:
    explicit FluentLottieWidget(QWidget *parent = nullptr);
    ~FluentLottieWidget() override;

    QString source() const;
    void setSource(const QString &path);

    bool load(const QString &path);
    bool loadData(const QByteArray &json,
                  const QString &cacheKey = QString(),
                  const QString &resourcePath = QString());

    bool isLoaded() const;
    QString errorString() const;

    QSize animationSize() const;
    int totalFrames() const;
    qreal frameRate() const;
    qreal duration() const;

    bool isPlaying() const;
    void setPlaying(bool playing);

    bool isLooping() const;
    void setLooping(bool looping);

    qreal speed() const;
    void setSpeed(qreal speed);

    int currentFrame() const;
    void setCurrentFrame(int frame);

    qreal progress() const;
    void setProgress(qreal progress);

    QIcon fallbackIcon() const;
    void setFallbackIcon(const QIcon &icon);

    QSize fallbackIconSize() const;
    void setFallbackIconSize(const QSize &size);

    QColor tintColor() const;
    void setTintColor(const QColor &color);
    void resetTintColor();

    QStringList markerNames() const;
    bool hasMarker(const QString &name) const;
    int markerFrame(const QString &name) const;
    int markerEndFrame(const QString &name) const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    void play();
    void pause();
    void stop();
    void playSegment(int startFrame, int endFrame);
    void playMarker(const QString &name);

signals:
    void sourceChanged(const QString &source);
    void loaded();
    void loadFailed(const QString &errorString);
    void playingChanged(bool playing);
    void currentFrameChanged(int frame);
    void progressChanged(qreal progress);
    void tintColorChanged(const QColor &color);
    void finished();

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void advanceFrame();
    void clearRenderCache();
    void syncPlaybackVisibility();
    void syncTimerInterval();
    void syncTimerState();
    void syncReducedMotionState();
    void finishActiveSegmentImmediately(bool emitFinished);
    void syncAnimationMetadata();
    void setError(const QString &error);

    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Fluent
