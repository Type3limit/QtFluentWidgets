#pragma once

#include "Fluent/FluentExport.h"

#include <QWidget>

class QHBoxLayout;
class QBoxLayout;
class QGraphicsOpacityEffect;
class QParallelAnimationGroup;
class QShowEvent;

namespace Fluent {

class FluentButton;
class FluentLabel;
class FluentToolButton;

class FLUENT_EXPORT FluentInfoBar final : public QWidget
{
    Q_OBJECT
public:
    enum class Severity {
        Info,
        Success,
        Warning,
        Error,
    };
    Q_ENUM(Severity)

    Q_PROPERTY(Severity severity READ severity WRITE setSeverity)
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QString message READ message WRITE setMessage)
    Q_PROPERTY(bool closable READ isClosable WRITE setClosable)
    Q_PROPERTY(QString actionText READ actionText WRITE setActionText)
    Q_PROPERTY(bool compact READ isCompact WRITE setCompact)

public:
    explicit FluentInfoBar(QWidget *parent = nullptr);
    FluentInfoBar(Severity severity, const QString &title, const QString &message, QWidget *parent = nullptr);

    Severity severity() const;
    void setSeverity(Severity severity);

    QString title() const;
    void setTitle(const QString &title);

    QString message() const;
    void setMessage(const QString &message);

    bool isClosable() const;
    void setClosable(bool closable);

    QString actionText() const;
    void setActionText(const QString &text);

    bool isCompact() const;
    void setCompact(bool compact);

signals:
    void actionTriggered();
    void closed();

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void applyTheme();
    void dismiss();
    void finishDismissImmediately();
    void restoreDismissGeometry();
    void updateLayoutMode();
    void updateContent();

    Severity m_severity = Severity::Info;
    QString m_title;
    QString m_message;
    QString m_actionText;
    bool m_closable = true;
    bool m_compact = false;

    QHBoxLayout *m_layout = nullptr;
    QBoxLayout *m_textLayout = nullptr;
    QWidget *m_icon = nullptr;
    FluentLabel *m_titleLabel = nullptr;
    FluentLabel *m_messageLabel = nullptr;
    FluentButton *m_actionButton = nullptr;
    FluentToolButton *m_closeButton = nullptr;
    QGraphicsOpacityEffect *m_opacityEffect = nullptr;
    QParallelAnimationGroup *m_dismissGroup = nullptr;

    bool m_dismissInProgress = false;
    bool m_hasDismissGeometry = false;
    int m_preDismissMinimumHeight = 0;
    int m_preDismissMaximumHeight = QWIDGETSIZE_MAX;
};

} // namespace Fluent
