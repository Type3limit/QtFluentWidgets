#pragma once

#include <QWidget>

#include "Fluent/FluentBorderEffect.h"

class QVariantAnimation;
class QEvent;
class QMouseEvent;

namespace Fluent {

class FluentToast final : public QWidget
{
    Q_OBJECT
public:
    enum class Position {
        TopLeft,
        TopCenter,
        TopRight,
        BottomLeft,
        BottomCenter,
        BottomRight,
    };

    explicit FluentToast(const QString &title, const QString &message, QWidget *parent = nullptr);

    // Shows a toast anchored to the given top-level window widget.
    // The toast will animate in, count down, and dismiss automatically.
    static void showToast(QWidget *window, const QString &title, const QString &message, int durationMs = 3500);

    static void showToast(QWidget *window,
                          const QString &title,
                          const QString &message,
                          Position position,
                          int durationMs = 3500);

signals:
    void dismissed();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void applyTheme();
    void start(int durationMs);
    void dismiss(bool animated);

    QString m_title;
    QString m_message;

    class ProgressBar;
    ProgressBar *m_progress = nullptr;

    bool m_dismissing = false;

    FluentBorderEffect m_border{this};
};

} // namespace Fluent
