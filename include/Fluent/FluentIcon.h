#pragma once

#include "Fluent/FluentExport.h"

#include <QColor>
#include <QIcon>
#include <QRectF>
#include <QSize>
#include <QString>

class QPainter;

namespace Fluent {

enum class FluentIconType {
    Add,
    ArrowDown,
    ArrowUp,
    Back,
    Bell,
    Bookmark,
    Calendar,
    Checkmark,
    ChevronDown,
    ChevronLeft,
    ChevronRight,
    ChevronUp,
    Close,
    Controls,
    Copy,
    Data,
    Delete,
    Dialog,
    Document,
    Download,
    Edit,
    Eye,
    Filter,
    Folder,
    Gauge,
    Heart,
    Home,
    Icons,
    Info,
    Input,
    Layout,
    Link,
    Lock,
    Mail,
    Menu,
    More,
    Person,
    Pin,
    Play,
    Refresh,
    Save,
    Search,
    Settings,
    Share,
    Sort,
    Star,
    Success,
    Upload,
    Warning,
    Window,
    ZoomIn,
    ZoomOut,
};

struct FLUENT_EXPORT FluentIconOptions {
    QColor color;
    bool autoTheme = true;
    bool reversed = false;
    qreal opacity = 1.0;
};

class FLUENT_EXPORT FluentIcon final
{
public:
    explicit FluentIcon(FluentIconType type = FluentIconType::Info);

    FluentIconType type() const;

    QIcon toQIcon(const FluentIconOptions &options = FluentIconOptions()) const;
    void paint(QPainter *painter,
               const QRectF &rect,
               const FluentIconOptions &options = FluentIconOptions(),
               QIcon::Mode mode = QIcon::Normal) const;

    static QIcon icon(FluentIconType type, const FluentIconOptions &options = FluentIconOptions());
    static void paintIcon(QPainter *painter,
                          FluentIconType type,
                          const QRectF &rect,
                          const FluentIconOptions &options = FluentIconOptions(),
                          QIcon::Mode mode = QIcon::Normal);
    static QColor resolveColor(const FluentIconOptions &options, QIcon::Mode mode = QIcon::Normal);
    static QString resourcePath(FluentIconType type);

private:
    FluentIconType m_type;
};

} // namespace Fluent
