#include "DemoWindow.h"

#include "DemoHelpers.h"
#include "DemoSidebar.h"

#include "pages/PageButtons.h"
#include "pages/PageContainers.h"
#include "pages/PageDataViews.h"
#include "pages/PageInputs.h"
#include "pages/PageMotion.h"
#include "pages/PageOverview.h"
#include "pages/PagePickers.h"
#include "pages/PageWindows.h"

#include <QAbstractButton>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QButtonGroup>
#include <QClipboard>
#include <QComboBox>
#include <QCursor>
#include <QDebug>
#include <QElapsedTimer>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QPropertyAnimation>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>
#include <array>
#include <functional>
#include <memory>
#include <utility>

#include "Fluent/FluentAnimatedButton.h"
#include "Fluent/FluentButton.h"
#include "Fluent/FluentCalendarPicker.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentCheckBox.h"
#include "Fluent/FluentColorDialog.h"
#include "Fluent/FluentColorPicker.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentDialog.h"
#include "Fluent/FluentFlowLayout.h"
#include "Fluent/FluentGroupBox.h"
#include "Fluent/FluentIcon.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentMenu.h"
#include "Fluent/FluentMenuBar.h"
#include "Fluent/FluentMessageBox.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentNavigationView.h"
#include "Fluent/FluentProgressBar.h"
#include "Fluent/FluentRadioButton.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentSlider.h"
#include "Fluent/FluentSpinBox.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTabWidget.h"
#include "Fluent/FluentTableView.h"
#include "Fluent/FluentTextEdit.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentTimePicker.h"
#include "Fluent/FluentToggleSwitch.h"
#include "Fluent/FluentToolButton.h"
#include "Fluent/FluentTreeView.h"
#include "Fluent/FluentToast.h"
#include "Fluent/FluentWidget.h"

namespace Demo {

using namespace Fluent;

namespace {

class DemoStartupTrace
{
public:
    explicit DemoStartupTrace(QString context)
        : m_context(std::move(context))
    {
        m_timer.start();
        qInfo().noquote() << QStringLiteral("[DemoStartup] %1 begin").arg(m_context);
    }

    ~DemoStartupTrace()
    {
        qInfo().noquote() << QStringLiteral("[DemoStartup] %1 done at %2 ms").arg(m_context).arg(m_timer.elapsed());
    }

    void mark(const QString &label)
    {
        const qint64 now = m_timer.elapsed();
        qInfo().noquote() << QStringLiteral("[DemoStartup] %1 %2 +%3 ms, total %4 ms")
                                  .arg(m_context, label)
                                  .arg(now - m_lastMark)
                                  .arg(now);
        m_lastMark = now;
    }

    template <typename Factory>
    QWidget *addPage(QStackedWidget *stack, const QString &name, Factory factory)
    {
        const qint64 beforeCreate = m_timer.elapsed();
        QWidget *page = factory();
        const qint64 afterCreate = m_timer.elapsed();
        qInfo().noquote() << QStringLiteral("[DemoStartup] %1 page %2 created +%3 ms, total %4 ms")
                                  .arg(m_context, name)
                                  .arg(afterCreate - beforeCreate)
                                  .arg(afterCreate);
        if (stack && page) {
            const qint64 beforeAdd = m_timer.elapsed();
            stack->addWidget(page);
            const qint64 afterAdd = m_timer.elapsed();
            qInfo().noquote() << QStringLiteral("[DemoStartup] %1 page %2 added +%3 ms, total %4 ms")
                                      .arg(m_context, name)
                                      .arg(afterAdd - beforeAdd)
                                      .arg(afterAdd);
        }
        return page;
    }

private:
    QString m_context;
    QElapsedTimer m_timer;
    qint64 m_lastMark = 0;
};

class AccentSwatchButton final : public QAbstractButton
{
public:
    explicit AccentSwatchButton(const QColor &color, QWidget *parent = nullptr)
        : QAbstractButton(parent)
        , m_color(color)
    {
        setCheckable(true);
        setFixedSize(20, 20);
        setCursor(Qt::PointingHandCursor);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(m_color);
        p.drawEllipse(QRectF(rect()).adjusted(3, 3, -3, -3));
        if (isChecked()) {
            QPen ring(m_color.lightness() > 140 ? QColor(0, 0, 0, 160) : QColor(255, 255, 255, 230));
            ring.setWidthF(2.0);
            p.setBrush(Qt::NoBrush);
            p.setPen(ring);
            p.drawEllipse(QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5));
        }
    }

private:
    QColor m_color;
};

void animateStackPageTransition(QStackedWidget *stack, QWidget *page, int previousIndex, int nextIndex)
{
    if (!stack || !page || previousIndex == nextIndex || !stack->isVisible()) {
        return;
    }

    const int duration = FluentMotion::duration(FluentMotionRole::Page);
    if (duration <= 0) {
        page->move(QPoint(0, 0));
        return;
    }

    if (QGraphicsEffect *oldEffect = page->graphicsEffect()) {
        if (!oldEffect->property("_demoPageTransitionEffect").toBool()) {
            return;
        }
        // A transition is already animating this page. The running animation owns
        // its cleanup; clearing the effect here would delete its animation target.
        return;
    }

    const QPoint finalPos = page->pos();
    const int slide = qMax(12, FluentMotion::popupSlideOffset() * 2);
    const int direction = nextIndex > previousIndex ? 1 : -1;
    const QPoint startPos = finalPos + QPoint(0, direction * slide);

    auto *effect = new QGraphicsOpacityEffect(page);
    effect->setProperty("_demoPageTransitionEffect", true);
    effect->setOpacity(0.0);
    page->setGraphicsEffect(effect);
    page->move(startPos);

    auto *group = new QParallelAnimationGroup(page);
    auto *opacity = new QPropertyAnimation(effect, "opacity", group);
    opacity->setDuration(duration);
    opacity->setEasingCurve(FluentMotion::easing(FluentMotionRole::Page));
    opacity->setStartValue(0.0);
    opacity->setEndValue(1.0);

    auto *position = new QPropertyAnimation(page, "pos", group);
    position->setDuration(duration);
    position->setEasingCurve(FluentMotion::easing(FluentMotionRole::Page));
    position->setStartValue(startPos);
    position->setEndValue(finalPos);

    group->addAnimation(opacity);
    group->addAnimation(position);
    QObject::connect(group, &QParallelAnimationGroup::finished, page, [page, effect, finalPos, group]() {
        page->move(finalPos);
        if (page->graphicsEffect() == effect) {
            // QWidget owns graphicsEffect(); setGraphicsEffect(nullptr) deletes it.
            page->setGraphicsEffect(nullptr);
        }
        group->deleteLater();
    });
    group->start();
}

} // namespace

DemoWindow::DemoWindow(QWidget *parent,
                       const QString &selectedNavigationKey,
                       FluentToast::Position toastPosition)
    : FluentMainWindow(parent)
    , m_toastPosition(toastPosition)
    , m_selectedNavigationKey(selectedNavigationKey)
{
    qInfo().noquote() << QStringLiteral("[DemoStartup] DemoWindow ctor body begin");
    buildUi();
    qInfo().noquote() << QStringLiteral("[DemoStartup] DemoWindow ctor body done");
}

void DemoWindow::clearUi()
{
    QPointer<QWidget> oldRoot = m_rootWidget;
    QPointer<QWidget> oldLeftWidget = m_titleBarLeftOwnedWidget;
    QPointer<QWidget> oldCenterWidget = m_titleBarCenterOwnedWidget;
    QPointer<QWidget> oldRightWidget = m_titleBarRightOwnedWidget;
    QPointer<FluentMenuBar> oldMenuBar = m_ownedMenuBar;

    if (m_titleBarLeftOwnedWidget && fluentTitleBarLeftWidget() == m_titleBarLeftOwnedWidget) {
        setFluentTitleBarLeftWidget(nullptr);
    }
    if (m_titleBarCenterOwnedWidget && fluentTitleBarCenterWidget() == m_titleBarCenterOwnedWidget) {
        setFluentTitleBarCenterWidget(nullptr);
    }
    if (m_titleBarRightOwnedWidget && fluentTitleBarRightWidget() == m_titleBarRightOwnedWidget) {
        setFluentTitleBarRightWidget(nullptr);
    }

    m_titleBarLeftOwnedWidget = nullptr;
    m_titleBarCenterOwnedWidget = nullptr;
    m_titleBarRightOwnedWidget = nullptr;

    if (oldMenuBar) {
        oldMenuBar->hide();
        oldMenuBar->setParent(nullptr);
        setFluentMenuBar(nullptr);
        oldMenuBar->deleteLater();
    }
    m_ownedMenuBar = nullptr;

    if (oldRoot) {
        oldRoot->hide();
        setCentralWidget(nullptr);
        oldRoot->deleteLater();
    }
    m_rootWidget = nullptr;

    if (oldLeftWidget) {
        oldLeftWidget->hide();
        oldLeftWidget->deleteLater();
    }
    if (oldCenterWidget) {
        oldCenterWidget->hide();
        oldCenterWidget->deleteLater();
    }
    if (oldRightWidget) {
        oldRightWidget->hide();
        oldRightWidget->deleteLater();
    }
}

void DemoWindow::buildUi()
{
    DemoStartupTrace trace(QStringLiteral("DemoWindow::buildUi"));
    auto &window = *this;

    window.setWindowTitle(QStringLiteral("QtFluentWidgets Showcase"));
    ThemeManager::instance().setAccentColor(QColor(QStringLiteral("#5A48E8")));
    ThemeManager::instance().setAccentBorderEnabled(true);
    trace.mark(QStringLiteral("window title"));

    // Menu / Toolbar / StatusBar: demonstrate window-level Fluent components.
    auto *menuBar = new FluentMenuBar();
    m_ownedMenuBar = menuBar;
    trace.mark(QStringLiteral("menu bar object"));

    auto *fileMenu = menuBar->addFluentMenu(DEMO_TEXT("文件", "File"));
    QAction *exitAction = fileMenu->addAction(DEMO_TEXT("退出", "Exit"));
    exitAction->setIcon(FluentIcon::icon(FluentIconType::Close));
    QObject::connect(exitAction, &QAction::triggered, this, &QWidget::close);
    trace.mark(QStringLiteral("file menu"));

    window.setFluentMenuBar(menuBar);
    trace.mark(QStringLiteral("menu bar"));

    auto *demoMenu = menuBar->addFluentMenu(DEMO_TEXT("演示", "Demo"));
    QAction *msgInfo = demoMenu->addAction(DEMO_TEXT("消息框", "Message box"));
    QAction *dlgAction = demoMenu->addAction(DEMO_TEXT("对话框", "Dialog"));
    QAction *toastAction = demoMenu->addAction(DEMO_TEXT("Toast 通知", "Toast notification"));
    msgInfo->setIcon(FluentIcon::icon(FluentIconType::Info));
    dlgAction->setIcon(FluentIcon::icon(FluentIconType::Dialog));
    toastAction->setIcon(FluentIcon::icon(FluentIconType::Bell));

    auto showToast = [this]() {
        FluentToast::showToast(this,
                               QStringLiteral("Toast"),
                               DEMO_TEXT("演示 Toast（点击可关闭）", "Demo toast (click to dismiss)"),
                               m_toastPosition,
                               2600);
    };
    QObject::connect(toastAction, &QAction::triggered, this, showToast);

    // TitleBar custom slots: centered search + accent swatches + theme toggle.
    auto *search = new FluentLineEdit();
    search->setPlaceholderText(DEMO_TEXT("搜索控件 Search controls…", "Search controls…"));
    search->setFixedWidth(420);
    m_titleBarCenterOwnedWidget = search;
    window.setFluentTitleBarCenterWidget(search);

    {
        auto *right = new QWidget();
        m_titleBarRightOwnedWidget = right;
        auto *rl = new QHBoxLayout(right);
        rl->setContentsMargins(0, 0, 8, 0);
        rl->setSpacing(10);

        auto *accentGroup = new QButtonGroup(right);
        accentGroup->setExclusive(true);
        const std::array<QColor, 4> accents = {
            QColor(QStringLiteral("#5A48E8")),
            QColor(QStringLiteral("#8A46D8")),
            QColor(QStringLiteral("#0F9B8E")),
            QColor(QStringLiteral("#2563EB")),
        };
        AccentSwatchButton *swatch0 = nullptr;
        for (int i = 0; i < static_cast<int>(accents.size()); ++i) {
            const QColor c = accents[static_cast<size_t>(i)];
            auto *btn = new AccentSwatchButton(c, right);
            if (i == 0) {
                swatch0 = btn;
            }
            QObject::connect(btn, &QAbstractButton::clicked, this, [c] { ThemeManager::instance().setAccentColor(c); });
            rl->addWidget(btn);
            accentGroup->addButton(btn);
        }
        if (swatch0) {
            swatch0->setChecked(true);
        }

        const bool isDarkAtStartup = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;
        auto *themeSwitch = new FluentToggleSwitch(right);
        themeSwitch->setChecked(isDarkAtStartup);
        themeSwitch->setText(isDarkAtStartup ? DEMO_TEXT("深色", "Dark") : DEMO_TEXT("浅色", "Light"));
        QObject::connect(themeSwitch, &FluentToggleSwitch::toggled, this, [themeSwitch](bool on) {
            on ? ThemeManager::instance().setDarkMode() : ThemeManager::instance().setLightMode();
            themeSwitch->setText(on ? DEMO_TEXT("深色", "Dark") : DEMO_TEXT("浅色", "Light"));
        });
        QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, themeSwitch, [themeSwitch] {
            const bool dark = ThemeManager::instance().themeMode() == ThemeManager::ThemeMode::Dark;
            {
                const QSignalBlocker b(themeSwitch);
                themeSwitch->setChecked(dark);
            }
            themeSwitch->setText(dark ? DEMO_TEXT("深色", "Dark") : DEMO_TEXT("浅色", "Light"));
        });
        rl->addWidget(themeSwitch);

        window.setFluentTitleBarRightWidget(right);
    }
    trace.mark(QStringLiteral("title bar controls"));

    auto *root = new FluentWidget();
    m_rootWidget = root;
    root->setBackgroundRole(FluentWidget::BackgroundRole::WindowBackground);
    window.setCentralWidget(root);

    auto *rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *contentHost = new QWidget(root);
    contentHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *contentLayout = new QVBoxLayout(contentHost);
    contentLayout->setContentsMargins(0, 8, 16, 16);
    contentLayout->setSpacing(0);
    trace.mark(QStringLiteral("root layout"));

    // ---- NavigationView (left pane) ----
    auto *nav = new FluentNavigationView(root);
    nav->setExpandedWidth(280);
    nav->setCompactWidth(48);
    nav->setAutoCollapseWidth(800);
    nav->setPaneTitle(QStringLiteral("Qt Fluent"));
    nav->loadPaneToggleAnimation(Demo::demoLottieResourcePath(QStringLiteral("menu.json")));
    nav->loadBackButtonAnimation(Demo::demoLottieResourcePath(QStringLiteral("left-arrow.json")));

    auto applyIcon = [](FluentNavigationItem &item, FluentIconType type) {
        item.hasFluentIcon = true;
        item.fluentIcon = type;
    };
    auto applyAnimatedIcon = [](FluentNavigationItem &item, const QString &fileName) {
        item.animatedIconSource = Demo::demoLottieResourcePath(fileName);
    };

    // Build navigation items matching the demo pages
    using NI = FluentNavigationItem;

    std::vector<NI> mainItems;
    auto add = [&](const QString &key, const QString &zh, const QString &en, FluentIconType icon) {
        NI it; it.key = key; it.text = Demo::text(zh, en);
        it.hasFluentIcon = true; it.fluentIcon = icon;
        mainItems.push_back(it);
    };
    add(QStringLiteral("overview"),   QStringLiteral("总览"),       QStringLiteral("Overview"), FluentIconType::Home);
    { NI sep; sep.separator = true; mainItems.push_back(sep); }
    add(QStringLiteral("input"),      QStringLiteral("基本输入"),    QStringLiteral("Input"),    FluentIconType::Input);
    add(QStringLiteral("buttons"),    QStringLiteral("按钮/命令"),   QStringLiteral("Buttons"),  FluentIconType::Controls);
    add(QStringLiteral("pickers"),    QStringLiteral("选择器"),      QStringLiteral("Pickers"),  FluentIconType::Calendar);
    add(QStringLiteral("data"),       QStringLiteral("数据视图"),    QStringLiteral("Data"),     FluentIconType::Data);
    add(QStringLiteral("containers"), QStringLiteral("容器/布局"),   QStringLiteral("Layout"),   FluentIconType::Layout);
    add(QStringLiteral("windows"),    QStringLiteral("窗口/对话框"), QStringLiteral("Dialogs"),  FluentIconType::Dialog);
    add(QStringLiteral("motion"),     QStringLiteral("动态/图标"),   QStringLiteral("Motion"),   FluentIconType::Play);
    nav->setItems(mainItems);
    trace.mark(QStringLiteral("navigation items"));

    // Footer items (e.g. Settings / Theme)
    {
        NI settings;
        settings.key  = QStringLiteral("settings");
        settings.text = DEMO_TEXT("设置", "Settings");
        applyIcon(settings, FluentIconType::Settings);
        applyAnimatedIcon(settings, QStringLiteral("setting.json"));
        nav->addFooterItem(settings);
    }
    trace.mark(QStringLiteral("navigation footer"));

    // ---- Content area (stacked pages) ----
    auto *stack = new QStackedWidget(contentHost);

    const auto jumpTo = [nav](int pageIndex) {
        const QStringList keys = {
            QStringLiteral("overview"),
            QStringLiteral("input"),
            QStringLiteral("buttons"),
            QStringLiteral("pickers"),
            QStringLiteral("data"),
            QStringLiteral("containers"),
            QStringLiteral("windows"),
            QStringLiteral("motion"),
        };
        if (pageIndex >= 0 && pageIndex < keys.size()) {
            nav->setSelectedKey(keys[pageIndex]);
        }
    };

    auto pageNames = std::make_shared<QStringList>();
    auto pageFactories = std::make_shared<QVector<std::function<QWidget *()>>>();
    auto pageWidgets = std::make_shared<QVector<QPointer<QWidget>>>();
    auto *windowPtr = this;

    const auto addLazyPage = [&](const QString &name, std::function<QWidget *()> factory) {
        auto *placeholder = new QWidget(stack);
        placeholder->setProperty("_demoLazyPlaceholder", true);
        placeholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        stack->addWidget(placeholder);
        pageNames->append(name);
        pageFactories->append(std::move(factory));
        pageWidgets->append(placeholder);
    };

    addLazyPage(QStringLiteral("Overview"),   [windowPtr, jumpTo]() { return Demo::Pages::createOverviewPage(windowPtr, jumpTo); }); // 0
    addLazyPage(QStringLiteral("Input"),      [windowPtr]() { return Demo::Pages::createInputPage(windowPtr); });                    // 1
    addLazyPage(QStringLiteral("Buttons"),    [windowPtr]() { return Demo::Pages::createButtonsPage(windowPtr); });                  // 2
    addLazyPage(QStringLiteral("Pickers"),    [windowPtr]() { return Demo::Pages::createPickersPage(windowPtr); });                  // 3
    addLazyPage(QStringLiteral("Data"),       [windowPtr]() { return Demo::Pages::createDataViewsPage(windowPtr); });                // 4
    addLazyPage(QStringLiteral("Containers"), [windowPtr]() { return Demo::Pages::createContainersPage(windowPtr); });               // 5
    addLazyPage(QStringLiteral("Windows"),    [windowPtr]() { return Demo::Pages::createWindowsPage(windowPtr); });                  // 6
    addLazyPage(QStringLiteral("Motion"),     []() { return Demo::Pages::createMotionPage(); });                                     // 7
    addLazyPage(QStringLiteral("SettingsSidebar"), [this, windowPtr]() {                                                            // 8
        auto *settingsPage = new DemoSidebar(windowPtr, nullptr, false);
        QObject::connect(settingsPage, &DemoSidebar::toastPositionChanged, this, [this](FluentToast::Position pos) { m_toastPosition = pos; });
        QObject::connect(settingsPage, &DemoSidebar::languageChanged, this, &DemoWindow::switchLanguage);
        return settingsPage;
    });
    trace.mark(QStringLiteral("lazy page placeholders"));

    auto ensurePage = [stack, pageNames, pageFactories, pageWidgets](int index) -> QWidget * {
        if (!stack || index < 0 || index >= pageFactories->size() || index >= pageWidgets->size()) {
            return nullptr;
        }

        QWidget *current = pageWidgets->at(index);
        if (current && !current->property("_demoLazyPlaceholder").toBool()) {
            return current;
        }

        const QString name = index < pageNames->size() ? pageNames->at(index) : QString::number(index);
        QElapsedTimer timer;
        timer.start();
        QWidget *page = pageFactories->at(index)();
        const qint64 createdMs = timer.elapsed();
        qInfo().noquote() << QStringLiteral("[DemoStartup] lazy page %1 created +%2 ms")
                                  .arg(name)
                                  .arg(createdMs);
        if (!page) {
            return nullptr;
        }

        const int stackIndex = current ? stack->indexOf(current) : index;
        if (current) {
            stack->removeWidget(current);
            current->deleteLater();
        }
        stack->insertWidget(qBound(0, stackIndex, stack->count()), page);
        pageWidgets->replace(index, page);

        qInfo().noquote() << QStringLiteral("[DemoStartup] lazy page %1 installed +%2 ms")
                                  .arg(name)
                                  .arg(timer.elapsed() - createdMs);
        return page;
    };

    // Map navigation keys to stack indices
    QObject::connect(nav, &FluentNavigationView::selectedKeyChanged, this, [this, stack, ensurePage](const QString &key) {
        m_selectedNavigationKey = key;
        static const QHash<QString, int> keyMap = {
            { QStringLiteral("overview"),   0 }, { QStringLiteral("input"),      1 },
            { QStringLiteral("buttons"),    2 }, { QStringLiteral("pickers"),    3 },
            { QStringLiteral("data"),       4 }, { QStringLiteral("containers"), 5 },
            { QStringLiteral("windows"),    6 }, { QStringLiteral("motion"),     7 },
            { QStringLiteral("settings"),   8 },
        };
        auto it = keyMap.find(key);
        if (it != keyMap.end()) {
            const int previousIndex = stack->currentIndex();
            ensurePage(it.value());
            stack->setCurrentIndex(it.value());
            animateStackPageTransition(stack, stack->widget(it.value()), previousIndex, it.value());
        }
    });

    QObject::connect(search, &QLineEdit::returnPressed, this, [search, nav, mainItems]() {
        const QString q = search->text().trimmed();
        if (q.isEmpty()) { return; }
        for (const auto &it : mainItems) {
            if (it.separator) { continue; }
            if (it.text.contains(q, Qt::CaseInsensitive) || it.key.contains(q, Qt::CaseInsensitive)) {
                nav->setSelectedKey(it.key); return;
            }
        }
    });

    nav->setSelectedKey(m_selectedNavigationKey.isEmpty() ? QStringLiteral("overview") : m_selectedNavigationKey);
    trace.mark(QStringLiteral("signals and selected page"));

    rootLayout->addWidget(nav);
    rootLayout->addSpacing(8);
    contentLayout->addWidget(stack);
    rootLayout->addWidget(contentHost, 1);

    QObject::connect(msgInfo, &QAction::triggered, this, [this]() {
        FluentMessageBox::information(this,
                                     DEMO_TEXT("消息框", "Message box"),
                                     DEMO_TEXT("这是从菜单触发的消息框示例。", "This message box was triggered from the menu."),
                                     DEMO_TEXT("用于展示 MessageBox 与 Theme/Accent 联动。", "It demonstrates MessageBox linkage with Theme and Accent."));
    });

    QObject::connect(dlgAction, &QAction::triggered, this, [nav]() {
        nav->setSelectedKey(QStringLiteral("windows"));
    });
    trace.mark(QStringLiteral("final connections"));
}

void DemoWindow::switchLanguage(DemoLanguage language)
{
    if (m_languageSwitchPending || language == currentLanguage()) {
        return;
    }

    m_languageSwitchPending = true;

    QTimer::singleShot(0,
                       this,
                       [this, language]() { performQueuedLanguageSwitch(language); });
}

void DemoWindow::performQueuedLanguageSwitch(DemoLanguage language)
{
    if (QWidget *popup = QApplication::activePopupWidget()) {
        popup->close();
        QTimer::singleShot(0,
                           this,
                           [this, language]() { performQueuedLanguageSwitch(language); });
        return;
    }

    setLanguage(language);
    setUpdatesEnabled(false);
    clearUi();
    buildUi();
    setUpdatesEnabled(true);
    update();
    m_languageSwitchPending = false;
}

} // namespace Demo
