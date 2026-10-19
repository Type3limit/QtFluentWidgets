#include "Fluent/FluentInfoBar.h"

#include "Fluent/FluentButton.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentIcon.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentToolButton.h"

#include <QEvent>
#include <QBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QParallelAnimationGroup>
#include <QPainter>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QSizePolicy>
#include <QShowEvent>
#include <QVBoxLayout>

namespace Fluent {

namespace {

FluentIconType severityIcon(FluentInfoBar::Severity severity)
{
    switch (severity) {
    case FluentInfoBar::Severity::Success:
        return FluentIconType::Success;
    case FluentInfoBar::Severity::Warning:
        return FluentIconType::Warning;
    case FluentInfoBar::Severity::Error:
        return FluentIconType::Close;
    case FluentInfoBar::Severity::Info:
    default:
        return FluentIconType::Info;
    }
}

QColor severityColor(FluentInfoBar::Severity severity, const ThemeColors &colors)
{
    const auto tokens = Theme::tokens(colors);
    switch (severity) {
    case FluentInfoBar::Severity::Success:
        return tokens.semantic.success;
    case FluentInfoBar::Severity::Warning:
        return tokens.semantic.warning;
    case FluentInfoBar::Severity::Error:
        return tokens.semantic.error;
    case FluentInfoBar::Severity::Info:
    default:
        return tokens.semantic.info;
    }
}

QColor severityVisualColor(FluentInfoBar::Severity severity, const ThemeColors &colors, bool enabled)
{
    const QColor accent = severityColor(severity, colors);
    if (enabled) {
        return accent;
    }

    return Style::mix(accent, colors.disabledText, Theme::isDark(colors) ? 0.60 : 0.68);
}

class FluentInfoBarIcon final : public QWidget
{
public:
    explicit FluentInfoBarIcon(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(28, 28);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    void setSeverity(FluentInfoBar::Severity severity)
    {
        if (m_severity == severity) {
            return;
        }
        m_severity = severity;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)

        QPainter painter(this);
        if (!painter.isActive()) {
            return;
        }
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const auto &colors = ThemeManager::instance().colors();
        const bool enabled = isEnabled();
        const QColor accent = severityVisualColor(m_severity, colors, enabled);

        FluentIconOptions options;
        options.autoTheme = false;
        options.color = accent;
        options.opacity = enabled ? 1.0 : 0.72;

        // Plain severity glyph (no pale badge behind it), ~20px like WinUI.
        const qreal side = qMin(width(), height()) - 6.0;
        const QRectF iconRect(QRectF(rect()).center() - QPointF(side / 2.0, side / 2.0), QSizeF(side, side));
        FluentIcon::paintIcon(&painter, severityIcon(m_severity), iconRect, options);
    }

private:
    FluentInfoBar::Severity m_severity = FluentInfoBar::Severity::Info;
};

} // namespace

FluentInfoBar::FluentInfoBar(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(1.0);
    setGraphicsEffect(m_opacityEffect);

    m_layout = new QHBoxLayout(this);

    m_icon = new FluentInfoBarIcon(this);
    m_icon->setObjectName(QStringLiteral("FluentInfoBarSeverityIcon"));

    m_textLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    m_textLayout->setContentsMargins(0, 0, 0, 0);

    m_titleLabel = new FluentLabel(this);
    m_titleLabel->setObjectName(QStringLiteral("FluentInfoBarTitleLabel"));
    m_titleLabel->setMinimumWidth(0);
    m_titleLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_titleLabel->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 650;"));
    m_messageLabel = new FluentLabel(this);
    m_messageLabel->setObjectName(QStringLiteral("FluentInfoBarMessageLabel"));
    m_messageLabel->setMinimumWidth(0);
    m_messageLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setStyleSheet(QStringLiteral("font-size: 12px; opacity: 0.88;"));

    m_textLayout->addWidget(m_titleLabel);
    m_textLayout->addWidget(m_messageLabel);

    m_actionButton = new FluentButton(this);
    m_actionButton->setObjectName(QStringLiteral("FluentInfoBarActionButton"));
    m_actionButton->setVisible(false);
    connect(m_actionButton, &QPushButton::clicked, this, &FluentInfoBar::actionTriggered);

    m_closeButton = new FluentToolButton(this);
    m_closeButton->setObjectName(QStringLiteral("FluentInfoBarCloseButton"));
    m_closeButton->setProperty("fluentWindowGlyph", 3);
    m_closeButton->setProperty("fluentNeutralCloseGlyph", true);
    m_closeButton->setFixedSize(34, 28);
    m_closeButton->setToolTip(tr("Close"));
    connect(m_closeButton, &QToolButton::clicked, this, [this]() {
        dismiss();
    });

    m_layout->addWidget(m_icon, 0, Qt::AlignTop);
    m_layout->addLayout(m_textLayout, 1);
    m_layout->addWidget(m_actionButton);
    m_layout->addWidget(m_closeButton, 0, Qt::AlignTop);

    updateLayoutMode();
    applyTheme();
    updateContent();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentInfoBar::applyTheme);
}

FluentInfoBar::FluentInfoBar(Severity severity, const QString &title, const QString &message, QWidget *parent)
    : FluentInfoBar(parent)
{
    m_severity = severity;
    m_title = title;
    m_message = message;
    updateContent();
}

FluentInfoBar::Severity FluentInfoBar::severity() const
{
    return m_severity;
}

void FluentInfoBar::setSeverity(Severity severity)
{
    if (m_severity == severity) {
        return;
    }
    m_severity = severity;
    updateContent();
    update();
}

QString FluentInfoBar::title() const
{
    return m_title;
}

void FluentInfoBar::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    updateContent();
}

QString FluentInfoBar::message() const
{
    return m_message;
}

void FluentInfoBar::setMessage(const QString &message)
{
    if (m_message == message) {
        return;
    }
    m_message = message;
    updateContent();
}

bool FluentInfoBar::isClosable() const
{
    return m_closable;
}

void FluentInfoBar::setClosable(bool closable)
{
    if (m_closable == closable) {
        return;
    }
    m_closable = closable;
    updateContent();
}

QString FluentInfoBar::actionText() const
{
    return m_actionText;
}

void FluentInfoBar::setActionText(const QString &text)
{
    if (m_actionText == text) {
        return;
    }
    m_actionText = text;
    updateContent();
}

bool FluentInfoBar::isCompact() const
{
    return m_compact;
}

void FluentInfoBar::setCompact(bool compact)
{
    if (m_compact == compact) {
        return;
    }

    m_compact = compact;
    updateLayoutMode();
    applyTheme();
    updateGeometry();
    update();
}

void FluentInfoBar::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentInfoBar::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    if (m_dismissGroup) {
        m_dismissGroup->stop();
        m_dismissGroup->deleteLater();
        m_dismissGroup = nullptr;
    }
    m_dismissInProgress = false;
    restoreDismissGeometry();
}

void FluentInfoBar::applyTheme()
{
    if (m_dismissInProgress && m_dismissGroup && FluentMotion::duration(FluentMotionRole::PopupClose) <= 0) {
        finishDismissImmediately();
        return;
    }

    const auto &colors = ThemeManager::instance().colors();
    const bool enabled = isEnabled();
    const QColor titleColor = enabled ? colors.text : colors.disabledText;
    QColor messageColor = enabled ? colors.text : colors.disabledText;
    if (enabled) {
        messageColor.setAlphaF(Theme::isDark(colors) ? 0.82 : 0.78);
    }
    static_cast<FluentInfoBarIcon *>(m_icon)->setSeverity(m_severity);
    m_titleLabel->setStyleSheet(QStringLiteral("font-size: %1px; font-weight: 650; color: %2;")
                                    .arg(m_compact ? 13 : 14)
                                    .arg(titleColor.name(QColor::HexArgb)));
    m_messageLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: %1;")
                                      .arg(messageColor.name(QColor::HexArgb)));
    update();
}

void FluentInfoBar::dismiss()
{
    if (m_dismissInProgress) {
        return;
    }

    m_dismissInProgress = true;
    m_hasDismissGeometry = true;
    m_preDismissMinimumHeight = minimumHeight();
    m_preDismissMaximumHeight = maximumHeight();

    if (m_opacityEffect) {
        m_opacityEffect->setOpacity(1.0);
    }

    const int duration = FluentMotion::duration(FluentMotionRole::PopupClose);
    if (duration <= 0 || !isVisible()) {
        hide();
        restoreDismissGeometry();
        m_dismissInProgress = false;
        emit closed();
        return;
    }

    setMinimumHeight(0);
    setMaximumHeight(qMax(height(), sizeHint().height()));

    auto *group = new QParallelAnimationGroup(this);
    m_dismissGroup = group;

    if (m_opacityEffect) {
        auto *fade = new QPropertyAnimation(m_opacityEffect, "opacity", group);
        FluentMotion::configure(fade, FluentMotionRole::PopupClose);
        fade->setStartValue(m_opacityEffect->opacity());
        fade->setEndValue(0.0);
        group->addAnimation(fade);
    }

    auto *collapse = new QPropertyAnimation(this, "maximumHeight", group);
    FluentMotion::configure(collapse, FluentMotionRole::PopupClose);
    collapse->setStartValue(maximumHeight());
    collapse->setEndValue(0);
    group->addAnimation(collapse);

    connect(group, &QParallelAnimationGroup::finished, this, [this, group]() {
        if (m_dismissGroup != group) {
            group->deleteLater();
            return;
        }
        m_dismissGroup = nullptr;
        hide();
        restoreDismissGeometry();
        m_dismissInProgress = false;
        emit closed();
        group->deleteLater();
    });

    connect(group, &QObject::destroyed, this, [this, group]() {
        if (m_dismissGroup == group) {
            m_dismissGroup = nullptr;
        }
    });

    group->start();
}

void FluentInfoBar::finishDismissImmediately()
{
    if (m_dismissGroup) {
        auto *group = m_dismissGroup;
        m_dismissGroup = nullptr;
        group->stop();
        group->deleteLater();
    }

    hide();
    restoreDismissGeometry();
    m_dismissInProgress = false;
    emit closed();
}

void FluentInfoBar::restoreDismissGeometry()
{
    if (m_hasDismissGeometry) {
        setMinimumHeight(m_preDismissMinimumHeight);
        setMaximumHeight(m_preDismissMaximumHeight);
        m_hasDismissGeometry = false;
    }
    if (m_opacityEffect) {
        m_opacityEffect->setOpacity(1.0);
    }
}

void FluentInfoBar::updateLayoutMode()
{
    if (!m_layout || !m_textLayout || !m_icon || !m_closeButton) {
        return;
    }

    const int minHeight = m_compact ? 40 : 48;
    setMinimumHeight(minHeight);
    m_layout->setContentsMargins(m_compact ? QMargins(12, 8, 8, 8)
                                           : QMargins(14, 12, 10, 12));
    m_layout->setSpacing(m_compact ? 8 : 12);

    m_textLayout->setDirection(m_compact ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
    m_textLayout->setSpacing(m_compact ? 12 : 2);
    m_textLayout->setAlignment(m_compact ? Qt::AlignVCenter : Qt::Alignment());

    m_icon->setFixedSize(m_compact ? QSize(24, 24) : QSize(28, 28));
    m_closeButton->setFixedSize(m_compact ? QSize(30, 26) : QSize(34, 28));
    m_layout->setAlignment(m_icon, m_compact ? Qt::AlignVCenter : Qt::AlignTop);
    m_layout->setAlignment(m_actionButton, Qt::AlignVCenter);
    m_layout->setAlignment(m_closeButton, m_compact ? Qt::AlignVCenter : Qt::AlignTop);

    // Compact: the title hugs its text so the message follows right after it
    // instead of both labels splitting the row in half.
    m_titleLabel->setSizePolicy(m_compact ? QSizePolicy::Maximum : QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_titleLabel->setWordWrap(false);
    m_messageLabel->setWordWrap(!m_compact);
    m_messageLabel->setTextInteractionFlags(Qt::NoTextInteraction);
}

void FluentInfoBar::updateContent()
{
    static_cast<FluentInfoBarIcon *>(m_icon)->setSeverity(m_severity);
    m_titleLabel->setText(m_title);
    m_titleLabel->setVisible(!m_title.isEmpty());
    m_messageLabel->setText(m_message);
    m_messageLabel->setVisible(!m_message.isEmpty());
    m_actionButton->setText(m_actionText);
    m_actionButton->setVisible(!m_actionText.isEmpty());
    m_closeButton->setVisible(m_closable);
    applyTheme();
}

void FluentInfoBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const auto &colors = ThemeManager::instance().colors();
    const bool enabled = isEnabled();
    const QColor accent = severityVisualColor(m_severity, colors, enabled);

    QPainter p(this);
    if (!p.isActive()) {
        return;
    }

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    FluentSurfaceSpec surface;
    surface.level = FluentSurfaceLevel::Raised;
    surface.radius = ThemeManager::instance().tokens().radius.overlay;
    surface.enabled = enabled;
    surface.tintColor = accent;
    surface.tintStrength = enabled
        ? (Theme::isDark(colors) ? 0.12 : 0.07)
        : (Theme::isDark(colors) ? 0.045 : 0.03);
    surface.borderColorOverride = Style::mix(fluentSurfaceBorder(colors, surface), accent, enabled ? 0.34 : 0.16);
    paintFluentSurface(p, r, colors, surface);

    p.setPen(Qt::NoPen);
    QColor stripe = accent;
    if (!enabled) {
        stripe.setAlphaF(Theme::isDark(colors) ? 0.72 : 0.62);
    }
    p.setBrush(stripe);
    p.drawRoundedRect(QRectF(r.left() + 1.0, r.top() + 8.0, 3.0, r.height() - 16.0), 1.5, 1.5);
}

} // namespace Fluent
