#pragma once

#include <QDialog>
#include <QPointer>
#include <QUrl>

#include "Fluent/FluentBorderEffect.h"

class QLabel;
class QTextEdit;
class QWidget;
class QVariantAnimation;
class QHideEvent;

namespace Fluent {

class FluentMessageBox final : public QDialog
{
    Q_OBJECT
public:
    enum IconType {
        Info,
        Warning,
        Error,
        Question
    };

    explicit FluentMessageBox(const QString &title,
                              const QString &message,
                              IconType icon = Info,
                              QWidget *parent = nullptr);

    explicit FluentMessageBox(const QString &title,
                              const QString &message,
                              const QString &detail,
                              IconType icon = Info,
                              QWidget *parent = nullptr,
                              const QString &linkText = QString(),
                              const QUrl &linkUrl = QUrl());

    // Optional modal mask (dim overlay) on top of the parent window while the message box is visible.
    void setMaskEnabled(bool enabled);
    bool maskEnabled() const;

    void setMaskOpacity(qreal opacity);
    qreal maskOpacity() const;

    static int information(QWidget *parent, const QString &title, const QString &message, const QString &detail = QString(),
                           const QString &linkText = QString(), const QUrl &linkUrl = QUrl());
    static int information(QWidget *parent, const QString &title, const QString &message, const QString &detail,
                           bool maskEnabled, const QString &linkText = QString(), const QUrl &linkUrl = QUrl());
    static int warning(QWidget *parent, const QString &title, const QString &message, const QString &detail = QString(),
                       const QString &linkText = QString(), const QUrl &linkUrl = QUrl());
    static int warning(QWidget *parent, const QString &title, const QString &message, const QString &detail,
                       bool maskEnabled, const QString &linkText = QString(), const QUrl &linkUrl = QUrl());
    static int error(QWidget *parent, const QString &title, const QString &message, const QString &detail = QString(),
                     const QString &linkText = QString(), const QUrl &linkUrl = QUrl());
    static int error(QWidget *parent, const QString &title, const QString &message, const QString &detail,
                     bool maskEnabled, const QString &linkText = QString(), const QUrl &linkUrl = QUrl());
    static int question(QWidget *parent, const QString &title, const QString &message, const QString &detail = QString(),
                        const QString &linkText = QString(), const QUrl &linkUrl = QUrl());
    static int question(QWidget *parent, const QString &title, const QString &message, const QString &detail,
                        bool maskEnabled, const QString &linkText = QString(), const QUrl &linkUrl = QUrl());

protected:
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void buildUi(const QString &message, const QString &detail, IconType icon,
                 const QString &linkText, const QUrl &linkUrl);

    void applyTheme();
    void updateMessageElideAndTooltip(bool allowResize);

    void ensureMaskOverlay();
    void teardownMaskOverlay();

    QWidget *m_panel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_iconLabel = nullptr;
    QLabel *m_messageLabel = nullptr;
    QTextEdit *m_detailEdit = nullptr;
    QLabel *m_linkLabel = nullptr;
    QWidget *m_divider = nullptr;
    IconType m_icon = Info;

    QString m_messageFullText;
    bool m_updatingMessageLayout = false;

    bool m_maskEnabled = false;
    qreal m_maskOpacity = 0.35;
    QPointer<QWidget> m_maskOverlay;

    bool m_dragging = false;
    QPoint m_dragOffset;

    FluentBorderEffect m_border{this};
};

} // namespace Fluent