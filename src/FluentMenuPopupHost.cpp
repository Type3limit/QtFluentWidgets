#include "FluentMenuPopupHost.h"

#include "Fluent/FluentMotion.h"
#include "Fluent/FluentPopupSurface.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "FluentPopupUtils.h"

#include <QApplication>
#include <QCursor>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QScreen>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>

#include <cmath>

namespace Fluent {

namespace {

constexpr int kActionRole = Qt::UserRole + 1;
constexpr int kSeparatorRole = Qt::UserRole + 2;
constexpr int kShortcutRole = Qt::UserRole + 3;

constexpr int kMenuMinWidth = 148;
constexpr int kMenuItemHeight = 34;
constexpr int kSeparatorHeight = 13;
constexpr int kViewportPaddingY = 6;
constexpr int kRowInsetX = 6;
constexpr int kRowInsetY = 2;
constexpr int kCheckSlotWidth = 24;
constexpr int kIconSlotWidth = 28;
constexpr int kShortcutGap = 16;
constexpr int kSubMenuSlotWidth = 22;
constexpr int kTextRightPadding = 12;

QString textWithoutMnemonic(QString text)
{
    text.remove(QLatin1Char('&'));
    return text;
}

QAction *actionFromIndex(const QModelIndex &index)
{
    const quintptr value = static_cast<quintptr>(index.data(kActionRole).toULongLong());
    return reinterpret_cast<QAction *>(value);
}

QAction *actionFromItem(const QListWidgetItem *item)
{
    if (!item) {
        return nullptr;
    }
    const quintptr value = static_cast<quintptr>(item->data(kActionRole).toULongLong());
    return reinterpret_cast<QAction *>(value);
}

QString actionShortcutText(QAction *action)
{
    if (!action) {
        return {};
    }

    QString text = action->text();
    const qsizetype tabPos = text.indexOf(QLatin1Char('\t'));
    if (tabPos >= 0) {
        return text.mid(tabPos + 1);
    }

    return action->shortcut().isEmpty()
        ? QString()
        : action->shortcut().toString(QKeySequence::NativeText);
}

QString actionLabelText(QAction *action)
{
    if (!action) {
        return {};
    }

    QString text = action->text();
    const qsizetype tabPos = text.indexOf(QLatin1Char('\t'));
    if (tabPos >= 0) {
        text = text.left(tabPos);
    }
    return text;
}

static QRect popupAnchorRect(const QMenu *menu)
{
    if (!menu) {
        return QRect();
    }

    const auto *parentMenu = qobject_cast<const QMenu *>(menu->parentWidget());
    if (parentMenu) {
        const QList<QAction *> parentActions = parentMenu->actions();
        for (QAction *action : parentActions) {
            if (!action || action->menu() != menu) {
                continue;
            }

            const QRect actionRect = parentMenu->actionGeometry(action);
            if (actionRect.isValid()) {
                return QRect(parentMenu->mapToGlobal(actionRect.topLeft()), actionRect.size());
            }
            break;
        }
    }

    if (const QWidget *anchor = menu->parentWidget()) {
        return QRect(anchor->mapToGlobal(QPoint(0, 0)), anchor->size());
    }

    return QRect();
}

static int popupOpenSlideOffsetY(const QMenu *menu, const QPoint &finalContentTopLeft, int contentHeight)
{
    const QRect anchorRect = popupAnchorRect(menu);
    if (anchorRect.isValid()) {
        if (finalContentTopLeft.y() >= anchorRect.bottom()) {
            return -FluentMotion::popupSlideOffset();
        }

        if (finalContentTopLeft.y() + contentHeight <= anchorRect.top()) {
            return FluentMotion::popupSlideOffset();
        }
    }

    QScreen *screen = QApplication::screenAt(finalContentTopLeft);
    if (!screen) {
        screen = QApplication::primaryScreen();
    }

    const QRect avail = screen ? screen->availableGeometry() : QRect();
    if (avail.isValid() && finalContentTopLeft.y() > avail.center().y()) {
        return FluentMotion::popupSlideOffset();
    }

    return -FluentMotion::popupSlideOffset();
}

class FluentMenuItemDelegate final : public QStyledItemDelegate
{
public:
    explicit FluentMenuItemDelegate(MenuActionListWidget *view)
        : QStyledItemDelegate(nullptr)
        , m_view(view)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    MenuActionListWidget *m_view = nullptr;
};

} // namespace

class MenuActionListWidget final : public QListWidget
{
public:
    explicit MenuActionListWidget(QWidget *parent = nullptr)
        : QListWidget(parent)
    {
        setObjectName(QStringLiteral("FluentMenuPopupList"));
        setFrameShape(QFrame::NoFrame);
        setMouseTracking(true);
        setFocusPolicy(Qt::NoFocus);
        setSelectionMode(QAbstractItemView::NoSelection);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setUniformItemSizes(false);
        setViewportMargins(0, kViewportPaddingY, 0, kViewportPaddingY);
        setAttribute(Qt::WA_StyledBackground, false);
        setAutoFillBackground(false);
        viewport()->setAttribute(Qt::WA_StyledBackground, false);
        viewport()->setAutoFillBackground(false);
        setStyleSheet(QStringLiteral(
            "QListWidget{background:transparent;border:none;outline:0px;}"
            "QListWidget::item{background:transparent;border:none;}"));
        auto *delegate = new FluentMenuItemDelegate(this);
        delegate->setParent(this);
        setItemDelegate(delegate);
    }

    std::function<void(QAction *)> onHoverChanged;
    std::function<void(QAction *)> onActionTriggered;
    std::function<void()> onEntered;
    std::function<void()> onLeft;

    void rebuildFromMenu(QMenu *menu)
    {
        clear();
        m_actionItems.clear();
        m_hoverAction = nullptr;
        m_hasIcons = false;
        m_hasCheckable = false;
        m_hasSubMenus = false;

        const QList<QAction *> actions = menu ? menu->actions() : QList<QAction *>();
        for (QAction *action : actions) {
            if (!action || !action->isVisible()) {
                continue;
            }
            if (!action->isSeparator()) {
                m_hasIcons = m_hasIcons || !action->icon().isNull();
                m_hasCheckable = m_hasCheckable || action->isCheckable();
                m_hasSubMenus = m_hasSubMenus || action->menu();
            }
        }

        QFontMetrics fm(font());
        int width = kMenuMinWidth;
        int height = kViewportPaddingY * 2;

        const int leftSlots = (m_hasCheckable ? kCheckSlotWidth : 0)
                            + (m_hasIcons ? kIconSlotWidth : 0);
        const int arrowSlot = m_hasSubMenus ? kSubMenuSlotWidth : 0;

        for (QAction *action : actions) {
            if (!action || !action->isVisible()) {
                continue;
            }

            auto *item = new QListWidgetItem();
            item->setData(kActionRole, QVariant::fromValue<qulonglong>(reinterpret_cast<quintptr>(action)));
            item->setData(kSeparatorRole, action->isSeparator());
            if (action->isSeparator()) {
                item->setSizeHint(QSize(width, kSeparatorHeight));
                height += kSeparatorHeight;
                addItem(item);
                continue;
            }

            const QString label = actionLabelText(action);
            const QString shortcut = actionShortcutText(action);
            item->setText(label);
            item->setData(kShortcutRole, shortcut);
            item->setSizeHint(QSize(width, kMenuItemHeight));
            addItem(item);
            m_actionItems.insert(action, item);

            const int textWidth = fm.horizontalAdvance(textWithoutMnemonic(label));
            const int shortcutWidth = shortcut.isEmpty() ? 0 : fm.horizontalAdvance(shortcut) + kShortcutGap;
            width = qMax(width,
                         kRowInsetX * 2 + leftSlots + 8 + textWidth + shortcutWidth
                             + arrowSlot + kTextRightPadding);
            height += kMenuItemHeight;
        }

        m_contentSize = QSize(width, height);
        for (int row = 0; row < count(); ++row) {
            if (auto *item = this->item(row)) {
                const int itemHeight = item->data(kSeparatorRole).toBool() ? kSeparatorHeight : kMenuItemHeight;
                item->setSizeHint(QSize(width, itemHeight));
            }
        }
        setFixedSize(m_contentSize);
    }

    QSize contentSizeHint() const { return m_contentSize.isValid() ? m_contentSize : QSize(kMenuMinWidth, kViewportPaddingY * 2); }
    bool hasIcons() const { return m_hasIcons; }
    bool hasCheckable() const { return m_hasCheckable; }
    bool hasSubMenus() const { return m_hasSubMenus; }
    QAction *hoveredAction() const { return m_hoverAction; }

    QRect actionViewportRect(QAction *action) const
    {
        QListWidgetItem *item = m_actionItems.value(action, nullptr);
        return item ? visualItemRect(item) : QRect();
    }

    QAction *actionAtViewportPos(const QPoint &pos) const
    {
        QListWidgetItem *item = itemAt(pos);
        if (!item || item->data(kSeparatorRole).toBool()) {
            return nullptr;
        }
        return actionFromItem(item);
    }

    void setHoveredAction(QAction *action)
    {
        if (action == m_hoverAction) {
            return;
        }

        m_hoverAction = action;
        if (QListWidgetItem *item = m_actionItems.value(action, nullptr)) {
            setCurrentItem(item);
        } else {
            setCurrentItem(nullptr);
        }
        viewport()->update();
    }

    void moveHover(int delta)
    {
        if (count() <= 0 || delta == 0) {
            return;
        }

        int row = currentRow();
        if (row < 0) {
            row = delta > 0 ? -1 : count();
        }

        for (int i = 0; i < count(); ++i) {
            row = (row + delta + count()) % count();
            QListWidgetItem *candidate = item(row);
            QAction *action = actionFromItem(candidate);
            if (action && !candidate->data(kSeparatorRole).toBool()) {
                setHoveredAction(action);
                if (onHoverChanged) {
                    onHoverChanged(action);
                }
                scrollToItem(candidate);
                return;
            }
        }
    }

protected:
    void enterEvent(FluentEnterEvent *event) override
    {
        if (onEntered) {
            onEntered();
        }
        QListWidget::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        if (onLeft) {
            onLeft();
        }
        QListWidget::leaveEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        QAction *action = actionAtViewportPos(event ? event->pos() : QPoint());
        setHoveredAction(action);
        if (onHoverChanged) {
            onHoverChanged(action);
        }
        QListWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event && event->button() == Qt::LeftButton) {
            if (QAction *action = actionAtViewportPos(event->pos())) {
                if (onActionTriggered) {
                    onActionTriggered(action);
                }
                event->accept();
                return;
            }
        }
        QListWidget::mouseReleaseEvent(event);
    }

private:
    QHash<QAction *, QListWidgetItem *> m_actionItems;
    QSize m_contentSize;
    QAction *m_hoverAction = nullptr;
    bool m_hasIcons = false;
    bool m_hasCheckable = false;
    bool m_hasSubMenus = false;
};

void FluentMenuItemDelegate::paint(QPainter *painter,
                                   const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    if (!painter || !m_view) {
        return;
    }

    const bool isSeparator = index.data(kSeparatorRole).toBool();
    QAction *action = actionFromIndex(index);
    const auto &colors = ThemeManager::instance().colors();
    const bool dark = colors.background.lightnessF() < 0.5;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    if (isSeparator) {
        QColor sep = colors.border;
        sep.setAlpha(dark ? 140 : 90);
        painter->setPen(QPen(sep, 1.0));
        painter->drawLine(QPointF(option.rect.left() + 12.0, option.rect.center().y() + 0.5),
                          QPointF(option.rect.right() - 12.0, option.rect.center().y() + 0.5));
        painter->restore();
        return;
    }

    if (!action) {
        painter->restore();
        return;
    }

    const QRect rowRect = option.rect.adjusted(kRowInsetX, kRowInsetY, -kRowInsetX, -kRowInsetY);
    const bool hovered = action == m_view->hoveredAction();
    if (hovered) {
        QColor fill = colors.hover;
        fill.setAlpha(110);
        painter->setPen(Qt::NoPen);
        painter->setBrush(fill);
        painter->drawRoundedRect(rowRect, 5, 5);

        painter->setBrush(colors.accent);
        painter->drawRoundedRect(QRectF(rowRect.left(), rowRect.center().y() - 8.0, 3.0, 16.0), 1.5, 1.5);
    }

    const bool enabled = action->isEnabled();
    const int checkSlot = m_view->hasCheckable() ? kCheckSlotWidth : 0;
    const int iconSlot = m_view->hasIcons() ? kIconSlotWidth : 0;
    const int arrowSlot = m_view->hasSubMenus() ? kSubMenuSlotWidth : 0;
    int x = rowRect.left() + 8;

    if (checkSlot > 0) {
        if (action->isCheckable() && action->isChecked()) {
            painter->setPen(QPen(enabled ? colors.accent : colors.disabledText,
                                 1.8,
                                 Qt::SolidLine,
                                 Qt::RoundCap,
                                 Qt::RoundJoin));
            const QPointF center(x + 9.0, rowRect.center().y());
            painter->drawLine(center + QPointF(-4.0, 0.5), center + QPointF(-1.2, 3.3));
            painter->drawLine(center + QPointF(-1.2, 3.3), center + QPointF(5.2, -3.0));
        }
        x += checkSlot;
    }

    if (iconSlot > 0) {
        if (!action->icon().isNull()) {
            const QRect iconRect(x + (iconSlot - 16) / 2,
                                 rowRect.center().y() - 8,
                                 16,
                                 16);
            action->icon().paint(painter,
                                 iconRect,
                                 Qt::AlignCenter,
                                 enabled ? QIcon::Normal : QIcon::Disabled);
        }
        x += iconSlot;
    }

    x += 4;
    const QString label = actionLabelText(action);
    const QString shortcut = index.data(kShortcutRole).toString();
    const QFontMetrics fm(option.font);
    const int shortcutWidth = shortcut.isEmpty() ? 0 : fm.horizontalAdvance(shortcut);
    const int right = rowRect.right() - kTextRightPadding - arrowSlot;
    const QRect shortcutRect = shortcut.isEmpty()
        ? QRect()
        : QRect(right - shortcutWidth, rowRect.top(), shortcutWidth, rowRect.height());
    const int labelRight = shortcut.isEmpty() ? right : shortcutRect.left() - kShortcutGap;
    const QRect labelRect(x, rowRect.top(), qMax(0, labelRight - x), rowRect.height());

    painter->setFont(option.font);
    painter->setPen(enabled ? colors.text : colors.disabledText);
    painter->drawText(labelRect,
                      Qt::AlignVCenter | Qt::AlignLeft | Qt::TextShowMnemonic,
                      fm.elidedText(label, Qt::ElideRight, labelRect.width()));

    if (!shortcut.isEmpty()) {
        painter->setPen(enabled ? colors.subText : colors.disabledText);
        painter->drawText(shortcutRect, Qt::AlignVCenter | Qt::AlignRight, shortcut);
    }

    if (action->menu()) {
        const QColor arrow = enabled ? colors.subText : colors.disabledText;
        Style::drawChevronRight(*painter,
                                QPointF(rowRect.right() - 12.0, rowRect.center().y()),
                                arrow,
                                7.5,
                                1.6);
    }

    painter->restore();
}

FluentMenuPopupHost::FluentMenuPopupHost(QMenu *sourceMenu)
    : QWidget(nullptr, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
    , m_menu(sourceMenu)
    , m_border(this, this)
{
    setObjectName(QStringLiteral("FluentMenuPopupHost"));
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QVBoxLayout(this);
    const QMargins margins = PopupSurface::shadowMargins();
    layout->setContentsMargins(margins.left(), margins.top(), margins.right(), margins.bottom());
    layout->setSpacing(0);

    m_view = new MenuActionListWidget(this);
    layout->addWidget(m_view);

    m_view->onHoverChanged = [this](QAction *action) {
        updateHoverAction(action);
    };
    m_view->onActionTriggered = [this](QAction *action) {
        triggerAction(action);
    };
    m_view->onEntered = [this]() {
        cancelPendingChildClose();
        if (onEntered) {
            onEntered();
        }
    };
    m_view->onLeft = [this]() {
        const QPoint cursorLocal = mapFromGlobal(QCursor::pos());
        if (!rect().contains(cursorLocal)) {
            updateHoverAction(nullptr);
            scheduleCloseChildPopup();
        }
    };
    m_view->rebuildFromMenu(m_menu.data());

    m_border.setRequestUpdate([this]() { update(); });
    m_border.syncFromTheme();

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        if (isVisible()) {
            m_border.onThemeChanged();
        } else {
            m_border.syncFromTheme();
        }
        if (m_view) {
            m_view->viewport()->update();
        }
        update();
    });
}

void FluentMenuPopupHost::closeFromParent()
{
    m_closingFromParent = true;
    close();
}

bool FluentMenuPopupHost::isClosingFromParent() const
{
    return m_closingFromParent;
}

QMenu *FluentMenuPopupHost::menu() const
{
    return m_menu.data();
}

void FluentMenuPopupHost::popupAt(const QPoint &contentTopLeft, QAction *atAction, int slideOffsetY)
{
    if (m_menu) {
        QMetaObject::invokeMethod(m_menu, "aboutToShow", Qt::DirectConnection);
    }

    if (m_view) {
        m_view->rebuildFromMenu(m_menu.data());
    }

    if (atAction) {
        updateHoverAction(atAction);
    }

    resize(sizeHint());
    const QPoint finalTopLeft = PopupSurface::topLeftForContentTopLeft(contentTopLeft);
    const QSize contentSize = PopupSurface::contentSizeFromShadowSize(size());
    if (slideOffsetY == std::numeric_limits<int>::min()) {
        slideOffsetY = popupOpenSlideOffsetY(m_menu.data(), contentTopLeft, contentSize.height());
    }

    const QRect targetGeometry(finalTopLeft, size());
    Detail::preparePopupOpen(this, targetGeometry, slideOffsetY);
    show();
    raise();
    activateWindow();
    setFocus(Qt::PopupFocusReason);
    m_border.playInitialTraceOnce(0);
    startOpenAnimation(targetGeometry, slideOffsetY);
}

QSize FluentMenuPopupHost::sizeHint() const
{
    return PopupSurface::withShadowMargins(m_view ? m_view->contentSizeHint() : QSize(kMenuMinWidth, kViewportPaddingY * 2));
}

void FluentMenuPopupHost::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const auto &colors = ThemeManager::instance().colors();
    {
        QPainter clear(this);
        if (!clear.isActive()) {
            return;
        }
        clear.setCompositionMode(QPainter::CompositionMode_Source);
        clear.fillRect(rect(), Qt::transparent);
    }

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);
    PopupSurface::paintPanelWithShadowMargins(painter, rect(), colors, &m_border);
}

void FluentMenuPopupHost::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    stopOpenAnimation();
    closeChildPopup();
    if (m_menu) {
        QMetaObject::invokeMethod(m_menu, "aboutToHide", Qt::DirectConnection);
    }
    m_border.resetInitial();
    if (onClosed) {
        onClosed();
    }
}

void FluentMenuPopupHost::keyPressEvent(QKeyEvent *event)
{
    if (!event) {
        return;
    }

    switch (event->key()) {
    case Qt::Key_Escape:
        close();
        return;
    case Qt::Key_Down:
        if (m_view) {
            m_view->moveHover(1);
        }
        return;
    case Qt::Key_Up:
        if (m_view) {
            m_view->moveHover(-1);
        }
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        if (m_hoverAction && m_hoverAction->isEnabled()) {
            triggerAction(m_hoverAction);
        }
        return;
    default:
        break;
    }

    QWidget::keyPressEvent(event);
}

void FluentMenuPopupHost::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint localPos = event ? event->pos() : QPoint();
    const QPoint globalPos = mapToGlobal(localPos);

    if (rect().contains(localPos)) {
        handleForeignMouseMove(globalPos);
    } else if (m_parentHost) {
        m_parentHost->handleForeignMouseMove(globalPos);
    }

    QWidget::mouseMoveEvent(event);
}

void FluentMenuPopupHost::enterEvent(FluentEnterEvent *event)
{
    cancelPendingChildClose();
    if (onEntered) {
        onEntered();
    }
    QWidget::enterEvent(event);
}

void FluentMenuPopupHost::leaveEvent(QEvent *event)
{
    const QPoint cursorLocal = mapFromGlobal(QCursor::pos());
    if (!rect().contains(cursorLocal)) {
        updateHoverAction(nullptr);
        scheduleCloseChildPopup();
    }
    QWidget::leaveEvent(event);
}

void FluentMenuPopupHost::handleForeignMouseMove(const QPoint &globalPos)
{
    const QPoint localPos = mapFromGlobal(globalPos);
    if (rect().contains(localPos)) {
        QAction *action = nullptr;
        if (m_view) {
            const QPoint viewportPos = m_view->viewport()->mapFromGlobal(globalPos);
            if (m_view->viewport()->rect().contains(viewportPos)) {
                action = m_view->actionAtViewportPos(viewportPos);
            }
        }
        updateHoverAction(action);
    } else if (m_parentHost) {
        m_parentHost->handleForeignMouseMove(globalPos);
    }
}

void FluentMenuPopupHost::updateHoverAction(QAction *action)
{
    if (action == m_hoverAction) {
        return;
    }

    m_hoverAction = action;
    if (m_view) {
        m_view->setHoveredAction(action);
    }

    if (m_hoverAction && m_hoverAction->menu() && m_hoverAction->isEnabled()) {
        cancelPendingChildClose();
        syncChildPopup();
    } else if (m_childPopup) {
        scheduleCloseChildPopup();
    }
}

void FluentMenuPopupHost::triggerAction(QAction *action)
{
    if (!action || !action->isEnabled()) {
        return;
    }

    if (action->menu()) {
        updateHoverAction(action);
        syncChildPopup();
        return;
    }

    closeChildPopup();
    close();
    if (onActionTriggered) {
        onActionTriggered(action);
    }
    action->trigger();
}

void FluentMenuPopupHost::syncChildPopup()
{
    QAction *action = m_hoverAction;
    if (!action || !action->menu() || !action->isEnabled()) {
        closeChildPopup();
        return;
    }

    QMenu *submenuMenu = action->menu();
    if (m_childPopup && m_childPopup->menu() == submenuMenu) {
        cancelPendingChildClose();
        return;
    }

    closeChildPopup();

    auto *popup = new FluentMenuPopupHost(submenuMenu);
    popup->setAttribute(Qt::WA_DeleteOnClose, true);
    popup->m_parentHost = this;
    QPointer<FluentMenuPopupHost> popupRef = popup;
    popup->onClosed = [this, popupRef]() {
        const bool fromParent = popupRef && popupRef->isClosingFromParent();
        m_childPopup = nullptr;
        if (!fromParent) {
            close();
        }
    };
    popup->onActionTriggered = [this](QAction *triggeredAction) {
        close();
        if (onActionTriggered) {
            onActionTriggered(triggeredAction);
        }
    };
    popup->onEntered = [this]() {
        cancelPendingChildClose();
    };
    m_childPopup = popup;
    popup->popupAt(submenuPopupPosition(action, popup->sizeHint()), nullptr, 0);
}

QPoint FluentMenuPopupHost::submenuPopupPosition(QAction *action, const QSize &popupSize) const
{
    if (!m_view || !action) {
        return mapToGlobal(QPoint(0, 0));
    }

    const QRect vr = m_view->actionViewportRect(action);
    QPoint pos = m_view->viewport()->mapToGlobal(vr.topRight() + QPoint(4, 0));

    QScreen *screen = QApplication::screenAt(pos);
    if (!screen) {
        screen = QApplication::primaryScreen();
    }
    const QRect avail = screen ? screen->availableGeometry() : QRect();
    if (!avail.isValid()) {
        return pos;
    }

    auto popupGeometry = [&popupSize](const QPoint &contentTopLeft) {
        return QRect(PopupSurface::topLeftForContentTopLeft(contentTopLeft), popupSize);
    };
    const QMargins popupMargins = PopupSurface::shadowMargins();
    const int popupContentWidth = PopupSurface::contentSizeFromShadowSize(popupSize).width();

    if (popupGeometry(pos).right() > avail.right()) {
        pos.setX(m_view->viewport()->mapToGlobal(vr.topLeft()).x() - 4 - popupContentWidth);
    }
    if (popupGeometry(pos).bottom() > avail.bottom()) {
        pos.setY(avail.bottom() - popupSize.height() + popupMargins.top());
    }
    if (popupGeometry(pos).top() < avail.top()) {
        pos.setY(avail.top() + popupMargins.top());
    }
    if (popupGeometry(pos).left() < avail.left()) {
        pos.setX(avail.left() + popupMargins.left());
    }
    return pos;
}

void FluentMenuPopupHost::closeChildPopup()
{
    cancelPendingChildClose();
    if (m_childPopup) {
        FluentMenuPopupHost *child = m_childPopup.data();
        m_childPopup = nullptr;
        child->closeFromParent();
    }
}

void FluentMenuPopupHost::scheduleCloseChildPopup(int delayMs)
{
    if (!m_childPopup) {
        return;
    }
    if (!m_pendingCloseTimer) {
        m_pendingCloseTimer = new QTimer(this);
        m_pendingCloseTimer->setSingleShot(true);
        connect(m_pendingCloseTimer, &QTimer::timeout, this, [this]() {
            if (m_childPopup) {
                FluentMenuPopupHost *child = m_childPopup.data();
                m_childPopup = nullptr;
                child->closeFromParent();
            }
        });
    }
    m_pendingCloseTimer->start(delayMs);
}

void FluentMenuPopupHost::cancelPendingChildClose()
{
    if (m_pendingCloseTimer && m_pendingCloseTimer->isActive()) {
        m_pendingCloseTimer->stop();
    }
}

void FluentMenuPopupHost::startOpenAnimation(const QRect &targetGeometry, int slideOffsetY)
{
    stopOpenAnimation(false);

    Detail::preparePopupOpen(this, targetGeometry, slideOffsetY);
    m_openFadeAnim = Detail::startPopupOpenAnimation(this, targetGeometry, slideOffsetY, true, [this]() {
        m_openFadeAnim = nullptr;
    });
}

void FluentMenuPopupHost::stopOpenAnimation(bool resetOpacity)
{
    if (m_openFadeAnim) {
        m_openFadeAnim->stop();
        m_openFadeAnim->deleteLater();
        m_openFadeAnim = nullptr;
    }
    if (m_openSlideAnim) {
        m_openSlideAnim->stop();
        m_openSlideAnim->deleteLater();
        m_openSlideAnim = nullptr;
    }
    if (resetOpacity) {
        Detail::resetPopupOpenState(this);
    }
}

} // namespace Fluent
