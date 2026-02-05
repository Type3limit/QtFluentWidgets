#include "Fluent/FluentMessageBox.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentButton.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QFrame>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QTextEdit>
#include <QStyle>
#include <QVBoxLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>
#include <QVariantAnimation>
#include <QWindow>
#include <QHideEvent>

#include <QSvgRenderer>

namespace Fluent {

namespace {
class ModalMaskOverlay final : public QWidget
{
public:
    explicit ModalMaskOverlay(QWidget *window, qreal opacity)
        : QWidget(window)
        , m_window(window)
        , m_opacity(qBound<qreal>(0.0, opacity, 1.0))
    {
        setObjectName("FluentModalMaskOverlay");
        setAttribute(Qt::WA_NoSystemBackground, false);
        setAttribute(Qt::WA_StyledBackground, false);
        setAttribute(Qt::WA_TransparentForMouseEvents, false);

        if (m_window) {
            m_window->installEventFilter(this);
            setGeometry(m_window->rect());
        }
    }

    void setOpacity(qreal opacity)
    {
        m_opacity = qBound<qreal>(0.0, opacity, 1.0);
        update();
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_window) {
            switch (event->type()) {
            case QEvent::Resize:
            case QEvent::Show:
            case QEvent::WindowStateChange:
                setGeometry(m_window->rect());
                raise();
                break;
            default:
                break;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)
        QPainter p(this);
        if (!p.isActive()) {
            return;
        }

        QColor c(0, 0, 0);
        c.setAlphaF(m_opacity);
        p.fillRect(rect(), c);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event) {
            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event) {
            event->accept();
        }
    }

private:
    QPointer<QWidget> m_window;
    qreal m_opacity = 0.35;
};

static int fmAdvance(const QFontMetrics &fm, const QString &s)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    return fm.horizontalAdvance(s);
#else
    return fm.width(s);
#endif
}

static QString elideMultilineText(const QString &text, const QFont &font, int width, int maxLines)
{
    if (maxLines <= 0 || width <= 0) {
        return text;
    }

    QStringList paragraphs = text.split('\n');
    if (paragraphs.isEmpty()) {
        return text;
    }

    const QFontMetrics fm(font);
    QStringList lines;
    lines.reserve(maxLines + 2);
    bool truncated = false;

    for (int p = 0; p < paragraphs.size(); ++p) {
        QString para = paragraphs[p];
        // Keep empty lines.
        if (para.isEmpty()) {
            lines << QString();
            if (lines.size() >= maxLines) {
                if (p < paragraphs.size() - 1) {
                    truncated = true;
                }
                break;
            }
            continue;
        }

        int i = 0;
        while (i < para.size()) {
            if (lines.size() >= maxLines) {
                truncated = true;
                break;
            }

            int bestBreak = i;
            int lastSpace = -1;
            int j = i;
            while (j < para.size()) {
                const QChar ch = para[j];
                if (ch.isSpace()) {
                    lastSpace = j;
                }

                const QString piece = para.mid(i, j - i + 1);
                if (fmAdvance(fm, piece) > width) {
                    break;
                }
                bestBreak = j + 1;
                ++j;
            }

            if (bestBreak == i) {
                // Even one char doesn't fit; force at least one char to avoid infinite loop.
                bestBreak = i + 1;
            } else if (j < para.size() && lastSpace >= i && lastSpace + 1 < bestBreak) {
                // Wrap at last space if it doesn't make the line empty.
                bestBreak = lastSpace + 1;
            }

            QString line = para.mid(i, bestBreak - i);
            line = line.trimmed();
            lines << line;
            i = bestBreak;
        }

        if (lines.size() >= maxLines) {
            if (p < paragraphs.size() - 1) {
                truncated = true;
            }
            break;
        }
    }

    if (!truncated) {
        return text;
    }

    if (!lines.isEmpty()) {
        const int lastIndex = qMin(maxLines - 1, lines.size() - 1);
        QString last = lines[lastIndex];
        last = fm.elidedText(last, Qt::ElideRight, width);
        lines[lastIndex] = last;
    }

    return lines.mid(0, maxLines).join("\n");
}
} // namespace

static QColor messageBoxGlyphColorForFill(const QColor &fill)
{
    // Use high-contrast glyph on top of the accent fill.
    return (fill.lightnessF() > 0.72) ? QColor(0, 0, 0, 220) : QColor(255, 255, 255, 235);
}

static QString svgColor(const QColor &c)
{
    return c.name(QColor::HexRgb);
}

static QString messageBoxIconSvg(FluentMessageBox::IconType icon, const QColor &accent)
{
    QColor fill = accent;
    fill.setAlpha(230);
    QColor ring = accent.darker(118);
    ring.setAlpha(255);
    const QColor glyph = messageBoxGlyphColorForFill(fill);
    QColor glyphRgb = glyph;
    glyphRgb.setAlpha(255);
    const QString glyphOpacity = QString::number(glyph.alphaF(), 'f', 3);

    // ViewBox is 48x48 to match our icon label size.
    // Use round caps/joins for a Fluent-like appearance.
    QString glyphMarkup;
    switch (icon) {
    case FluentMessageBox::Warning:
        // Warning icon (filled triangle + exclamation) based on the provided SVG geometry.
        // Center and scale into the circular badge; keep conservative scale to avoid overflow.
        glyphMarkup = QString(
            "<g transform='translate(24 24) scale(0.032) translate(-512 -512)'>"
            "<path d='M558 563c0 24.852-20.148 45-45 45S468 587.852 468 563v-150c0-24.852 20.148-45 45-45s45 20.148 45 45v150z m0 132c0 24.852-20.148 45-45 45S468 719.852 468 695v-1c0-24.852 20.148-45 45-45S558 669.148 558 694v1z m-355.006 65.804a15 15 0 0 0 14.986 15.014l589.36 0.55a15 15 0 0 0 12.916-22.646L525.56 256.376a15 15 0 0 0-25.806-0.006l-294.66 496.796a15 15 0 0 0-2.098 7.638z m-75.31-53.552l294.66-496.794c29.584-49.878 93.998-66.328 143.874-36.746a105 105 0 0 1 36.768 36.784l294.7 497.346c29.56 49.89 13.08 114.298-36.808 143.86a105 105 0 0 1-53.624 14.666l-589.358-0.55c-57.99-0.054-104.956-47.108-104.9-105.1a105 105 0 0 1 14.688-53.466z' fill='%1' fill-opacity='%2'/>"
            "</g>"
        ).arg(svgColor(glyphRgb), glyphOpacity);
        break;
    case FluentMessageBox::Error:
        glyphMarkup = QString(
            "<line x1='18' y1='18' x2='30' y2='30' stroke='%1' stroke-width='3.2' stroke-linecap='round'/>"
            "<line x1='30' y1='18' x2='18' y2='30' stroke='%1' stroke-width='3.2' stroke-linecap='round'/>"
        ).arg(svgColor(glyph));
        break;
    case FluentMessageBox::Question:
        glyphMarkup = QString(
            "<path d='M19 20 C19 16.7 21.2 15 24 15 C26.8 15 29 16.7 29 19.3 C29 21.4 27.8 22.5 26 23.4 C24.8 24 24 24.8 24 26.2' "
            "fill='none' stroke='%1' stroke-width='3.2' stroke-linecap='round' stroke-linejoin='round'/>"
            "<line x1='24' y1='26.8' x2='24' y2='28.2' stroke='%1' stroke-width='3.2' stroke-linecap='round'/>"
            "<circle cx='24' cy='34' r='2.4' fill='%1'/>"
        ).arg(svgColor(glyph));
        break;
    case FluentMessageBox::Info:
    default:
        glyphMarkup = QString(
            "<line x1='24' y1='21' x2='24' y2='33' stroke='%1' stroke-width='3.2' stroke-linecap='round'/>"
            "<circle cx='24' cy='16.5' r='2.4' fill='%1'/>"
        ).arg(svgColor(glyph));
        break;
    }

    return QString(
        "<svg xmlns='http://www.w3.org/2000/svg' width='48' height='48' viewBox='0 0 48 48'>"
        "<defs>"
        "  <clipPath id='mbClip'>"
        "    <circle cx='24' cy='24' r='22'/>"
        "  </clipPath>"
        "</defs>"
        "<circle cx='24' cy='24' r='22' fill='%1' fill-opacity='0.90'/>"
        "<circle cx='24' cy='24' r='22' fill='none' stroke='%2' stroke-width='1.6'/>"
        "<g clip-path='url(#mbClip)'>%3</g>"
        "</svg>"
    ).arg(svgColor(fill), svgColor(ring), glyphMarkup);
}

static void paintMessageBoxGlyph(QPainter &p, const QRectF &bounds, FluentMessageBox::IconType icon, const QColor &glyph)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal s = qMin(bounds.width(), bounds.height());
    const QPointF c = bounds.center();

    // Stroke width tuned for ~48px icon.
    const qreal stroke = qMax<qreal>(3.0, s * 0.090);
    QPen pen(glyph, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    // Slightly larger dot reads better after HiDPI downsampling.
    const qreal dotR = stroke * 0.72;
    auto snap = [](qreal v) {
        // Align to half-pixel for crisper strokes on common rasterizers.
        return std::floor(v) + 0.5;
    };
    auto snapPoint = [&](QPointF pt) {
        pt.setX(snap(pt.x()));
        pt.setY(snap(pt.y()));
        return pt;
    };
    auto dotAt = [&](const QPointF &pt) {
        p.save();
        p.setPen(Qt::NoPen);
        p.setBrush(glyph);
        p.drawEllipse(snapPoint(pt), dotR, dotR);
        p.restore();
    };

    // Inner glyph area (keeps shapes centered and away from the outer ring)
    const qreal pad = qMax<qreal>(8.0, s * 0.20);
    const QRectF g = bounds.adjusted(pad, pad, -pad, -pad);
    const QPointF gc = g.center();
    const qreal gs = qMin(g.width(), g.height());

    switch (icon) {
    case FluentMessageBox::Warning: {
        // Warning triangle + '!'
        QPainterPath tri;
        const QPointF top(gc.x(), g.top() + 0.06 * gs);
        const QPointF left(g.left() + 0.08 * gs, g.bottom() - 0.06 * gs);
        const QPointF right(g.right() - 0.08 * gs, g.bottom() - 0.06 * gs);
        tri.moveTo(top);
        tri.lineTo(right);
        tri.lineTo(left);
        tri.closeSubpath();
        p.drawPath(tri);

        // Exclamation inside (with padding from the triangle)
        // Increase padding so the mark doesn't feel glued to the triangle.
        const QRectF inner = g.adjusted(0.26 * gs, 0.24 * gs, -0.26 * gs, -0.24 * gs);
        const qreal ih = inner.height();
        const QPointF x0(inner.center().x(), 0.0);
        p.drawLine(snapPoint(QPointF(x0.x(), inner.top() + 0.16 * ih)),
                   snapPoint(QPointF(x0.x(), inner.top() + 0.58 * ih)));
        dotAt(QPointF(x0.x(), inner.top() + 0.86 * ih));
        break;
    }
    case FluentMessageBox::Error: {
        // Cross (inside ring)
        const QPointF a(g.left() + 0.20 * gs, g.top() + 0.20 * gs);
        const QPointF b(g.left() + 0.80 * gs, g.top() + 0.80 * gs);
        const QPointF c1(g.left() + 0.80 * gs, g.top() + 0.20 * gs);
        const QPointF d(g.left() + 0.20 * gs, g.top() + 0.80 * gs);
        p.drawLine(a, b);
        p.drawLine(c1, d);
        break;
    }
    case FluentMessageBox::Question: {
        // A vector '?' (centered by its visual bounds, including the dot)
        QPainterPath path;
        path.moveTo(gc.x() - 0.24 * gs, g.top() + 0.38 * gs);
        path.cubicTo(gc.x() - 0.24 * gs, g.top() + 0.16 * gs,
                     gc.x() + 0.24 * gs, g.top() + 0.16 * gs,
                     gc.x() + 0.24 * gs, g.top() + 0.38 * gs);
        path.cubicTo(gc.x() + 0.24 * gs, g.top() + 0.54 * gs,
                     gc.x(), g.top() + 0.56 * gs,
                     gc.x(), g.top() + 0.66 * gs);

        // Give the dot more breathing room below the stem.
        const QLineF stem(QPointF(gc.x(), g.top() + 0.64 * gs), QPointF(gc.x(), g.top() + 0.70 * gs));
        QPointF dotCenter(gc.x(), g.top() + 0.92 * gs);

        QRectF united = path.boundingRect();
        united = united.united(QRectF(stem.p1(), stem.p2()).normalized().adjusted(-stroke, -stroke, stroke, stroke));
        united = united.united(QRectF(dotCenter.x() - dotR, dotCenter.y() - dotR, 2 * dotR, 2 * dotR));

        const QPointF delta = g.center() - united.center();
        path.translate(delta);
        const QLineF stem2(stem.p1() + delta, stem.p2() + delta);
        dotCenter += delta;

        p.drawPath(path);
        p.drawLine(snapPoint(stem2.p1()), snapPoint(stem2.p2()));
        dotAt(dotCenter);
        break;
    }
    case FluentMessageBox::Info:
    default: {
        // 'i' (stem + dot)
        p.drawLine(snapPoint(QPointF(gc.x(), g.top() + 0.34 * gs)), snapPoint(QPointF(gc.x(), g.top() + 0.78 * gs)));
        dotAt(QPointF(gc.x(), g.top() + 0.16 * gs));
        break;
    }
    }

    p.restore();
}

static QColor messageBoxIconColor(const ThemeColors &colors, FluentMessageBox::IconType icon)
{
    switch (icon) {
    case FluentMessageBox::Warning:
        return QColor("#F5A623");
    case FluentMessageBox::Error:
        return colors.error;
    case FluentMessageBox::Question:
        return QColor("#5C6BC0");
    case FluentMessageBox::Info:
    default:
        return colors.accent;
    }
}

FluentMessageBox::FluentMessageBox(const QString &title,
                                   const QString &message,
                                   IconType icon,
                                   QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setSizeGripEnabled(false);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_StyledBackground, true);
    setWindowFlag(Qt::FramelessWindowHint, true);
    setWindowFlag(Qt::NoDropShadowWindowHint, true);
    setStyleSheet(
        "QDialog { background: transparent; }"
        "QWidget#FluentMessageBoxPanel { background: transparent; }");
    setMinimumWidth(420);
    buildUi(message, QString(), icon, QString(), QUrl());
    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentMessageBox::applyTheme);

    m_border.syncFromTheme();
}

FluentMessageBox::FluentMessageBox(const QString &title,
                                   const QString &message,
                                   const QString &detail,
                                   IconType icon,
                                   QWidget *parent,
                                   const QString &linkText,
                                   const QUrl &linkUrl)
    : QDialog(parent)
{
    setWindowTitle(title);
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setSizeGripEnabled(false);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_StyledBackground, true);
    setWindowFlag(Qt::FramelessWindowHint, true);
    setWindowFlag(Qt::NoDropShadowWindowHint, true);
    setStyleSheet(
        "QDialog { background: transparent; }"
        "QWidget#FluentMessageBoxPanel { background: transparent; }");
    setMinimumWidth(420);
    buildUi(message, detail, icon, linkText, linkUrl);
    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentMessageBox::applyTheme);

    m_border.syncFromTheme();
}

int FluentMessageBox::information(QWidget *parent, const QString &title, const QString &message, const QString &detail,
                                  const QString &linkText, const QUrl &linkUrl)
{
    FluentMessageBox box(title, message, detail, Info, parent, linkText, linkUrl);
    return box.exec();
}

int FluentMessageBox::information(QWidget *parent, const QString &title, const QString &message, const QString &detail,
                                  bool maskEnabled, const QString &linkText, const QUrl &linkUrl)
{
    FluentMessageBox box(title, message, detail, Info, parent, linkText, linkUrl);
    box.setMaskEnabled(maskEnabled);
    return box.exec();
}

int FluentMessageBox::warning(QWidget *parent, const QString &title, const QString &message, const QString &detail,
                              const QString &linkText, const QUrl &linkUrl)
{
    FluentMessageBox box(title, message, detail, Warning, parent, linkText, linkUrl);
    return box.exec();
}

int FluentMessageBox::warning(QWidget *parent, const QString &title, const QString &message, const QString &detail,
                              bool maskEnabled, const QString &linkText, const QUrl &linkUrl)
{
    FluentMessageBox box(title, message, detail, Warning, parent, linkText, linkUrl);
    box.setMaskEnabled(maskEnabled);
    return box.exec();
}

int FluentMessageBox::error(QWidget *parent, const QString &title, const QString &message, const QString &detail,
                            const QString &linkText, const QUrl &linkUrl)
{
    FluentMessageBox box(title, message, detail, Error, parent, linkText, linkUrl);
    return box.exec();
}

int FluentMessageBox::error(QWidget *parent, const QString &title, const QString &message, const QString &detail,
                            bool maskEnabled, const QString &linkText, const QUrl &linkUrl)
{
    FluentMessageBox box(title, message, detail, Error, parent, linkText, linkUrl);
    box.setMaskEnabled(maskEnabled);
    return box.exec();
}

int FluentMessageBox::question(QWidget *parent, const QString &title, const QString &message, const QString &detail,
                               const QString &linkText, const QUrl &linkUrl)
{
    FluentMessageBox box(title, message, detail, Question, parent, linkText, linkUrl);
    return box.exec();
}

int FluentMessageBox::question(QWidget *parent, const QString &title, const QString &message, const QString &detail,
                               bool maskEnabled, const QString &linkText, const QUrl &linkUrl)
{
    FluentMessageBox box(title, message, detail, Question, parent, linkText, linkUrl);
    box.setMaskEnabled(maskEnabled);
    return box.exec();
}

void FluentMessageBox::setMaskEnabled(bool enabled)
{
    m_maskEnabled = enabled;
    if (isVisible()) {
        if (m_maskEnabled) {
            ensureMaskOverlay();
        } else {
            teardownMaskOverlay();
        }
    }
}

bool FluentMessageBox::maskEnabled() const
{
    return m_maskEnabled;
}

void FluentMessageBox::setMaskOpacity(qreal opacity)
{
    m_maskOpacity = qBound<qreal>(0.0, opacity, 1.0);
    if (m_maskOverlay) {
        if (auto *overlay = dynamic_cast<ModalMaskOverlay *>(m_maskOverlay.data())) {
            overlay->setOpacity(m_maskOpacity);
        } else {
            m_maskOverlay->update();
        }
    }
}

qreal FluentMessageBox::maskOpacity() const
{
    return m_maskOpacity;
}

void FluentMessageBox::buildUi(const QString &message, const QString &detail, IconType icon,
                               const QString &linkText, const QUrl &linkUrl)
{
    m_icon = icon;
    m_messageFullText = message;

    auto *root = new QVBoxLayout(this);
    // No shadow: keep panel flush to the dialog edges.
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_panel = new QWidget(this);
    m_panel->setObjectName("FluentMessageBoxPanel");
    root->addWidget(m_panel);

    auto *panelLayout = new QVBoxLayout(m_panel);
    panelLayout->setContentsMargins(22, 20, 22, 16);
    panelLayout->setSpacing(14);

    // Content grid (icon column + text column) to keep everything perfectly aligned.
    auto *grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(8);
    grid->setColumnStretch(1, 1);

    m_iconLabel = new QLabel();
    m_iconLabel->setFixedSize(48, 48);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    m_titleLabel = new QLabel(windowTitle());
    m_titleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Dragging support (frameless)
    m_titleLabel->installEventFilter(this);
    m_panel->installEventFilter(this);

    m_messageLabel = new QLabel(message);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_messageLabel->setToolTip(QString());
    m_messageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_messageLabel->setMinimumHeight(0);

    if (!detail.isEmpty()) {
        m_detailEdit = new QTextEdit();
        m_detailEdit->setReadOnly(true);
        m_detailEdit->setText(detail);
        m_detailEdit->setMinimumHeight(84);
        m_detailEdit->setMaximumHeight(160);
        m_detailEdit->setFrameShape(QFrame::NoFrame);
        m_detailEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    if (!linkText.isEmpty() && linkUrl.isValid()) {
        m_linkLabel = new QLabel(QString("<a href=\"%1\">%2</a>").arg(linkUrl.toString(), linkText));
        m_linkLabel->setTextFormat(Qt::RichText);
        m_linkLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_linkLabel->setOpenExternalLinks(true);
        m_linkLabel->setObjectName("FluentLink");
    }

    m_divider = new QWidget(m_panel);
    m_divider->setFixedHeight(1);

    // Place content into the grid.
    // Center the icon vertically against the title+message block.
    grid->addWidget(m_iconLabel, 0, 0, 2, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(m_titleLabel, 0, 1);
    grid->addWidget(m_messageLabel, 1, 1);
    // Let the message row take extra vertical space when the dialog grows.
    grid->setRowStretch(1, 1);
    int row = 2;
    if (m_detailEdit) {
        grid->addWidget(m_detailEdit, row++, 1, 1, 1);
    }
    if (m_linkLabel) {
        grid->addWidget(m_linkLabel, row++, 1);
    }

    panelLayout->addLayout(grid);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(8);
    buttonRow->addStretch();

    FluentButton *primary = nullptr;
    FluentButton *secondary = nullptr;
    if (icon == Question) {
        primary = new FluentButton(tr(u8"确定"));
        primary->setPrimary(true);
        secondary = new FluentButton(tr(u8"取消"));
    } else {
        primary = new FluentButton(tr(u8"确定"));
        primary->setPrimary(true);
    }

    if (secondary) {
        buttonRow->addWidget(secondary);
    }
    buttonRow->addWidget(primary);

    primary->setDefault(true);
    primary->setMinimumWidth(88);
    if (secondary) {
        secondary->setMinimumWidth(88);
    }

    panelLayout->addWidget(m_divider);
    panelLayout->addLayout(buttonRow);

    connect(primary, &QPushButton::clicked, this, &QDialog::accept);
    if (secondary) {
        connect(secondary, &QPushButton::clicked, this, &QDialog::reject);
    }

    // Esc closes all variants; matches common message box behavior.
    setWindowModality(Qt::WindowModal);
}

void FluentMessageBox::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    if (m_maskEnabled) {
        ensureMaskOverlay();
    }

    m_border.playInitialTraceOnce(0);

    // Defer one tick so layouts have valid geometries (label widths are known).
    QTimer::singleShot(0, this, [this]() {
        updateMessageElideAndTooltip(true);
    });
}

void FluentMessageBox::hideEvent(QHideEvent *event)
{
    teardownMaskOverlay();
    QDialog::hideEvent(event);
}

void FluentMessageBox::ensureMaskOverlay()
{
    QWidget *w = parentWidget() ? parentWidget()->window() : nullptr;
    if (!w) {
        return;
    }

    if (m_maskOverlay && m_maskOverlay->parentWidget() == w) {
        if (auto *overlay = dynamic_cast<ModalMaskOverlay *>(m_maskOverlay.data())) {
            overlay->setOpacity(m_maskOpacity);
        }
        m_maskOverlay->setGeometry(w->rect());
        m_maskOverlay->show();
        m_maskOverlay->raise();
        return;
    }

    teardownMaskOverlay();
    auto *overlay = new ModalMaskOverlay(w, m_maskOpacity);
    overlay->setGeometry(w->rect());
    overlay->show();
    overlay->raise();
    m_maskOverlay = overlay;
}

void FluentMessageBox::teardownMaskOverlay()
{
    if (!m_maskOverlay) {
        return;
    }

    m_maskOverlay->hide();
    m_maskOverlay->deleteLater();
    m_maskOverlay = nullptr;
}

void FluentMessageBox::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    updateMessageElideAndTooltip(false);
}

void FluentMessageBox::updateMessageElideAndTooltip(bool allowResize)
{
    if (m_updatingMessageLayout) {
        return;
    }
    m_updatingMessageLayout = true;

    if (!m_messageLabel) {
        m_updatingMessageLayout = false;
        return;
    }

    QScreen *screen = nullptr;
    if (windowHandle()) {
        screen = windowHandle()->screen();
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        m_updatingMessageLayout = false;
        return;
    }

    const int maxDialogH = qMax(240, qRound(screen->availableGeometry().height() * 0.72));

    // First, try to layout the dialog with full message. Only elide if we truly exceed
    // the screen-constrained maximum height.
    m_messageLabel->setMaximumHeight(QWIDGETSIZE_MAX);
    m_messageLabel->setText(m_messageFullText);
    m_messageLabel->setToolTip(QString());

    setMaximumHeight(QWIDGETSIZE_MAX);
    if (layout()) layout()->activate();
    if (allowResize) {
        adjustSize();
    }

    const QFontMetrics fm(m_messageLabel->font());
    const int lineH = qMax(1, fm.lineSpacing());

    // Derive text column width.
    int textW = m_messageLabel->contentsRect().width();
    if (textW <= 0) {
        textW = qMax(120, width() - 44 - 14 - 48);
    }

    // For resizeEvent / non-resize path, decide purely based on the current label height.
    if (!allowResize) {
        const int availH = qMax(lineH, m_messageLabel->contentsRect().height());
        const QRect textRect(0, 0, textW, 200000);
        const int fullMsgH = fm.boundingRect(textRect, Qt::TextWordWrap, m_messageFullText).height();
        if (fullMsgH <= availH + 1) {
            m_messageLabel->setText(m_messageFullText);
            m_messageLabel->setToolTip(QString());
            m_updatingMessageLayout = false;
            return;
        }

        const int maxLines = qMax(1, availH / lineH);
        const QString elided = elideMultilineText(m_messageFullText, m_messageLabel->font(), textW, maxLines);
        m_messageLabel->setText(elided);
        m_messageLabel->setToolTip(m_messageFullText);
        m_updatingMessageLayout = false;
        return;
    }

    // If the dialog fits within the max height, keep full text.
    if (height() <= maxDialogH) {
        setMaximumHeight(maxDialogH);
        m_updatingMessageLayout = false;
        return;
    }

    // Constrain to max height, then compute how many lines fit and elide accordingly.
    setMaximumHeight(maxDialogH);
    if (allowResize) {
        resize(width(), maxDialogH);
    }
    if (layout()) layout()->activate();

    const int availH = qMax(lineH, m_messageLabel->contentsRect().height());

    const QRect textRect(0, 0, textW, 200000);
    const int fullMsgH = fm.boundingRect(textRect, Qt::TextWordWrap, m_messageFullText).height();

    if (fullMsgH <= availH + 1) {
        // Even after clamping, message fits. No need to elide.
        m_messageLabel->setText(m_messageFullText);
        m_messageLabel->setToolTip(QString());
        m_updatingMessageLayout = false;
        return;
    }

    const int maxLines = qMax(1, availH / lineH);
    const QString elided = elideMultilineText(m_messageFullText, m_messageLabel->font(), textW, maxLines);
    m_messageLabel->setText(elided);
    m_messageLabel->setToolTip(m_messageFullText);

    m_updatingMessageLayout = false;
}

bool FluentMessageBox::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            const QPoint globalPos = me->globalPos();
            const QPoint localInDialog = mapFromGlobal(globalPos);

            // Only allow dragging from the title row / top region.
            const int dragHeight = 48;
            const bool allow = (watched == m_titleLabel) || (localInDialog.y() >= 0 && localInDialog.y() <= dragHeight);
            if (allow) {
                m_dragging = true;
                m_dragOffset = globalPos - frameGeometry().topLeft();
                return true;
            }
        }
    } else if (event->type() == QEvent::MouseMove) {
        if (m_dragging) {
            auto *me = static_cast<QMouseEvent *>(event);
            const QPoint globalPos = me->globalPos();
            move(globalPos - m_dragOffset);
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton && m_dragging) {
            m_dragging = false;
            return true;
        }
    }

    return QDialog::eventFilter(watched, event);
}

void FluentMessageBox::applyTheme()
{
    if (isVisible()) {
        m_border.onThemeChanged();
    } else {
        m_border.syncFromTheme();
    }

    const auto &colors = ThemeManager::instance().colors();

    QFont titleFont = font();
    const qreal ps = titleFont.pointSizeF();
    if (ps > 0.0) {
        titleFont.setPointSizeF(ps + 1.0);
    } else {
        const int px = titleFont.pixelSize();
        if (px > 0) {
            titleFont.setPixelSize(px + 1);
        } else {
            titleFont.setPointSizeF(11.0);
        }
    }
    titleFont.setWeight(QFont::DemiBold);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setStyleSheet(QString("color: %1;").arg(colors.text.name()));

    m_messageLabel->setStyleSheet(QString("color: %1;").arg(colors.text.name()));

    if (m_detailEdit) {
        m_detailEdit->setStyleSheet(QString(
            "QTextEdit {"
            "  color: %1;"
            "  background: %2;"
            "  border: 1px solid %3;"
            "  border-radius: 8px;"
            "  padding: 8px 10px;"
            "}"
        ).arg(colors.subText.name(), colors.hover.name(), colors.border.name()));
    }

    if (m_divider) {
        const QColor div = Style::withAlpha(colors.border, 180);
        m_divider->setStyleSheet(QString("background: %1;").arg(div.name(QColor::HexArgb)));
    }

    if (m_linkLabel) {
        // Base stylesheet already styles #FluentLink, but keep explicit for dialogs.
        m_linkLabel->setStyleSheet(QString("color: %1;").arg(colors.accent.name()));
    }

    // Icon
    const QColor accent = messageBoxIconColor(colors, m_icon);

    const QSize logicalSize = m_iconLabel ? m_iconLabel->size() : QSize(48, 48);
    qreal dpr = devicePixelRatioF();
    if (m_iconLabel && m_iconLabel->windowHandle()) {
        dpr = m_iconLabel->windowHandle()->devicePixelRatio();
    }
    const QSize pixelSize(qMax(1, qRound(logicalSize.width() * dpr)), qMax(1, qRound(logicalSize.height() * dpr)));

    // SVG rendering for crisp glyphs.
    const QString svg = messageBoxIconSvg(m_icon, accent);
    QSvgRenderer renderer(svg.toUtf8());

    QImage img(pixelSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    {
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing, true);
        renderer.render(&p, QRectF(0, 0, pixelSize.width(), pixelSize.height()));
    }

    QPixmap pm = QPixmap::fromImage(img);
    pm.setDevicePixelRatio(dpr);
    // Avoid an extra scale step; QLabel will map device pixels via DPR.
    m_iconLabel->setPixmap(pm);

    update();
}

void FluentMessageBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (!m_panel) {
        return;
    }

    const auto &colors = ThemeManager::instance().colors();
    const QRectF panelRect = QRectF(m_panel->geometry()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = 10.0;

    QPainter p(this);
    if (!p.isActive()) {
        return;
    }
    p.setRenderHint(QPainter::Antialiasing, true);

    // Clear to transparent first; avoids corner artifacts on translucent windows.
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(rect(), Qt::transparent);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    FluentFrameSpec frame;
    frame.radius = radius;
    frame.maximized = false;
    frame.borderWidth = 1.0;
    m_border.applyToFrameSpec(frame, colors);

    paintFluentPanel(p, panelRect, colors, frame);
}

void FluentMessageBox::changeEvent(QEvent *event)
{
    QDialog::changeEvent(event);
    if (event->type() == QEvent::EnabledChange || event->type() == QEvent::PaletteChange) {
        applyTheme();
    }
}

} // namespace Fluent