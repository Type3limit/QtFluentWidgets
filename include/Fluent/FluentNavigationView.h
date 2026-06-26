#pragma once

#include "Fluent/FluentExport.h"

#include "Fluent/FluentIcon.h"
#include "Fluent/FluentQtCompat.h"

#include <QByteArray>
#include <QIcon>
#include <QString>
#include <QWidget>

#include <functional>
#include <memory>
#include <vector>

class QPropertyAnimation;
class QVariantAnimation;

namespace Fluent {

// ---------------------------------------------------------------------------
// Data model for navigation items
// ---------------------------------------------------------------------------

struct FLUENT_EXPORT FluentNavigationItem
{
    QString key;           // unique identifier
    QString text;          // display label
    QIcon   icon;          // optional icon (shown in both compact and expanded)
    bool hasFluentIcon = false; // optional built-in SVG icon rendered with current theme
    FluentIconType fluentIcon = FluentIconType::Info;
    QString iconGlyph;     // optional monochrome glyph icon (preferred for font icons)
    QString iconFontFamily; // optional font family for iconGlyph, defaults to Segoe Fluent Icons
    QString animatedIconSource; // optional Lottie JSON path / qrc resource used instead of icon/iconGlyph
    QByteArray animatedIconData; // optional in-memory Lottie JSON used instead of animatedIconSource
    QString animatedIconCacheKey; // optional cache key for animatedIconData
    QString animatedIconResourcePath; // optional resource path for animatedIconData image assets
    bool animatedIconLooping = false; // loop while active instead of playing once on hover/selection
    bool    separator = false; // if true, rendered as a horizontal line (text/icon ignored)
    bool    selectsOnInvoked = true; // if false, clicking invokes without changing selectedKey

    std::vector<FluentNavigationItem> children; // sub-items (arbitrary nesting supported)
};

// ---------------------------------------------------------------------------
// FluentNavigationView – hierarchical navigation sidebar
// ---------------------------------------------------------------------------

class FLUENT_EXPORT FluentNavigationView : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int expandedWidth READ expandedWidth WRITE setExpandedWidth)
    Q_PROPERTY(int compactWidth READ compactWidth WRITE setCompactWidth)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(PaneDisplayMode paneDisplayMode READ paneDisplayMode WRITE setPaneDisplayMode NOTIFY paneDisplayModeChanged)
    Q_PROPERTY(bool backButtonVisible READ isBackButtonVisible WRITE setBackButtonVisible NOTIFY backButtonVisibleChanged)
    Q_PROPERTY(bool backButtonEnabled READ isBackButtonEnabled WRITE setBackButtonEnabled NOTIFY backButtonEnabledChanged)
    Q_PROPERTY(bool footerVisible READ isFooterVisible WRITE setFooterVisible NOTIFY footerVisibleChanged)
    Q_PROPERTY(QString paneTitle READ paneTitle WRITE setPaneTitle NOTIFY paneTitleChanged)
public:
    enum PaneDisplayMode {
        Left,
        LeftCompact,
        Top,
    };
    Q_ENUM(PaneDisplayMode)

    explicit FluentNavigationView(QWidget *parent = nullptr);
    ~FluentNavigationView() override;

    // --- Item management ---------------------------------------------------
    void setItems(const std::vector<FluentNavigationItem> &items);
    std::vector<FluentNavigationItem> items() const;
    void addItem(const FluentNavigationItem &item);
    void clearItems();

    void setFooterItems(const std::vector<FluentNavigationItem> &items);
    std::vector<FluentNavigationItem> footerItems() const;
    void addFooterItem(const FluentNavigationItem &item);
    void clearFooterItems();

    // --- Selection ---------------------------------------------------------
    QString selectedKey() const;
    void    setSelectedKey(const QString &key);

    // --- Pane mode ---------------------------------------------------------
    PaneDisplayMode paneDisplayMode() const;
    void setPaneDisplayMode(PaneDisplayMode mode);

    // --- Expand / Collapse -------------------------------------------------
    bool isExpanded() const;
    void setExpanded(bool expanded);
    void toggleExpanded();

    int expandedWidth() const;
    void setExpandedWidth(int w);

    int compactWidth() const;
    void setCompactWidth(int w);

    // --- Header (optional widget above items, e.g. a title) ----------------
    void setHeaderWidget(QWidget *widget);

    // --- Pane chrome motion ------------------------------------------------
    bool loadPaneToggleAnimation(const QString &path);
    bool loadPaneToggleAnimationData(const QByteArray &json,
                                     const QString &cacheKey = QString(),
                                     const QString &resourcePath = QString());
    void clearPaneToggleAnimation();

    bool loadBackButtonAnimation(const QString &path);
    bool loadBackButtonAnimationData(const QByteArray &json,
                                     const QString &cacheKey = QString(),
                                     const QString &resourcePath = QString());
    void clearBackButtonAnimation();

    // --- Pane chrome -------------------------------------------------------
    bool isBackButtonVisible() const;
    void setBackButtonVisible(bool visible);

    bool isBackButtonEnabled() const;
    void setBackButtonEnabled(bool enabled);

    bool isFooterVisible() const;
    void setFooterVisible(bool visible);

    QString paneTitle() const;
    void setPaneTitle(const QString &title);

    // --- Auto-collapse on narrow window ------------------------------------
    void setAutoCollapseWidth(int threshold);
    int  autoCollapseWidth() const;

signals:
    void selectedKeyChanged(const QString &key);
    void itemInvoked(const QString &key);
    void expandedChanged(bool expanded);
    void paneDisplayModeChanged(PaneDisplayMode mode);
    void backRequested();
    void backButtonVisibleChanged(bool visible);
    void backButtonEnabledChanged(bool enabled);
    void footerVisibleChanged(bool visible);
    void paneTitleChanged(const QString &title);

protected:
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool event(QEvent *event) override;

private:
    void syncAutoCollapseState();
    void updateParentEventFilter();
    void updateModeGeometry();

    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace Fluent
