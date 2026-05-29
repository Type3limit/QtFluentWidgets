#include "PageMotion.h"

#include "../DemoHelpers.h"

#include <QColor>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QSlider>
#include <QtGlobal>
#include <QVector>
#include <QVBoxLayout>

#include "Fluent/FluentButton.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentLottieWidget.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentProgressBar.h"
#include "Fluent/FluentProgressRing.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentToggleSwitch.h"
#include "Fluent/FluentWidget.h"

namespace Demo::Pages {

using namespace Fluent;

namespace {

struct MotionSample
{
    QString title;
    QString fileName;
    QString description;
};

QWidget *makeSampleTile(const MotionSample &sample, const QSize &animationSize, QWidget *parent = nullptr)
{
    auto *tile = new QWidget(parent);
    tile->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    tile->setMinimumWidth(132);

    auto *layout = new QVBoxLayout(tile);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(6);

    auto *animation = new FluentLottieWidget(tile);
    animation->setFixedSize(animationSize);
    animation->setToolTip(Demo::demoLottieResourcePath(sample.fileName));
    if (Demo::loadDemoLottieResource(animation, sample.fileName)) {
        animation->play();
    }

    auto *title = new FluentLabel(sample.title, tile);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("font-size: 12px; font-weight: 650;"));

    auto *description = new FluentLabel(sample.description, tile);
    description->setAlignment(Qt::AlignCenter);
    description->setWordWrap(true);
    description->setStyleSheet(QStringLiteral("font-size: 11px; opacity: 0.82;"));

    layout->addWidget(animation, 0, Qt::AlignCenter);
    layout->addWidget(title);
    layout->addWidget(description);
    return tile;
}

void addSubsectionTitle(QVBoxLayout *body, const QString &title, const QString &description = QString())
{
    auto *titleLabel = new FluentLabel(title);
    titleLabel->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 650;"));
    body->addWidget(titleLabel);

    if (!description.isEmpty()) {
        auto *descriptionLabel = new FluentLabel(description);
        descriptionLabel->setWordWrap(true);
        descriptionLabel->setStyleSheet(QStringLiteral("font-size: 12px; opacity: 0.82;"));
        body->addWidget(descriptionLabel);
    }
}

} // namespace

QWidget *createMotionPage()
{
    return Demo::makePage([&](QVBoxLayout *page) {
        auto s = Demo::makeSection(
            DEMO_TEXT("动态", "Motion"),
            DEMO_TEXT("展示 FluentLottieWidget 加载内嵌 JSON 资源、播放控制、主题联动以及图标型 Lottie 的 tint 效果。",
                      "Show FluentLottieWidget loading embedded JSON assets, playback controls, theme linkage, and tinting for icon-like Lottie files."));
        page->addWidget(s.card);

        auto *summary = new FluentLabel(DEMO_TEXT(
            "这些示例使用 demo/lottie_assets.qrc 内嵌的图标型 JSON。它们适合导航、展开按钮、设置入口和滚动提示等微交互动效；按钮状态动画仍建议使用专门制作的 marker 资源。",
            "These examples use icon-like JSON files embedded through demo/lottie_assets.qrc. They fit navigation, expander buttons, settings entry points, and scroll hints; button state animations should still use purpose-built marker assets."));
        summary->setWordWrap(true);
        summary->setStyleSheet(QStringLiteral("font-size: 12px; opacity: 0.88;"));
        s.body->addWidget(summary);

        // Global motion switch
        {
            QString code;
#define MOTION_REDUCED(X) \
    X(auto *motion = new FluentToggleSwitch(QStringLiteral("Animations"));) \
    X(motion->setChecked(ThemeManager::instance().animationsEnabled());) \
    X(QObject::connect(motion, &FluentToggleSwitch::toggled, motion, [](bool enabled) {) \
    X(    ThemeManager::instance().setAnimationsEnabled(enabled);) \
    X(});) \
    X(FluentMotion::setDuration(FluentMotionRole::Hover, 120);) \
    X(FluentMotion::setDuration(FluentMotionRole::PopupOpen, 150);) \
    X(FluentMotion::setDuration(FluentMotionRole::Selection, 180);) \
    X(auto *button = new FluentButton(QStringLiteral("Hover me"));) \
    X(auto *edit = new FluentLineEdit();) \
    X(auto *combo = new FluentComboBox();) \
    X(auto *progress = new FluentProgressBar();) \
    X(auto *ring = new FluentProgressRing();) \
    X(progress->setValue(66);) \
    X(ring->setValue(66);)

#define X(line) code += QStringLiteral(#line "\n");
            MOTION_REDUCED(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("Motion Token"),
                DEMO_TEXT("运行时切换 ThemeManager::animationsEnabled()，观察已创建控件的 hover、focus、popup 与进度动画。",
                          "Toggle ThemeManager::animationsEnabled() at runtime and observe hover, focus, popup, and progress feedback on existing controls."),
                DEMO_TEXT("要点：\n"
                          "-FluentMotion::duration(role) 在关闭动效时返回 0\n"
                          "-FluentMotion::setDuration(role, ms) 可按语义单独配置时长\n"
                          "-已创建控件会在 themeChanged 后重新同步 token\n"
                          "-关闭动效后，popup 直接到最终位置，进度和 hover/focus 直接到目标状态",
                          "Highlights:\n"
                          "-FluentMotion::duration(role) returns 0 when animations are disabled\n"
                          "-FluentMotion::setDuration(role, ms) configures each semantic duration independently\n"
                          "-Existing controls resync motion tokens on themeChanged\n"
                          "-With animations disabled, popups jump to final geometry and progress / hover / focus jump to target states"),
                code,
                [](QVBoxLayout *body) {
                    auto *host = new QWidget();
                    auto *layout = new QVBoxLayout(host);
                    layout->setContentsMargins(0, 0, 0, 0);
                    layout->setSpacing(12);

                    auto *switchRow = new QHBoxLayout();
                    switchRow->setContentsMargins(0, 0, 0, 0);
                    switchRow->setSpacing(12);

                    auto *animations = new FluentToggleSwitch(DEMO_TEXT("全局动效", "Animations"), host);
                    animations->setChecked(ThemeManager::instance().animationsEnabled());
                    auto *resetMotion = new FluentButton(DEMO_TEXT("重置时长", "Reset durations"), host);

                    auto *tokenLabel = new FluentLabel(host);
                    tokenLabel->setWordWrap(true);
                    tokenLabel->setStyleSheet(QStringLiteral("font-size: 12px; opacity: 0.86;"));

                    switchRow->addWidget(animations);
                    switchRow->addWidget(resetMotion);
                    switchRow->addWidget(tokenLabel, 1);

                    auto *controlRow = new QHBoxLayout();
                    controlRow->setContentsMargins(0, 0, 0, 0);
                    controlRow->setSpacing(10);

                    auto *button = new FluentButton(DEMO_TEXT("Hover 按钮", "Hover button"), host);
                    auto *edit = new FluentLineEdit(host);
                    edit->setPlaceholderText(DEMO_TEXT("Focus 输入框", "Focus input"));
                    edit->setMinimumWidth(180);

                    auto *combo = new FluentComboBox(host);
                    combo->addItems({DEMO_TEXT("Popup 动效", "Popup motion"),
                                     DEMO_TEXT("Hover 同步", "Hover sync"),
                                     DEMO_TEXT("Reduced motion", "Reduced motion")});
                    combo->setMinimumWidth(170);

                    controlRow->addWidget(button);
                    controlRow->addWidget(edit);
                    controlRow->addWidget(combo);
                    controlRow->addStretch(1);

                    auto *progressRow = new QHBoxLayout();
                    progressRow->setContentsMargins(0, 0, 0, 0);
                    progressRow->setSpacing(10);

                    auto *advance = new FluentButton(DEMO_TEXT("推进进度", "Advance"), host);
                    auto *bar = new FluentProgressBar(host);
                    bar->setRange(0, 100);
                    bar->setValue(35);
                    bar->setTextPosition(FluentProgressBar::TextPosition::Right);
                    auto *ring = new FluentProgressRing(host);
                    ring->setFixedSize(44, 44);
                    ring->setRange(0, 100);
                    ring->setValue(35);
                    auto *spinner = new FluentProgressRing(host);
                    spinner->setFixedSize(44, 44);
                    spinner->setIndeterminate(true);

                    progressRow->addWidget(advance);
                    progressRow->addWidget(bar, 1);
                    progressRow->addWidget(ring);
                    progressRow->addWidget(spinner);

                    auto *durationHost = new QWidget(host);
                    auto *durationGrid = new QGridLayout(durationHost);
                    durationGrid->setContentsMargins(0, 0, 0, 0);
                    durationGrid->setHorizontalSpacing(12);
                    durationGrid->setVerticalSpacing(8);

                    struct DurationSpec
                    {
                        FluentMotionRole role;
                        QString label;
                        int maximum = 700;
                    };

                    struct DurationBinding
                    {
                        FluentMotionRole role;
                        QSlider *slider = nullptr;
                        FluentLabel *value = nullptr;
                    };

                    const QVector<DurationSpec> specs = {
                        {FluentMotionRole::Hover, QStringLiteral("Hover"), 400},
                        {FluentMotionRole::Press, QStringLiteral("Press"), 400},
                        {FluentMotionRole::Focus, QStringLiteral("Focus"), 500},
                        {FluentMotionRole::PopupOpen, QStringLiteral("Popup open"), 700},
                        {FluentMotionRole::PopupClose, QStringLiteral("Popup close"), 500},
                        {FluentMotionRole::Collapse, QStringLiteral("Collapse"), 700},
                        {FluentMotionRole::Selection, QStringLiteral("Selection"), 700},
                        {FluentMotionRole::Navigation, QStringLiteral("Navigation"), 900},
                        {FluentMotionRole::Layout, QStringLiteral("Layout"), 700},
                        {FluentMotionRole::Page, QStringLiteral("Page"), 900},
                        {FluentMotionRole::Toast, QStringLiteral("Toast"), 700},
                        {FluentMotionRole::WheelSnap, QStringLiteral("Wheel snap"), 500},
                    };

                    QVector<DurationBinding> durationBindings;
                    durationBindings.reserve(specs.size());

                    for (int i = 0; i < specs.size(); ++i) {
                        const DurationSpec spec = specs[i];
                        const int group = i % 2;
                        const int row = i / 2;
                        const int baseCol = group * 3;

                        auto *label = new FluentLabel(spec.label, durationHost);
                        label->setStyleSheet(QStringLiteral("font-size: 12px; opacity: 0.86;"));

                        auto *slider = new QSlider(Qt::Horizontal, durationHost);
                        slider->setRange(0, spec.maximum);
                        slider->setSingleStep(10);
                        slider->setPageStep(50);
                        slider->setValue(FluentMotion::configuredDuration(spec.role));

                        auto *value = new FluentLabel(durationHost);
                        value->setMinimumWidth(54);
                        value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                        value->setStyleSheet(QStringLiteral("font-size: 12px; opacity: 0.78;"));
                        value->setText(QStringLiteral("%1 ms").arg(slider->value()));

                        QObject::connect(slider, &QSlider::valueChanged, host, [role = spec.role, value](int ms) {
                            value->setText(QStringLiteral("%1 ms").arg(ms));
                            FluentMotion::setDuration(role, ms);
                        });

                        durationGrid->addWidget(label, row, baseCol);
                        durationGrid->addWidget(slider, row, baseCol + 1);
                        durationGrid->addWidget(value, row, baseCol + 2);
                        durationBindings.push_back({spec.role, slider, value});
                    }
                    durationGrid->setColumnStretch(1, 1);
                    durationGrid->setColumnStretch(4, 1);

                    auto *hint = new FluentLabel(
                        DEMO_TEXT("切换开关后不用重建这些控件：再次 hover 按钮、聚焦输入框、展开 ComboBox 或推进进度，就能看到 token 是否即时生效。",
                                  "No controls are recreated after toggling: hover the button, focus the input, open the ComboBox, or advance progress to see whether tokens apply immediately."),
                        host);
                    hint->setWordWrap(true);
                    hint->setStyleSheet(QStringLiteral("font-size: 12px; opacity: 0.82;"));

                    auto syncState = [animations, tokenLabel, durationBindings]() {
                        const bool enabled = ThemeManager::instance().animationsEnabled();
                        {
                            const QSignalBlocker blocker(animations);
                            animations->setChecked(enabled);
                        }
                        for (const DurationBinding &binding : durationBindings) {
                            if (!binding.slider || !binding.value) {
                                continue;
                            }
                            const int configured = FluentMotion::configuredDuration(binding.role);
                            {
                                const QSignalBlocker blocker(binding.slider);
                                binding.slider->setValue(configured);
                            }
                            binding.value->setText(QStringLiteral("%1 ms").arg(configured));
                        }
                        tokenLabel->setText(DEMO_TEXT(
                            "当前：%1 | Hover %2 ms | Popup %3 ms | Selection %4 ms",
                            "Current: %1 | Hover %2 ms | Popup %3 ms | Selection %4 ms")
                                                .arg(enabled ? DEMO_TEXT("开启", "On") : DEMO_TEXT("关闭", "Off"))
                                                .arg(FluentMotion::duration(FluentMotionRole::Hover))
                                                .arg(FluentMotion::duration(FluentMotionRole::PopupOpen))
                                                .arg(FluentMotion::duration(FluentMotionRole::Selection)));
                    };

                    QObject::connect(animations, &FluentToggleSwitch::toggled, host, [](bool enabled) {
                        ThemeManager::instance().setAnimationsEnabled(enabled);
                    });
                    QObject::connect(resetMotion, &QPushButton::clicked, host, []() {
                        FluentMotion::resetTokens();
                    });
                    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, host, syncState);
                    QObject::connect(advance, &QPushButton::clicked, host, [bar, ring]() {
                        const int next = (bar->value() + 31) % 101;
                        bar->setValue(next);
                        ring->setValue(next);
                    });

                    syncState();

                    layout->addLayout(switchRow);
                    layout->addWidget(durationHost);
                    layout->addLayout(controlRow);
                    layout->addLayout(progressRow);
                    layout->addWidget(hint);
                    body->addWidget(host);
                },
                false,
                170));

#undef MOTION_REDUCED
        }

        // Lottie widget
        {
            QString code;
#define MOTION_LOTTIE_WIDGET(X) \
    X(auto *animation = new FluentLottieWidget();) \
    X(animation->setFixedSize(96, 96);) \
    X(animation->load(Demo::demoLottieResourcePath(QStringLiteral("menu.json")));) \
    X(animation->setLooping(true);) \
    X(animation->setSpeed(1.0);) \
    X(animation->setTintColor(ThemeManager::instance().colors().accent);) \
    X(auto applyTheme = [animation]() {) \
    X(    animation->setTintColor(ThemeManager::instance().colors().accent);) \
    X(};) \
    X(applyTheme();) \
    X(QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, animation, applyTheme);) \
    X(animation->play();)

#define X(line) code += QStringLiteral(#line "\n");
            MOTION_LOTTIE_WIDGET(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentLottieWidget"),
                DEMO_TEXT("资源矩阵、播放控制、Tint 与主题联动集中展示",
                          "Resource gallery, playback control, tinting, and theme linkage in one place"),
                DEMO_TEXT("要点：\n"
                          "-load() 可直接加载 Lottie JSON 文件\n"
                          "-play()/pause()/stop()/setProgress()/setSpeed() 控制播放\n"
                          "-setTintColor() 适合把图标型 Lottie 映射到语义色\n"
                          "-监听 ThemeManager::themeChanged 后可实时刷新 tint",
                          "Highlights:\n"
                          "-load() can load a Lottie JSON file directly\n"
                          "-play()/pause()/stop()/setProgress()/setSpeed() control playback\n"
                          "-setTintColor() maps icon-like Lottie animations to semantic colors\n"
                          "-Listen to ThemeManager::themeChanged to refresh tint live"),
                code,
                [](QVBoxLayout *body) {
                    addSubsectionTitle(body,
                                       DEMO_TEXT("内嵌资源矩阵", "Embedded Resource Matrix"),
                                       DEMO_TEXT("这些资源适合导航图标、展开指示器、设置入口和滚动提示等微交互。",
                                                 "These assets fit navigation icons, expand indicators, settings entries, and scroll hints."));

                    const QVector<MotionSample> samples = {
                        {DEMO_TEXT("菜单", "Menu"), QStringLiteral("menu.json"), DEMO_TEXT("导航入口", "Navigation")},
                        {DEMO_TEXT("展开", "Dropdown"), QStringLiteral("dropdown.json"), DEMO_TEXT("折叠状态", "Collapse state")},
                        {DEMO_TEXT("设置", "Settings"), QStringLiteral("setting.json"), DEMO_TEXT("系统入口", "System entry")},
                        {DEMO_TEXT("左箭头", "Left arrow"), QStringLiteral("left-arrow.json"), DEMO_TEXT("返回动作", "Back action")},
                        {DEMO_TEXT("右箭头", "Right arrow"), QStringLiteral("right-arrow.json"), DEMO_TEXT("前进动作", "Forward action")},
                        {DEMO_TEXT("向下滚动", "Scroll down"), QStringLiteral("scroll-down.json"), DEMO_TEXT("滚动提示", "Scroll hint")},
                    };

                    auto *gridHost = new QWidget();
                    auto *grid = new QGridLayout(gridHost);
                    grid->setContentsMargins(0, 0, 0, 0);
                    grid->setHorizontalSpacing(12);
                    grid->setVerticalSpacing(10);
                    for (int i = 0; i < samples.size(); ++i) {
                        grid->addWidget(makeSampleTile(samples[i], QSize(82, 82), gridHost), i / 3, i % 3);
                    }
                    body->addWidget(gridHost);
                    body->addSpacing(10);

                    addSubsectionTitle(body,
                                       DEMO_TEXT("播放控制", "Playback Control"),
                                       DEMO_TEXT("播放、暂停、停止、进度定位与播放速度都可以独立控制。",
                                                 "Playback, pause, stop, progress seek, and speed are independently controllable."));

                    auto *playbackRow = new QHBoxLayout();
                    playbackRow->setContentsMargins(0, 0, 0, 0);
                    playbackRow->setSpacing(14);

                    auto *animation = new FluentLottieWidget();
                    animation->setFixedSize(128, 96);
                    animation->setLooping(true);
                    if (Demo::loadDemoLottieResource(animation, QStringLiteral("menu.json"))) {
                        animation->play();
                    }

                    auto *controls = new QWidget();
                    auto *controlsLayout = new QVBoxLayout(controls);
                    controlsLayout->setContentsMargins(0, 0, 0, 0);
                    controlsLayout->setSpacing(8);

                    auto *buttons = new QHBoxLayout();
                    buttons->setContentsMargins(0, 0, 0, 0);
                    buttons->setSpacing(8);

                    auto *play = new FluentButton(QStringLiteral("Play"));
                    auto *pause = new FluentButton(QStringLiteral("Pause"));
                    auto *stop = new FluentButton(QStringLiteral("Stop"));

                    buttons->addWidget(play);
                    buttons->addWidget(pause);
                    buttons->addWidget(stop);
                    buttons->addStretch(1);

                    auto *progress = new QSlider(Qt::Horizontal);
                    progress->setRange(0, 1000);
                    auto *status = new FluentLabel(QStringLiteral("0%"));
                    status->setMinimumWidth(42);

                    auto *progressRow = new QHBoxLayout();
                    progressRow->setContentsMargins(0, 0, 0, 0);
                    progressRow->setSpacing(8);
                    progressRow->addWidget(progress, 1);
                    progressRow->addWidget(status);

                    auto *speed = new QSlider(Qt::Horizontal);
                    speed->setRange(25, 400);
                    speed->setValue(100);
                    speed->setSingleStep(25);
                    speed->setPageStep(50);
                    auto *speedLabel = new FluentLabel(QStringLiteral("Speed 1.00x"));
                    speedLabel->setMinimumWidth(92);

                    auto *speedRow = new QHBoxLayout();
                    speedRow->setContentsMargins(0, 0, 0, 0);
                    speedRow->setSpacing(8);
                    speedRow->addWidget(speed, 1);
                    speedRow->addWidget(speedLabel);

                    controlsLayout->addLayout(buttons);
                    controlsLayout->addLayout(progressRow);
                    controlsLayout->addLayout(speedRow);
                    controlsLayout->addStretch(1);

                    QObject::connect(play, &QPushButton::clicked, animation, &FluentLottieWidget::play);
                    QObject::connect(pause, &QPushButton::clicked, animation, &FluentLottieWidget::pause);
                    QObject::connect(stop, &QPushButton::clicked, animation, &FluentLottieWidget::stop);
                    QObject::connect(animation, &FluentLottieWidget::progressChanged, progress, [progress, status](qreal value) {
                        const int next = qRound(value * 1000.0);
                        const QSignalBlocker blocker(progress);
                        progress->setValue(next);
                        status->setText(QStringLiteral("%1%").arg(qRound(value * 100.0)));
                    });
                    QObject::connect(progress, &QSlider::sliderPressed, animation, &FluentLottieWidget::pause);
                    QObject::connect(progress, &QSlider::valueChanged, animation, [animation, progress](int value) {
                        if (progress->isSliderDown()) {
                            animation->setProgress(static_cast<qreal>(value) / 1000.0);
                        }
                    });
                    QObject::connect(speed, &QSlider::valueChanged, animation, [animation, speedLabel](int value) {
                        const qreal speedValue = static_cast<qreal>(value) / 100.0;
                        animation->setSpeed(speedValue);
                        speedLabel->setText(QStringLiteral("Speed %1x").arg(speedValue, 0, 'f', 2));
                    });

                    playbackRow->addWidget(animation);
                    playbackRow->addWidget(controls, 1);
                    body->addLayout(playbackRow);
                    body->addSpacing(10);

                    addSubsectionTitle(body,
                                       DEMO_TEXT("Tint 与主题联动", "Tint and Theme Linkage"),
                                       DEMO_TEXT("图标型 Lottie 可以按 alpha 重染为 accent、text、error 等语义色。",
                                                 "Icon-like Lottie files can be recolored by alpha into semantic colors such as accent, text, and error."));

                    auto *themeHost = new QWidget();
                    auto *themeLayout = new QVBoxLayout(themeHost);
                    themeLayout->setContentsMargins(0, 0, 0, 0);
                    themeLayout->setSpacing(12);

                    auto *modeRow = new QHBoxLayout();
                    modeRow->setContentsMargins(0, 0, 0, 0);
                    modeRow->setSpacing(10);

                    auto *darkMode = new FluentToggleSwitch(DEMO_TEXT("深色模式", "Dark mode"), themeHost);
                    auto *modeLabel = new FluentLabel(themeHost);
                    modeLabel->setStyleSheet(QStringLiteral("font-size: 12px; opacity: 0.86;"));
                    modeRow->addWidget(darkMode);
                    modeRow->addWidget(modeLabel, 1);

                    struct ThemeLinkedTile
                    {
                        QWidget *host = nullptr;
                        FluentLottieWidget *animation = nullptr;
                        FluentLabel *value = nullptr;
                    };

                    auto makeThemeTile = [](const QString &title, const QString &fileName, QWidget *parent) {
                        ThemeLinkedTile tile;
                        tile.host = new QWidget(parent);
                        tile.host->setObjectName(QStringLiteral("MotionThemeTile"));

                        auto *tileLayout = new QVBoxLayout(tile.host);
                        tileLayout->setContentsMargins(10, 8, 10, 8);
                        tileLayout->setSpacing(6);

                        tile.animation = new FluentLottieWidget(tile.host);
                        tile.animation->setFixedSize(76, 76);
                        if (Demo::loadDemoLottieResource(tile.animation, fileName)) {
                            tile.animation->play();
                        }

                        auto *titleLabel = new FluentLabel(title, tile.host);
                        titleLabel->setAlignment(Qt::AlignCenter);
                        titleLabel->setStyleSheet(QStringLiteral("font-size: 12px; font-weight: 650;"));

                        tile.value = new FluentLabel(tile.host);
                        tile.value->setAlignment(Qt::AlignCenter);
                        tile.value->setStyleSheet(QStringLiteral("font-size: 11px; opacity: 0.78;"));

                        tileLayout->addWidget(tile.animation, 0, Qt::AlignCenter);
                        tileLayout->addWidget(titleLabel);
                        tileLayout->addWidget(tile.value);
                        return tile;
                    };

                    ThemeLinkedTile original = makeThemeTile(DEMO_TEXT("原色", "Original"), QStringLiteral("setting.json"), themeHost);
                    ThemeLinkedTile accent = makeThemeTile(QStringLiteral("Accent"), QStringLiteral("setting.json"), themeHost);
                    ThemeLinkedTile text = makeThemeTile(QStringLiteral("Text"), QStringLiteral("dropdown.json"), themeHost);
                    ThemeLinkedTile error = makeThemeTile(QStringLiteral("Error"), QStringLiteral("delete.json"), themeHost);
                    ThemeLinkedTile disabled = makeThemeTile(DEMO_TEXT("禁用", "Disabled"), QStringLiteral("scroll-down.json"), themeHost);

                    original.value->setText(DEMO_TEXT("原始颜色", "Original colors"));

                    auto applyTheme = [themeHost, darkMode, modeLabel, original, accent, text, error, disabled]() {
                        const auto &colors = ThemeManager::instance().colors();
                        const bool isDark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;

                        {
                            const QSignalBlocker blocker(darkMode);
                            darkMode->setChecked(isDark);
                        }
                        modeLabel->setText(DEMO_TEXT("当前主题：%1，切换后 Lottie tint 会跟随语义色刷新",
                                                     "Current theme: %1. Lottie tint follows semantic colors after switching")
                                               .arg(isDark ? DEMO_TEXT("深色", "Dark") : DEMO_TEXT("浅色", "Light")));

                        original.animation->resetTintColor();
                        accent.animation->setTintColor(colors.accent);
                        text.animation->setTintColor(colors.text);
                        error.animation->setTintColor(colors.error);
                        disabled.animation->setTintColor(colors.disabledText);

                        accent.value->setText(colors.accent.name(QColor::HexRgb).toUpper());
                        text.value->setText(colors.text.name(QColor::HexRgb).toUpper());
                        error.value->setText(colors.error.name(QColor::HexRgb).toUpper());
                        disabled.value->setText(colors.disabledText.name(QColor::HexRgb).toUpper());

                        const QString tileStyle = QStringLiteral(
                            "#MotionThemeTile { background: %1; border: 1px solid %2; border-radius: 8px; }")
                                                        .arg(colors.surface.name(QColor::HexRgb),
                                                             colors.border.name(QColor::HexRgb));
                        original.host->setStyleSheet(tileStyle);
                        accent.host->setStyleSheet(tileStyle);
                        text.host->setStyleSheet(tileStyle);
                        error.host->setStyleSheet(tileStyle);
                        disabled.host->setStyleSheet(tileStyle);
                    };

                    QObject::connect(darkMode, &FluentToggleSwitch::toggled, themeHost, [](bool checked) {
                        ThemeManager::instance().setThemeMode(checked ? ThemeManager::ThemeMode::Dark
                                                                      : ThemeManager::ThemeMode::Light);
                    });
                    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, themeHost, applyTheme);
                    applyTheme();

                    auto *themeGrid = new QGridLayout();
                    themeGrid->setContentsMargins(0, 0, 0, 0);
                    themeGrid->setHorizontalSpacing(12);
                    themeGrid->setVerticalSpacing(10);
                    themeGrid->addWidget(original.host, 0, 0);
                    themeGrid->addWidget(accent.host, 0, 1);
                    themeGrid->addWidget(text.host, 0, 2);
                    themeGrid->addWidget(error.host, 0, 3);
                    themeGrid->addWidget(disabled.host, 0, 4);
                    themeGrid->setColumnStretch(5, 1);

                    themeLayout->addLayout(modeRow);
                    themeLayout->addLayout(themeGrid);
                    body->addWidget(themeHost);
                },
                false,
                210));

#undef MOTION_LOTTIE_WIDGET
        }
    });
}

} // namespace Demo::Pages
