#include "Fluent/FluentCodeEditor.h"

#include "Fluent/FluentCppHighlighter.h"
#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QAction>
#include <QEvent>
#include <QFileInfo>
#include <QFocusEvent>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPen>
#include <QProcess>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollBar>
#include <QShowEvent>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTextCursor>
#include <QTimer>
#include <QToolTip>
#include <QVariantAnimation>

namespace Fluent {

class FluentCodeEditorBorderOverlay final : public QWidget
{
public:
    explicit FluentCodeEditorBorderOverlay(FluentCodeEditor *owner)
        : QWidget(owner)
        , m_owner(owner)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAutoFillBackground(false);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        if (!m_owner) {
            return;
        }

        QPainter p(this);
        if (!p.isActive()) {
            return;
        }

        const auto &colors = ThemeManager::instance().colors();
        const auto m = Style::metrics();

        const QColor stroke = m_owner->isEnabled() ? colors.border : Style::mix(colors.border, colors.disabledText, 0.35);

        p.setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(rect());

        qreal dpr = 1.0;
        if (p.device()) {
            dpr = p.device()->devicePixelRatioF();
            if (dpr <= 0.0) {
                dpr = 1.0;
            }
        }
        const qreal px = 0.5 / dpr;
        const QRectF rr = r.adjusted(px, px, -px, -px);

        p.setPen(QPen(stroke, 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawPath(Style::roundedRectPath(rr, m.radius));

        if (m_owner->isEnabled() && m_owner->m_focusLevel > 0.0) {
            QColor focus = colors.focus;
            focus.setAlphaF(0.9 * qBound<qreal>(0.0, m_owner->m_focusLevel, 1.0));
            p.setPen(QPen(focus, 2.0));
            p.drawPath(Style::roundedRectPath(rr.adjusted(1.0, 1.0, -1.0, -1.0), m.radius - 1));
        }
    }

private:
    FluentCodeEditor *m_owner = nullptr;
};

class FluentCodeEditorLineNumberArea final : public QWidget
{
public:
    explicit FluentCodeEditorLineNumberArea(FluentCodeEditor *editor)
        : QWidget(editor)
        , m_editor(editor)
    {
        setAutoFillBackground(false);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
    }

    QSize sizeHint() const override { return {m_editor->lineNumberAreaWidth(), 0}; }

protected:
    void paintEvent(QPaintEvent *event) override { m_editor->lineNumberAreaPaintEvent(event); }

private:
    FluentCodeEditor *m_editor = nullptr;
};

namespace {

static bool isOpenBracket(QChar c)
{
    return c == QLatin1Char('(') || c == QLatin1Char('{') || c == QLatin1Char('[');
}

static bool isCloseBracket(QChar c)
{
    return c == QLatin1Char(')') || c == QLatin1Char('}') || c == QLatin1Char(']');
}

static QChar matchingBracket(QChar c)
{
    switch (c.unicode()) {
    case '(':
        return QLatin1Char(')');
    case ')':
        return QLatin1Char('(');
    case '{':
        return QLatin1Char('}');
    case '}':
        return QLatin1Char('{');
    case '[':
        return QLatin1Char(']');
    case ']':
        return QLatin1Char('[');
    default:
        return QChar();
    }
}

} // namespace

static void snapshotCursor(QPlainTextEdit *edit,
                           int &outBlock,
                           int &outColumn,
                           int &outAnchorBlock,
                           int &outAnchorColumn,
                           bool &outHadSelection)
{
    QTextCursor c = edit->textCursor();
    outHadSelection = c.hasSelection();

    QTextCursor a = c;
    a.setPosition(c.anchor());

    outBlock = c.blockNumber();
    outColumn = c.positionInBlock();
    outAnchorBlock = a.blockNumber();
    outAnchorColumn = a.positionInBlock();
}

static void restoreCursor(QPlainTextEdit *edit,
                          int block,
                          int column,
                          int anchorBlock,
                          int anchorColumn,
                          bool hadSelection)
{
    auto clampToDoc = [&](int b, int col) {
        b = qMax(0, qMin(b, edit->document()->blockCount() - 1));
        QTextBlock blk = edit->document()->findBlockByNumber(b);
        col = qMax(0, qMin(col, blk.length() - 1));
        return qMakePair(b, col);
    };

    auto p1 = clampToDoc(anchorBlock, anchorColumn);
    auto p2 = clampToDoc(block, column);

    QTextBlock blk1 = edit->document()->findBlockByNumber(p1.first);
    QTextBlock blk2 = edit->document()->findBlockByNumber(p2.first);

    int pos1 = blk1.position() + p1.second;
    int pos2 = blk2.position() + p2.second;

    QTextCursor c(edit->document());
    c.setPosition(pos2);
    if (hadSelection) {
        c.setPosition(pos1, QTextCursor::KeepAnchor);
    }
    edit->setTextCursor(c);
}

FluentCodeEditor::FluentCodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setFrameStyle(QFrame::NoFrame);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(false);

    setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * 4);
    setWordWrapMode(QTextOption::NoWrap);

    setVerticalScrollBar(new FluentScrollBar(Qt::Vertical, this));
    setHorizontalScrollBar(new FluentScrollBar(Qt::Horizontal, this));

    if (viewport()) {
        viewport()->setMouseTracking(true);
        viewport()->setAutoFillBackground(false);
        viewport()->setAttribute(Qt::WA_StyledBackground, true);
        viewport()->installEventFilter(this);
    }

    m_lineNumberArea = new FluentCodeEditorLineNumberArea(this);
    updateLineNumberAreaWidth();

    m_borderOverlay = new FluentCodeEditorBorderOverlay(this);
    m_borderOverlay->setGeometry(rect());
    m_borderOverlay->hide();

    connect(this, &QPlainTextEdit::blockCountChanged, this, [this] { updateLineNumberAreaWidth(); });
    connect(this, &QPlainTextEdit::updateRequest, this, &FluentCodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, [this] { updateExtraSelections(); });

    m_formatAction = new QAction(tr("Format Document"), this);
    m_formatAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+F")));
    m_formatAction->setShortcutContext(Qt::WidgetShortcut);
    m_formatAction->setToolTip(tr("Format document (clang-format if available)"));
    connect(m_formatAction, &QAction::triggered, this, [this] {
        m_manualFormatTriggered = true;
        formatDocumentNow();
    });
    addAction(m_formatAction);

    m_clangFormatPath = findDefaultClangFormat();

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_hoverLevel = qBound<qreal>(0.0, v.toReal(), 1.0);
        update();
        if (m_lineNumberArea) {
            m_lineNumberArea->update();
        }
        if (m_borderOverlay) {
            m_borderOverlay->update();
        }
    });

    m_focusAnim = new QVariantAnimation(this);
    m_focusAnim->setDuration(200);
    m_focusAnim->setEasingCurve(QEasingCurve::OutQuad);
    connect(m_focusAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_focusLevel = qBound<qreal>(0.0, v.toReal(), 1.0);
        update();
        if (m_lineNumberArea) {
            m_lineNumberArea->update();
        }
        if (m_borderOverlay) {
            m_borderOverlay->update();
        }
    });

    connect(this, &FluentCodeEditor::clangFormatAvailabilityChanged, this, [this](bool available) {
        if (!m_formatAction) {
            return;
        }
        if (available) {
            m_formatAction->setText(tr("Format (clang-format)"));
            m_formatAction->setToolTip(tr("Format document using clang-format"));
        } else {
            m_formatAction->setText(tr("Format (basic)"));
            m_formatAction->setToolTip(tr("clang-format not found; using basic formatter"));
        }
    });

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] {
        applyTheme();
        // Highlighter captures theme colors at construction time.
        if (m_highlighter) {
            delete m_highlighter;
            m_highlighter = nullptr;
        }
        ensureHighlighter();
        updateExtraSelections();
        viewport()->update();
        if (m_lineNumberArea) {
            m_lineNumberArea->update();
        }
    });

    m_autoFormatTimer = new QTimer(this);
    m_autoFormatTimer->setSingleShot(true);
    m_autoFormatTimer->setInterval(m_autoFormatDebounceMs);

    connect(this, &QPlainTextEdit::textChanged, this, [this] {
        if (m_internalChange) {
            return;
        }
        if (m_autoFormatTriggerPolicy == FluentCodeEditor::AutoFormatTriggerPolicy::DebouncedOnTextChange) {
            scheduleAutoFormat();
        }
    });

    connect(m_autoFormatTimer, &QTimer::timeout, this, [this] {
        if (!m_autoFormatEnabled) {
            return;
        }
        if (isReadOnly()) {
            return;
        }
        if (m_inPreedit) {
            // User is still composing (IME). Wait until composition finishes.
            m_autoFormatTimer->start();
            return;
        }
        if (document()->characterCount() > m_maxAutoFormatChars) {
            return;
        }
        formatDocumentNow();
    });

    ensureHighlighter();
    updateExtraSelections();

    // Defer theme application until shown (like other Fluent inputs)
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentCodeEditor::applyTheme);
}

void FluentCodeEditor::changeEvent(QEvent *event)
{
    QPlainTextEdit::changeEvent(event);
    if (event && event->type() == QEvent::EnabledChange) {
        applyTheme();
        update();
    }
}

void FluentCodeEditor::applyTheme()
{
    const auto &colors = ThemeManager::instance().colors();
    const bool dark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;
    const QColor textColor = isEnabled() ? colors.text : colors.disabledText;

    QColor selectionBg = colors.accent;
    selectionBg.setAlphaF(dark ? 0.35 : 0.22);
    const QString selectionBgStr = QStringLiteral("rgba(%1,%2,%3,%4)")
                                       .arg(selectionBg.red())
                                       .arg(selectionBg.green())
                                       .arg(selectionBg.blue())
                                       .arg(QString::number(selectionBg.alphaF(), 'f', 3));

    // Let paintEvent draw the Fluent surface/border; keep viewport transparent.
    const QString next = QString(
        "QPlainTextEdit { background: transparent; color: %1; border: none; selection-background-color: %2; selection-color: %3; }"
        "QPlainTextEdit QAbstractScrollArea::viewport { background: transparent; }")
                             .arg(textColor.name())
                             .arg(selectionBgStr)
                             .arg(colors.text.name());

    if (styleSheet() != next) {
        setStyleSheet(next);
    }

    QPalette pal = palette();
    // Keep caret accent like other Fluent inputs; text color itself is controlled by stylesheet.
    pal.setColor(QPalette::Text, colors.accent);
    pal.setColor(QPalette::Highlight, selectionBg);
    pal.setColor(QPalette::HighlightedText, colors.text);
    setPalette(pal);
}

void FluentCodeEditor::paintEvent(QPaintEvent *event)
{
    const auto &colors = ThemeManager::instance().colors();

    QPainter painter(this);
    if (!painter.isActive()) {
        QPlainTextEdit::paintEvent(event);
        return;
    }

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Draw only the Fluent surface fill here; border/focus ring is painted by overlay
    // to avoid being overwritten by QPlainTextEdit internals.
    {
        const auto m = Style::metrics();

        QColor fill = colors.surface;
        if (!isEnabled()) {
            fill = Style::mix(colors.surface, colors.hover, 0.35);
        } else if (m_hoverLevel > 0.0) {
            fill = Style::mix(colors.surface, colors.hover, 0.45 * qBound<qreal>(0.0, m_hoverLevel, 1.0));
        }

        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);

        qreal dpr = 1.0;
        if (painter.device()) {
            dpr = painter.device()->devicePixelRatioF();
            if (dpr <= 0.0) {
                dpr = 1.0;
            }
        }
        const qreal px = 0.5 / dpr;
        const QRectF r = QRectF(rect()).adjusted(px, px, -px, -px);

        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);
        painter.drawPath(Style::roundedRectPath(r, m.radius));
        painter.restore();
    }

    // Keep viewport marked as in paint while QPlainTextEdit paints its viewport.
    struct ScopedViewportPaintFlag {
        QWidget *vp = nullptr;
        bool prev = false;
        explicit ScopedViewportPaintFlag(QWidget *v)
            : vp(v)
        {
            if (!vp) {
                return;
            }
            prev = vp->testAttribute(Qt::WA_WState_InPaintEvent);
            if (!prev) {
                vp->setAttribute(Qt::WA_WState_InPaintEvent, true);
            }
        }
        ~ScopedViewportPaintFlag()
        {
            if (vp && !prev) {
                vp->setAttribute(Qt::WA_WState_InPaintEvent, false);
            }
        }
    } scopedViewportPaint(viewport());

    QPlainTextEdit::paintEvent(event);
}

bool FluentCodeEditor::viewportEvent(QEvent *event)
{
    return QPlainTextEdit::viewportEvent(event);
}

void FluentCodeEditor::inputMethodEvent(QInputMethodEvent *event)
{
    if (event) {
        // While IME preedit is active, avoid auto-formatting; it feels disruptive.
        m_inPreedit = !event->preeditString().isEmpty();
    }
    QPlainTextEdit::inputMethodEvent(event);
}

void FluentCodeEditor::showEvent(QShowEvent *event)
{
    QPlainTextEdit::showEvent(event);
    if (!m_themeAppliedOnce) {
        m_themeAppliedOnce = true;
        applyTheme();
    }
    update();
    if (m_borderOverlay) {
        m_borderOverlay->setGeometry(rect());
        m_borderOverlay->raise();
        m_borderOverlay->show();
        m_borderOverlay->update();
    }
    if (m_lineNumberArea) {
        m_lineNumberArea->raise();
        m_lineNumberArea->show();
        m_lineNumberArea->update();
    }
}

void FluentCodeEditor::startHoverAnimation(qreal to)
{
    if (!m_hoverAnim) {
        m_hoverLevel = qBound<qreal>(0.0, to, 1.0);
        update();
        return;
    }
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(qBound<qreal>(0.0, to, 1.0));
    m_hoverAnim->start();
}

void FluentCodeEditor::startFocusAnimation(qreal to)
{
    if (!m_focusAnim) {
        m_focusLevel = qBound<qreal>(0.0, to, 1.0);
        update();
        return;
    }
    m_focusAnim->stop();
    m_focusAnim->setStartValue(m_focusLevel);
    m_focusAnim->setEndValue(qBound<qreal>(0.0, to, 1.0));
    m_focusAnim->start();
}

void FluentCodeEditor::focusInEvent(QFocusEvent *event)
{
    QPlainTextEdit::focusInEvent(event);
    startFocusAnimation(1.0);
    updateExtraSelections();
}

void FluentCodeEditor::focusOutEvent(QFocusEvent *event)
{
    QPlainTextEdit::focusOutEvent(event);
    startFocusAnimation(0.0);
    updateExtraSelections();

    if (m_autoFormatEnabled && m_autoFormatTriggerPolicy == FluentCodeEditor::AutoFormatTriggerPolicy::OnEnterOrFocusOut) {
        scheduleAutoFormat();
    }
}

void FluentCodeEditor::enterEvent(QEvent *event)
{
    QPlainTextEdit::enterEvent(event);
    startHoverAnimation(1.0);
}

void FluentCodeEditor::leaveEvent(QEvent *event)
{
    QPlainTextEdit::leaveEvent(event);
    startHoverAnimation(0.0);
}

bool FluentCodeEditor::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == viewport() && event) {
        if (event->type() == QEvent::Enter || event->type() == QEvent::HoverEnter) {
            startHoverAnimation(1.0);
        } else if (event->type() == QEvent::Leave || event->type() == QEvent::HoverLeave) {
            startHoverAnimation(0.0);
        }
    }
    return QPlainTextEdit::eventFilter(watched, event);
}

void FluentCodeEditor::setLineNumbersEnabled(bool enabled)
{
    if (m_lineNumbersEnabled == enabled) {
        return;
    }
    m_lineNumbersEnabled = enabled;
    updateLineNumberAreaWidth();
    if (m_lineNumberArea) {
        m_lineNumberArea->setVisible(enabled);
    }
}

bool FluentCodeEditor::lineNumbersEnabled() const
{
    return m_lineNumbersEnabled;
}

void FluentCodeEditor::setCurrentLineHighlightEnabled(bool enabled)
{
    m_currentLineHighlightEnabled = enabled;
    updateExtraSelections();
}

bool FluentCodeEditor::currentLineHighlightEnabled() const
{
    return m_currentLineHighlightEnabled;
}

void FluentCodeEditor::setBracketMatchHighlightEnabled(bool enabled)
{
    m_bracketMatchHighlightEnabled = enabled;
    updateExtraSelections();
}

bool FluentCodeEditor::bracketMatchHighlightEnabled() const
{
    return m_bracketMatchHighlightEnabled;
}

void FluentCodeEditor::ensureHighlighter()
{
    if (!m_cppHighlightingEnabled) {
        if (m_highlighter) {
            delete m_highlighter;
            m_highlighter = nullptr;
        }
        return;
    }

    if (!m_highlighter) {
        m_highlighter = new FluentCppHighlighter(document());
    }
}

void FluentCodeEditor::setCppHighlightingEnabled(bool enabled)
{
    if (m_cppHighlightingEnabled == enabled) {
        return;
    }
    m_cppHighlightingEnabled = enabled;
    ensureHighlighter();
}

bool FluentCodeEditor::cppHighlightingEnabled() const
{
    return m_cppHighlightingEnabled;
}

void FluentCodeEditor::setAutoBraceNewlineEnabled(bool enabled)
{
    m_autoBraceNewlineEnabled = enabled;
}

bool FluentCodeEditor::autoBraceNewlineEnabled() const
{
    return m_autoBraceNewlineEnabled;
}

QString FluentCodeEditor::findDefaultClangFormat()
{
    const QString exe = QStandardPaths::findExecutable(QStringLiteral("clang-format"));
    return exe;
}

void FluentCodeEditor::setClangFormatPath(const QString &path)
{
    const bool oldAvail = clangFormatAvailable();

    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        m_clangFormatPath = findDefaultClangFormat();
    } else {
        // Accept either an absolute path or a program name available on PATH.
        m_clangFormatPath = QStandardPaths::findExecutable(trimmed);
    }

    const bool newAvail = clangFormatAvailable();
    if (oldAvail != newAvail) {
        emit clangFormatAvailabilityChanged(newAvail);
    }
}

QString FluentCodeEditor::clangFormatPath() const
{
    return m_clangFormatPath;
}

bool FluentCodeEditor::clangFormatAvailable() const
{
    if (m_clangFormatPath.isEmpty()) {
        return false;
    }
    const QFileInfo fi(m_clangFormatPath);
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

void FluentCodeEditor::setClangFormatMissingHintEnabled(bool enabled)
{
    m_clangFormatMissingHintEnabled = enabled;
}

bool FluentCodeEditor::clangFormatMissingHintEnabled() const
{
    return m_clangFormatMissingHintEnabled;
}

void FluentCodeEditor::setAutoFormatEnabled(bool enabled)
{
    m_autoFormatEnabled = enabled;
    if (!enabled) {
        m_autoFormatTimer->stop();
    }
}

bool FluentCodeEditor::autoFormatEnabled() const
{
    return m_autoFormatEnabled;
}

void FluentCodeEditor::setAutoFormatDebounceMs(int ms)
{
    ms = qMax(0, ms);
    m_autoFormatDebounceMs = ms;
    m_autoFormatTimer->setInterval(ms);
}

int FluentCodeEditor::autoFormatDebounceMs() const
{
    return m_autoFormatDebounceMs;
}

void FluentCodeEditor::setAutoFormatTriggerPolicy(FluentCodeEditor::AutoFormatTriggerPolicy policy)
{
    if (m_autoFormatTriggerPolicy == policy) {
        return;
    }
    m_autoFormatTriggerPolicy = policy;

    // If we switch away from debounced typing, stop any pending timer.
    if (policy != FluentCodeEditor::AutoFormatTriggerPolicy::DebouncedOnTextChange && m_autoFormatTimer) {
        m_autoFormatTimer->stop();
    }
}

FluentCodeEditor::AutoFormatTriggerPolicy FluentCodeEditor::autoFormatTriggerPolicy() const
{
    return m_autoFormatTriggerPolicy;
}

void FluentCodeEditor::setMaxAutoFormatCharacters(int chars)
{
    m_maxAutoFormatChars = qMax(0, chars);
}

int FluentCodeEditor::maxAutoFormatCharacters() const
{
    return m_maxAutoFormatChars;
}

void FluentCodeEditor::scheduleAutoFormat()
{
    if (!m_autoFormatEnabled) {
        return;
    }
    if (isReadOnly()) {
        return;
    }
    if (m_inPreedit) {
        return;
    }
    m_autoFormatTimer->start();
}

void FluentCodeEditor::formatDocumentNow()
{
    if (m_clangProc && m_clangProc->state() != QProcess::NotRunning) {
        m_formatPending = true;
        return;
    }

    snapshotCursor(this, m_cursorBlock, m_cursorColumn, m_anchorBlock, m_anchorColumn, m_hadSelection);
    m_formatStartRevision = document()->revision();

    if (clangFormatAvailable()) {
        runClangFormatAsync();
        m_manualFormatTriggered = false;
        return;
    }

    if (m_manualFormatTriggered) {
        m_manualFormatTriggered = false;
        if (m_clangFormatMissingHintEnabled && !m_clangFormatHintShown) {
            m_clangFormatHintShown = true;
            QToolTip::showText(mapToGlobal(QPoint(12, 12)),
                               tr("clang-format not found. Falling back to basic formatter.\n"
                                  "Install clang-format (LLVM) or call setClangFormatPath()."),
                               this);
        }
    }

    const QString input = toPlainText();
    const QString formatted = runBasicFormatter(input);
    const bool applied = applyFormattedText(formatted);
    emit formatFinished(applied);
}

int FluentCodeEditor::lineNumberAreaWidth() const
{
    if (!m_lineNumbersEnabled) {
        return 0;
    }

    const int leftPad = 8;
    const int rightPad = 6;

    int digits = 1;
    int maxNum = qMax(1, blockCount());
    while (maxNum >= 10) {
        maxNum /= 10;
        ++digits;
    }
    const int space = leftPad + rightPad + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void FluentCodeEditor::updateLineNumberAreaWidth()
{
    const auto m = Style::metrics();
    setViewportMargins(lineNumberAreaWidth() + m.paddingX, m.paddingY, m.paddingX, m.paddingY);
}

void FluentCodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (!m_lineNumberArea) {
        return;
    }
    const int yOffset = viewport() ? viewport()->geometry().top() : 0;
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y() + yOffset, m_lineNumberArea->width(), rect.height());
    }
    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth();
    }
}

void FluentCodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    if (m_borderOverlay) {
        m_borderOverlay->setGeometry(rect());
    }
    if (!m_lineNumberArea) {
        return;
    }
    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void FluentCodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    if (!m_lineNumbersEnabled) {
        return;
    }

    QPainter painter(m_lineNumberArea);
    painter.setRenderHint(QPainter::Antialiasing, false);

    const auto colors = ThemeManager::instance().colors();
    // Add a subtle gutter tint on top of the surface to read more like an IDE.
    QColor gutter = Style::mix(colors.surface, colors.hover, 0.22);
    gutter.setAlpha(140);
    painter.fillRect(event->rect(), gutter);

    const bool dark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;
    QColor selectionBg = colors.accent;
    selectionBg.setAlphaF(dark ? 0.28 : 0.18);
    QColor currentLineBg = colors.hover;
    currentLineBg.setAlphaF(dark ? 0.22 : 0.18);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    const int yOffset = viewport() ? viewport()->geometry().top() : 0;
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top()) + yOffset;
    int bottom = top + qRound(blockBoundingRect(block).height());

    const int currentLine = textCursor().blockNumber();
    int selStartBlock = -1;
    int selEndBlock = -1;
    {
        const QTextCursor cur = textCursor();
        if (cur.hasSelection()) {
            const int a = cur.anchor();
            const int p = cur.position();
            int s = qMin(a, p);
            int e = qMax(a, p);

            // If selection ends exactly at the start of a block, don't include that next block.
            if (e > 0) {
                const QTextBlock eb = document()->findBlock(e);
                if (eb.isValid() && eb.position() == e) {
                    --e;
                }
            }

            selStartBlock = document()->findBlock(s).blockNumber();
            selEndBlock = document()->findBlock(e).blockNumber();
            if (selStartBlock > selEndBlock) {
                qSwap(selStartBlock, selEndBlock);
            }
        }
    }
    const QFont baseFont = painter.font();
    const int leftPad = 8;
    const int rightPad = 6;

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const QRect lineRect(0, top, m_lineNumberArea->width(), bottom - top);

            const bool inSelection = (selStartBlock >= 0 && blockNumber >= selStartBlock && blockNumber <= selEndBlock);
            if (inSelection) {
                painter.fillRect(lineRect, selectionBg);
            } else if (blockNumber == currentLine) {
                painter.fillRect(lineRect, currentLineBg);
            }

            const QString number = QString::number(blockNumber + 1);

            if (blockNumber == currentLine) {
                painter.setPen(colors.accent);
                QFont f = baseFont;
                f.setWeight(QFont::DemiBold);
                painter.setFont(f);
            } else if (inSelection) {
                painter.setPen(colors.text);
                painter.setFont(baseFont);
            } else {
                painter.setPen(colors.subText);
                painter.setFont(baseFont);
            }

            painter.drawText(leftPad,
                             top,
                             qMax(0, m_lineNumberArea->width() - leftPad - rightPad),
                             fontMetrics().height(),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }

    // subtle divider line
    painter.setPen(colors.border);
    painter.drawLine(m_lineNumberArea->width() - 1, event->rect().top(), m_lineNumberArea->width() - 1, event->rect().bottom());
}

int FluentCodeEditor::findMatchingBracket(int position, QChar bracket) const
{
    if (position < 0 || position >= document()->characterCount()) {
        return -1;
    }
    const QChar open = isOpenBracket(bracket) ? bracket : matchingBracket(bracket);
    const QChar close = isCloseBracket(bracket) ? bracket : matchingBracket(bracket);

    const bool forward = isOpenBracket(bracket);
    int depth = 0;

    const int docLen = document()->characterCount();
    int i = position;

    if (forward) {
        for (i = position + 1; i < docLen; ++i) {
            const QChar c = document()->characterAt(i);
            if (c == open) {
                ++depth;
            } else if (c == close) {
                if (depth == 0) {
                    return i;
                }
                --depth;
            }
        }
    } else {
        for (i = position - 1; i >= 0; --i) {
            const QChar c = document()->characterAt(i);
            if (c == close) {
                ++depth;
            } else if (c == open) {
                if (depth == 0) {
                    return i;
                }
                --depth;
            }
        }
    }
    return -1;
}

void FluentCodeEditor::updateExtraSelections()
{
    QList<QTextEdit::ExtraSelection> selections;
    const auto colors = ThemeManager::instance().colors();

    if (m_currentLineHighlightEnabled && !isReadOnly()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        QColor bg = colors.hover;
        bg.setAlpha(qMin(70, bg.alpha() == 255 ? 70 : bg.alpha()));
        sel.format.setBackground(bg);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        selections.append(sel);
    }

    if (m_bracketMatchHighlightEnabled) {
        const QTextCursor cur = textCursor();
        const int pos = cur.position();
        int bracketPos = -1;
        QChar bracket;

        if (pos > 0) {
            const QChar prev = document()->characterAt(pos - 1);
            if (isOpenBracket(prev) || isCloseBracket(prev)) {
                bracketPos = pos - 1;
                bracket = prev;
            }
        }
        if (bracketPos < 0) {
            const QChar at = document()->characterAt(pos);
            if (isOpenBracket(at) || isCloseBracket(at)) {
                bracketPos = pos;
                bracket = at;
            }
        }

        if (bracketPos >= 0) {
            const int matchPos = findMatchingBracket(bracketPos, bracket);
            auto addMark = [&](int p, const QColor &c) {
                if (p < 0) {
                    return;
                }
                QTextEdit::ExtraSelection s;
                QColor bg = c;
                bg.setAlpha(90);
                s.format.setBackground(bg);
                s.format.setFontWeight(QFont::DemiBold);
                s.cursor = QTextCursor(document());
                s.cursor.setPosition(p);
                s.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);
                selections.append(s);
            };

            addMark(bracketPos, colors.accent);
            if (matchPos >= 0) {
                addMark(matchPos, colors.accent);
            } else {
                addMark(bracketPos, colors.error);
            }
        }
    }

    setExtraSelections(selections);
}

QString FluentCodeEditor::runBasicFormatter(const QString &input) const
{
    // Minimal, fast formatting for when clang-format is not available.
    // - normalize indentation based on brace depth
    // - keep content otherwise unchanged

    QString out;
    out.reserve(input.size());

    const QStringList lines = input.split(QLatin1Char('\n'));

    int indentLevel = 0;
    bool inBlockComment = false;

    auto countBraces = [&](const QString &line, int &openCount, int &closeCount) {
        openCount = 0;
        closeCount = 0;

        bool inString = false;
        bool inChar = false;
        bool escaped = false;

        for (int i = 0; i < line.size(); ++i) {
            const QChar c = line[i];
            const QChar n = (i + 1 < line.size()) ? line[i + 1] : QChar();

            if (!inString && !inChar) {
                if (!inBlockComment && c == QLatin1Char('/') && n == QLatin1Char('*')) {
                    inBlockComment = true;
                    ++i;
                    continue;
                }
                if (inBlockComment && c == QLatin1Char('*') && n == QLatin1Char('/')) {
                    inBlockComment = false;
                    ++i;
                    continue;
                }
                if (!inBlockComment && c == QLatin1Char('/') && n == QLatin1Char('/')) {
                    break;
                }
            }

            if (inBlockComment) {
                continue;
            }

            if (escaped) {
                escaped = false;
                continue;
            }
            if (c == QLatin1Char('\\')) {
                escaped = true;
                continue;
            }
            if (!inChar && c == QLatin1Char('"')) {
                inString = !inString;
                continue;
            }
            if (!inString && c == QLatin1Char('\'')) {
                inChar = !inChar;
                continue;
            }
            if (inString || inChar) {
                continue;
            }

            if (c == QLatin1Char('{')) {
                ++openCount;
            } else if (c == QLatin1Char('}')) {
                ++closeCount;
            }
        }
    };

    for (int idx = 0; idx < lines.size(); ++idx) {
        QString line = lines[idx];

        int openCount = 0;
        int closeCount = 0;
        countBraces(line, openCount, closeCount);

        // If line starts with '}', reduce indentation for this line first.
        QString trimmed = line;
        trimmed.remove(QRegularExpression(QStringLiteral(R"(^\s+)")));

        int lineIndent = indentLevel;
        if (trimmed.startsWith(QLatin1Char('}'))) {
            lineIndent = qMax(0, indentLevel - 1);
        }

        // Preserve empty lines
        if (!trimmed.isEmpty()) {
            out += QString(lineIndent * 4, QLatin1Char(' '));
            out += trimmed;
        }

        if (idx != lines.size() - 1) {
            out += QLatin1Char('\n');
        }

        indentLevel = qMax(0, indentLevel + openCount - closeCount);
    }

    return out;
}

void FluentCodeEditor::runClangFormatAsync()
{
    if (m_clangFormatPath.isEmpty()) {
        return;
    }

    if (!m_clangProc) {
        m_clangProc = new QProcess(this);
        m_clangProc->setProcessChannelMode(QProcess::SeparateChannels);

        connect(m_clangProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
                [this](int exitCode, QProcess::ExitStatus status) {
                    const QByteArray out = m_clangProc->readAllStandardOutput();
                    const bool ok = (status == QProcess::NormalExit && exitCode == 0);
                    bool applied = false;

                    // If user kept typing, don't apply stale output; just reschedule.
                    if (ok && document()->revision() == m_formatStartRevision) {
                        applied = applyFormattedText(QString::fromUtf8(out));
                    } else if (ok) {
                        m_formatPending = true;
                    }

                    emit formatFinished(applied);

                    if (m_formatPending) {
                        m_formatPending = false;
                        scheduleAutoFormat();
                    }
                });
    }

    emit formatStarted();

    m_clangProc->setProgram(m_clangFormatPath);

    // Read from stdin; assume C++ for better defaults.
    QStringList args;
    args << QStringLiteral("-assume-filename=code.cpp");
    m_clangProc->setArguments(args);

    const QByteArray input = toPlainText().toUtf8();
    m_clangProc->start();
    m_clangProc->write(input);
    m_clangProc->closeWriteChannel();
}

bool FluentCodeEditor::applyFormattedText(const QString &formatted)
{
    const QString current = toPlainText();
    if (formatted == current) {
        // still restore (in case selection drifted)
        restoreCursor(this, m_cursorBlock, m_cursorColumn, m_anchorBlock, m_anchorColumn, m_hadSelection);
        return false;
    }

    m_internalChange = true;

    QTextCursor c(document());
    c.beginEditBlock();
    c.select(QTextCursor::Document);
    c.insertText(formatted);
    c.endEditBlock();

    m_internalChange = false;

    restoreCursor(this, m_cursorBlock, m_cursorColumn, m_anchorBlock, m_anchorColumn, m_hadSelection);

    return true;
}

void FluentCodeEditor::keyPressEvent(QKeyEvent *event)
{
    const int key = event ? event->key() : 0;
    const bool isEnterKey = (key == Qt::Key_Return || key == Qt::Key_Enter);

    if (m_autoBraceNewlineEnabled && event->text() == QStringLiteral("{")) {
        QTextCursor c = textCursor();
        c.beginEditBlock();
        c.insertText(QStringLiteral("{"));

        // If next char is not newline, apply a lightweight brace-newline template.
        const int blockPos = c.positionInBlock();
        const QString line = c.block().text();
        const bool atEol = (blockPos >= line.size());
        if (atEol) {
            int firstNonSpace = 0;
            while (firstNonSpace < line.size() && line[firstNonSpace].isSpace()) {
                ++firstNonSpace;
            }
            const QString indent = line.left(firstNonSpace).replace(QRegularExpression(QStringLiteral("\\t")), QStringLiteral("    "));
            c.insertText(QStringLiteral("\n"));
            c.insertText(indent + QStringLiteral("    \n"));
            c.insertText(indent + QStringLiteral("}"));

            // place cursor on the indented empty line
            c.movePosition(QTextCursor::Up);
            c.movePosition(QTextCursor::EndOfLine);
            setTextCursor(c);
        }

        c.endEditBlock();

        if (m_autoFormatTriggerPolicy == FluentCodeEditor::AutoFormatTriggerPolicy::DebouncedOnTextChange) {
            scheduleAutoFormat();
        }
        return;
    }

    QPlainTextEdit::keyPressEvent(event);

    if (isEnterKey && m_autoFormatEnabled && m_autoFormatTriggerPolicy == FluentCodeEditor::AutoFormatTriggerPolicy::OnEnterOrFocusOut) {
        scheduleAutoFormat();
    }
}

} // namespace Fluent
