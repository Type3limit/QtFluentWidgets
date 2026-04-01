#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentBorderEffect.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentPopupSurface.h"
#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QAbstractItemView>
#include <QHideEvent>
#include <QItemSelectionModel>
#include <QEvent>
#include <QFrame>
#include <QKeyEvent>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QShowEvent>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QCursor>
#include <QPointer>
#include <QScreen>
#include <QVariantAnimation>

#include <cmath>

namespace Fluent {

constexpr int kFluentComboPopupGap = 5;
constexpr int kFluentComboPopupSlideOffset = PopupSurface::kOpenSlideOffsetPx;

class FluentComboPopup final : public QWidget
{
public:
    explicit FluentComboPopup(FluentComboBox *combo)
        : QWidget(combo)
        , m_combo(combo)
        , m_border(this, this)
    {
        setObjectName(QStringLiteral("FluentComboPopup"));
        setWindowFlag(Qt::Popup, true);
        setWindowFlag(Qt::FramelessWindowHint, true);
        setWindowFlag(Qt::NoDropShadowWindowHint, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_StyledBackground, false);
        setAutoFillBackground(false);
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);

        m_border.setRequestUpdate([this]() { update(); });
        m_border.syncFromTheme();

        m_openAnim = new QVariantAnimation(this);
        m_openAnim->setDuration(PopupSurface::kOpenDurationMs);
        m_openAnim->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_openAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            const qreal t = qBound<qreal>(0.0, value.toReal(), 1.0);
            setWindowOpacity(t);
            if (m_targetGeom.isValid()) {
                QRect g = m_targetGeom;
                g.translate(0, static_cast<int>(std::lround((1.0 - t) * m_openSlideOffsetY)));
                setGeometry(g);
            }
        });
        connect(m_openAnim, &QVariantAnimation::finished, this, [this]() {
            setWindowOpacity(1.0);
            if (m_targetGeom.isValid()) {
                setGeometry(m_targetGeom);
            }
        });

        connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
            if (isVisible()) {
                m_border.onThemeChanged();
                syncFromCombo();
            } else {
                m_border.syncFromTheme();
            }
            update();
        });

        if (m_combo) {
            connect(m_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
                if (isVisible()) {
                    syncSelectionFromCombo();
                }
            });
        }
    }

    void syncFromCombo()
    {
        ensureHostedView();
        updateScrollPolicy();
        layoutHostedView();
        syncSelectionFromCombo();
        update();
    }

    void popup()
    {
        if (!m_combo) {
            return;
        }

        syncFromCombo();
        positionPopupBelowOrAbove();

        m_openAnim->stop();
        setWindowOpacity(0.0);
        if (m_targetGeom.isValid()) {
            QRect startGeom = m_targetGeom;
            startGeom.translate(0, m_openSlideOffsetY);
            setGeometry(startGeom);
        }

        show();
        raise();
        activateWindow();
        setFocus(Qt::PopupFocusReason);

        if (m_view) {
            m_view->show();
            m_view->raise();
            m_view->setFocus(Qt::PopupFocusReason);
        }

        if (!m_appFilterInstalled && qApp) {
            qApp->installEventFilter(this);
            m_appFilterInstalled = true;
        }

        m_openAnim->setStartValue(0.0);
        m_openAnim->setEndValue(1.0);
        m_openAnim->start();
    }

    void dismiss(bool returnFocusToCombo = true)
    {
        if (m_dismissing || !isVisible()) {
            return;
        }

        m_dismissing = true;

        if (m_appFilterInstalled && qApp) {
            qApp->removeEventFilter(this);
            m_appFilterInstalled = false;
        }

        if (m_openAnim) {
            m_openAnim->stop();
        }

        hide();

        if (m_view) {
            m_view->hide();
        }

        if (returnFocusToCombo && m_combo) {
            m_combo->setFocus(Qt::PopupFocusReason);
        }

        if (m_combo) {
            m_combo->update();
        }

        m_dismissing = false;
    }

protected:
    bool event(QEvent *event) override
    {
        if (event) {
            if (event->type() == QEvent::WindowDeactivate ||
                event->type() == QEvent::ApplicationDeactivate) {
                dismiss(false);
                return true;
            }
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
                    if (!mouseEvent) {
                        return QWidget::eventFilter(watched, event);
                    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                    globalPos = mouseEvent->globalPosition().toPoint();
#else
                    globalPos = mouseEvent->globalPos();
#endif
                } else {
                    globalPos = QCursor::pos();
                }

                if (!rect().contains(mapFromGlobal(globalPos))) {
                    const bool clickedAnchor = m_combo && m_combo->rect().contains(m_combo->mapFromGlobal(globalPos));
                    dismiss(!clickedAnchor);
                    if (clickedAnchor) {
                        return true;
                    }
                }
            }
            return QWidget::eventFilter(watched, event);
        }

        if ((watched == m_view.data() || (m_view && watched == m_view->viewport())) && event->type() == QEvent::KeyPress) {
            auto *keyEvent = static_cast<QKeyEvent *>(event);
            if (!keyEvent) {
                return QWidget::eventFilter(watched, event);
            }

            switch (keyEvent->key()) {
            case Qt::Key_Escape:
                dismiss(true);
                return true;
            case Qt::Key_Return:
            case Qt::Key_Enter:
                activateCurrent();
                return true;
            case Qt::Key_Up:
                moveCurrent(-1);
                return true;
            case Qt::Key_Down:
                moveCurrent(1);
                return true;
            case Qt::Key_PageUp:
                moveCurrent(-qMax(1, visibleItemCount() - 1));
                return true;
            case Qt::Key_PageDown:
                moveCurrent(qMax(1, visibleItemCount() - 1));
                return true;
            case Qt::Key_Home:
                moveToEdge(false);
                return true;
            case Qt::Key_End:
                moveToEdge(true);
                return true;
            case Qt::Key_Tab:
            case Qt::Key_Backtab:
                dismiss(true);
                return false;
            default:
                break;
            }
        }

        return QWidget::eventFilter(watched, event);
    }

    void showEvent(QShowEvent *event) override
    {
        QWidget::showEvent(event);
        m_border.playInitialTraceOnce(0);
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
        layoutHostedView();
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
    QModelIndex comboIndexForRow(int row) const
    {
        if (!m_combo || !m_combo->model() || row < 0) {
            return QModelIndex();
        }

        const int column = qMax(0, m_combo->modelColumn());
        QModelIndex index = m_combo->model()->index(row, column, m_combo->rootModelIndex());
        if (!index.isValid() && column != 0) {
            index = m_combo->model()->index(row, 0, m_combo->rootModelIndex());
        }
        return index;
    }

    int rowCount() const
    {
        if (!m_combo || !m_combo->model()) {
            return 0;
        }
        return m_combo->model()->rowCount(m_combo->rootModelIndex());
    }

    int defaultRowHeight() const
    {
        if (m_view) {
            const QModelIndex current = m_view->currentIndex();
            if (current.isValid()) {
                const int h = m_view->sizeHintForIndex(current).height();
                if (h > 0) {
                    return h;
                }
            }
        }
        return 32;
    }

    int visibleItemCount() const
    {
        if (!m_combo) {
            return 0;
        }
        const int threshold = qMax(1, m_combo->effectivePopupScrollThreshold());
        const int rows = qMax(0, rowCount());
        return qMin(threshold, rows);
    }

    bool needsVerticalScrollBar() const
    {
        return rowCount() > visibleItemCount();
    }

    QSize popupSizeHint() const
    {
        constexpr int kWidthPadding = 24;
        constexpr int kBorderAllowance = 2;
        constexpr int kViewInset = 6;

        int width = m_combo ? m_combo->width() : 160;
        int height = defaultRowHeight() + kBorderAllowance;

        if (!m_view) {
            return QSize(width, height);
        }

        const int rows = rowCount();
        const bool needScroll = needsVerticalScrollBar();
        const int visibleRows = qMax(1, needScroll ? visibleItemCount() : rows);
        const int spacing = [&]() {
            if (const auto *list = qobject_cast<const QListView *>(m_view.data())) {
                return list->spacing();
            }
            return 0;
        }();

        const QMargins viewportMargins(kViewInset, kViewInset, kViewInset, kViewInset);
        int contentHeight = viewportMargins.top() + viewportMargins.bottom();
        int widest = width;

        const int sampleCount = qMax(visibleRows, qMin(rows, 24));
        for (int row = 0; row < sampleCount; ++row) {
            const QModelIndex index = comboIndexForRow(row);
            if (!index.isValid()) {
                continue;
            }

            const QSize hint = m_view->sizeHintForIndex(index);
            widest = qMax(widest, hint.width());

            if (row < visibleRows) {
                contentHeight += qMax(defaultRowHeight(), hint.height());
                if (row + 1 < visibleRows) {
                    contentHeight += spacing;
                }
            }
        }

        if (needScroll) {
            if (QScrollBar *scrollBar = m_view->verticalScrollBar()) {
                widest += qMax(10, scrollBar->sizeHint().width());
            } else {
                widest += 10;
            }
        }

        width = qMax(width, widest + viewportMargins.left() + viewportMargins.right() + kWidthPadding);
        height = qMax(defaultRowHeight() + kBorderAllowance, contentHeight + kBorderAllowance);

        return QSize(width, height);
    }

    void updateScrollPolicy()
    {
        if (!m_view) {
            return;
        }

        const bool needScroll = needsVerticalScrollBar();
        m_view->setVerticalScrollBarPolicy(needScroll ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);

        if (!needScroll && m_view->verticalScrollBar()) {
            m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->minimum());
        }
    }

    void ensureHostedView()
    {
        QAbstractItemView *view = m_combo ? m_combo->view() : nullptr;
        if (m_view == view && m_view) {
            return;
        }

        if (m_view) {
            QObject::disconnect(m_view, nullptr, this, nullptr);
            if (m_view->selectionModel()) {
                QObject::disconnect(m_view->selectionModel(), nullptr, this, nullptr);
            }
            m_view->removeEventFilter(this);
            if (m_view->viewport()) {
                m_view->viewport()->removeEventFilter(this);
            }
        }

        m_view = view;
        if (!m_view) {
            return;
        }

        m_view->setParent(this);
        m_view->setWindowFlags(Qt::Widget);
        m_view->setMouseTracking(true);
        m_view->setFocusPolicy(Qt::StrongFocus);
        m_view->setAttribute(Qt::WA_StyledBackground, false);
        m_view->setAttribute(Qt::WA_TranslucentBackground, false);
        m_view->setAutoFillBackground(false);

        if (auto *frame = qobject_cast<QFrame *>(m_view.data())) {
            frame->setFrameShape(QFrame::NoFrame);
        }

        if (auto *list = qobject_cast<QListView *>(m_view.data())) {
            list->setSpacing(2);
            list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
            list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            if (!qobject_cast<FluentScrollBar *>(list->verticalScrollBar())) {
                list->setVerticalScrollBar(new FluentScrollBar(Qt::Vertical, list));
            }
        }

        if (m_view->viewport()) {
            m_view->viewport()->setAttribute(Qt::WA_StyledBackground, false);
            m_view->viewport()->setAttribute(Qt::WA_TranslucentBackground, false);
            m_view->viewport()->setAutoFillBackground(false);
            m_view->viewport()->installEventFilter(this);
        }

        m_view->installEventFilter(this);

        connect(m_view, &QAbstractItemView::clicked, this, [this](const QModelIndex &index) {
            activateIndex(index);
        });
        connect(m_view, &QAbstractItemView::activated, this, [this](const QModelIndex &index) {
            activateIndex(index);
        });

        if (m_view->selectionModel()) {
            connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged, this,
                    [this](const QModelIndex &current, const QModelIndex &) {
                        if (m_view && current.isValid()) {
                            m_view->scrollTo(current, QAbstractItemView::PositionAtCenter);
                        }
                    });
        }
    }

    void layoutHostedView()
    {
        if (!m_view) {
            return;
        }

        m_view->setGeometry(rect().adjusted(1, 1, -1, -1));
    }

    void syncSelectionFromCombo()
    {
        if (!m_view) {
            return;
        }

        const QModelIndex index = comboIndexForRow(m_combo ? m_combo->currentIndex() : -1);
        if (!index.isValid()) {
            m_view->clearSelection();
            return;
        }

        m_view->setCurrentIndex(index);
        if (m_view->selectionModel()) {
            m_view->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        }
        m_view->scrollTo(index, QAbstractItemView::PositionAtCenter);
    }

    void moveCurrent(int delta)
    {
        const int rows = rowCount();
        if (rows <= 0) {
            return;
        }

        int row = m_view && m_view->currentIndex().isValid() ? m_view->currentIndex().row()
                                                              : (m_combo ? m_combo->currentIndex() : 0);
        row = qBound(0, row + delta, rows - 1);

        const QModelIndex index = comboIndexForRow(row);
        if (!index.isValid() || !m_view) {
            return;
        }

        m_view->setCurrentIndex(index);
        if (m_view->selectionModel()) {
            m_view->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        }
        m_view->scrollTo(index, QAbstractItemView::PositionAtCenter);
    }

    void moveToEdge(bool toEnd)
    {
        const int rows = rowCount();
        if (rows <= 0 || !m_view) {
            return;
        }

        const QModelIndex index = comboIndexForRow(toEnd ? rows - 1 : 0);
        if (!index.isValid()) {
            return;
        }

        m_view->setCurrentIndex(index);
        if (m_view->selectionModel()) {
            m_view->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        }
        m_view->scrollTo(index, QAbstractItemView::PositionAtCenter);
    }

    void activateCurrent()
    {
        if (!m_view) {
            return;
        }
        activateIndex(m_view->currentIndex());
    }

    void activateIndex(const QModelIndex &index)
    {
        if (!m_combo || !index.isValid()) {
            dismiss(true);
            return;
        }

        m_combo->commitPopupIndex(index.row());
        dismiss(true);
    }

    void positionPopupBelowOrAbove()
    {
        if (!m_combo) {
            return;
        }

        const QSize popupSize = popupSizeHint();
        const QPoint globalTopLeft = m_combo->mapToGlobal(QPoint(0, 0));
        const int comboTopY = globalTopLeft.y();
        const int comboBottomY = globalTopLeft.y() + m_combo->height();

        QScreen *screen = QApplication::screenAt(globalTopLeft);
        if (!screen) {
            screen = QApplication::primaryScreen();
        }
        const QRect avail = screen ? screen->availableGeometry() : QRect();

        QRect popupGeom(globalTopLeft, popupSize);

        const int belowTop = comboBottomY + kFluentComboPopupGap;
        const int aboveTop = comboTopY - kFluentComboPopupGap - popupSize.height();

        const bool fitsBelow = !avail.isValid() || (belowTop + popupSize.height() <= avail.bottom() + 1);
        const bool fitsAbove = !avail.isValid() || (aboveTop >= avail.top());

        popupGeom.moveTop((fitsBelow || !fitsAbove) ? belowTop : aboveTop);
        m_openSlideOffsetY = (fitsBelow || !fitsAbove) ? -kFluentComboPopupSlideOffset : kFluentComboPopupSlideOffset;

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

    QPointer<FluentComboBox> m_combo;
    QPointer<QAbstractItemView> m_view;
    FluentBorderEffect m_border;
    QVariantAnimation *m_openAnim = nullptr;
    QRect m_targetGeom;
    int m_openSlideOffsetY = -kFluentComboPopupSlideOffset;
    bool m_appFilterInstalled = false;
    bool m_dismissing = false;
};

namespace {
class FluentComboPopupView final : public QListView
{
public:
    explicit FluentComboPopupView(QWidget *parent = nullptr)
        : QListView(parent)
    {
        // QComboBox owns a private popup container; the view itself should not be a top-level popup.
        setObjectName(QStringLiteral("FluentComboPopupView"));
        setAttribute(Qt::WA_StyledBackground, false);
        setAutoFillBackground(false);
        setFrameShape(QFrame::NoFrame);

        setSpacing(2);
        // Keep a small inset for rounded corners; no shadows for performance.
        setViewportMargins(6, 6, 6, 6);
        setUniformItemSizes(true);
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        setVerticalScrollBar(new FluentScrollBar(Qt::Vertical, this));

        m_hoverAnim = new QVariantAnimation(this);
        m_hoverAnim->setDuration(120);
        connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            m_hoverLevel = value.toReal();
            if (viewport()) {
                viewport()->update();
            }
        });

        m_selAnim = new QVariantAnimation(this);
        m_selAnim->setDuration(180);
        m_selAnim->setEasingCurve(QEasingCurve::InOutCubic);
        connect(m_selAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            const qreal t = qBound<qreal>(0.0, value.toReal(), 1.0);
            m_selRect = QRectF(
                m_selStartRect.x() + (m_selTargetRect.x() - m_selStartRect.x()) * t,
                m_selStartRect.y() + (m_selTargetRect.y() - m_selStartRect.y()) * t,
                m_selStartRect.width() + (m_selTargetRect.width() - m_selStartRect.width()) * t,
                m_selStartRect.height() + (m_selTargetRect.height() - m_selStartRect.height()) * t);
            m_selOpacity = m_selStartOpacity + (m_selTargetOpacity - m_selStartOpacity) * t;
            if (viewport()) {
                viewport()->update();
            }
        });
        connect(m_selAnim, &QVariantAnimation::finished, this, [this]() {
            if (m_selTargetOpacity <= 0.0) {
                m_selRect = QRectF();
                m_selOpacity = 0.0;
            } else {
                m_selOpacity = 1.0;
            }
            if (viewport()) {
                viewport()->update();
            }
        });

        if (viewport()) {
            viewport()->setAutoFillBackground(false);
            viewport()->setAttribute(Qt::WA_StyledBackground, false);
        }

        hookSelectionModel();
    }

    QModelIndex hoverIndex() const { return m_hoverIndex; }
    qreal hoverLevel() const { return m_hoverLevel; }

    void setModel(QAbstractItemModel *model) override
    {
        QListView::setModel(model);
        hookSelectionModel();
    }

protected:
    bool event(QEvent *event) override
    {
        return QListView::event(event);
    }

    void paintEvent(QPaintEvent *event) override
    {
        const QModelIndex current = currentIndex();
        const bool paintAnim = m_selAnim && m_selAnim->state() == QAbstractAnimation::Running && m_selRect.isValid() && m_selOpacity > 0.0;
        const bool paintStatic = current.isValid() && selectionModel() && selectionModel()->isSelected(current);

        if (paintAnim || paintStatic) {
            const auto &colors = ThemeManager::instance().colors();
            const QRectF rect = paintAnim ? m_selRect : selectionRectForIndex(current);
            const qreal opacity = paintAnim ? qBound<qreal>(0.0, m_selOpacity, 1.0) : 1.0;

            QPainter painter(viewport());
            if (painter.isActive()) {
                painter.setRenderHint(QPainter::Antialiasing, true);

                QColor fill = colors.accent;
                fill.setAlpha(qBound(0, int(std::lround(40.0 * opacity)), 40));
                painter.setPen(Qt::NoPen);
                painter.setBrush(fill);
                painter.drawRoundedRect(rect, 4.0, 4.0);

                QColor indicator = colors.accent;
                indicator.setAlpha(qBound(0, int(std::lround(255.0 * opacity)), 255));
                painter.setBrush(indicator);
                const qreal indicatorHeight = 16.0;
                const QRectF indicatorRect(rect.left(), rect.center().y() - indicatorHeight / 2.0, 3.0, indicatorHeight);
                painter.drawRoundedRect(indicatorRect, 1.5, 1.5);
            }
        }

        QListView::paintEvent(event);
    }

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
        startHoverAnimation(0.0);
        m_hoverIndex = QModelIndex();
        QListView::leaveEvent(event);
    }

private:
    void hookSelectionModel()
    {
        if (!selectionModel()) {
            return;
        }

        disconnect(selectionModel(), nullptr, this, nullptr);
        connect(selectionModel(), &QItemSelectionModel::currentChanged, this,
                [this](const QModelIndex &current, const QModelIndex &previous) {
                    startSelectionAnimation(previous, current);
                });

        const QModelIndex current = currentIndex();
        m_selRect = selectionRectForIndex(current);
        m_selStartRect = m_selRect;
        m_selTargetRect = m_selRect;
        m_selOpacity = current.isValid() ? 1.0 : 0.0;
    }

    QRectF selectionRectForIndex(const QModelIndex &index) const
    {
        if (!index.isValid()) {
            return QRectF();
        }
        const QRect rect = visualRect(index);
        if (!rect.isValid()) {
            return QRectF();
        }
        return QRectF(rect).adjusted(4, 2, -4, -2);
    }

    void startSelectionAnimation(const QModelIndex &from, const QModelIndex &to)
    {
        const bool animRunning = m_selAnim && m_selAnim->state() == QAbstractAnimation::Running;
        const QRectF startRect = animRunning ? m_selRect : selectionRectForIndex(from);
        const qreal startOpacity = animRunning ? m_selOpacity : (startRect.isValid() ? 1.0 : 0.0);
        const QRectF targetRect = selectionRectForIndex(to);

        if (!targetRect.isValid()) {
            if (!startRect.isValid()) {
                m_selRect = QRectF();
                m_selOpacity = 0.0;
                if (viewport()) {
                    viewport()->update();
                }
                return;
            }

            m_selStartRect = startRect;
            m_selTargetRect = startRect;
            m_selStartOpacity = startOpacity;
            m_selTargetOpacity = 0.0;
            m_selRect = startRect;
            m_selOpacity = startOpacity;
            m_selAnim->stop();
            m_selAnim->setStartValue(0.0);
            m_selAnim->setEndValue(1.0);
            m_selAnim->start();
            return;
        }

        if (!startRect.isValid()) {
            m_selStartRect = targetRect;
            m_selTargetRect = targetRect;
            m_selStartOpacity = 0.0;
            m_selTargetOpacity = 1.0;
            m_selRect = targetRect;
            m_selOpacity = 0.0;
            m_selAnim->stop();
            m_selAnim->setStartValue(0.0);
            m_selAnim->setEndValue(1.0);
            m_selAnim->start();
            return;
        }

        if (startRect == targetRect) {
            m_selRect = targetRect;
            m_selOpacity = 1.0;
            if (viewport()) {
                viewport()->update();
            }
            return;
        }

        m_selStartRect = startRect;
        m_selTargetRect = targetRect;
        m_selStartOpacity = 0.75;
        m_selTargetOpacity = 1.0;
        m_selRect = startRect;
        m_selOpacity = m_selStartOpacity;

        m_selAnim->stop();
        m_selAnim->setStartValue(0.0);
        m_selAnim->setEndValue(1.0);
        m_selAnim->start();
    }

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
    QVariantAnimation *m_selAnim = nullptr;
    QRectF m_selRect;
    QRectF m_selStartRect;
    QRectF m_selTargetRect;
    qreal m_selOpacity = 0.0;
    qreal m_selStartOpacity = 0.0;
    qreal m_selTargetOpacity = 0.0;
};

class FluentComboItemDelegate final : public QStyledItemDelegate
{
public:
    explicit FluentComboItemDelegate(FluentComboPopupView *view)
        : QStyledItemDelegate(view)
        , m_view(view)
    {
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        // Increase item height in popup logic
        QSize sz = QStyledItemDelegate::sizeHint(option, index);
        if (sz.height() < 32) sz.setHeight(32); // Minimum 32px height for Fluent ComboBox items
        return sz;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        opt.state &= ~QStyle::State_HasFocus; // Remove dotted line

        const auto &colors = ThemeManager::instance().colors();

        const bool enabled = opt.state.testFlag(QStyle::State_Enabled);

        const QRectF itemRect = QRectF(opt.rect).adjusted(4, 2, -4, -2);
        const bool selected = opt.state.testFlag(QStyle::State_Selected);
        const bool isCurrent = m_view && m_view->currentIndex().isValid() && index == m_view->currentIndex();
        const bool hovered = m_view && index == m_view->hoverIndex();

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        // Background
        QColor bgColor = Qt::transparent;
        if (selected && !isCurrent) {
            bgColor = colors.accent;
            bgColor.setAlpha(40);
        } else if (hovered) {
            QColor hover = colors.hover;
            hover.setAlphaF(0.3 * m_view->hoverLevel());
            bgColor = hover;
        }

        if (bgColor.alpha() > 0) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(bgColor);
            painter->drawRoundedRect(itemRect, 4, 4);
        }

        // Selected indicator
        // Icon + text (fully custom to avoid native selection rendering)
        const int iconSize = 16;
        const int leftPadding = 10;
        qreal x = itemRect.left() + leftPadding;

        const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
        if (!icon.isNull()) {
            const QRect iconRect(static_cast<int>(x), static_cast<int>(itemRect.center().y() - iconSize / 2), iconSize, iconSize);
            icon.paint(painter, iconRect, Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
            x += iconSize + 8;
        }

        const QString text = opt.text;
        const QRectF textRect(x, itemRect.top(), itemRect.right() - x - 8, itemRect.height());
        painter->setPen(enabled ? colors.text : colors.disabledText);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

        painter->restore();
    }

private:
    FluentComboPopupView *m_view = nullptr;
};

} // namespace

FluentComboBox::FluentComboBox(QWidget *parent)
    : QComboBox(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);

    setMinimumHeight(Style::metrics().height);

    m_hoverAnim = new QPropertyAnimation(this, "hoverLevel", this);
    m_hoverAnim->setDuration(120);

    m_popup = new FluentComboPopup(this);

    auto *popupView = new FluentComboPopupView(this);
    setView(popupView);
    if (view()) {
        view()->setItemDelegate(new FluentComboItemDelegate(popupView));
        view()->setMouseTracking(true);
        view()->setAttribute(Qt::WA_TranslucentBackground, false);
        view()->setAttribute(Qt::WA_StyledBackground, false);
        view()->setAutoFillBackground(false);
        if (view()->viewport()) {
            view()->viewport()->setAttribute(Qt::WA_TranslucentBackground, false);
            view()->viewport()->setAttribute(Qt::WA_StyledBackground, false);
            view()->viewport()->setAutoFillBackground(false);
        }
    }

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentComboBox::applyTheme);
}
QSize FluentComboBox::sizeHint() const
{
    QSize sz = QComboBox::sizeHint();
    if (sz.height() < 32) sz.setHeight(32);
    return sz;
}
qreal FluentComboBox::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentComboBox::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

void FluentComboBox::changeEvent(QEvent *event)
{
    QComboBox::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentComboBox::applyTheme()
{
    const auto &colors = ThemeManager::instance().colors();
    if (view()) {
        QPalette viewPal = view()->palette();
        viewPal.setColor(QPalette::Text, colors.text);
        view()->setPalette(viewPal);

        const QString viewNext = QString(
            "QListView#FluentComboPopupView {"
            "  background: transparent;"
            "  color: %1;"
            "  border: none;"
            "  outline: 0;"
            "}"
            "QListView#FluentComboPopupView::viewport {"
            "  background: transparent;"
            "}"
            "QScrollBar:vertical {"
            "  background: transparent;"
            "  width: 10px;"
            "  margin: 2px;"
            "}"
            "QScrollBar::handle:vertical {"
            "  background-color: %2;"
            "  border: 1px solid transparent;"
            "  border-radius: 999px;"
            "  min-height: 24px;"
            "}"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
            "  height: 0px;"
            "}"
        ).arg(colors.text.name())
         .arg(colors.border.name());

        if (view()->styleSheet() != viewNext) {
            view()->setStyleSheet(viewNext);
        }

        if (m_popup) {
            m_popup->syncFromCombo();
        }
    }
    update();
}

void FluentComboBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    const auto &colors = ThemeManager::instance().colors();

    const bool enabled = isEnabled();
    const bool expanded = isPopupVisible();

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF rect = QRectF(this->rect());
    Style::paintControlSurface(painter, rect, colors, m_hoverLevel, hasFocus() ? 1.0 : 0.0, enabled, expanded);

    const auto m = Style::metrics();
    const QRect textRect = this->rect().adjusted(m.paddingX, 0, -m.iconAreaWidth, 0);
    const QColor textColor = enabled ? colors.text : colors.disabledText;

    painter.setPen(textColor);
    const QString elided = fontMetrics().elidedText(currentText(), Qt::ElideRight, textRect.width());
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elided);

    const QRect arrowRect(this->rect().right() - m.iconAreaWidth, this->rect().top(), m.iconAreaWidth, this->rect().height());
    QColor separator = colors.border;
    separator.setAlpha(80);
    painter.setPen(QPen(separator, 1));
    painter.drawLine(QPointF(arrowRect.left() + 0.5, arrowRect.top() + 8), QPointF(arrowRect.left() + 0.5, arrowRect.bottom() - 8));

    const QColor iconColor = enabled ? colors.subText : colors.disabledText;
    Style::drawChevronDown(painter, arrowRect.center(), iconColor, 8.0, 1.7);
}

void FluentComboBox::enterEvent(FluentEnterEvent *event)
{
    QComboBox::enterEvent(event);
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();
}

void FluentComboBox::leaveEvent(QEvent *event)
{
    QComboBox::leaveEvent(event);
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(0.0);
    m_hoverAnim->start();
}

void FluentComboBox::showPopup()
{
    if (!m_popup) {
        return;
    }

    if (m_popup->isVisible()) {
        hidePopup();
        return;
    }

    m_popup->popup();
    update();
}

void FluentComboBox::hidePopup()
{
    if (m_popup) {
        m_popup->dismiss(false);
    }
    QComboBox::hidePopup();
    update();
}

void FluentComboBox::commitPopupIndex(int row)
{
    if (row < 0 || row >= count()) {
        return;
    }

    setCurrentIndex(row);
    emit activated(row);
    emit textActivated(itemText(row));
}

bool FluentComboBox::isPopupVisible() const
{
    return m_popup && m_popup->isVisible();
}

int FluentComboBox::popupScrollThreshold() const
{
    return effectivePopupScrollThreshold();
}

void FluentComboBox::setPopupScrollThreshold(int threshold)
{
    threshold = qMax(0, threshold);
    if (m_popupScrollThreshold == threshold) {
        return;
    }

    m_popupScrollThreshold = threshold;
    if (m_popup) {
        m_popup->syncFromCombo();
    }
}

int FluentComboBox::effectivePopupScrollThreshold() const
{
    return m_popupScrollThreshold > 0 ? m_popupScrollThreshold : qMax(1, maxVisibleItems());
}

} // namespace Fluent
