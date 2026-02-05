#include "Fluent/FluentColorPicker.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentColorDialog.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QIcon>
#include <QPixmap>

namespace Fluent {

FluentColorPicker::FluentColorPicker(QWidget *parent)
    : QWidget(parent)
    , m_preview(new QLineEdit(this))
    , m_button(new QPushButton(tr("选择颜色"), this))
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_preview->setReadOnly(true);
    m_preview->setMinimumWidth(120);

    layout->addWidget(m_preview, 1);
    layout->addWidget(m_button);

    connect(m_button, &QPushButton::clicked, this, &FluentColorPicker::openDialog);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentColorPicker::applyTheme);

    setColor(QColor("#0067C0"));
    applyTheme();
}

QColor FluentColorPicker::color() const
{
    return m_color;
}

void FluentColorPicker::setColor(const QColor &color)
{
    if (m_color == color) {
        return;
    }

    m_color = color;
    updatePreview();
    emit colorChanged(m_color);
}

void FluentColorPicker::openDialog()
{
    const QColor before = m_color;

    FluentColorDialog dialog(m_color, this);
    QObject::connect(&dialog, &FluentColorDialog::colorChanged, this, [this](const QColor &c) {
        if (c.isValid()) {
            setColor(c);
        }
    });

    const int rc = dialog.exec();
    if (rc == QDialog::Accepted) {
        const QColor selected = dialog.selectedColor();
        if (selected.isValid()) {
            setColor(selected);
        }
        return;
    }

    // Revert on cancel/auto-close.
    setColor(before);
}

void FluentColorPicker::applyTheme()
{
    const auto &colors = ThemeManager::instance().colors();
    {
        const QString next = Theme::lineEditStyle(colors);
        if (m_preview->styleSheet() != next) {
            m_preview->setStyleSheet(next);
        }
    }
    {
        const QString next = Theme::buttonStyle(colors, false);
        if (m_button->styleSheet() != next) {
            m_button->setStyleSheet(next);
        }
    }
}

void FluentColorPicker::updatePreview()
{
    for (auto *action : m_preview->actions()) {
        m_preview->removeAction(action);
    }

    m_preview->setText(m_color.name(QColor::HexRgb).toUpper());
    {
        const QString next = QString(
            "%1 QLineEdit { padding-left: 26px; } QLineEdit:disabled { padding-left: 26px; }"
        ).arg(Theme::lineEditStyle(ThemeManager::instance().colors()));
        if (m_preview->styleSheet() != next) {
            m_preview->setStyleSheet(next);
        }
    }

    QPixmap pix(16, 16);
    pix.fill(m_color);
    m_preview->addAction(QIcon(pix), QLineEdit::LeadingPosition);
}

} // namespace Fluent
