#pragma once

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QVector>

class QTextDocument;

namespace Fluent {

class FluentCppHighlighter final : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit FluentCppHighlighter(QTextDocument *parent);

    void setOperatorHighlightEnabled(bool enabled);
    bool operatorHighlightEnabled() const;

    void setPreprocessorHighlightEnabled(bool enabled);
    bool preprocessorHighlightEnabled() const;

protected:
    void highlightBlock(const QString &text) override;

private:
    void rebuildRulesIfNeeded();

    bool m_operatorHighlightEnabled = true;
    bool m_preprocessorHighlightEnabled = true;

    struct Rule {
        QRegularExpression re;
        QTextCharFormat fmt;
    };

    bool m_rulesBuilt = false;
    QVector<Rule> m_rules;

    QTextCharFormat m_keywordFmt;
    QTextCharFormat m_typeFmt;
    QTextCharFormat m_numberFmt;
    QTextCharFormat m_stringFmt;
    QTextCharFormat m_charFmt;
    QTextCharFormat m_commentFmt;
    QTextCharFormat m_operatorFmt;
    QTextCharFormat m_preprocessorFmt;
};

} // namespace Fluent
