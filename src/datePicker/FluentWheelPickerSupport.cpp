#include "FluentWheelPickerSupport.h"

#include "Fluent/FluentPopupSurface.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QApplication>
#include <QCursor>
#include <QHideEvent>
#include <QKeyEvent>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QScrollBar>
#include <QShowEvent>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QVariantAnimation>

#include <climits>
#include <numeric>

namespace Fluent::Detail {

namespace {

class FluentWheelPickerItemDelegate final : public QStyledItemDelegate
{
public:
    explicit FluentWheelPickerItemDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (!painter) {
            return;
        }

        const QVariant value = index.data(Qt::UserRole);
        if (!value.isValid()) {
            return;
        }

        const auto *column = dynamic_cast<const FluentWheelPickerColumn *>(option.widget);
        const auto &colors = ThemeManager::instance().colors();

        const QRect viewportRect = column && column->viewport() ? column->viewport()->rect() : option.rect;
        const qreal distance = qAbs(option.rect.center().y() - viewportRect.center().y()) /
                               qMax<qreal>(1.0, qreal(column ? column->itemHeight() * 2 : 108));
        const qreal opacity = qBound<qreal>(0.38, 1.0 - distance * 0.42, 1.0);

        QColor textColor = colors.text;
        textColor.setAlpha(qBound(0, int(opacity * 255.0), 255));

        painter->save();
        painter->setRenderHint(QPainter::TextAntialiasing, true);
        painter->setPen(textColor);

        QFont font = option.font;
        if (column && index.row() == column->currentRow()) {
            font.setWeight(QFont::DemiBold);
        }
        painter->setFont(font);

        const QString text = option.fontMetrics.elidedText(index.data(Qt::DisplayRole).toString(),
                                                           Qt::ElideRight,
                                                           qMax(0, option.rect.width() - 18));
        const Qt::Alignment alignment = column ? column->textAlignment() : Qt::AlignCenter;
        painter->drawText(option.rect.adjusted(9, 0, -9, 0), alignment | Qt::AlignVCenter, text);
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const auto *column = dynamic_cast<const FluentWheelPickerColumn *>(option.widget);
        const int height = column ? column->itemHeight() : 54;
        return QSize(QStyledItemDelegate::sizeHint(option, index).width(), height);
    }
};

void drawAcceptGlyph(QPainter &painter, const QRectF &rect, const QColor &color)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    const QPointF center = rect.center();
    const QRectF glyphBox(center.x() - 8.0, center.y() - 8.0, 16.0, 16.0);
    const QRectF r = glyphBox.adjusted(2.0, 2.0, -2.0, -2.0);

    QPainterPath path;
    path.moveTo(r.left() + r.width() * 0.10, r.top() + r.height() * 0.56);
    path.lineTo(r.left() + r.width() * 0.42, r.top() + r.height() * 0.86);
    path.lineTo(r.left() + r.width() * 0.92, r.top() + r.height() * 0.12);
    painter.drawPath(path);
    painter.restore();
}

void drawDismissGlyph(QPainter &painter, const QRectF &rect, const QColor &color)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    const QPointF center = rect.center();
    const QRectF glyphBox(center.x() - 8.0, center.y() - 8.0, 16.0, 16.0);
    const QRectF r = glyphBox.adjusted(2.2, 2.2, -2.2, -2.2);

    painter.drawLine(QPointF(r.left(), r.top()), QPointF(r.right(), r.bottom()));
    painter.drawLine(QPointF(r.right(), r.top()), QPointF(r.left(), r.bottom()));
    painter.restore();
}

} // namespace

FluentWheelPickerColumn::FluentWheelPickerColumn(QWidget *parent)
    : QListWidget(parent)
{
    setFrameShape(QFrame::NoFrame);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setMouseTracking(true);
    setUniformItemSizes(true);
    setSpacing(0);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);
    setItemDelegate(new FluentWheelPickerItemDelegate(this));

    if (viewport()) {
        viewport()->setAttribute(Qt::WA_StyledBackground, false);
        viewport()->setAutoFillBackground(false);
    }

    m_scrollAnim = new QVariantAnimation(this);
    m_scrollAnim->setDuration(120);
    m_scrollAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_scrollAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        if (!verticalScrollBar()) {
            return;
        }

        m_syncingFromScroll = true;
        verticalScrollBar()->setValue(value.toInt());
        m_syncingFromScroll = false;
        syncCurrentFromScroll();
        if (viewport()) {
            viewport()->update();
        }
    });

    m_snapTimer = new QTimer(this);
    m_snapTimer->setSingleShot(true);
    m_snapTimer->setInterval(90);
    connect(m_snapTimer, &QTimer::timeout, this, [this]() {
        snapToCurrent(true);
    });

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
        if (m_settingOptions) {
            return;
        }
        syncCurrentFromScroll();
        if (!m_syncingFromScroll) {
            scheduleSnap();
        }
        if (viewport()) {
            viewport()->update();
        }
    });
}

void FluentWheelPickerColumn::setOptions(const QVector<PickerOption> &options, const QVariant &selectedValue)
{
    m_settingOptions = true;
    m_options = options;
    m_wheelAngleRemainder = 0;

    if (m_scrollAnim) {
        m_scrollAnim->stop();
    }
    if (m_snapTimer) {
        m_snapTimer->stop();
    }

    clear();

    auto addSpacer = [this]() {
        auto *item = new QListWidgetItem();
        item->setFlags(Qt::NoItemFlags);
        item->setData(Qt::UserRole, QVariant());
        item->setSizeHint(QSize(0, m_itemHeight));
        addItem(item);
    };

    for (int i = 0; i < spacerRowCount(); ++i) {
        addSpacer();
    }

    for (const PickerOption &option : m_options) {
        auto *item = new QListWidgetItem(option.text);
        item->setData(Qt::UserRole, option.value);
        item->setTextAlignment(int(m_alignment | Qt::AlignVCenter));
        item->setSizeHint(QSize(0, m_itemHeight));
        addItem(item);
    }

    for (int i = 0; i < spacerRowCount(); ++i) {
        addSpacer();
    }

    const int row = rowForValue(selectedValue);
    if (row >= 0) {
        setCurrentRow(row);
        scrollToRowCentered(row, false);
    }

    m_settingOptions = false;
    if (viewport()) {
        viewport()->update();
    }
}

QVariant FluentWheelPickerColumn::currentValue() const
{
    const int row = currentRow();
    if (row < firstDataRow() || row > lastDataRow()) {
        return QVariant();
    }
    if (const QListWidgetItem *item = this->item(row)) {
        return item->data(Qt::UserRole);
    }
    return QVariant();
}

QString FluentWheelPickerColumn::currentText() const
{
    const int row = currentRow();
    if (row < firstDataRow() || row > lastDataRow()) {
        return QString();
    }
    if (const QListWidgetItem *item = this->item(row)) {
        return item->text();
    }
    return QString();
}

void FluentWheelPickerColumn::setCurrentValue(const QVariant &value)
{
    const int row = rowForValue(value);
    if (row < 0) {
        return;
    }

    setCurrentRow(row);
    scrollToRowCentered(row, false);
    if (viewport()) {
        viewport()->update();
    }
}

int FluentWheelPickerColumn::itemHeight() const
{
    return m_itemHeight;
}

void FluentWheelPickerColumn::setTextAlignment(Qt::Alignment alignment)
{
    if (m_alignment == alignment) {
        return;
    }

    m_alignment = alignment;
    for (int row = firstDataRow(); row <= lastDataRow(); ++row) {
        if (QListWidgetItem *listItem = item(row)) {
            listItem->setTextAlignment(int(m_alignment | Qt::AlignVCenter));
        }
    }
    viewport()->update();
}

Qt::Alignment FluentWheelPickerColumn::textAlignment() const
{
    return m_alignment;
}

void FluentWheelPickerColumn::snapToCurrent(bool animated)
{
    const int row = currentRow() < firstDataRow() ? firstDataRow() : currentRow();
    scrollToRowCentered(qBound(firstDataRow(), row, lastDataRow()), animated);
}

void FluentWheelPickerColumn::paintEvent(QPaintEvent *event)
{
    {
        QPainter painter(viewport());
        if (painter.isActive()) {
            painter.setRenderHint(QPainter::Antialiasing, true);

            const auto &colors = ThemeManager::instance().colors();
            QColor fill = colors.accent;
            fill.setAlpha(44);
            QColor outline = colors.accent;
            outline.setAlpha(72);

            painter.setPen(QPen(outline, 1));
            painter.setBrush(fill);
            painter.drawRoundedRect(selectionRect(), 5.0, 5.0);
        }
    }

    QListWidget::paintEvent(event);
}

void FluentWheelPickerColumn::wheelEvent(QWheelEvent *event)
{
    if (!event) {
        QListWidget::wheelEvent(event);
        return;
    }

    const int steps = stepsFromWheelEvent(event);
    if (steps == 0) {
        event->accept();
        return;
    }

    moveCurrentBySteps(steps, true);
    event->accept();
}

void FluentWheelPickerColumn::mousePressEvent(QMouseEvent *event)
{
    if (m_scrollAnim) {
        m_scrollAnim->stop();
    }
    m_wheelAngleRemainder = 0;
    QListWidget::mousePressEvent(event);
}

void FluentWheelPickerColumn::mouseReleaseEvent(QMouseEvent *event)
{
    QListWidget::mouseReleaseEvent(event);

    const QModelIndex index = indexAt(event ? event->pos() : QPoint());
    if (index.isValid()) {
        const int row = qBound(firstDataRow(), index.row(), lastDataRow());
        setCurrentRow(row);
    }

    scheduleSnap();
}

void FluentWheelPickerColumn::keyPressEvent(QKeyEvent *event)
{
    if (!event) {
        QListWidget::keyPressEvent(event);
        return;
    }

    auto moveCurrent = [this](int actualIndex) {
        const int clamped = qBound(0, actualIndex, qMax(0, m_options.size() - 1));
        setCurrentRow(rowForActualIndex(clamped));
        snapToCurrent(true);
    };

    switch (event->key()) {
    case Qt::Key_Up:
        moveCurrent(actualIndexForRow(currentRow()) - 1);
        event->accept();
        return;
    case Qt::Key_Down:
        moveCurrent(actualIndexForRow(currentRow()) + 1);
        event->accept();
        return;
    case Qt::Key_PageUp:
        moveCurrent(actualIndexForRow(currentRow()) - 3);
        event->accept();
        return;
    case Qt::Key_PageDown:
        moveCurrent(actualIndexForRow(currentRow()) + 3);
        event->accept();
        return;
    case Qt::Key_Home:
        moveCurrent(0);
        event->accept();
        return;
    case Qt::Key_End:
        moveCurrent(m_options.size() - 1);
        event->accept();
        return;
    default:
        break;
    }

    QListWidget::keyPressEvent(event);
}

int FluentWheelPickerColumn::spacerRowCount() const
{
    return 2;
}

int FluentWheelPickerColumn::firstDataRow() const
{
    return spacerRowCount();
}

int FluentWheelPickerColumn::lastDataRow() const
{
    return qMax(firstDataRow() - 1, firstDataRow() + m_options.size() - 1);
}

int FluentWheelPickerColumn::actualIndexForRow(int row) const
{
    return qBound(0, row - firstDataRow(), qMax(0, m_options.size() - 1));
}

int FluentWheelPickerColumn::rowForActualIndex(int index) const
{
    if (m_options.isEmpty()) {
        return -1;
    }
    return firstDataRow() + qBound(0, index, m_options.size() - 1);
}

int FluentWheelPickerColumn::rowForValue(const QVariant &value) const
{
    if (m_options.isEmpty()) {
        return -1;
    }

    if (value.isValid()) {
        for (int i = 0; i < m_options.size(); ++i) {
            if (m_options.at(i).value == value) {
                return rowForActualIndex(i);
            }
        }
    }

    return rowForActualIndex(0);
}

int FluentWheelPickerColumn::nearestDataRow() const
{
    if (m_options.isEmpty()) {
        return -1;
    }

    const QModelIndex centered = indexAt(QPoint(viewport()->rect().center().x(), viewport()->rect().center().y()));
    if (centered.isValid()) {
        return qBound(firstDataRow(), centered.row(), lastDataRow());
    }

    int nearestRow = firstDataRow();
    int nearestDistance = INT_MAX;
    for (int row = firstDataRow(); row <= lastDataRow(); ++row) {
        const QModelIndex idx = model()->index(row, 0);
        const QRect rect = visualRect(idx);
        const int distance = qAbs(rect.center().y() - viewport()->rect().center().y());
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearestRow = row;
        }
    }
    return nearestRow;
}

QRectF FluentWheelPickerColumn::selectionRect() const
{
    return QRectF(4.0,
                  viewport()->rect().center().y() - m_itemHeight / 2.0,
                  qMax(0, viewport()->width() - 8),
                  m_itemHeight);
}

void FluentWheelPickerColumn::scheduleSnap()
{
    if (m_snapTimer && !m_settingOptions) {
        m_snapTimer->start();
    }
}

void FluentWheelPickerColumn::syncCurrentFromScroll()
{
    const int row = nearestDataRow();
    if (row < 0 || row == currentRow()) {
        return;
    }

    m_syncingFromScroll = true;
    setCurrentRow(row);
    m_syncingFromScroll = false;
}

int FluentWheelPickerColumn::stepsFromWheelEvent(QWheelEvent *event)
{
    if (!event) {
        return 0;
    }

    const int angleDelta = event->angleDelta().y();
    if (angleDelta != 0) {
        m_wheelAngleRemainder += angleDelta;
        const int steps = m_wheelAngleRemainder / 120;
        m_wheelAngleRemainder %= 120;
        return steps;
    }

    const QPoint pixelDelta = event->pixelDelta();
    if (!pixelDelta.isNull()) {
        return pixelDelta.y() > 0 ? 1 : -1;
    }

    return 0;
}

void FluentWheelPickerColumn::moveCurrentBySteps(int steps, bool animated)
{
    if (m_options.isEmpty() || steps == 0) {
        return;
    }

    if (m_scrollAnim) {
        m_scrollAnim->stop();
    }
    if (m_snapTimer) {
        m_snapTimer->stop();
    }

    const int currentDataRow = qMax(firstDataRow(), nearestDataRow());
    const int currentIndex = actualIndexForRow(currentDataRow);
    const int targetIndex = qBound(0, currentIndex - steps, qMax(0, m_options.size() - 1));
    const int targetRow = rowForActualIndex(targetIndex);
    if (targetRow < 0) {
        return;
    }

    setCurrentRow(targetRow);
    scrollToRowCentered(targetRow, animated);
    if (viewport()) {
        viewport()->update();
    }
}

void FluentWheelPickerColumn::scrollToRowCentered(int row, bool animated)
{
    if (row < firstDataRow() || row > lastDataRow() || !verticalScrollBar()) {
        return;
    }

    const QModelIndex idx = model()->index(row, 0);
    const QRect rect = visualRect(idx);
    const int delta = rect.center().y() - viewport()->rect().center().y();
    const int target = qBound(verticalScrollBar()->minimum(),
                              verticalScrollBar()->value() + delta,
                              verticalScrollBar()->maximum());

    if (!animated || qAbs(target - verticalScrollBar()->value()) < 2) {
        m_syncingFromScroll = true;
        verticalScrollBar()->setValue(target);
        m_syncingFromScroll = false;
        syncCurrentFromScroll();
        if (viewport()) {
            viewport()->update();
        }
        return;
    }

    if (m_scrollAnim) {
        m_scrollAnim->stop();
        const int distance = qAbs(target - verticalScrollBar()->value());
        m_scrollAnim->setDuration(qBound(120, 120 + distance / 3, 220));
        m_scrollAnim->setStartValue(verticalScrollBar()->value());
        m_scrollAnim->setEndValue(target);
        m_scrollAnim->start();
    }
}

FluentWheelPickerPopup::FluentWheelPickerPopup(QWidget *anchor)
    : QWidget(nullptr, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
    , m_anchor(anchor)
    , m_border(this, this)
{
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
        const qreal progress = qBound<qreal>(0.0, value.toReal(), 1.0);
        setWindowOpacity(progress);
        if (m_targetGeom.isValid()) {
            QRect geom = m_targetGeom;
            geom.translate(0, int((1.0 - progress) * m_openSlideOffsetY));
            setGeometry(geom);
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
        } else {
            m_border.syncFromTheme();
        }
        update();
    });
}

void FluentWheelPickerPopup::setAnchor(QWidget *anchor)
{
    m_anchor = anchor;
}

QWidget *FluentWheelPickerPopup::anchor() const
{
    return m_anchor.data();
}

void FluentWheelPickerPopup::setColumnConfigs(const QVector<PickerColumnConfig> &configs)
{
    ensureColumns(configs.size());
    m_columnWidths.clear();
    m_columnWidths.reserve(configs.size());

    for (int i = 0; i < configs.size(); ++i) {
        FluentWheelPickerColumn *column = m_columns.at(i);
        const PickerColumnConfig &config = configs.at(i);

        column->setTextAlignment(config.alignment);
        column->setFixedWidth(config.width);
        column->setOptions(config.options, config.currentValue);
        column->show();

        m_columnWidths.push_back(config.width);
    }

    for (int i = configs.size(); i < m_columns.size(); ++i) {
        m_columns.at(i)->hide();
    }

    relayoutColumns();
    update();
}

int FluentWheelPickerPopup::columnCount() const
{
    return m_columnWidths.size();
}

FluentWheelPickerColumn *FluentWheelPickerPopup::columnAt(int index) const
{
    if (index < 0 || index >= columnCount()) {
        return nullptr;
    }
    return m_columns.value(index, nullptr);
}

QVariant FluentWheelPickerPopup::columnValue(int index) const
{
    if (FluentWheelPickerColumn *column = columnAt(index)) {
        return column->currentValue();
    }
    return QVariant();
}

void FluentWheelPickerPopup::setColumnValue(int index, const QVariant &value)
{
    if (FluentWheelPickerColumn *column = columnAt(index)) {
        column->setCurrentValue(value);
    }
}

void FluentWheelPickerPopup::popup()
{
    if (columnCount() <= 0) {
        return;
    }

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

    if (!m_appFilterInstalled && qApp) {
        qApp->installEventFilter(this);
        m_appFilterInstalled = true;
    }

    if (FluentWheelPickerColumn *column = columnAt(0)) {
        column->setFocus(Qt::PopupFocusReason);
    }

    m_openAnim->setStartValue(0.0);
    m_openAnim->setEndValue(1.0);
    m_openAnim->start();
}

void FluentWheelPickerPopup::dismiss(bool accepted, bool returnFocus)
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

    if (accepted && onAccepted) {
        onAccepted();
    }

    hide();

    if (returnFocus && m_anchor) {
        m_anchor->setFocus(Qt::PopupFocusReason);
    }

    if (m_anchor) {
        m_anchor->update();
    }

    if (onDismissed) {
        onDismissed(accepted);
    }

    m_dismissing = false;
}

bool FluentWheelPickerPopup::event(QEvent *event)
{
    if (event) {
        if (event->type() == QEvent::WindowDeactivate ||
            event->type() == QEvent::ApplicationDeactivate) {
            dismiss(false, false);
            return true;
        }
    }

    return QWidget::event(event);
}

bool FluentWheelPickerPopup::eventFilter(QObject *watched, QEvent *event)
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
                const bool clickedAnchor = m_anchor && m_anchor->rect().contains(m_anchor->mapFromGlobal(globalPos));
                dismiss(false, !clickedAnchor);
                if (clickedAnchor) {
                    return true;
                }
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    const bool watchedColumn = dynamic_cast<FluentWheelPickerColumn *>(watched) != nullptr;
    const bool watchedViewport = watched && watched->parent() && dynamic_cast<FluentWheelPickerColumn *>(watched->parent()) != nullptr;
    if ((watchedColumn || watchedViewport) && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (!keyEvent) {
            return QWidget::eventFilter(watched, event);
        }

        switch (keyEvent->key()) {
        case Qt::Key_Left:
            focusNextColumn(-1);
            keyEvent->accept();
            return true;
        case Qt::Key_Right:
            focusNextColumn(1);
            keyEvent->accept();
            return true;
        case Qt::Key_Tab:
            focusNextColumn(keyEvent->modifiers() & Qt::ShiftModifier ? -1 : 1);
            keyEvent->accept();
            return true;
        case Qt::Key_Backtab:
            focusNextColumn(-1);
            keyEvent->accept();
            return true;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            triggerAccept();
            keyEvent->accept();
            return true;
        case Qt::Key_Escape:
            dismiss(false, true);
            keyEvent->accept();
            return true;
        default:
            break;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void FluentWheelPickerPopup::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_border.playInitialTraceOnce(0);
}

void FluentWheelPickerPopup::hideEvent(QHideEvent *event)
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

void FluentWheelPickerPopup::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayoutColumns();
}

void FluentWheelPickerPopup::paintEvent(QPaintEvent *event)
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

    painter.save();
    painter.setClipPath(PopupSurface::contentClipPath(rect()));

    const QRect columnsRect = columnAreaRect();
    QColor divider = colors.border;
    divider.setAlpha(96);
    painter.setPen(QPen(divider, 1));

    int x = columnsRect.left();
    for (int i = 0; i + 1 < columnCount(); ++i) {
        x += m_columnWidths.at(i);
        painter.drawLine(QPointF(x + 0.5, columnsRect.top() + 6), QPointF(x + 0.5, columnsRect.bottom() - 6));
    }

    const QRect actions = actionBarRect();
    painter.drawLine(QPointF(actions.left() + 1, actions.top() + 0.5), QPointF(actions.right() - 1, actions.top() + 0.5));
    painter.drawLine(QPointF(width() / 2.0, actions.top() + 8), QPointF(width() / 2.0, actions.bottom() - 8));

    auto paintAction = [&](const QRect &actionRect, ActionButton button) {
        if (button == ActionButton::None) {
            return;
        }

        const bool hovered = (m_hoverAction == button);
        const bool pressed = (m_pressAction == button);
        if (!hovered && !pressed) {
            return;
        }

        QColor fill = Style::mix(colors.surface, colors.accent, pressed ? 0.18 : 0.10);
        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);
        painter.drawRect(actionRect.adjusted(1, 1, -1, 0));
    };

    paintAction(acceptRect(), ActionButton::Accept);
    paintAction(cancelRect(), ActionButton::Cancel);

    QColor iconColor = colors.text;
    iconColor.setAlpha(232);
    drawAcceptGlyph(painter, QRectF(acceptRect()).adjusted(0, 0, 0, -1), iconColor);
    drawDismissGlyph(painter, QRectF(cancelRect()).adjusted(0, 0, 0, -1), iconColor);

    painter.restore();
}

void FluentWheelPickerPopup::keyPressEvent(QKeyEvent *event)
{
    if (!event) {
        QWidget::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        triggerAccept();
        event->accept();
        return;
    case Qt::Key_Escape:
        dismiss(false, true);
        event->accept();
        return;
    default:
        break;
    }

    QWidget::keyPressEvent(event);
}

void FluentWheelPickerPopup::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    m_hoverAction = hitTestAction(event ? event->pos() : QPoint());
    update(actionBarRect());
}

void FluentWheelPickerPopup::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    if (!event || event->button() != Qt::LeftButton) {
        return;
    }

    m_pressAction = hitTestAction(event->pos());
    update(actionBarRect());
}

void FluentWheelPickerPopup::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);

    const ActionButton released = hitTestAction(event ? event->pos() : QPoint());
    const ActionButton pressed = m_pressAction;
    m_pressAction = ActionButton::None;
    update(actionBarRect());

    if (!event || event->button() != Qt::LeftButton || released != pressed) {
        return;
    }

    if (released == ActionButton::Accept) {
        triggerAccept();
    } else if (released == ActionButton::Cancel) {
        dismiss(false, true);
    }
}

void FluentWheelPickerPopup::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    m_hoverAction = ActionButton::None;
    update(actionBarRect());
}

QSize FluentWheelPickerPopup::popupSizeHint() const
{
    const int topPadding = 14;
    const int bottomPadding = 12;
    const int actionHeight = 52;
    const int rowHeight = columnCount() > 0 && columnAt(0) ? columnAt(0)->itemHeight() : 54;
    const int columnsHeight = rowHeight * 5;

    int width = 32;
    for (int i = 0; i < columnCount(); ++i) {
        width += m_columnWidths.at(i);
    }
    width += qMax(0, columnCount() - 1);

    return QSize(width, topPadding + columnsHeight + bottomPadding + actionHeight);
}

QRect FluentWheelPickerPopup::columnAreaRect() const
{
    const int topPadding = 14;
    const int actionHeight = 52;
    const int bottomPadding = 12;
    return QRect(16, topPadding, qMax(0, width() - 32), qMax(0, height() - topPadding - bottomPadding - actionHeight));
}

QRect FluentWheelPickerPopup::actionBarRect() const
{
    return QRect(0, qMax(0, height() - 52), width(), 52);
}

QRect FluentWheelPickerPopup::acceptRect() const
{
    const QRect actionRect = actionBarRect();
    return QRect(actionRect.left(), actionRect.top(), actionRect.width() / 2, actionRect.height());
}

QRect FluentWheelPickerPopup::cancelRect() const
{
    const QRect actionRect = actionBarRect();
    return QRect(actionRect.left() + actionRect.width() / 2,
                 actionRect.top(),
                 actionRect.width() - actionRect.width() / 2,
                 actionRect.height());
}

FluentWheelPickerPopup::ActionButton FluentWheelPickerPopup::hitTestAction(const QPoint &pos) const
{
    if (acceptRect().contains(pos)) {
        return ActionButton::Accept;
    }
    if (cancelRect().contains(pos)) {
        return ActionButton::Cancel;
    }
    return ActionButton::None;
}

void FluentWheelPickerPopup::ensureColumns(int count)
{
    while (m_columns.size() < count) {
        auto *column = new FluentWheelPickerColumn(this);
        column->installEventFilter(this);
        if (column->viewport()) {
            column->viewport()->installEventFilter(this);
        }
        m_columns.push_back(column);
    }
}

void FluentWheelPickerPopup::relayoutColumns()
{
    const QRect columnsRect = columnAreaRect();
    const int totalWidth = std::accumulate(m_columnWidths.begin(), m_columnWidths.end(), 0) + qMax(0, columnCount() - 1);
    int x = columnsRect.left() + qMax(0, (columnsRect.width() - totalWidth) / 2);

    for (int i = 0; i < columnCount(); ++i) {
        FluentWheelPickerColumn *column = columnAt(i);
        if (!column) {
            continue;
        }
        column->setGeometry(x, columnsRect.top(), m_columnWidths.at(i), columnsRect.height());
        x += m_columnWidths.at(i) + 1;
    }
}

void FluentWheelPickerPopup::positionPopupBelowOrAbove()
{
    if (!m_anchor) {
        return;
    }

    const QSize popupSize = popupSizeHint();
    const QPoint globalTopLeft = m_anchor->mapToGlobal(QPoint(0, 0));
    const int anchorTopY = globalTopLeft.y();
    const int anchorBottomY = globalTopLeft.y() + m_anchor->height();

    QScreen *screen = QApplication::screenAt(globalTopLeft);
    if (!screen) {
        screen = QApplication::primaryScreen();
    }
    const QRect available = screen ? screen->availableGeometry() : QRect();

    QRect popupGeom(globalTopLeft, popupSize);

    const int gap = 6;
    const int belowTop = anchorBottomY + gap;
    const int aboveTop = anchorTopY - gap - popupSize.height();

    const bool fitsBelow = !available.isValid() || (belowTop + popupSize.height() <= available.bottom() + 1);
    const bool fitsAbove = !available.isValid() || (aboveTop >= available.top());
    const bool placeBelow = fitsBelow || !fitsAbove;

    popupGeom.moveTop(placeBelow ? belowTop : aboveTop);
    m_openSlideOffsetY = placeBelow ? -PopupSurface::kOpenSlideOffsetPx : PopupSurface::kOpenSlideOffsetPx;

    if (available.isValid()) {
        if (popupGeom.left() < available.left()) {
            popupGeom.moveLeft(available.left());
        }
        if (popupGeom.right() > available.right()) {
            popupGeom.moveRight(available.right());
        }
        if (popupGeom.top() < available.top()) {
            popupGeom.moveTop(available.top());
        }
        if (popupGeom.bottom() > available.bottom()) {
            popupGeom.moveBottom(available.bottom());
        }
    }

    m_targetGeom = popupGeom;
    setGeometry(popupGeom);
    relayoutColumns();
}

void FluentWheelPickerPopup::focusNextColumn(int delta)
{
    if (columnCount() <= 0) {
        return;
    }

    int current = 0;
    QWidget *focusWidget = QApplication::focusWidget();
    for (int i = 0; i < columnCount(); ++i) {
        FluentWheelPickerColumn *column = columnAt(i);
        if (!column) {
            continue;
        }

        if (focusWidget == column || focusWidget == column->viewport()) {
            current = i;
            break;
        }
    }

    const int next = (current + delta + columnCount()) % columnCount();
    if (FluentWheelPickerColumn *column = columnAt(next)) {
        column->setFocus(Qt::TabFocusReason);
    }
}

void FluentWheelPickerPopup::triggerAccept()
{
    dismiss(true, true);
}

} // namespace Fluent::Detail