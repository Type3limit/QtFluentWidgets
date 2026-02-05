#include "Fluent/FluentCard.h"

#include "Fluent/FluentLabel.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentToolButton.h"

#include <QAbstractButton>
#include <QEvent>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QVBoxLayout>

namespace Fluent {

namespace {

static constexpr const char *kFlowDisableAnimationProperty = "fluentFlowDisableAnimation";

static QIcon makeChevronIcon(bool collapsed, const QColor &color)
{
    constexpr int iconSize = 16;
    QPixmap pm(iconSize, iconSize);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    const QPointF center(iconSize / 2.0, iconSize / 2.0);
    if (collapsed) {
        Style::drawChevronRight(p, center, color);
    } else {
        Style::drawChevronDown(p, center, color);
    }

    return QIcon(pm);
}

} // namespace

FluentCard::FluentCard(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("FluentCard");
    setAttribute(Qt::WA_StyledBackground, true);

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentCard::applyTheme);
}

void FluentCard::setCollapsible(bool on)
{
    if (m_collapsible == on) {
        return;
    }

    m_collapsible = on;
    if (m_collapsible) {
        ensureCollapsibleUi();
        applyCollapsedState(false);
        updateHeaderUi();
    } else {
        setProperty(kFlowDisableAnimationProperty, false);
        if (m_header) {
            m_header->hide();
            m_header->removeEventFilter(this);
        }
        if (m_content) {
            m_content->setVisible(true);
            m_content->setMaximumHeight(QWIDGETSIZE_MAX);
        }
        updateGeometry();
    }
}

bool FluentCard::isCollapsible() const
{
    return m_collapsible;
}

void FluentCard::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    updateHeaderUi();
}

QString FluentCard::title() const
{
    return m_title;
}

void FluentCard::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed) {
        return;
    }

    m_collapsed = collapsed;
    emit collapsedChanged(m_collapsed);

    if (m_collapsible) {
        applyCollapsedState(m_animateCollapse);
        updateHeaderUi();
    }
}

bool FluentCard::isCollapsed() const
{
    return m_collapsed;
}

void FluentCard::setCollapseAnimationEnabled(bool enabled)
{
    m_animateCollapse = enabled;
}

bool FluentCard::isCollapseAnimationEnabled() const
{
    return m_animateCollapse;
}

QWidget *FluentCard::contentWidget() const
{
    return m_content;
}

QVBoxLayout *FluentCard::contentLayout() const
{
    return m_contentLayout;
}

void FluentCard::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

bool FluentCard::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_header && m_collapsible && event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            setCollapsed(!m_collapsed);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void FluentCard::applyTheme()
{
    const QString next = Theme::cardStyle(ThemeManager::instance().colors());
    if (styleSheet() != next) {
        setStyleSheet(next);
    }
    updateHeaderUi();
}

void FluentCard::ensureCollapsibleUi()
{
    if (m_header && m_content && m_contentLayout) {
        return;
    }

    auto *root = qobject_cast<QVBoxLayout *>(layout());
    if (!root) {
        root = new QVBoxLayout(this);
        root->setContentsMargins(16, 14, 16, 14);
        root->setSpacing(10);
        setLayout(root);
    }

    // If the user already placed widgets/layouts on this card, move them into the collapsible content container.
    if (!m_content && root->count() > 0) {
        m_content = new QWidget(this);
        m_content->setObjectName("FluentCardContent");

        m_contentLayout = new QVBoxLayout(m_content);
        m_contentLayout->setContentsMargins(0, 0, 0, 0);
        m_contentLayout->setSpacing(8);

        QList<QLayoutItem *> items;
        items.reserve(root->count());
        while (root->count() > 0) {
            items.append(root->takeAt(0));
        }

        for (QLayoutItem *it : items) {
            if (!it) {
                continue;
            }
            if (auto *w = it->widget()) {
                w->setParent(m_content);
                m_contentLayout->addWidget(w);
                delete it;
                continue;
            }
            if (auto *l = it->layout()) {
                m_contentLayout->addLayout(l);
                delete it;
                continue;
            }
            m_contentLayout->addItem(it);
        }
    }

    if (!m_header) {
        m_header = new QWidget(this);
        m_header->setObjectName("FluentCardHeader");
        m_header->setCursor(Qt::PointingHandCursor);
        m_header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_header->installEventFilter(this);

        auto *hl = new QHBoxLayout(m_header);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(8);

        m_titleLabel = new FluentLabel(m_header);
        m_titleLabel->setObjectName("FluentCardTitle");
        m_titleLabel->setStyleSheet("font-size: 13px; font-weight: 600;");

        m_chevronButton = new FluentToolButton(m_header);
        m_chevronButton->setObjectName("FluentCardChevron");
        m_chevronButton->setFixedSize(28, 28);
        connect(m_chevronButton, &QAbstractButton::clicked, this, [this] {
            if (m_collapsible) {
                setCollapsed(!m_collapsed);
            }
        });

        hl->addWidget(m_titleLabel, 1);
        hl->addWidget(m_chevronButton, 0, Qt::AlignRight | Qt::AlignVCenter);

        root->insertWidget(0, m_header);
    }

    if (!m_content) {
        m_content = new QWidget(this);
        m_content->setObjectName("FluentCardContent");

        m_contentLayout = new QVBoxLayout(m_content);
        m_contentLayout->setContentsMargins(0, 0, 0, 0);
        m_contentLayout->setSpacing(8);

        root->addWidget(m_content, 1);
    } else if (root->indexOf(m_content) < 0) {
        root->addWidget(m_content, 1);
    }

    if (!m_collapseAnim) {
        m_collapseAnim = new QPropertyAnimation(m_content, "maximumHeight", this);
        m_collapseAnim->setDuration(160);
        m_collapseAnim->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_collapseAnim, &QPropertyAnimation::finished, this, [this] {
            if (!m_content) {
                return;
            }
            setProperty(kFlowDisableAnimationProperty, false);
            if (m_collapsed) {
                m_content->setVisible(false);
                m_content->setMaximumHeight(0);
            } else {
                m_content->setVisible(true);
                m_content->setMaximumHeight(QWIDGETSIZE_MAX);
            }
            updateGeometry();
        });
    }
}

void FluentCard::updateHeaderUi()
{
    if (!m_collapsible || !m_header) {
        return;
    }

    if (m_titleLabel) {
        m_titleLabel->setText(m_title);
    }

    if (m_chevronButton) {
        const auto colors = ThemeManager::instance().colors();
        m_chevronButton->setIcon(makeChevronIcon(m_collapsed, colors.subText));
        m_chevronButton->setIconSize(QSize(16, 16));
    }
}

void FluentCard::applyCollapsedState(bool animated)
{
    if (!m_collapsible) {
        return;
    }
    ensureCollapsibleUi();
    if (!m_content) {
        return;
    }

    if (!animated || !m_collapseAnim) {
        setProperty(kFlowDisableAnimationProperty, false);
        if (m_collapseAnim) {
            m_collapseAnim->stop();
        }
        if (m_collapsed) {
            m_content->setVisible(false);
            m_content->setMaximumHeight(0);
        } else {
            m_content->setVisible(true);
            m_content->setMaximumHeight(QWIDGETSIZE_MAX);
        }
        updateGeometry();
        return;
    }

    m_collapseAnim->stop();
    m_content->setVisible(true);
    setProperty(kFlowDisableAnimationProperty, true);

    const int start = (m_content->maximumHeight() == QWIDGETSIZE_MAX) ? m_content->height() : m_content->maximumHeight();
    const int end = m_collapsed
        ? 0
        : qMax(0, m_content->layout() ? m_content->layout()->sizeHint().height() : m_content->sizeHint().height());

    m_collapseAnim->setStartValue(start);
    m_collapseAnim->setEndValue(end);
    m_collapseAnim->start();
}

} // namespace Fluent
