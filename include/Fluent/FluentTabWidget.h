#pragma once

#include <QTabWidget>

namespace Fluent {

class FluentTabWidget final : public QTabWidget
{
    Q_OBJECT
public:
    enum class TabDisplayMode {
        Underline,
        Document
    };
    Q_ENUM(TabDisplayMode)
    Q_PROPERTY(TabDisplayMode tabDisplayMode READ tabDisplayMode WRITE setTabDisplayMode)
    Q_PROPERTY(int contentMargin READ contentMargin WRITE setContentMargin)

    explicit FluentTabWidget(QWidget *parent = nullptr);

    TabDisplayMode tabDisplayMode() const;
    void setTabDisplayMode(TabDisplayMode mode);

    int contentMargin() const;
    void setContentMargin(int margin);

protected:
    bool event(QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void tabInserted(int index) override;
    void tabRemoved(int index) override;

private:
    void applyTheme();
    void applyContentMargins();
    void applyContentMarginToPage(int index);
    void scheduleDocumentCornerWidgetPosition();
    void positionDocumentCornerWidget();

    QWidget *m_frameOverlay = nullptr;
    int m_contentMargin = 5;
    bool m_documentCornerPositionPending = false;
};

} // namespace Fluent
