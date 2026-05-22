#include "Fluent/FluentAutoSuggestBox.h"

#include "Fluent/FluentBorderEffect.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentPopupSurface.h"
#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentToolButton.h"

#include <QApplication>
#include <QCursor>
#include <QEasingCurve>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QResizeEvent>
#include <QScreen>
#include <QScrollBar>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QVariantAnimation>

#include <cmath>

namespace Fluent {

namespace {

constexpr int kAutoSuggestPopupGap = 5;
constexpr int kAutoSuggestPopupSlideOffset = PopupSurface::kOpenSlideOffsetPx;
constexpr int kAutoSuggestMaxVisibleRows = 6;
constexpr int kAutoSuggestViewInset = 6;

class AutoSuggestListView final : public QListView
{
public:
    explicit AutoSuggestListView(QWidget *parent = nullptr)
        : QListView(parent)
    {
        setObjectName(QStringLiteral("FluentAutoSuggestPopupView"));
        setAttribute(Qt::WA_StyledBackground, false);
        setAutoFillBackground(false);
        setFrameShape(QFrame::NoFrame);
        setMouseTracking(true);
        setFocusPolicy(Qt::NoFocus);
        setSpacing(2);
        setViewportMargins(kAutoSuggestViewInset, kAutoSuggestViewInset, kAutoSuggestViewInset, kAutoSuggestViewInset);
        setUniformItemSizes(true);
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBar(new FluentScrollBar(Qt::Vertical, this));

        if (viewport()) {
            viewport()->setAutoFillBackground(false);
            viewport()->setAttribute(Qt::WA_StyledBackground, false);
        }

        m_hoverAnim = new QVariantAnimation(this);
        m_hoverAnim->setDuration(120);
        m_hoverAnim->setEasingCurve(QEasingCurve::OutQuad);
        connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            m_hoverLevel = value.toReal();
            if (viewport()) {
                viewport()->update();
            }
        });
    }

    QModelIndex hoverIndex() const { return m_hoverIndex; }
    qreal hoverLevel() const { return m_hoverLevel; }

protected:
    void mouseMoveEvent(QMouseEvent *event) override
    {
        const QModelIndex index = indexAt(event->pos());
        if (index != m_hoverIndex) {
            m_hoverIndex = index;
            startHoverAnimation(index.isValid() ? 1.0 : 0.0);
        }
        QListView::mouseMoveEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        m_hoverIndex = QModelIndex();
        startHoverAnimation(0.0);
        QListView::leaveEvent(event);
    }

private:
    void startHoverAnimation(qreal endValue)
    {
        if (!m_hoverAnim) {
            return;
        }
        m_hoverAnim->stop();
        m_hoverAnim->setStartValue(m_hoverLevel);
        m_hoverAnim->setEndValue(endValue);
        m_hoverAnim->start();
    }

    QModelIndex m_hoverIndex;
    qreal m_hoverLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
};

class AutoSuggestItemDelegate final : public QStyledItemDelegate
{
public:
    explicit AutoSuggestItemDelegate(AutoSuggestListView *view)
        : QStyledItemDelegate(view)
        , m_view(view)
    {
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(qMax(32, size.height()));
        size.setWidth(qMax(size.width(), option.fontMetrics.horizontalAdvance(index.data(Qt::DisplayRole).toString()) + 28));
        return size;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        opt.state &= ~QStyle::State_HasFocus;

        const auto &colors = ThemeManager::instance().colors();
        const QRectF itemRect = QRectF(opt.rect).adjusted(4, 2, -4, -2);
        const bool enabled = opt.state.testFlag(QStyle::State_Enabled);
        const bool selected = opt.state.testFlag(QStyle::State_Selected);
        const bool hovered = m_view && index == m_view->hoverIndex();

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QColor bg = Qt::transparent;
        if (selected) {
            bg = colors.accent;
            bg.setAlpha(38);
        } else if (hovered) {
            bg = colors.hover;
            bg.setAlphaF(0.32 * (m_view ? m_view->hoverLevel() : 1.0));
        }

        if (bg.alpha() > 0) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(bg);
            painter->drawRoundedRect(itemRect, 4.0, 4.0);
        }

        if (selected) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(colors.accent);
            const QRectF indicator(itemRect.left(),
                                   itemRect.center().y() - 8.0,
                                   3.0,
                                   16.0);
            painter->drawRoundedRect(indicator, 1.5, 1.5);
        }

        const QRectF textRect = itemRect.adjusted(12, 0, -10, 0);
        painter->setPen(enabled ? colors.text : colors.disabledText);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, opt.text);
        painter->restore();
    }

private:
    AutoSuggestListView *m_view = nullptr;
};

} // namespace

class FluentAutoSuggestPopup final : public QWidget
{
public:
    explicit FluentAutoSuggestPopup(FluentAutoSuggestBox *owner)
        : QWidget(owner)
        , m_owner(owner)
        , m_border(this, this)
    {
        setObjectName(QStringLiteral("FluentAutoSuggestPopup"));
        setWindowFlags(Qt::Tool |
                       Qt::FramelessWindowHint |
                       Qt::NoDropShadowWindowHint |
                       Qt::WindowDoesNotAcceptFocus);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_StyledBackground, false);
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setAutoFillBackground(false);
        setMouseTracking(true);
        setFocusPolicy(Qt::NoFocus);

        m_border.setRequestUpdate([this]() { update(); });
        m_border.syncFromTheme();

        m_model = new QStringListModel(this);
        m_view = new AutoSuggestListView(this);
        m_view->setModel(m_model);
        m_view->setItemDelegate(new AutoSuggestItemDelegate(m_view));
        m_view->installEventFilter(this);
        if (m_view->viewport()) {
            m_view->viewport()->installEventFilter(this);
        }

        connect(m_view, &QAbstractItemView::clicked, this, [this](const QModelIndex &index) {
            activateIndex(index);
        });
        connect(m_view, &QAbstractItemView::activated, this, [this](const QModelIndex &index) {
            activateIndex(index);
        });

        m_openAnim = new QVariantAnimation(this);
        m_openAnim->setDuration(PopupSurface::kOpenDurationMs);
        m_openAnim->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_openAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            const qreal t = qBound<qreal>(0.0, value.toReal(), 1.0);
            setWindowOpacity(t);
            if (m_targetGeom.isValid()) {
                QRect geometry = m_targetGeom;
                geometry.translate(0, static_cast<int>(std::lround((1.0 - t) * m_openSlideOffsetY)));
                setGeometry(geometry);
            }
        });
        connect(m_openAnim, &QVariantAnimation::finished, this, [this]() {
            setWindowOpacity(1.0);
            if (m_targetGeom.isValid()) {
                setGeometry(m_targetGeom);
            }
        });

        if (owner && owner->lineEdit()) {
            owner->lineEdit()->installEventFilter(this);
        }

        connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
            applyTheme();
        });
    }

    void applyTheme()
    {
        m_border.syncFromTheme();
        if (m_view) {
            QPalette palette = m_view->palette();
            const auto &colors = ThemeManager::instance().colors();
            palette.setColor(QPalette::Text, colors.text);
            palette.setColor(QPalette::Mid, colors.border);
            m_view->setPalette(palette);
            m_view->setStyleSheet(QStringLiteral(
                "QListView#FluentAutoSuggestPopupView {"
                "  background: transparent;"
                "  border: none;"
                "  outline: 0;"
                "}"
                "QListView#FluentAutoSuggestPopupView::viewport {"
                "  background: transparent;"
                "}"
                "QScrollBar:vertical {"
                "  background: transparent;"
                "  width: 10px;"
                "  margin: 2px;"
                "}"
                "QScrollBar::handle:vertical {"
                "  background-color: palette(mid);"
                "  border: 1px solid transparent;"
                "  border-radius: 999px;"
                "  min-height: 24px;"
                "}"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
                "  height: 0px;"
                "}"));
        }
        update();
    }

    void setSuggestions(const QStringList &suggestions)
    {
        m_model->setStringList(suggestions);
        if (!suggestions.isEmpty()) {
            const QModelIndex first = m_model->index(0, 0);
            m_view->setCurrentIndex(first);
        }
        updateScrollPolicy();
    }

    void popup()
    {
        if (!m_owner || !m_owner->lineEdit() || m_model->rowCount() <= 0) {
            dismiss(false);
            return;
        }

        positionPopup();
        layoutView();

        if (isVisible()) {
            raise();
            update();
            return;
        }

        m_openAnim->stop();
        setWindowOpacity(0.0);
        if (m_targetGeom.isValid()) {
            QRect startGeom = m_targetGeom;
            startGeom.translate(0, m_openSlideOffsetY);
            setGeometry(startGeom);
        }

        show();
        raise();
        m_view->show();
        m_view->raise();
        if (m_owner && m_owner->lineEdit()) {
            m_owner->lineEdit()->setFocus(Qt::OtherFocusReason);
        }

        if (!m_appFilterInstalled && qApp) {
            qApp->installEventFilter(this);
            m_appFilterInstalled = true;
        }

        m_openAnim->setStartValue(0.0);
        m_openAnim->setEndValue(1.0);
        m_openAnim->start();
    }

    void dismiss(bool returnFocusToLineEdit = true)
    {
        if (!isVisible()) {
            return;
        }

        if (m_appFilterInstalled && qApp) {
            qApp->removeEventFilter(this);
            m_appFilterInstalled = false;
        }
        if (m_openAnim) {
            m_openAnim->stop();
        }

        hide();
        if (returnFocusToLineEdit && m_owner && m_owner->lineEdit()) {
            m_owner->lineEdit()->setFocus(Qt::PopupFocusReason);
        }
    }

protected:
    bool event(QEvent *event) override
    {
        if (event && (event->type() == QEvent::WindowDeactivate ||
                      event->type() == QEvent::ApplicationDeactivate)) {
            dismiss(false);
            return true;
        }
        return QWidget::event(event);
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (!event) {
            return QWidget::eventFilter(watched, event);
        }

        if (watched == qApp) {
            if ((event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin) && isVisible()) {
                QPoint globalPos;
                if (event->type() == QEvent::MouseButtonPress) {
                    auto *mouseEvent = static_cast<QMouseEvent *>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                    globalPos = mouseEvent->globalPosition().toPoint();
#else
                    globalPos = mouseEvent->globalPos();
#endif
                } else {
                    globalPos = QCursor::pos();
                }

                const bool insidePopup = rect().contains(mapFromGlobal(globalPos));
                const bool insideOwner = m_owner && m_owner->rect().contains(m_owner->mapFromGlobal(globalPos));
                if (!insidePopup && !insideOwner) {
                    dismiss(false);
                }
            }
            return QWidget::eventFilter(watched, event);
        }

        if ((m_owner && watched == m_owner->lineEdit()) ||
            watched == m_view ||
            (m_view && watched == m_view->viewport())) {
            if (event->type() == QEvent::KeyPress) {
                auto *keyEvent = static_cast<QKeyEvent *>(event);
                switch (keyEvent->key()) {
                case Qt::Key_Down:
                    if (!isVisible() && m_owner) {
                        m_owner->updatePopup();
                    }
                    moveCurrent(1);
                    return isVisible();
                case Qt::Key_Up:
                    moveCurrent(-1);
                    return isVisible();
                case Qt::Key_PageDown:
                    moveCurrent(qMax(1, visibleRowCount() - 1));
                    return isVisible();
                case Qt::Key_PageUp:
                    moveCurrent(-qMax(1, visibleRowCount() - 1));
                    return isVisible();
                case Qt::Key_Escape:
                    dismiss(true);
                    return isVisible();
                case Qt::Key_Return:
                case Qt::Key_Enter:
                    if (isVisible() && activateIndex(m_view->currentIndex())) {
                        return true;
                    }
                    break;
                case Qt::Key_Tab:
                case Qt::Key_Backtab:
                    dismiss(false);
                    return false;
                default:
                    break;
                }
            }
        }

        return QWidget::eventFilter(watched, event);
    }

    void hideEvent(QHideEvent *event) override
    {
        QWidget::hideEvent(event);
        if (m_appFilterInstalled && qApp) {
            qApp->removeEventFilter(this);
            m_appFilterInstalled = false;
        }
        if (m_openAnim) {
            m_openAnim->stop();
        }
        setWindowOpacity(1.0);
        m_border.resetInitial();
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        layoutView();
    }

    void showEvent(QShowEvent *event) override
    {
        QWidget::showEvent(event);
        m_border.playInitialTraceOnce(0);
    }

    void paintEvent(QPaintEvent *event) override
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
        PopupSurface::paintPanel(painter, rect(), colors, &m_border);
    }

private:
    int visibleRowCount() const
    {
        return qMin(kAutoSuggestMaxVisibleRows, qMax(0, m_model->rowCount()));
    }

    int itemHeight() const
    {
        if (m_view && m_model->rowCount() > 0) {
            const int height = m_view->sizeHintForIndex(m_model->index(0, 0)).height();
            if (height > 0) {
                return height;
            }
        }
        return 32;
    }

    QSize popupSizeHint() const
    {
        const QWidget *anchor = m_owner ? m_owner->lineEdit() : nullptr;
        int width = anchor ? anchor->width() : 220;
        const int rows = visibleRowCount();
        const int spacing = m_view ? m_view->spacing() : 0;
        int height = kAutoSuggestViewInset * 2 + qMax(1, rows) * itemHeight();
        if (rows > 1) {
            height += (rows - 1) * spacing;
        }
        height += 2;

        const int sampleCount = qMin(m_model->rowCount(), 24);
        for (int row = 0; row < sampleCount; ++row) {
            const QString text = m_model->data(m_model->index(row, 0), Qt::DisplayRole).toString();
            width = qMax(width, m_view->fontMetrics().horizontalAdvance(text) + 44);
        }
        if (m_model->rowCount() > rows && m_view && m_view->verticalScrollBar()) {
            width += qMax(10, m_view->verticalScrollBar()->sizeHint().width());
        }

        return QSize(qMax(width, 180), height);
    }

    void updateScrollPolicy()
    {
        if (!m_view) {
            return;
        }
        const bool needsScroll = m_model->rowCount() > visibleRowCount();
        m_view->setVerticalScrollBarPolicy(needsScroll ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
        if (!needsScroll && m_view->verticalScrollBar()) {
            m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->minimum());
        }
    }

    void layoutView()
    {
        if (m_view) {
            m_view->setGeometry(rect().adjusted(1, 1, -1, -1));
        }
    }

    void positionPopup()
    {
        QWidget *anchor = m_owner ? m_owner->lineEdit() : nullptr;
        if (!anchor) {
            return;
        }

        const QSize popupSize = popupSizeHint();
        const QPoint anchorTopLeft = anchor->mapToGlobal(QPoint(0, 0));
        const int anchorTopY = anchorTopLeft.y();
        const int anchorBottomY = anchorTopLeft.y() + anchor->height();

        QScreen *screen = QApplication::screenAt(anchorTopLeft);
        if (!screen) {
            screen = QApplication::primaryScreen();
        }
        const QRect avail = screen ? screen->availableGeometry() : QRect();

        QRect popupGeom(anchorTopLeft, popupSize);
        const int belowTop = anchorBottomY + kAutoSuggestPopupGap;
        const int aboveTop = anchorTopY - kAutoSuggestPopupGap - popupSize.height();

        const bool fitsBelow = !avail.isValid() || (belowTop + popupSize.height() <= avail.bottom() + 1);
        const bool fitsAbove = !avail.isValid() || (aboveTop >= avail.top());

        popupGeom.moveTop((fitsBelow || !fitsAbove) ? belowTop : aboveTop);
        m_openSlideOffsetY = (fitsBelow || !fitsAbove) ? -kAutoSuggestPopupSlideOffset : kAutoSuggestPopupSlideOffset;

        if (avail.isValid()) {
            if (popupGeom.left() < avail.left()) {
                popupGeom.moveLeft(avail.left());
            }
            if (popupGeom.right() > avail.right()) {
                popupGeom.moveRight(avail.right());
            }
            if (popupGeom.top() < avail.top()) {
                popupGeom.moveTop(avail.top());
            }
            if (popupGeom.bottom() > avail.bottom()) {
                popupGeom.moveBottom(avail.bottom());
            }
        }

        m_targetGeom = popupGeom;
        setGeometry(popupGeom);
    }

    void moveCurrent(int delta)
    {
        if (!isVisible() || m_model->rowCount() <= 0) {
            return;
        }

        int row = m_view->currentIndex().isValid() ? m_view->currentIndex().row() : 0;
        row = qBound(0, row + delta, m_model->rowCount() - 1);
        const QModelIndex index = m_model->index(row, 0);
        m_view->setCurrentIndex(index);
        m_view->scrollTo(index, QAbstractItemView::PositionAtCenter);
    }

    bool activateIndex(const QModelIndex &index)
    {
        if (!m_owner || !index.isValid()) {
            dismiss(true);
            return false;
        }

        const QString value = index.data(Qt::DisplayRole).toString();
        m_owner->acceptSuggestion(value);
        return true;
    }

    QPointer<FluentAutoSuggestBox> m_owner;
    QStringListModel *m_model = nullptr;
    AutoSuggestListView *m_view = nullptr;
    FluentBorderEffect m_border;
    QVariantAnimation *m_openAnim = nullptr;
    QRect m_targetGeom;
    int m_openSlideOffsetY = -kAutoSuggestPopupSlideOffset;
    bool m_appFilterInstalled = false;
};

FluentAutoSuggestBox::FluentAutoSuggestBox(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(8);

    m_lineEdit = new FluentLineEdit(this);
    m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_searchButton = new FluentToolButton(QStringLiteral("Search"), this);
    m_searchButton->setVisible(false);
    m_searchButton->setMinimumWidth(72);

    m_popup = new FluentAutoSuggestPopup(this);

    m_layout->addWidget(m_lineEdit, 1);
    m_layout->addWidget(m_searchButton);

    connect(m_lineEdit, &QLineEdit::textChanged, this, [this](const QString &value) {
        emit textChanged(value);
        updatePopup();
    });
    connect(m_lineEdit, &QLineEdit::returnPressed, this, &FluentAutoSuggestBox::submit);
    connect(m_searchButton, &QToolButton::clicked, this, &FluentAutoSuggestBox::submit);

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentAutoSuggestBox::applyTheme);
}

FluentLineEdit *FluentAutoSuggestBox::lineEdit() const
{
    return m_lineEdit;
}

QString FluentAutoSuggestBox::text() const
{
    return m_lineEdit->text();
}

void FluentAutoSuggestBox::setText(const QString &text)
{
    m_lineEdit->setText(text);
}

QString FluentAutoSuggestBox::placeholderText() const
{
    return m_lineEdit->placeholderText();
}

void FluentAutoSuggestBox::setPlaceholderText(const QString &text)
{
    m_lineEdit->setPlaceholderText(text);
}

QStringList FluentAutoSuggestBox::suggestions() const
{
    return m_suggestions;
}

void FluentAutoSuggestBox::setSuggestions(const QStringList &suggestions)
{
    m_suggestions = suggestions;
    updatePopup();
}

bool FluentAutoSuggestBox::isSearchButtonVisible() const
{
    return m_searchButton->isVisible();
}

void FluentAutoSuggestBox::setSearchButtonVisible(bool visible)
{
    m_searchButton->setVisible(visible);
}

QString FluentAutoSuggestBox::searchButtonText() const
{
    return m_searchButton->text();
}

void FluentAutoSuggestBox::setSearchButtonText(const QString &text)
{
    m_searchButton->setText(text);
    m_searchButton->setMinimumWidth(qMax(42, m_searchButton->fontMetrics().horizontalAdvance(text) + 24));
}

FluentToolButton *FluentAutoSuggestBox::searchButton() const
{
    return m_searchButton;
}

void FluentAutoSuggestBox::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        m_lineEdit->setEnabled(isEnabled());
        m_searchButton->setEnabled(isEnabled());
        if (!isEnabled() && m_popup) {
            m_popup->dismiss(false);
        }
        applyTheme();
    }
}

void FluentAutoSuggestBox::applyTheme()
{
    if (m_popup) {
        m_popup->applyTheme();
    }
}

QStringList FluentAutoSuggestBox::filteredSuggestions() const
{
    const QString query = text().trimmed();
    if (query.isEmpty()) {
        return {};
    }

    QStringList result;
    for (const QString &suggestion : m_suggestions) {
        if (suggestion.contains(query, Qt::CaseInsensitive)) {
            result.append(suggestion);
        }
    }
    return result;
}

void FluentAutoSuggestBox::updatePopup()
{
    if (!m_popup || !m_lineEdit->hasFocus() || !isEnabled()) {
        if (m_popup) {
            m_popup->dismiss(false);
        }
        return;
    }

    const QStringList filtered = filteredSuggestions();
    m_popup->setSuggestions(filtered);
    if (filtered.isEmpty()) {
        m_popup->dismiss(false);
        return;
    }
    m_popup->popup();
}

void FluentAutoSuggestBox::acceptSuggestion(const QString &text)
{
    if (m_popup) {
        m_popup->dismiss(true);
    }
    {
        const QSignalBlocker blocker(m_lineEdit);
        m_lineEdit->setText(text);
    }
    m_lineEdit->selectAll();
    emit textChanged(text);
    emit suggestionChosen(text);
    emit submitted(text);
}

void FluentAutoSuggestBox::submit()
{
    if (m_popup) {
        m_popup->dismiss(false);
    }
    const QString value = text();
    emit submitted(value);
    emit searchRequested(value);
}

FluentSearchBox::FluentSearchBox(QWidget *parent)
    : FluentAutoSuggestBox(parent)
{
    setSearchButtonVisible(true);
    setSearchButtonText(tr("Search"));
}

} // namespace Fluent
