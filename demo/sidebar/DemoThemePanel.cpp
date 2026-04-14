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

    makeRow(DEMO_TEXT("时长", "Duration"), durSlider, durSpin);
    makeRow(DEMO_TEXT("弹性", "Overshoot"), overSlider, overSpin);
    makeRow(DEMO_TEXT("弹性点", "Overshoot point"), atSlider, atSpin);

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

    addGroupTitle(DEMO_TEXT("语言", "Language"));

    auto *languageRow = new QHBoxLayout();
    languageRow->setContentsMargins(0, 0, 0, 0);
    auto *languageLabel = new FluentLabel(DEMO_TEXT("显示语言", "Display language"));
    auto *languageCombo = new FluentComboBox();
    languageCombo->addItem(DEMO_TEXT("简体中文", "Chinese (Simplified)"), static_cast<int>(DemoLanguage::Chinese));
    languageCombo->addItem(QStringLiteral("English"), static_cast<int>(DemoLanguage::English));
    languageCombo->setCurrentIndex(currentLanguage() == DemoLanguage::English ? 1 : 0);
    languageRow->addWidget(languageLabel);
    languageRow->addStretch(1);
    languageRow->addWidget(languageCombo);
    themeLayout->addLayout(languageRow);

    QObject::connect(languageCombo,
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this,
                     [this, languageCombo](int) {
                         emit languageChanged(static_cast<DemoLanguage>(languageCombo->currentData().toInt()));
                     });

    addGroupTitle(DEMO_TEXT("主题", "Theme"));

    auto *modeRow = new QHBoxLayout();
    modeRow->setContentsMargins(0, 0, 0, 0);
    auto *modeLabel = new FluentLabel(DEMO_TEXT("深色模式", "Dark mode"));
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

    addGroupTitle(DEMO_TEXT("描边", "Border"));

    auto *borderRow = new QHBoxLayout();
    borderRow->setContentsMargins(0, 0, 0, 0);
    auto *borderLabel = new FluentLabel(DEMO_TEXT("Accent 描边", "Accent border"));
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
        auto *animTitle = new FluentLabel(DEMO_TEXT("动画参数", "Animation parameters"));
        animTitle->setStyleSheet("font-size: 12px; font-weight: 600; opacity: 0.9;");
        themeLayout->addWidget(animTitle);
        themeLayout->addWidget(makeAccentBorderAnimWidget(this));
    }

    addGroupTitle(DEMO_TEXT("颜色", "Colors"));

    {
        auto *hint = new FluentLabel(DEMO_TEXT(
            "-Accent：强调色（Primary 按钮、开关高亮、进度条、焦点边框等）\n"
            "-Background：窗口底色（大面积背景）\n"
            "-Surface：卡片/控件表面底色（更适合承载内容）\n"
            "提示：如果你只改 Accent，整体不会变暗/变亮；想要氛围变化请配合 Theme 或 Background/Surface。",
            "-Accent: emphasis color for primary buttons, toggles, progress bars, focus borders, and more\n"
            "-Background: main window background tone\n"
            "-Surface: card and control surface color for content\n"
            "Tip: changing only Accent will not shift the whole atmosphere. Pair it with Theme or Background/Surface for a broader change."));
        hint->setWordWrap(true);
        hint->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        hint->setMinimumWidth(0);
        hint->setStyleSheet("font-size: 12px; opacity: 0.85;");
        themeLayout->addWidget(hint);
    }

    themeLayout->addWidget(makeColorPreviewCard(this));

    {
        auto *t = new FluentLabel(DEMO_TEXT("Accent 预设", "Accent presets"));
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

    auto *accentBlue = new FluentButton(DEMO_TEXT("蓝", "Blue"));
    auto *accentGreen = new FluentButton(DEMO_TEXT("绿", "Green"));
    auto *accentPurple = new FluentButton(DEMO_TEXT("紫", "Purple"));
    auto *accentPick = new FluentButton(DEMO_TEXT("自定义…", "Custom..."));
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
        auto *t = new FluentLabel(DEMO_TEXT("背景 / 表面", "Background / Surface"));
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
    auto *toneReset = new FluentButton(DEMO_TEXT("重置", "Reset"));
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

    addGroupTitle(DEMO_TEXT("信息", "Info"));

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
        themeInfo->setText(DEMO_TEXT(
                       "主题：%1\n"
                       "Accent：%2\n"
                       "背景：%3  表面：%4\n"
                       "样式：height=%5  radius=%6  paddingX=%7  paddingY=%8",
                       "Theme: %1\n"
                       "Accent: %2\n"
                       "Background: %3  Surface: %4\n"
                       "Style: height=%5  radius=%6  paddingX=%7  paddingY=%8")
                      .arg(isDark ? DEMO_TEXT("深色", "Dark") : DEMO_TEXT("浅色", "Light"))
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
        toastPosCombo->addItem(DEMO_TEXT("左上", "Top left"), static_cast<int>(FluentToast::Position::TopLeft));
        toastPosCombo->addItem(DEMO_TEXT("顶部居中", "Top center"), static_cast<int>(FluentToast::Position::TopCenter));
        toastPosCombo->addItem(DEMO_TEXT("右上", "Top right"), static_cast<int>(FluentToast::Position::TopRight));
        toastPosCombo->addItem(DEMO_TEXT("左下", "Bottom left"), static_cast<int>(FluentToast::Position::BottomLeft));
        toastPosCombo->addItem(DEMO_TEXT("底部居中", "Bottom center"), static_cast<int>(FluentToast::Position::BottomCenter));
        toastPosCombo->addItem(DEMO_TEXT("右下", "Bottom right"), static_cast<int>(FluentToast::Position::BottomRight));
        toastPosCombo->setCurrentIndex(5);

        auto *toastOne = new FluentButton(DEMO_TEXT("发一条", "Send one"));
        auto *toastAll = new FluentButton(DEMO_TEXT("全位置", "All positions"));
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
                return DEMO_TEXT("左上", "Top left");
            case FluentToast::Position::TopCenter:
                return DEMO_TEXT("顶部居中", "Top center");
            case FluentToast::Position::TopRight:
                return DEMO_TEXT("右上", "Top right");
            case FluentToast::Position::BottomLeft:
                return DEMO_TEXT("左下", "Bottom left");
            case FluentToast::Position::BottomCenter:
                return DEMO_TEXT("底部居中", "Bottom center");
            case FluentToast::Position::BottomRight:
                return DEMO_TEXT("右下", "Bottom right");
            }
            return DEMO_TEXT("右下", "Bottom right");
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
                                  DEMO_TEXT("当前弹出位置：%1（点击可关闭）", "Current position: %1 (click to dismiss)").arg(posName(m_toastPosition)),
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
                                          DEMO_TEXT("位置：%1（点击可关闭）", "Position: %1 (click to dismiss)").arg(posName(p)),
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
