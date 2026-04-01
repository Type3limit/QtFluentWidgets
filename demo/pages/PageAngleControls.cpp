#include "PageAngleControls.h"

#include "../DemoHelpers.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include "Fluent/FluentAngleSelector.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentDial.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentScrollArea.h"

namespace Demo::Pages {

using namespace Fluent;

QWidget *createAngleControlsPage(FluentMainWindow *window)
{
    Q_UNUSED(window)

    return Demo::makePage([&](QVBoxLayout *page) {
        auto s = Demo::makeSection(QStringLiteral("角度控件"),
                                   QStringLiteral("FluentDial / FluentAngleSelector：指针、高亮、复合编辑器"));
        page->addWidget(s.card);

        {
            QString code;
#define ANGLE_DIALS(X) \
    X(auto *row = new QHBoxLayout(); ) \
    X(row->setContentsMargins(0, 0, 0, 0); ) \
    X(row->setSpacing(14); ) \
    X(auto *dialA = new FluentDial(); ) \
    X(dialA->setValue(30); ) \
    X(auto *dialB = new FluentDial(); ) \
    X(dialB->setValue(135); ) \
    X(auto *dialC = new FluentDial(); ) \
    X(dialC->setValue(270); ) \
    X(row->addWidget(dialA); ) \
    X(row->addWidget(dialB); ) \
    X(row->addWidget(dialC); ) \
    X(row->addStretch(1); ) \
    X(body->addLayout(row); )
#define X(line) code += QStringLiteral(#line "\n");
            ANGLE_DIALS(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentDial"),
                QStringLiteral("基础角度旋钮"),
                QStringLiteral("要点：\n"
                               "-角度指针\n"
                               "-hover/focus 高亮"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    ANGLE_DIALS(X)
#undef X
                },
                false,
                170));

#undef ANGLE_DIALS
        }

        {
            QString code;
#define ANGLE_SELECTOR_BASIC(X) \
    X(auto *row = new QHBoxLayout(); ) \
    X(row->setContentsMargins(0, 0, 0, 0); ) \
    X(row->setSpacing(12); ) \
    X(auto *editor = new FluentAngleSelector(); ) \
    X(editor->setValue(128); ) \
    X(auto *valueLabel = new FluentLabel(QStringLiteral("128°")); ) \
    X(valueLabel->setStyleSheet("font-size: 12px; opacity: 0.85;"); ) \
    X(QObject::connect(editor, &FluentAngleSelector::valueChanged, valueLabel, [valueLabel](int v) { valueLabel->setText(QString::number(v) + QStringLiteral("°")); }); ) \
    X(row->addWidget(editor); ) \
    X(row->addWidget(valueLabel); ) \
    X(row->addStretch(1); ) \
    X(body->addLayout(row); )
#define X(line) code += QStringLiteral(#line "\n");
            ANGLE_SELECTOR_BASIC(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentAngleSelector"),
                QStringLiteral("复合角度编辑器"),
                QStringLiteral("要点：\n"
                               "-标签 + Dial + SpinBox 一体化\n"
                               "-只暴露一个 valueChanged(int)\n"
                               "-可直接复用到渐变、图形旋转等场景"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    ANGLE_SELECTOR_BASIC(X)
#undef X
                },
                false,
                180));

#undef ANGLE_SELECTOR_BASIC
        }

        {
            QString code;
#define ANGLE_SELECTOR_VARIANTS(X) \
    X(auto *col = new QVBoxLayout(); ) \
    X(col->setContentsMargins(0, 0, 0, 0); ) \
    X(col->setSpacing(10); ) \
    X(auto *withLabel = new FluentAngleSelector(); ) \
    X(withLabel->setValue(45); ) \
    X(auto *dialOnly = new FluentAngleSelector(); ) \
    X(dialOnly->setValue(210); ) \
    X(dialOnly->setLabelVisible(false); ) \
    X(auto *spinOnly = new FluentAngleSelector(); ) \
    X(spinOnly->setValue(300); ) \
    X(spinOnly->setDialVisible(false); ) \
    X(col->addWidget(withLabel); ) \
    X(col->addWidget(dialOnly); ) \
    X(col->addWidget(spinOnly); ) \
    X(body->addLayout(col); )
#define X(line) code += QStringLiteral(#line "\n");
            ANGLE_SELECTOR_VARIANTS(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentAngleSelector（可见性变体）"),
                QStringLiteral("支持单独隐藏标签或 Dial"),
                QStringLiteral("要点：\n"
                               "-setLabelVisible(bool)\n"
                               "-setDialVisible(bool)\n"
                               "-适应紧凑布局和无标题表单"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    ANGLE_SELECTOR_VARIANTS(X)
#undef X
                },
                true,
                190));

#undef ANGLE_SELECTOR_VARIANTS
        }
    });
}

} // namespace Demo::Pages


