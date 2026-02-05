#include "DemoThemePanel.h"

#include "../DemoHelpers.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <QVBoxLayout>

#include "Fluent/FluentButton.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentColorDialog.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentFlowLayout.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentSlider.h"
#include "Fluent/FluentSpinBox.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentToggleSwitch.h"

namespace Demo {

using namespace Fluent;

namespace {

class ColorSwatch final : public QWidget
{
public:
    explicit ColorSwatch(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(26, 18);
    }

    void setColor(const QColor &c)
    {
        if (m_color == c) {
            return;
        }
        m_color = c;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)
        QPainter p(this);
        if (!p.isActive()) {
            return;
        }
        p.setRenderHint(QPainter::Antialiasing, true);

        const auto &tc = ThemeManager::instance().colors();
        QColor border = tc.border;
        border.setAlpha(200);

        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        p.setPen(QPen(border, 1.0));
        p.setBrush(m_color);
        p.drawRoundedRect(r, 4.0, 4.0);
    }

private:
    QColor m_color;
};

static QWidget *makeAccentBorderAnimWidget(QWidget *parent)
{
    auto *w = new QWidget(parent);
    auto *layout = new QVBoxLayout(w);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    const auto initial = Style::windowMetrics();

    auto *durSlider = new FluentSlider(Qt::Horizontal);
    durSlider->setRange(150, 1600);
    durSlider->setValue(initial.accentBorderTraceDurationMs);
    durSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *durSpin = new FluentSpinBox();
    durSpin->setRange(150, 3000);
    durSpin->setValue(initial.accentBorderTraceDurationMs);
    durSpin->setSuffix(QStringLiteral(" ms"));
    durSpin->setFixedWidth(96);

    auto *overSlider = new FluentSlider(Qt::Horizontal);
    overSlider->setRange(0, 10);
    overSlider->setValue(qRound(initial.accentBorderTraceEnableOvershoot * 100.0));
    overSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *overSpin = new FluentDoubleSpinBox();
    overSpin->setDecimals(2);
    overSpin->setRange(0.00, 0.10);
    overSpin->setSingleStep(0.01);
    overSpin->setValue(initial.accentBorderTraceEnableOvershoot);
    overSpin->setFixedWidth(96);

    auto *atSlider = new FluentSlider(Qt::Horizontal);
    atSlider->setRange(50, 98);
    atSlider->setValue(qRound(initial.accentBorderTraceEnableOvershootAt * 100.0));
    atSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *atSpin = new FluentDoubleSpinBox();
    atSpin->setDecimals(2);
    atSpin->setRange(0.50, 0.98);
    atSpin->setSingleStep(0.01);
    atSpin->setValue(initial.accentBorderTraceEnableOvershootAt);
    atSpin->setFixedWidth(96);

    auto applyAnim = [durSpin, overSpin, atSpin]() {
        auto m = Style::windowMetrics();
        m.accentBorderTraceDurationMs = durSpin->value();
        m.accentBorderTraceEnableOvershoot = overSpin->value();
        m.accentBorderTraceEnableOvershootAt = atSpin->value();
        Style::setWindowMetrics(m);
    };

    auto makeRow = [&](const QString &name, QWidget *left, QWidget *right) {
        auto *rowW = new QWidget(w);
        auto *rowL = new QHBoxLayout(rowW);
        rowL->setContentsMargins(0, 0, 0, 0);
        rowL->setSpacing(8);
        rowL->addWidget(new FluentLabel(name));
        rowL->addWidget(left, 1);
        rowL->addWidget(right);
        layout->addWidget(rowW);
    };

    makeRow(QStringLiteral("时长"), durSlider, durSpin);
    makeRow(QStringLiteral("弹性"), overSlider, overSpin);
    makeRow(QStringLiteral("弹性点"), atSlider, atSpin);

    QObject::connect(durSlider, &QSlider::valueChanged, w, [durSpin](int v) {
        if (durSpin->value() != v) {
            durSpin->setValue(v);
        }
    });
    QObject::connect(durSpin, qOverload<int>(&QSpinBox::valueChanged), w, [durSlider, applyAnim](int v) {
        if (durSlider->value() != v) {
            durSlider->setValue(v);
        }
        applyAnim();
    });

    QObject::connect(overSlider, &QSlider::valueChanged, w, [overSpin](int v) {
        const double ov = v / 100.0;
        if (!qFuzzyCompare(overSpin->value() + 1.0, ov + 1.0)) {
            overSpin->setValue(ov);
        }
    });
    QObject::connect(overSpin,
                     qOverload<double>(&QDoubleSpinBox::valueChanged),
                     w,
                     [overSlider, applyAnim](double v) {
                         const int sv = qRound(v * 100.0);
                         if (overSlider->value() != sv) {
                             overSlider->setValue(sv);
                         }
                         applyAnim();
                     });

    QObject::connect(atSlider, &QSlider::valueChanged, w, [atSpin](int v) {
        const double av = v / 100.0;
        if (!qFuzzyCompare(atSpin->value() + 1.0, av + 1.0)) {
            atSpin->setValue(av);
        }
    });
    QObject::connect(atSpin,
                     qOverload<double>(&QDoubleSpinBox::valueChanged),
                     w,
                     [atSlider, applyAnim](double v) {
                         const int sv = qRound(v * 100.0);
                         if (atSlider->value() != sv) {
                             atSlider->setValue(sv);
                         }
                         applyAnim();
                     });

    applyAnim();
    return w;
}

static FluentCard *makeColorPreviewCard(QWidget *parent)
{
    auto *card = new FluentCard(parent);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *pl = new QVBoxLayout(card);
    pl->setContentsMargins(10, 10, 10, 10);
    pl->setSpacing(6);

    ColorSwatch *accentSw = nullptr;
    ColorSwatch *bgSw = nullptr;
    ColorSwatch *surfaceSw = nullptr;
    auto *accentHex = new FluentLabel();
    auto *bgHex = new FluentLabel();
    auto *surfaceHex = new FluentLabel();

    auto styleHex = [](FluentLabel *l) {
        if (!l)
            return;
        l->setStyleSheet("font-size: 12px; opacity: 0.88; font-family: Consolas, 'Cascadia Mono', monospace;");
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        l->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        l->setMinimumWidth(86);
    };

    styleHex(accentHex);
    styleHex(bgHex);
    styleHex(surfaceHex);

    auto makeRow = [&](const QString &name, ColorSwatch **outSwatch, FluentLabel *hexLabel) {
        auto *rowW = new QWidget(card);
        auto *rowL = new QHBoxLayout(rowW);
        rowL->setContentsMargins(0, 0, 0, 0);
        rowL->setSpacing(8);

        auto *sw = new ColorSwatch(rowW);
        auto *n = new FluentLabel(name);
        n->setStyleSheet("font-size: 12px; opacity: 0.92;");
        n->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        rowL->addWidget(sw);
        rowL->addWidget(n, 1);
        rowL->addWidget(hexLabel);

        if (outSwatch) {
            *outSwatch = sw;
        }

        pl->addWidget(rowW);
    };

    makeRow(QStringLiteral("Accent"), &accentSw, accentHex);
    makeRow(QStringLiteral("Background"), &bgSw, bgHex);
    makeRow(QStringLiteral("Surface"), &surfaceSw, surfaceHex);

    auto updatePreview = [accentSw, bgSw, surfaceSw, accentHex, bgHex, surfaceHex]() {
        const auto &c = ThemeManager::instance().colors();
        if (accentSw)
            accentSw->setColor(c.accent);
        if (bgSw)
            bgSw->setColor(c.background);
        if (surfaceSw)
            surfaceSw->setColor(c.surface);
        accentHex->setText(c.accent.name(QColor::HexRgb).toUpper());
        bgHex->setText(c.background.name(QColor::HexRgb).toUpper());
        surfaceHex->setText(c.surface.name(QColor::HexRgb).toUpper());
    };

    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, card, updatePreview);
    updatePreview();

    return card;
}

} // namespace

DemoThemePanel::DemoThemePanel(QWidget *hostWindow, QWidget *parent, bool showToastControls)
    : QWidget(parent)
    , m_hostWindow(hostWindow)
    , m_showToastControls(showToastControls)
{
    auto *themeLayout = new QVBoxLayout(this);
    themeLayout->setContentsMargins(0, 0, 0, 0);
    themeLayout->setSpacing(10);

    bool firstGroup = true;
    auto addGroupTitle = [&](const QString &t) {
        if (!firstGroup) {
            themeLayout->addSpacing(6);
        }
        firstGroup = false;
        auto *lab = new FluentLabel(t);
        lab->setStyleSheet("font-size: 12px; font-weight: 700; opacity: 0.92;");
        themeLayout->addWidget(lab);
    };

    addGroupTitle(QStringLiteral("主题"));

    auto *modeRow = new QHBoxLayout();
    modeRow->setContentsMargins(0, 0, 0, 0);
    auto *modeLabel = new FluentLabel(QStringLiteral("深色模式"));
    auto *darkToggle = new FluentToggleSwitch();
    modeRow->addWidget(modeLabel);
    modeRow->addStretch(1);
    modeRow->addWidget(darkToggle);
    themeLayout->addLayout(modeRow);

    QObject::connect(darkToggle, &FluentToggleSwitch::toggled, this, [](bool checked) {
        ThemeManager::instance().setThemeMode(checked ? ThemeManager::ThemeMode::Dark : ThemeManager::ThemeMode::Light);
    });
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, darkToggle, [darkToggle]() {
        const bool isDark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;
        if (darkToggle->isChecked() != isDark) {
            darkToggle->setChecked(isDark);
        }
    });
    darkToggle->setChecked(ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark);

    addGroupTitle(QStringLiteral("描边"));

    auto *borderRow = new QHBoxLayout();
    borderRow->setContentsMargins(0, 0, 0, 0);
    auto *borderLabel = new FluentLabel(QStringLiteral("Accent 描边"));
    auto *borderToggle = new FluentToggleSwitch();
    borderToggle->setChecked(ThemeManager::instance().accentBorderEnabled());
    borderRow->addWidget(borderLabel);
    borderRow->addStretch(1);
    borderRow->addWidget(borderToggle);
    themeLayout->addLayout(borderRow);

    QObject::connect(borderToggle, &FluentToggleSwitch::toggled, this, [](bool checked) {
        ThemeManager::instance().setAccentBorderEnabled(checked);
    });
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, borderToggle, [borderToggle]() {
        const bool enabled = ThemeManager::instance().accentBorderEnabled();
        if (borderToggle->isChecked() != enabled) {
            borderToggle->setChecked(enabled);
        }
    });

    // Border trace animation tuning as sub-widget
    {
        auto *animTitle = new FluentLabel(QStringLiteral("动画参数"));
        animTitle->setStyleSheet("font-size: 12px; font-weight: 600; opacity: 0.9;");
        themeLayout->addWidget(animTitle);
        themeLayout->addWidget(makeAccentBorderAnimWidget(this));
    }

    addGroupTitle(QStringLiteral("颜色"));

    {
        auto *hint = new FluentLabel(QStringLiteral(
            "-Accent：强调色（Primary 按钮、开关高亮、进度条、焦点边框等）\n"
            "-Background：窗口底色（大面积背景）\n"
            "-Surface：卡片/控件表面底色（更适合承载内容）\n"
            "提示：如果你只改 Accent，整体不会变暗/变亮；想要氛围变化请配合 Theme 或 Background/Surface。"));
        hint->setWordWrap(true);
        hint->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        hint->setMinimumWidth(0);
        hint->setStyleSheet("font-size: 12px; opacity: 0.85;");
        themeLayout->addWidget(hint);
    }

    themeLayout->addWidget(makeColorPreviewCard(this));

    {
        auto *t = new FluentLabel(QStringLiteral("Accent 预设"));
        t->setStyleSheet("font-size: 12px; font-weight: 600; opacity: 0.9;");
        themeLayout->addWidget(t);
    }

    auto *accentButtonsHost = new QWidget(this);
    accentButtonsHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    accentButtonsHost->setMinimumWidth(0);
    auto *accentFlow = new FluentFlowLayout(accentButtonsHost, 0, 8, 8);
    accentFlow->setUniformItemWidthEnabled(true);
    accentFlow->setMinimumItemWidth(72);
    accentFlow->setAnimationEnabled(true);
    accentButtonsHost->setLayout(accentFlow);

    auto *accentBlue = new FluentButton(QStringLiteral("蓝"));
    auto *accentGreen = new FluentButton(QStringLiteral("绿"));
    auto *accentPurple = new FluentButton(QStringLiteral("紫"));
    auto *accentPick = new FluentButton(QStringLiteral("自定义…"));
    accentFlow->addWidget(accentBlue);
    accentFlow->addWidget(accentGreen);
    accentFlow->addWidget(accentPurple);
    accentFlow->addWidget(accentPick);
    themeLayout->addWidget(accentButtonsHost);

    QObject::connect(accentBlue, &QPushButton::clicked, this, []() { Demo::applyAccent(QColor("#0067C0")); });
    QObject::connect(accentGreen, &QPushButton::clicked, this, []() { Demo::applyAccent(QColor("#0F7B0F")); });
    QObject::connect(accentPurple, &QPushButton::clicked, this, []() { Demo::applyAccent(QColor("#6B4EFF")); });
    QObject::connect(accentPick, &QPushButton::clicked, this, [this]() {
        if (!m_hostWindow) {
            return;
        }
        const QColor before = ThemeManager::instance().colors().accent;
        FluentColorDialog dlg(before, m_hostWindow);

        QObject::connect(&dlg, &FluentColorDialog::colorChanged, this, [](const QColor &c) {
            if (c.isValid()) {
                Demo::applyAccent(c);
            }
        });

        if (dlg.exec() == QDialog::Accepted) {
            const QColor selected = dlg.selectedColor();
            if (selected.isValid()) {
                Demo::applyAccent(selected);
            }
        } else {
            Demo::applyAccent(before);
        }
    });

    {
        auto *t = new FluentLabel(QStringLiteral("背景 / 表面"));
        t->setStyleSheet("font-size: 12px; font-weight: 600; opacity: 0.9;");
        themeLayout->addWidget(t);
    }

    auto *toneButtonsHost = new QWidget(this);
    toneButtonsHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toneButtonsHost->setMinimumWidth(0);
    auto *toneFlow = new FluentFlowLayout(toneButtonsHost, 0, 8, 8);
    toneFlow->setUniformItemWidthEnabled(true);
    toneFlow->setMinimumItemWidth(104);
    toneFlow->setAnimationEnabled(true);
    toneButtonsHost->setLayout(toneFlow);

    auto *bgPick = new FluentButton(QStringLiteral("Background…"));
    auto *surfacePick = new FluentButton(QStringLiteral("Surface…"));
    auto *toneReset = new FluentButton(QStringLiteral("重置"));
    toneFlow->addWidget(bgPick);
    toneFlow->addWidget(surfacePick);
    toneFlow->addWidget(toneReset);
    themeLayout->addWidget(toneButtonsHost);

    QObject::connect(bgPick, &QPushButton::clicked, this, [this]() {
        if (!m_hostWindow) {
            return;
        }
        const QColor before = ThemeManager::instance().colors().background;
        FluentColorDialog dlg(before, m_hostWindow);

        QObject::connect(&dlg, &FluentColorDialog::colorChanged, this, [](const QColor &c) {
            if (c.isValid()) {
                Demo::applyBackground(c);
            }
        });

        if (dlg.exec() == QDialog::Accepted) {
            const QColor selected = dlg.selectedColor();
            if (selected.isValid()) {
                Demo::applyBackground(selected);
            }
        } else {
            Demo::applyBackground(before);
        }
    });

    QObject::connect(surfacePick, &QPushButton::clicked, this, [this]() {
        if (!m_hostWindow) {
            return;
        }
        const QColor before = ThemeManager::instance().colors().surface;
        FluentColorDialog dlg(before, m_hostWindow);

        QObject::connect(&dlg, &FluentColorDialog::colorChanged, this, [](const QColor &c) {
            if (c.isValid()) {
                Demo::applySurface(c);
            }
        });

        if (dlg.exec() == QDialog::Accepted) {
            const QColor selected = dlg.selectedColor();
            if (selected.isValid()) {
                Demo::applySurface(selected);
            }
        } else {
            Demo::applySurface(before);
        }
    });

    QObject::connect(toneReset, &QPushButton::clicked, this, []() {
        const bool isDark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;
        const ThemeColors defaults = isDark ? Theme::dark() : Theme::light();
        auto colors = ThemeManager::instance().colors();
        colors.background = defaults.background;
        colors.surface = defaults.surface;
        colors.border = defaults.border;
        colors.hover = defaults.hover;
        colors.pressed = defaults.pressed;
        ThemeManager::instance().setColors(colors);
    });

    addGroupTitle(QStringLiteral("信息"));

    auto *themeInfo = new FluentLabel();
    themeInfo->setWordWrap(true);
    themeInfo->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    themeInfo->setMinimumWidth(0);
    themeInfo->setStyleSheet("font-size: 12px; opacity: 0.9;");
    themeLayout->addWidget(themeInfo);

    auto updateThemeInfo = [themeInfo]() {
        const auto &c = ThemeManager::instance().colors();
        const bool isDark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;
        const auto m = Style::metrics();
        themeInfo->setText(QStringLiteral(
                               "Theme: %1\n"
                               "Accent: %2\n"
                               "BG: %3  Surface: %4\n"
                               "Style: height=%5  radius=%6  paddingX=%7  paddingY=%8")
                              .arg(isDark ? QStringLiteral("Dark") : QStringLiteral("Light"))
                              .arg(c.accent.name())
                              .arg(c.background.name())
                              .arg(c.surface.name())
                              .arg(m.height)
                              .arg(m.radius)
                              .arg(m.paddingX)
                              .arg(m.paddingY));
    };
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, updateThemeInfo);
    updateThemeInfo();

    if (m_showToastControls) {
        addGroupTitle(QStringLiteral("Toast"));

        // Toast controls as a small sub-component
        auto *toastRowW = new QWidget(this);
        auto *toastRowL = new QHBoxLayout(toastRowW);
        toastRowL->setContentsMargins(0, 0, 0, 0);
        toastRowL->setSpacing(8);

        auto *toastPosCombo = new FluentComboBox();
        toastPosCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        toastPosCombo->addItem(QStringLiteral("左上"), static_cast<int>(FluentToast::Position::TopLeft));
        toastPosCombo->addItem(QStringLiteral("顶部居中"), static_cast<int>(FluentToast::Position::TopCenter));
        toastPosCombo->addItem(QStringLiteral("右上"), static_cast<int>(FluentToast::Position::TopRight));
        toastPosCombo->addItem(QStringLiteral("左下"), static_cast<int>(FluentToast::Position::BottomLeft));
        toastPosCombo->addItem(QStringLiteral("底部居中"), static_cast<int>(FluentToast::Position::BottomCenter));
        toastPosCombo->addItem(QStringLiteral("右下"), static_cast<int>(FluentToast::Position::BottomRight));
        toastPosCombo->setCurrentIndex(5);

        auto *toastOne = new FluentButton(QStringLiteral("发一条"));
        auto *toastAll = new FluentButton(QStringLiteral("全位置"));
        toastOne->setFixedHeight(28);
        toastAll->setFixedHeight(28);
        toastOne->setFixedWidth(72);
        toastAll->setFixedWidth(72);

        toastRowL->addWidget(toastPosCombo, 1);
        toastRowL->addWidget(toastOne);
        toastRowL->addWidget(toastAll);
        themeLayout->addWidget(toastRowW);

        auto posName = [](FluentToast::Position p) {
            switch (p) {
            case FluentToast::Position::TopLeft:
                return QStringLiteral("左上");
            case FluentToast::Position::TopCenter:
                return QStringLiteral("顶部居中");
            case FluentToast::Position::TopRight:
                return QStringLiteral("右上");
            case FluentToast::Position::BottomLeft:
                return QStringLiteral("左下");
            case FluentToast::Position::BottomCenter:
                return QStringLiteral("底部居中");
            case FluentToast::Position::BottomRight:
                return QStringLiteral("右下");
            }
            return QStringLiteral("右下");
        };

        m_toastPosition = FluentToast::Position::BottomRight;

        QObject::connect(toastPosCombo,
                         QOverload<int>::of(&QComboBox::currentIndexChanged),
                         this,
                         [this, toastPosCombo](int) {
                             m_toastPosition = static_cast<FluentToast::Position>(toastPosCombo->currentData().toInt());
                             emit toastPositionChanged(m_toastPosition);
                         });

        QObject::connect(toastOne, &QPushButton::clicked, this, [this, posName]() {
            if (!m_hostWindow) {
                return;
            }
            FluentToast::showToast(m_hostWindow,
                                  QStringLiteral("Toast"),
                                  QStringLiteral("当前弹出位置：%1（点击可关闭）").arg(posName(m_toastPosition)),
                                  m_toastPosition,
                                  2600);
        });

        QObject::connect(toastAll, &QPushButton::clicked, this, [this, posName]() {
            if (!m_hostWindow) {
                return;
            }
            const QVector<FluentToast::Position> positions = {
                FluentToast::Position::TopLeft,
                FluentToast::Position::TopCenter,
                FluentToast::Position::TopRight,
                FluentToast::Position::BottomLeft,
                FluentToast::Position::BottomCenter,
                FluentToast::Position::BottomRight,
            };

            for (int i = 0; i < positions.size(); ++i) {
                const auto p = positions[i];
                QTimer::singleShot(i * 120, this, [this, p, posName]() {
                    if (!m_hostWindow) {
                        return;
                    }
                    FluentToast::showToast(m_hostWindow,
                                          QStringLiteral("Toast"),
                                          QStringLiteral("位置：%1（点击可关闭）").arg(posName(p)),
                                          p,
                                          2400);
                });
            }
        });
    }
}

FluentToast::Position DemoThemePanel::toastPosition() const
{
    return m_toastPosition;
}

} // namespace Demo
