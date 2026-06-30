#include "PageButtons.h"

#include "../DemoHelpers.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QPixmap>
#include <QSize>
#include <QVBoxLayout>

#include "Fluent/FluentAnimatedButton.h"
#include "Fluent/FluentButton.h"
#include "Fluent/FluentAnimatedIcon.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentCheckBox.h"
#include "Fluent/FluentCommandBar.h"
#include "Fluent/FluentDropDownButton.h"
#include "Fluent/FluentIcon.h"
#include "Fluent/FluentIconButton.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentMenu.h"
#include "Fluent/FluentProgressBar.h"
#include "Fluent/FluentProgressRing.h"
#include "Fluent/FluentRadioButton.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentSplitButton.h"
#include "Fluent/FluentToggleSwitch.h"
#include "Fluent/FluentToolButton.h"

namespace Demo::Pages {

using namespace Fluent;

QWidget *createButtonsPage(FluentMainWindow *window)
{
    return Demo::makePage([&](QVBoxLayout *page) {
        page->addWidget(Demo::makePageHeader(DEMO_TEXT("按钮/命令","COMPONENTS · BUTTONS"), DEMO_TEXT("按钮 / 命令 Buttons","Buttons & Commands"), DEMO_TEXT("按钮、命令栏、下拉与拆分按钮等命令控件。","Buttons, command bar, dropdown and split buttons.")));

        auto s = Demo::makeSection(DEMO_TEXT("按钮 / 开关", "Buttons / Toggles"),
                                   DEMO_TEXT("包含 primary/secondary、disabled、checked，以及 CheckBox/Radio/Toggle", "Includes primary / secondary, disabled, checked, plus CheckBox / Radio / Toggle"));

        page->addWidget(s.card);

        // P0 command/button state matrix
        {
            const QString code = QStringLiteral(
                "auto *primary = new FluentButton(QStringLiteral(\"Primary\"));\n"
                "primary->setPrimary(true);\n"
                "auto *checked = new FluentButton(QStringLiteral(\"Checked\"));\n"
                "checked->setCheckable(true);\n"
                "checked->setChecked(true);\n"
                "auto *disabled = new FluentButton(QStringLiteral(\"Disabled\"));\n"
                "disabled->setDisabled(true);\n");

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("P0 Button State Matrix"),
                DEMO_TEXT("高频命令控件的 normal / active / disabled 横向对比", "Side-by-side normal / active / disabled states for high-frequency command controls"),
                DEMO_TEXT("要点：\n"
                          "-每行展示同一控件在常用状态下的密度、描边、文字与图标强度\n"
                          "-Primary、checked、menu、disabled 都应在浅色/深色主题下保持可读\n"
                          "-DropDownButton / SplitButton 与 CommandBar 使用同一 Fluent menu popup 体系",
                          "Highlights:\n"
                          "-Each row compares density, stroke, text, and icon strength across common states\n"
                          "-Primary, checked, menu, and disabled states should stay readable in light and dark themes\n"
                          "-DropDownButton / SplitButton and CommandBar use the same Fluent menu popup system"),
                code,
                [=](QVBoxLayout *body) {
                    auto *matrix = new QVBoxLayout();
                    matrix->setContentsMargins(0, 0, 0, 0);
                    matrix->setSpacing(12);

                    auto makeCaption = [](const QString &text, bool strong = false) {
                        auto *label = new FluentLabel(text);
                        label->setStyleSheet(strong
                                                 ? QStringLiteral("font-size: 12px; font-weight: 600; opacity: 0.9;")
                                                 : QStringLiteral("font-size: 12px; opacity: 0.78;"));
                        return label;
                    };

                    auto prepareButton = [](QWidget *widget) {
                        widget->setMinimumWidth(118);
                        return widget;
                    };

                    auto *header = new QHBoxLayout();
                    header->setContentsMargins(0, 0, 0, 0);
                    header->setSpacing(12);
                    auto *controlHeader = makeCaption(DEMO_TEXT("控件", "Control"), true);
                    controlHeader->setMinimumWidth(150);
                    header->addWidget(controlHeader, 0);
                    header->addWidget(makeCaption(DEMO_TEXT("Normal", "Normal"), true), 1);
                    header->addWidget(makeCaption(DEMO_TEXT("Active", "Active"), true), 1);
                    header->addWidget(makeCaption(DEMO_TEXT("Disabled", "Disabled"), true), 1);
                    matrix->addLayout(header);

                    auto addRow = [&](int row, const QString &name, QWidget *normal, QWidget *active, QWidget *disabled) {
                        Q_UNUSED(row)
                        auto *line = new QHBoxLayout();
                        line->setContentsMargins(0, 0, 0, 0);
                        line->setSpacing(12);
                        auto *caption = makeCaption(name);
                        caption->setMinimumWidth(150);
                        line->addWidget(caption, 0);
                        line->addWidget(prepareButton(normal), 1);
                        line->addWidget(prepareButton(active), 1);
                        line->addWidget(prepareButton(disabled), 1);
                        matrix->addLayout(line);
                    };

                    auto *buttonNormal = new FluentButton(DEMO_TEXT("Secondary", "Secondary"));
                    auto *buttonActive = new FluentButton(DEMO_TEXT("Checked", "Checked"));
                    buttonActive->setCheckable(true);
                    buttonActive->setChecked(true);
                    auto *buttonDisabled = new FluentButton(DEMO_TEXT("Disabled", "Disabled"));
                    buttonDisabled->setDisabled(true);
                    addRow(1, QStringLiteral("FluentButton"), buttonNormal, buttonActive, buttonDisabled);

                    auto *primaryNormal = new FluentButton(DEMO_TEXT("Primary", "Primary"));
                    primaryNormal->setPrimary(true);
                    auto *primaryActive = new FluentButton(DEMO_TEXT("Checked", "Checked"));
                    primaryActive->setPrimary(true);
                    primaryActive->setCheckable(true);
                    primaryActive->setChecked(true);
                    auto *primaryDisabled = new FluentButton(DEMO_TEXT("Disabled", "Disabled"));
                    primaryDisabled->setPrimary(true);
                    primaryDisabled->setDisabled(true);
                    addRow(2, DEMO_TEXT("Primary", "Primary"), primaryNormal, primaryActive, primaryDisabled);

                    auto *toolNormal = new FluentToolButton(DEMO_TEXT("Tool", "Tool"));
                    auto *toolActive = new FluentToolButton(DEMO_TEXT("Pinned", "Pinned"));
                    toolActive->setCheckable(true);
                    toolActive->setChecked(true);
                    auto *toolDisabled = new FluentToolButton(DEMO_TEXT("Disabled", "Disabled"));
                    toolDisabled->setDisabled(true);
                    addRow(3, QStringLiteral("FluentToolButton"), toolNormal, toolActive, toolDisabled);
                    body->addLayout(matrix);
                },
                false,
                120));
        }

        // FluentButton demo
        {
            QString code;
#define BUTTONS_FLUENT_BUTTON(X) \
    X(auto *btnRow = new QHBoxLayout();) \
    X(btnRow->setContentsMargins(0, 0, 0, 0);) \
    X(btnRow->setSpacing(10);) \
    X(auto *p = new FluentButton(QStringLiteral("Primary"));) \
    X(p->setPrimary(true);) \
    X(auto *pDis = new FluentButton(QStringLiteral("Primary Disabled"));) \
    X(pDis->setPrimary(true);) \
    X(pDis->setDisabled(true);) \
    X(auto *sec = new FluentButton(QStringLiteral("Secondary"));) \
    X(auto *secChecked = new FluentButton(QStringLiteral("Secondary Checked"));) \
    X(secChecked->setCheckable(true);) \
    X(secChecked->setChecked(true);) \
    X(btnRow->addWidget(p);) \
    X(btnRow->addWidget(pDis);) \
    X(btnRow->addWidget(sec);) \
    X(btnRow->addWidget(secChecked);) \
    X(btnRow->addStretch(1);) \
    X(body->addLayout(btnRow);) \
    X(auto *iconRow = new QHBoxLayout();) \
    X(iconRow->setContentsMargins(0, 0, 0, 0);) \
    X(iconRow->setSpacing(10);) \
    X(const QIcon searchIcon = FluentIcon::icon(FluentIconType::Search);) \
    X(auto *iconLeft = new FluentButton(searchIcon, QStringLiteral("Left"));) \
    X(auto *iconRight = new FluentButton(searchIcon, QStringLiteral("Right"));) \
    X(iconRight->setIconPosition(FluentButton::IconPosition::Right);) \
    X(auto *iconTop = new FluentButton(searchIcon, QStringLiteral("Top"));) \
    X(iconTop->setIconPosition(FluentButton::IconPosition::Top);) \
    X(auto *iconBottom = new FluentButton(searchIcon, QStringLiteral("Bottom"));) \
    X(iconBottom->setIconPosition(FluentButton::IconPosition::Bottom);) \
    X(auto *pill = new FluentButton(FluentIcon::icon(FluentIconType::Bookmark), QStringLiteral("Tag"));) \
    X(pill->setPrimary(true);) \
    X(pill->setShape(FluentButton::Shape::Pill);) \
    X(auto *iconOnly = new FluentIconButton(searchIcon);) \
    X(iconOnly->setShape(FluentButton::Shape::Circular);) \
    X(iconOnly->setButtonExtent(36);) \
    X(iconRow->addWidget(iconLeft);) \
    X(iconRow->addWidget(iconRight);) \
    X(iconRow->addWidget(iconTop);) \
    X(iconRow->addWidget(iconBottom);) \
    X(iconRow->addWidget(pill);) \
    X(iconRow->addWidget(iconOnly);) \
    X(iconRow->addStretch(1);) \
    X(body->addLayout(iconRow);)

#define X(line) code += QStringLiteral(#line "\n");
            BUTTONS_FLUENT_BUTTON(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentButton"),
                DEMO_TEXT("Primary/Secondary、禁用、可勾选状态与图标位置",
                          "Primary / secondary, disabled, checkable states, and icon placement"),
                DEMO_TEXT("要点：\n"
                          "-setPrimary(true) 切换主按钮样式\n"
                          "-setDisabled(true) 展示禁用态\n"
                          "-setCheckable(true) + setChecked(true) 展示 Checked\n"
                          "-setIconPosition(...) 调整图标位置\n"
                          "-setShape(Pill/Circular) 展示胶囊/圆形按钮",
                          "Highlights:\n"
                          "-Use setPrimary(true) to switch to the primary button style\n"
                          "-Use setDisabled(true) to show the disabled state\n"
                          "-Use setCheckable(true) + setChecked(true) to show the checked state\n"
                          "-Use setIconPosition(...) to adjust icon placement\n"
                          "-Use setShape(Pill/Circular) for pill and circular buttons"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    BUTTONS_FLUENT_BUTTON(X)
#undef X
                },
                false));

#undef BUTTONS_FLUENT_BUTTON
        }

        // Local image icon demo
        {
            QString code;
#define BUTTONS_LOCAL_IMAGE_ICON(X) \
    X(auto *row = new QHBoxLayout();) \
    X(row->setContentsMargins(0, 0, 0, 0);) \
    X(row->setSpacing(10);) \
    X(auto *preview = new FluentButton(FluentIcon::icon(FluentIconType::Folder), DEMO_TEXT("图片图标", "Image icon"));) \
    X(preview->setIconSize(QSize(24, 24));) \
    X(preview->setMinimumWidth(150);) \
    X(auto *choose = new FluentButton(FluentIcon::icon(FluentIconType::Folder), DEMO_TEXT("选择图片", "Choose image"));) \
    X(choose->setPrimary(true);) \
    X(row->addWidget(choose);) \
    X(row->addWidget(preview);) \
    X(row->addStretch(1);) \
    X(body->addLayout(row);) \
    X(auto *status = new FluentLabel(DEMO_TEXT("尚未选择本地图片。", "No local image selected."));) \
    X(status->setWordWrap(true);) \
    X(body->addWidget(status);) \
    X(QObject::connect(choose, &QPushButton::clicked, preview, [=]() { const QString fileName = QFileDialog::getOpenFileName(window, DEMO_TEXT("选择按钮图标", "Choose button icon"), QString(), DEMO_TEXT("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.ico);;所有文件 (*)", "Image files (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.ico);;All files (*)")); if (fileName.isEmpty()) { return; } const QPixmap pixmap(fileName); if (pixmap.isNull()) { status->setText(DEMO_TEXT("无法加载所选图片。", "Unable to load the selected image.")); return; } preview->setIcon(QIcon(pixmap)); preview->setIconSize(QSize(24, 24)); status->setText(DEMO_TEXT("已将本地图片设置为按钮图标：%1", "Local image is now used as the button icon: %1").arg(QFileInfo(fileName).fileName())); });)

#define X(line) code += QStringLiteral(#line "\n");
            BUTTONS_LOCAL_IMAGE_ICON(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("Local image button icon"),
                DEMO_TEXT("从本地图片文件创建 QIcon，并应用到 FluentButton",
                          "Create a QIcon from a local image file and apply it to FluentButton"),
                DEMO_TEXT("要点：\n"
                          "-QFileDialog 选择 png/jpg/webp/ico 等图片\n"
                          "-QPixmap 校验加载结果，再用 QIcon(pixmap) 设置按钮图标\n"
                          "-setIconSize(...) 控制本地图标在按钮中的显示尺寸",
                          "Highlights:\n"
                          "-Use QFileDialog to choose png / jpg / webp / ico images\n"
                          "-Validate loading with QPixmap, then use QIcon(pixmap) as the button icon\n"
                          "-Use setIconSize(...) to control how large the local image appears in the button"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    BUTTONS_LOCAL_IMAGE_ICON(X)
#undef X
                },
                false));

#undef BUTTONS_LOCAL_IMAGE_ICON
        }

        // FluentToolButton demo
        {
            QString code;
#define BUTTONS_TOOL_BUTTON(X) \
    X(auto *toolRow = new QHBoxLayout();) \
    X(toolRow->setContentsMargins(0, 0, 0, 0);) \
    X(toolRow->setSpacing(10);) \
    X(auto *tb1 = new FluentToolButton(QStringLiteral("Tool"));) \
    X(auto *tb2 = new FluentToolButton(QStringLiteral("Tool Checked"));) \
    X(tb2->setCheckable(true);) \
    X(tb2->setChecked(true);) \
    X(auto *roundTool = new FluentToolButton();) \
    X(roundTool->setIcon(FluentIcon::icon(FluentIconType::Mail));) \
    X(roundTool->setShape(FluentToolButton::Shape::Circular);) \
    X(FluentIconOptions reversedIcon;) \
    X(reversedIcon.reversed = true;) \
    X(auto *accentMenu = new FluentToolButton(QStringLiteral("Mail"));) \
    X(accentMenu->setPrimary(true);) \
    X(accentMenu->setShape(FluentToolButton::Shape::Pill);) \
    X(accentMenu->setIcon(FluentIcon::icon(FluentIconType::Mail, reversedIcon));) \
    X(accentMenu->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);) \
    X(accentMenu->setPopupMode(QToolButton::MenuButtonPopup);) \
    X(auto *mailMenu = new FluentMenu(accentMenu);) \
    X(mailMenu->addAction(FluentIcon::icon(FluentIconType::Save), QStringLiteral("Save"));) \
    X(mailMenu->addAction(FluentIcon::icon(FluentIconType::Upload), QStringLiteral("Send"));) \
    X(accentMenu->setMenu(mailMenu);) \
    X(toolRow->addWidget(tb1);) \
    X(toolRow->addWidget(tb2);) \
    X(toolRow->addWidget(roundTool);) \
    X(toolRow->addWidget(accentMenu);) \
    X(toolRow->addStretch(1);) \
    X(body->addLayout(toolRow);)

#define X(line) code += QStringLiteral(#line "\n");
            BUTTONS_TOOL_BUTTON(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentToolButton"),
                DEMO_TEXT("更紧凑的按钮，适合工具栏/图标动作", "A more compact button suited to toolbars and icon actions"),
                DEMO_TEXT("要点：\n"
                          "-可 checkable（用于 Toggle 工具）\n"
                          "-支持 Primary、胶囊/圆形形态与 FluentMenu 菜单",
                          "Highlights:\n"
                          "-Can be checkable for toggle-style tools\n"
                          "-Supports Primary, pill/circular shapes, and FluentMenu popups"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    BUTTONS_TOOL_BUTTON(X)
#undef X
                }));

#undef BUTTONS_TOOL_BUTTON
        }

        // FluentAnimatedButton demo
        {
            QString code;
#define BUTTONS_ANIMATED_BUTTON(X) \
    X(auto *row = new QHBoxLayout();) \
    X(row->setContentsMargins(0, 0, 0, 0);) \
    X(row->setSpacing(10);) \
    X(auto *search = new FluentAnimatedButton(DEMO_TEXT("搜索", "Search"));) \
    X(search->loadAnimationData(Demo::animatedSearchIconJson(), QStringLiteral("demo-search-button"));) \
    X(auto *primary = new FluentAnimatedButton(DEMO_TEXT("Primary Search", "Primary Search"));) \
    X(primary->setPrimary(true);) \
    X(primary->loadAnimationData(Demo::animatedSearchIconJson(), QStringLiteral("demo-primary-search-button"));) \
    X(auto *iconOnly = new FluentAnimatedButton();) \
    X(iconOnly->setFixedSize(40, 32);) \
    X(iconOnly->loadAnimationData(Demo::animatedSearchIconJson(), QStringLiteral("demo-icon-only-search-button"));) \
    X(row->addWidget(search);) \
    X(row->addWidget(primary);) \
    X(row->addWidget(iconOnly);) \
    X(row->addStretch(1);) \
    X(body->addLayout(row);)

#define X(line) code += QStringLiteral(#line "\n");
            BUTTONS_ANIMATED_BUTTON(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentAnimatedButton"),
                DEMO_TEXT("带 Lottie marker 动效的按钮", "Button with Lottie marker animation"),
                DEMO_TEXT("要点：\n"
                          "-按钮本身仍是 QPushButton 语义，可直接连接 clicked/toggled\n"
                          "-hover 自动播放 NormalToPointerOver，press 自动播放 PointerOverToPressed\n"
                          "-支持 Primary 与纯图标尺寸",
                          "Highlights:\n"
                          "-Still uses QPushButton semantics, so clicked/toggled work normally\n"
                          "-Hover plays NormalToPointerOver; press plays PointerOverToPressed\n"
                          "-Supports primary and icon-only layouts"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    BUTTONS_ANIMATED_BUTTON(X)
#undef X
                },
                false));

#undef BUTTONS_ANIMATED_BUTTON
        }

        // AnimatedIcon demo
        {
            QString code;
#define BUTTONS_ANIMATED_ICON(X) \
    X(auto *row = new QHBoxLayout();) \
    X(row->setContentsMargins(0, 0, 0, 0);) \
    X(row->setSpacing(10);) \
    X(auto *icon = new FluentAnimatedIcon();) \
    X(icon->setFixedSize(64, 64);) \
    X(icon->loadData(Demo::animatedSearchIconJson(), QStringLiteral("demo-search-icon"));) \
    X(icon->setInteractive(true);) \
    X(icon->setState(QStringLiteral("Normal"), false);) \
    X(auto *normal = new FluentButton(QStringLiteral("Normal"));) \
    X(auto *over = new FluentButton(QStringLiteral("PointerOver"));) \
    X(auto *pressed = new FluentButton(QStringLiteral("Pressed"));) \
    X(QObject::connect(normal, &QPushButton::clicked, icon, [icon]() { icon->setState(QStringLiteral("Normal")); });) \
    X(QObject::connect(over, &QPushButton::clicked, icon, [icon]() { icon->setState(QStringLiteral("PointerOver")); });) \
    X(QObject::connect(pressed, &QPushButton::clicked, icon, [icon]() { icon->setState(QStringLiteral("Pressed")); });) \
    X(row->addWidget(icon);) \
    X(row->addWidget(normal);) \
    X(row->addWidget(over);) \
    X(row->addWidget(pressed);) \
    X(row->addStretch(1);) \
    X(body->addLayout(row);)

#define X(line) code += QStringLiteral(#line "\n");
            BUTTONS_ANIMATED_ICON(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentAnimatedIcon"),
                DEMO_TEXT("基于 Lottie marker 的状态动画图标", "Stateful animated icon driven by Lottie markers"),
                DEMO_TEXT("要点：\n"
                          "-loadData()/load() 加载 Lottie JSON\n"
                          "-setState(\"Normal/PointerOver/Pressed\") 使用 marker 切换状态\n"
                          "-setInteractive(true) 让控件自身 hover/press 自动切换状态",
                          "Highlights:\n"
                          "-Use loadData() or load() to load Lottie JSON\n"
                          "-Use setState(\"Normal/PointerOver/Pressed\") to switch marker states\n"
                          "-Use setInteractive(true) for automatic hover / press states"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    BUTTONS_ANIMATED_ICON(X)
#undef X
                }));

#undef BUTTONS_ANIMATED_ICON
        }

        // Commanding controls
        {
            QString code;
#define BUTTONS_COMMANDING(X) \
    X(auto *status = new FluentLabel(DEMO_TEXT("命令状态：选择一个命令，或打开只读模式查看 QAction 状态同步。", "Command status: choose a command, or enable read-only mode to see QAction state sync."));) \
    X(status->setWordWrap(true);) \
    X(auto *row = new QHBoxLayout();) \
    X(row->setContentsMargins(0, 0, 0, 0);) \
    X(row->setSpacing(10);) \
    X(auto *drop = new FluentDropDownButton(DEMO_TEXT("更多操作", "More actions"));) \
    X(auto *copyFromDrop = drop->addAction(DEMO_TEXT("复制链接", "Copy link"));) \
    X(auto *moveFromDrop = drop->addAction(DEMO_TEXT("移动到...", "Move to..."));) \
    X(auto *split = new FluentSplitButton(DEMO_TEXT("保存", "Save"));) \
    X(auto *saveAction = new QAction(DEMO_TEXT("保存", "Save"), split);) \
    X(split->setDefaultAction(saveAction);) \
    X(auto *saveMenu = new FluentMenu(split);) \
    X(saveMenu->addAction(DEMO_TEXT("另存为", "Save as"));) \
    X(saveMenu->addAction(DEMO_TEXT("保存副本", "Save a copy"));) \
    X(split->setMenu(saveMenu);) \
    X(row->addWidget(drop);) \
    X(row->addWidget(split);) \
    X(row->addStretch(1);) \
    X(body->addLayout(row);) \
    X(auto *bar = new FluentCommandBar();) \
    X(auto *newAction = new QAction(DEMO_TEXT("新建", "New"), bar);) \
    X(auto *renameAction = new QAction(DEMO_TEXT("重命名", "Rename"), bar);) \
    X(auto *deleteAction = new QAction(DEMO_TEXT("删除", "Delete"), bar);) \
    X(bar->addCommand(newAction);) \
    X(bar->addCommand(renameAction);) \
    X(bar->addSeparator();) \
    X(bar->addCommand(deleteAction);) \
    X(auto *more = new FluentMenu(bar);) \
    X(more->setTitle(DEMO_TEXT("导出", "Export"));) \
    X(auto *pdfAction = more->addAction(QStringLiteral("PDF"));) \
    X(auto *pngAction = more->addAction(QStringLiteral("PNG"));) \
    X(bar->addCommand(more->menuAction());) \
    X(bar->addSeparator();) \
    X(auto *readOnly = new FluentToggleSwitch(DEMO_TEXT("只读", "Read only"));) \
    X(bar->addWidget(readOnly);) \
    X(body->addWidget(bar);) \
    X(body->addWidget(status);) \
    X(QObject::connect(copyFromDrop, &QAction::triggered, status, [=]() { status->setText(DEMO_TEXT("命令状态：已复制链接。", "Command status: copied link.")); });) \
    X(QObject::connect(moveFromDrop, &QAction::triggered, status, [=]() { status->setText(DEMO_TEXT("命令状态：打开移动目标选择。", "Command status: opened move destination picker.")); });) \
    X(QObject::connect(saveAction, &QAction::triggered, status, [=]() { status->setText(DEMO_TEXT("命令状态：默认保存动作已执行。", "Command status: default save action ran.")); });) \
    X(QObject::connect(newAction, &QAction::triggered, status, [=]() { status->setText(DEMO_TEXT("命令状态：新建文档。", "Command status: new document.")); });) \
    X(QObject::connect(renameAction, &QAction::triggered, status, [=]() { status->setText(DEMO_TEXT("命令状态：进入重命名。", "Command status: renaming.")); });) \
    X(QObject::connect(deleteAction, &QAction::triggered, status, [=]() { status->setText(DEMO_TEXT("命令状态：删除命令已执行。", "Command status: delete command ran.")); });) \
    X(QObject::connect(pdfAction, &QAction::triggered, status, [=]() { status->setText(DEMO_TEXT("命令状态：导出 PDF。", "Command status: export PDF.")); });) \
    X(QObject::connect(pngAction, &QAction::triggered, status, [=]() { status->setText(DEMO_TEXT("命令状态：导出 PNG。", "Command status: export PNG.")); });) \
    X(QObject::connect(readOnly, &FluentToggleSwitch::toggled, status, [=](bool checked) { renameAction->setEnabled(!checked); deleteAction->setEnabled(!checked); status->setText(checked ? DEMO_TEXT("命令状态：只读模式已开启，编辑类命令自动禁用。", "Command status: read-only mode is on; editing commands are disabled.") : DEMO_TEXT("命令状态：只读模式已关闭，命令恢复可用。", "Command status: read-only mode is off; commands are enabled.")); });)

#define X(line) code += QStringLiteral(#line "\n");
            BUTTONS_COMMANDING(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("CommandBar / DropDownButton / SplitButton"),
                DEMO_TEXT("命令入口：独立下拉、默认动作与状态同步命令栏", "Command entry points: standalone drop-down, default action, and state-synced command bar"),
                DEMO_TEXT("要点：\n"
                          "-FluentDropDownButton 适合单个按钮展开一组次级动作\n"
                          "-FluentSplitButton 左侧执行默认动作，右侧展开同一动作的变体\n"
                          "-FluentCommandBar 以 QAction 为中心组织常用命令、分隔线、菜单命令和自定义控件",
                          "Highlights:\n"
                          "-FluentDropDownButton is for one button that opens secondary actions\n"
                          "-FluentSplitButton runs a default action on the left and opens variants on the right\n"
                          "-FluentCommandBar organizes frequent QAction commands, separators, menu commands, and custom widgets"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    BUTTONS_COMMANDING(X)
#undef X
                },
                false,
                250));

#undef BUTTONS_COMMANDING
        }

        // Check/Radio/Toggle demo
        {
            QString code;
#define BUTTONS_OPTION(X) \
    X(auto *optRow = new QHBoxLayout();) \
    X(optRow->setContentsMargins(0, 0, 0, 0);) \
    X(optRow->setSpacing(14);) \
    X(auto *cb = new FluentCheckBox(QStringLiteral("CheckBox"));) \
    X(auto *mixed = new FluentCheckBox(QStringLiteral("Mixed"));) \
    X(mixed->setTristate(true);) \
    X(mixed->setCheckState(Qt::PartiallyChecked);) \
    X(auto *ra = new FluentRadioButton(QStringLiteral("Radio A"));) \
    X(auto *rb = new FluentRadioButton(QStringLiteral("Radio B"));) \
    X(ra->setChecked(true);) \
    X(auto *tg = new FluentToggleSwitch(QStringLiteral("Toggle"));) \
    X(tg->setChecked(true);) \
    X(optRow->addWidget(cb);) \
    X(optRow->addWidget(mixed);) \
    X(optRow->addWidget(ra);) \
    X(optRow->addWidget(rb);) \
    X(optRow->addWidget(tg);) \
    X(optRow->addStretch(1);) \
    X(body->addLayout(optRow);)

#define X(line) code += QStringLiteral(#line "\n");
            BUTTONS_OPTION(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("CheckBox / RadioButton / ToggleSwitch"),
                DEMO_TEXT("常见选择/开关控件", "Common selection and toggle controls"),
                DEMO_TEXT("要点：\n"
                          "-CheckBox：多选\n"
                          "-CheckBox 支持 Qt::PartiallyChecked 三态显示\n"
                          "-RadioButton：单选（同父布局/同组）\n"
                          "-ToggleSwitch：开关语义更明确",
                          "Highlights:\n"
                          "-CheckBox: multi-selection\n"
                          "-CheckBox supports the Qt::PartiallyChecked tristate visual\n"
                          "-RadioButton: single-selection within the same parent or group\n"
                          "-ToggleSwitch: clearer on/off semantics"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    BUTTONS_OPTION(X)
#undef X
                }));

#undef BUTTONS_OPTION
        }

        // ProgressBar demo
        {
            QString code;
#define BUTTONS_PROGRESS(X) \
    X(auto *pb = new FluentProgressBar();) \
    X(pb->setRange(0, 100);) \
    X(pb->setValue(62);) \
    X(pb->setFixedWidth(340);) \
    X(auto *row = new QHBoxLayout();) \
    X(row->setContentsMargins(0, 0, 0, 0);) \
    X(row->setSpacing(14);) \
    X(auto *ring = new FluentProgressRing();) \
    X(ring->setFixedSize(42, 42);) \
    X(ring->setValue(62);) \
    X(auto *busy = new FluentProgressRing();) \
    X(busy->setFixedSize(42, 42);) \
    X(busy->setIndeterminate(true);) \
    X(row->addWidget(pb, 1);) \
    X(row->addWidget(ring);) \
    X(row->addWidget(busy);) \
    X(body->addLayout(row);)

#define X(line) code += QStringLiteral(#line "\n");
            BUTTONS_PROGRESS(X)
#undef X

            page->addWidget(Demo::makeCollapsedExample(
                QStringLiteral("FluentProgressBar / FluentProgressRing"),
                DEMO_TEXT("线性进度与环形进度（Accent 会影响进度颜色）", "Linear and ring progress indicators (Accent affects the progress color)"),
                DEMO_TEXT("要点：\n"
                          "-setRange()/setValue()\n"
                          "-ProgressRing 支持确定进度与 indeterminate 忙碌态\n"
                          "-主题/Accent 联动（填充色、背景）",
                          "Highlights:\n"
                          "-Use setRange() and setValue()\n"
                          "-ProgressRing supports determinate progress and indeterminate busy state\n"
                          "-Theme and Accent both influence the fill and background"),
                code,
                [=](QVBoxLayout *body) {
#define X(line) line
                    BUTTONS_PROGRESS(X)
#undef X
                }));

#undef BUTTONS_PROGRESS
        }
    });
}

} // namespace Demo::Pages
