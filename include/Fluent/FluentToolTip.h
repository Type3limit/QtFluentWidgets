#pragma once

#include <QPoint>
#include <QString>

class QWidget;

namespace Fluent {

class FluentToolTip final
{
public:
    static void ensureInstalled();
    static void showText(const QPoint &globalPos, const QString &text, QWidget *anchor = nullptr, int msecDisplayTime = -1);
    static void hideText();
};

} // namespace Fluent

