#pragma once

#include <QPlainTextEdit>

class QAction;
class QTimer;
class QProcess;
class QVariantAnimation;

namespace Fluent {

class FluentCppHighlighter;
class FluentCodeEditorLineNumberArea;
class FluentCodeEditorBorderOverlay;

class FluentCodeEditor : public QPlainTextEdit
{
    Q_OBJECT
    Q_PROPERTY(QString clangFormatPath READ clangFormatPath WRITE setClangFormatPath)
    Q_PROPERTY(bool autoFormatEnabled READ autoFormatEnabled WRITE setAutoFormatEnabled)
    Q_PROPERTY(int autoFormatDebounceMs READ autoFormatDebounceMs WRITE setAutoFormatDebounceMs)
    Q_PROPERTY(bool lineNumbersEnabled READ lineNumbersEnabled WRITE setLineNumbersEnabled)
    Q_PROPERTY(AutoFormatTriggerPolicy autoFormatTriggerPolicy READ autoFormatTriggerPolicy WRITE setAutoFormatTriggerPolicy)
public:
    enum class AutoFormatTriggerPolicy {
        DebouncedOnTextChange = 0,
        OnEnterOrFocusOut = 1,
    };
    Q_ENUM(AutoFormatTriggerPolicy)

    explicit FluentCodeEditor(QWidget *parent = nullptr);

    // IDE-like visuals
    void setLineNumbersEnabled(bool enabled);
    bool lineNumbersEnabled() const;

    void setCurrentLineHighlightEnabled(bool enabled);
    bool currentLineHighlightEnabled() const;

    void setBracketMatchHighlightEnabled(bool enabled);
    bool bracketMatchHighlightEnabled() const;

    // Highlighter
    void setCppHighlightingEnabled(bool enabled);
    bool cppHighlightingEnabled() const;

    // Lightweight editor behaviors (work even without clang-format)
    void setAutoBraceNewlineEnabled(bool enabled);
    bool autoBraceNewlineEnabled() const;

    // clang-format integration
    void setClangFormatPath(const QString &path);
    QString clangFormatPath() const;
    bool clangFormatAvailable() const;

    void setClangFormatMissingHintEnabled(bool enabled);
    bool clangFormatMissingHintEnabled() const;

    // Auto formatting (debounced)
    void setAutoFormatEnabled(bool enabled);
    bool autoFormatEnabled() const;

    void setAutoFormatDebounceMs(int ms);
    int autoFormatDebounceMs() const;

    void setAutoFormatTriggerPolicy(AutoFormatTriggerPolicy policy);
    AutoFormatTriggerPolicy autoFormatTriggerPolicy() const;

    void setMaxAutoFormatCharacters(int chars);
    int maxAutoFormatCharacters() const;

public slots:
    void formatDocumentNow();

signals:
    void clangFormatAvailabilityChanged(bool available);
    void formatStarted();
    void formatFinished(bool applied);

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool viewportEvent(QEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    friend class FluentCodeEditorLineNumberArea;
    friend class FluentCodeEditorBorderOverlay;

    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void updateExtraSelections();
    int findMatchingBracket(int position, QChar bracket) const;

    void applyTheme();
    void startHoverAnimation(qreal to);
    void startFocusAnimation(qreal to);

    void ensureHighlighter();
    void scheduleAutoFormat();
    void runClangFormatAsync();
    bool applyFormattedText(const QString &formatted);

    static QString findDefaultClangFormat();
    QString runBasicFormatter(const QString &input) const;

    bool m_cppHighlightingEnabled = true;
    bool m_autoBraceNewlineEnabled = true;

    bool m_lineNumbersEnabled = true;
    bool m_currentLineHighlightEnabled = true;
    bool m_bracketMatchHighlightEnabled = true;

    FluentCppHighlighter *m_highlighter = nullptr;

    QWidget *m_lineNumberArea = nullptr;
    QWidget *m_borderOverlay = nullptr;

    QString m_clangFormatPath;

    bool m_clangFormatMissingHintEnabled = true;
    bool m_clangFormatHintShown = false;

    QAction *m_formatAction = nullptr;
    bool m_manualFormatTriggered = false;

    bool m_autoFormatEnabled = true;
    int m_autoFormatDebounceMs = 1200;
    int m_maxAutoFormatChars = 120000;
    AutoFormatTriggerPolicy m_autoFormatTriggerPolicy = AutoFormatTriggerPolicy::DebouncedOnTextChange;

    QTimer *m_autoFormatTimer = nullptr;
    QProcess *m_clangProc = nullptr;
    bool m_internalChange = false;
    bool m_formatPending = false;
    int m_formatStartRevision = 0;

    bool m_inPreedit = false;

    qreal m_hoverLevel = 0.0;
    qreal m_focusLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_focusAnim = nullptr;

    bool m_themeAppliedOnce = false;

    // cursor restore snapshot
    int m_cursorBlock = 0;
    int m_cursorColumn = 0;
    int m_anchorBlock = 0;
    int m_anchorColumn = 0;
    bool m_hadSelection = false;
};

} // namespace Fluent
