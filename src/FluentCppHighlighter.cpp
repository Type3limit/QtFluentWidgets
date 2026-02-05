#include "Fluent/FluentCppHighlighter.h"

#include "Fluent/FluentTheme.h"

#include <QRegularExpression>
#include <QTextDocument>

namespace Fluent {

static QStringList cppKeywords()
{
    return {
        // C++ keywords (common set)
        QStringLiteral("alignas"), QStringLiteral("alignof"), QStringLiteral("and"), QStringLiteral("and_eq"),
        QStringLiteral("asm"), QStringLiteral("auto"), QStringLiteral("bitand"), QStringLiteral("bitor"),
        QStringLiteral("bool"), QStringLiteral("break"), QStringLiteral("case"), QStringLiteral("catch"),
        QStringLiteral("char"), QStringLiteral("char8_t"), QStringLiteral("char16_t"), QStringLiteral("char32_t"),
        QStringLiteral("class"), QStringLiteral("compl"), QStringLiteral("concept"), QStringLiteral("const"),
        QStringLiteral("consteval"), QStringLiteral("constexpr"), QStringLiteral("constinit"), QStringLiteral("const_cast"),
        QStringLiteral("continue"), QStringLiteral("co_await"), QStringLiteral("co_return"), QStringLiteral("co_yield"),
        QStringLiteral("decltype"), QStringLiteral("default"), QStringLiteral("delete"), QStringLiteral("do"),
        QStringLiteral("double"), QStringLiteral("dynamic_cast"), QStringLiteral("else"), QStringLiteral("enum"),
        QStringLiteral("explicit"), QStringLiteral("export"), QStringLiteral("extern"), QStringLiteral("false"),
        QStringLiteral("float"), QStringLiteral("for"), QStringLiteral("friend"), QStringLiteral("goto"),
        QStringLiteral("if"), QStringLiteral("inline"), QStringLiteral("int"), QStringLiteral("long"),
        QStringLiteral("mutable"), QStringLiteral("namespace"), QStringLiteral("new"), QStringLiteral("noexcept"),
        QStringLiteral("not"), QStringLiteral("not_eq"), QStringLiteral("nullptr"), QStringLiteral("operator"),
        QStringLiteral("or"), QStringLiteral("or_eq"), QStringLiteral("private"), QStringLiteral("protected"),
        QStringLiteral("public"), QStringLiteral("register"), QStringLiteral("reinterpret_cast"), QStringLiteral("requires"),
        QStringLiteral("return"), QStringLiteral("short"), QStringLiteral("signed"), QStringLiteral("sizeof"),
        QStringLiteral("static"), QStringLiteral("static_assert"), QStringLiteral("static_cast"), QStringLiteral("struct"),
        QStringLiteral("switch"), QStringLiteral("template"), QStringLiteral("this"), QStringLiteral("thread_local"),
        QStringLiteral("throw"), QStringLiteral("true"), QStringLiteral("try"), QStringLiteral("typedef"),
        QStringLiteral("typeid"), QStringLiteral("typename"), QStringLiteral("union"), QStringLiteral("unsigned"),
        QStringLiteral("using"), QStringLiteral("virtual"), QStringLiteral("void"), QStringLiteral("volatile"),
        QStringLiteral("wchar_t"), QStringLiteral("while"), QStringLiteral("xor"), QStringLiteral("xor_eq")
    };
}

static QStringList cppTypes()
{
    return {
        QStringLiteral("size_t"), QStringLiteral("ssize_t"),
        QStringLiteral("uint8_t"), QStringLiteral("uint16_t"), QStringLiteral("uint32_t"), QStringLiteral("uint64_t"),
        QStringLiteral("int8_t"), QStringLiteral("int16_t"), QStringLiteral("int32_t"), QStringLiteral("int64_t"),
        QStringLiteral("QString"), QStringLiteral("QWidget"), QStringLiteral("QObject"), QStringLiteral("QVector"),
        QStringLiteral("QList"), QStringLiteral("QMap"), QStringLiteral("QHash"), QStringLiteral("QRect"),
        QStringLiteral("QSize"), QStringLiteral("QPoint"), QStringLiteral("QColor")
    };
}

FluentCppHighlighter::FluentCppHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    auto colors = ThemeManager::instance().colors();

    m_keywordFmt.setForeground(colors.accent);
    m_keywordFmt.setFontWeight(QFont::DemiBold);

    m_typeFmt.setForeground(colors.text);
    m_typeFmt.setFontWeight(QFont::DemiBold);

    m_numberFmt.setForeground(colors.subText);

    m_stringFmt.setForeground(colors.text);

    m_charFmt.setForeground(colors.text);

    m_commentFmt.setForeground(colors.subText);
    m_commentFmt.setFontItalic(true);

    m_operatorFmt.setForeground(colors.subText);

    m_preprocessorFmt.setForeground(colors.accent);

    rebuildRulesIfNeeded();
}

void FluentCppHighlighter::setOperatorHighlightEnabled(bool enabled)
{
    if (m_operatorHighlightEnabled == enabled) {
        return;
    }
    m_operatorHighlightEnabled = enabled;
    m_rulesBuilt = false;
    rehighlight();
}

bool FluentCppHighlighter::operatorHighlightEnabled() const
{
    return m_operatorHighlightEnabled;
}

void FluentCppHighlighter::setPreprocessorHighlightEnabled(bool enabled)
{
    if (m_preprocessorHighlightEnabled == enabled) {
        return;
    }
    m_preprocessorHighlightEnabled = enabled;
    m_rulesBuilt = false;
    rehighlight();
}

bool FluentCppHighlighter::preprocessorHighlightEnabled() const
{
    return m_preprocessorHighlightEnabled;
}

void FluentCppHighlighter::rebuildRulesIfNeeded()
{
    if (m_rulesBuilt) {
        return;
    }

    m_rules.clear();

    // Strings / chars (basic; comments handled separately)
    m_rules.push_back({QRegularExpression(QStringLiteral(R"("([^"\\]|\\.)*")")), m_stringFmt});
    m_rules.push_back({QRegularExpression(QStringLiteral(R"('([^'\\]|\\.)*')")), m_charFmt});

    // Numbers
    m_rules.push_back({QRegularExpression(QStringLiteral(R"(\b(0x[0-9A-Fa-f']+|\d[\d']*(\.\d[\d']*)?)([uUlLfF]*)\b)")),
                       m_numberFmt});

    // Keywords
    {
        const auto kws = cppKeywords();
        QString pat = QStringLiteral("\\b(");
        for (int i = 0; i < kws.size(); ++i) {
            if (i) {
                pat += QLatin1Char('|');
            }
            pat += QRegularExpression::escape(kws[i]);
        }
        pat += QStringLiteral(")\\b");
        m_rules.push_back({QRegularExpression(pat), m_keywordFmt});
    }

    // Some common types
    {
        const auto tys = cppTypes();
        QString pat = QStringLiteral("\\b(");
        for (int i = 0; i < tys.size(); ++i) {
            if (i) {
                pat += QLatin1Char('|');
            }
            pat += QRegularExpression::escape(tys[i]);
        }
        pat += QStringLiteral(")\\b");
        m_rules.push_back({QRegularExpression(pat), m_typeFmt});
    }

    // Preprocessor
    if (m_preprocessorHighlightEnabled) {
        m_rules.push_back({QRegularExpression(QStringLiteral(R"(^\s*#\s*\w+.*$)")), m_preprocessorFmt});
    }

    // Operators
    if (m_operatorHighlightEnabled) {
        m_rules.push_back({QRegularExpression(QStringLiteral(R"([+\-*/%=&|^!<>]=?|::|->\*?|\?\:|\.|,|;|\(|\)|\[|\]|\{|\})")),
                           m_operatorFmt});
    }

    m_rulesBuilt = true;
}

void FluentCppHighlighter::highlightBlock(const QString &text)
{
    rebuildRulesIfNeeded();

    // Handle multi-line comments with block state
    // state 1: inside /* */ comment
    int state = previousBlockState();
    int start = 0;

    auto applyInlineComment = [&](int from) {
        if (from >= 0 && from < text.size()) {
            setFormat(from, text.size() - from, m_commentFmt);
        }
    };

    // If we are already inside a block comment
    if (state == 1) {
        int end = text.indexOf(QStringLiteral("*/"));
        if (end < 0) {
            setFormat(0, text.size(), m_commentFmt);
            setCurrentBlockState(1);
            return;
        }
        setFormat(0, end + 2, m_commentFmt);
        start = end + 2;
        state = 0;
    }

    // Apply rules first (they'll be overwritten by comment formats as needed)
    for (const auto &r : m_rules) {
        auto it = r.re.globalMatch(text);
        while (it.hasNext()) {
            const auto m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), r.fmt);
        }
    }

    // Single-line comments (//)
    {
        int idx = -1;
        bool inString = false;
        bool escaped = false;
        for (int i = 0; i < text.size(); ++i) {
            const QChar c = text[i];
            if (escaped) {
                escaped = false;
                continue;
            }
            if (c == QLatin1Char('\\')) {
                escaped = true;
                continue;
            }
            if (c == QLatin1Char('"')) {
                inString = !inString;
                continue;
            }
            if (!inString && c == QLatin1Char('/') && i + 1 < text.size() && text[i + 1] == QLatin1Char('/')) {
                idx = i;
                break;
            }
        }
        if (idx >= 0) {
            applyInlineComment(idx);
        }
    }

    // Multi-line comments /* */
    int i = start;
    while (i < text.size()) {
        int open = text.indexOf(QStringLiteral("/*"), i);
        if (open < 0) {
            break;
        }
        int close = text.indexOf(QStringLiteral("*/"), open + 2);
        if (close < 0) {
            setFormat(open, text.size() - open, m_commentFmt);
            setCurrentBlockState(1);
            return;
        }
        setFormat(open, close + 2 - open, m_commentFmt);
        i = close + 2;
    }

    setCurrentBlockState(0);
}

} // namespace Fluent
