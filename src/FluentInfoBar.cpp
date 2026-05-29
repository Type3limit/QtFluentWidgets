#include "Fluent/FluentInfoBar.h"

#include "Fluent/FluentButton.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentIcon.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentToolButton.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QSizePolicy>
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
        const QColor accent = severityColor(m_severity, colors);

        QColor wash = accent;
        wash.setAlphaF(Theme::isDark(colors) ? 0.18 : 0.12);
        painter.setPen(Qt::NoPen);
        painter.setBrush(wash);

        const QRectF badge = QRectF(rect()).adjusted(3.0, 3.0, -3.0, -3.0);
        painter.drawEllipse(badge);

        FluentIconOptions options;
        options.autoTheme = false;
        options.color = accent;
        options.opacity = isEnabled() ? 1.0 : 0.42;

        const QRectF iconRect = badge.adjusted(4.0, 4.0, -4.0, -4.0);
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
    setMinimumHeight(48);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(14, 12, 10, 12);
    m_layout->setSpacing(12);

    m_icon = new FluentInfoBarIcon(this);

    auto *textColumn = new QVBoxLayout();
    textColumn->setContentsMargins(0, 0, 0, 0);
    textColumn->setSpacing(2);

    m_titleLabel = new FluentLabel(this);
    m_titleLabel->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 650;"));
    m_messageLabel = new FluentLabel(this);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setStyleSheet(QStringLiteral("font-size: 12px; opacity: 0.88;"));

    textColumn->addWidget(m_titleLabel);
    textColumn->addWidget(m_messageLabel);

    m_actionButton = new FluentButton(this);
    m_actionButton->setVisible(false);
    connect(m_actionButton, &QPushButton::clicked, this, &FluentInfoBar::actionTriggered);

    m_closeButton = new FluentToolButton(this);
    m_closeButton->setProperty("fluentWindowGlyph", 3);
    m_closeButton->setProperty("fluentNeutralCloseGlyph", true);
    m_closeButton->setFixedSize(34, 28);
    connect(m_closeButton, &QToolButton::clicked, this, [this]() {
        hide();
        emit closed();
    });

    m_layout->addWidget(m_icon, 0, Qt::AlignTop);
    m_layout->addLayout(textColumn, 1);
    m_layout->addWidget(m_actionButton);
    m_layout->addWidget(m_closeButton, 0, Qt::AlignTop);

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

void FluentInfoBar::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentInfoBar::applyTheme()
{
    const auto &colors = ThemeManager::instance().colors();
    const QColor titleColor = colors.text;
    QColor messageColor = colors.text;
    messageColor.setAlphaF(Theme::isDark(colors) ? 0.82 : 0.78);
    static_cast<FluentInfoBarIcon *>(m_icon)->setSeverity(m_severity);
    m_titleLabel->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 650; color: %1;")
                                    .arg(titleColor.name(QColor::HexArgb)));
    m_messageLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: %1;")
                                      .arg(messageColor.name(QColor::HexArgb)));
    update();
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
    const QColor accent = severityColor(m_severity, colors);

    QPainter p(this);
    if (!p.isActive()) {
        return;
    }

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    FluentSurfaceSpec surface;
    surface.level = FluentSurfaceLevel::Raised;
    surface.radius = ThemeManager::instance().tokens().radius.overlay;
    surface.enabled = isEnabled();
    surface.tintColor = accent;
    surface.tintStrength = Theme::isDark(colors) ? 0.12 : 0.07;
    surface.borderColorOverride = Style::mix(fluentSurfaceBorder(colors, surface), accent, isEnabled() ? 0.34 : 0.16);
    paintFluentSurface(p, r, colors, surface);

    p.setPen(Qt::NoPen);
    p.setBrush(accent);
    p.drawRoundedRect(QRectF(r.left() + 1.0, r.top() + 8.0, 3.0, r.height() - 16.0), 1.5, 1.5);
}

} // namespace Fluent
