#pragma once

#include "Fluent/FluentExport.h"

#include <QStringList>
#include <QWidget>

class QHBoxLayout;

namespace Fluent {

class FluentAutoSuggestPopup;
class FluentLineEdit;
class FluentToolButton;

class FLUENT_EXPORT FluentAutoSuggestBox : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText)
    Q_PROPERTY(QStringList suggestions READ suggestions WRITE setSuggestions)
    Q_PROPERTY(bool searchButtonVisible READ isSearchButtonVisible WRITE setSearchButtonVisible)
    Q_PROPERTY(QString searchButtonText READ searchButtonText WRITE setSearchButtonText)

public:
    explicit FluentAutoSuggestBox(QWidget *parent = nullptr);

    FluentLineEdit *lineEdit() const;

    QString text() const;
    void setText(const QString &text);

    QString placeholderText() const;
    void setPlaceholderText(const QString &text);

    QStringList suggestions() const;
    void setSuggestions(const QStringList &suggestions);

    bool isSearchButtonVisible() const;
    void setSearchButtonVisible(bool visible);

    QString searchButtonText() const;
    void setSearchButtonText(const QString &text);

signals:
    void textChanged(const QString &text);
    void suggestionChosen(const QString &text);
    void submitted(const QString &text);
    void searchRequested(const QString &text);

protected:
    FluentToolButton *searchButton() const;
    void changeEvent(QEvent *event) override;

private:
    friend class FluentAutoSuggestPopup;

    void applyTheme();
    void acceptSuggestion(const QString &text);
    QStringList filteredSuggestions() const;
    void submit();
    void updatePopup();

    QHBoxLayout *m_layout = nullptr;
    FluentLineEdit *m_lineEdit = nullptr;
    FluentToolButton *m_searchButton = nullptr;
    FluentAutoSuggestPopup *m_popup = nullptr;
    QStringList m_suggestions;
};

class FLUENT_EXPORT FluentSearchBox final : public FluentAutoSuggestBox
{
    Q_OBJECT
public:
    explicit FluentSearchBox(QWidget *parent = nullptr);
};

} // namespace Fluent
