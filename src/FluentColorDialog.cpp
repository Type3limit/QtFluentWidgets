#include "Fluent/FluentColorDialog.h"

#include "Fluent/FluentButton.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentFlowLayout.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentSlider.h"
#include "Fluent/FluentSpinBox.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentToolButton.h"

#include "colorPicker/EyeDropper.h"
#include "colorPicker/ColorPickerWidgets.h"

#include <QApplication>
#include <QCursor>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QScreen>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QEnterEvent>
#endif

namespace Fluent {

namespace {

static QPointF mouseLocalPosF(QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->position();
#else
    return event->localPos();
#endif
}

static QPoint mouseGlobalPos(QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->globalPosition().toPoint();
#else
    return event->globalPos();
#endif
}

static QString toHexArgb(const QColor &c)
{
    if (!c.isValid()) {
        return QString();
    }
    const auto a = QString::number(c.alpha(), 16).rightJustified(2, QLatin1Char('0')).toUpper();
    const auto r = QString::number(c.red(), 16).rightJustified(2, QLatin1Char('0')).toUpper();
    const auto g = QString::number(c.green(), 16).rightJustified(2, QLatin1Char('0')).toUpper();
    const auto b = QString::number(c.blue(), 16).rightJustified(2, QLatin1Char('0')).toUpper();
    return QStringLiteral("#%1%2%3%4").arg(a, r, g, b);
}

static QColor parseHexColor(QString s)
{
    s = s.trimmed();
    if (s.startsWith('#')) {
        s.remove(0, 1);
    }
    if (s.size() == 6) {
        const QColor c(QStringLiteral("#%1").arg(s));
        return c;
    }
    if (s.size() == 8) {
        bool ok = false;
        const int a = s.mid(0, 2).toInt(&ok, 16);
        if (!ok)
            return QColor();
        const int r = s.mid(2, 2).toInt(&ok, 16);
        if (!ok)
            return QColor();
        const int g = s.mid(4, 2).toInt(&ok, 16);
        if (!ok)
            return QColor();
        const int b = s.mid(6, 2).toInt(&ok, 16);
        if (!ok)
            return QColor();
        return QColor(r, g, b, a);
    }
    return QColor();
}

static QVector<QColor> commonColors()
{
    return {
        QColor("#0067C0"),
        QColor("#0F7B0F"),
        QColor("#6B4EFF"),
        QColor("#D13438"),
        QColor("#FF8C00"),
        QColor("#FFD700"),
        QColor("#00B7C3"),
        QColor("#107C10"),
        QColor("#5C2D91"),
        QColor("#393939"),
        QColor("#767676"),
        QColor("#FFFFFF"),
        QColor("#000000"),
    };
}
} // namespace

bool FluentColorDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_dragHandle || watched != m_dragHandle.data()) {
        return QDialog::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragOffset = mouseGlobalPos(me) - frameGeometry().topLeft();
            return true;
        }
        break;
    }
    case QEvent::MouseMove: {
        if (!m_dragging) {
            break;
        }
        auto *me = static_cast<QMouseEvent *>(event);
        move(mouseGlobalPos(me) - m_dragOffset);
        return true;
    }
    case QEvent::MouseButtonRelease: {
        auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                m_dragging = false;
                return true;
            }
            break;
    }
    default:
        break;
    }

    return QDialog::eventFilter(watched, event);
}

FluentColorDialog::FluentColorDialog(const QColor &initial, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr(u8"选择颜色"));
    setModal(false);
    setSizeGripEnabled(false);
    // Use Tool window and implement popup behavior ourselves.
    // Qt::Popup combined with translucent/frameless windows is crash-prone on some Windows setups
    // when showing/hiding or when activation changes (esp. with eyedropper overlays).
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    // Keep child widgets styled, but make the native window background fully transparent.
    setStyleSheet(Theme::dialogStyle(ThemeManager::instance().colors()) + QStringLiteral("QDialog{background: transparent; border: none; border-radius: 0px;}"));

    m_border.syncFromTheme();

    m_reset = initial.isValid() ? initial : QColor("#0067C0");
    m_selected = m_reset;
    int h = 0, s = 0, v = 0, a = m_selected.alpha();
    m_selected.getHsv(&h, &s, &v);
    if (h < 0) {
        h = 0;
    }

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 12);
    root->setSpacing(8);

    // Header (draggable)
    const auto wm = Style::windowMetrics();

    auto *header = new QWidget(this);
    header->setObjectName(QStringLiteral("fluentColorHeader"));
    header->setFixedHeight(wm.titleBarHeight);
    auto *hl = new QHBoxLayout(header);
    hl->setContentsMargins(wm.titleBarPaddingX, wm.titleBarPaddingY, wm.titleBarPaddingX, wm.titleBarPaddingY);
    hl->setSpacing(8);

    auto *title = new QLabel(windowTitle(), header);
    title->setObjectName("FluentColorDialogTitle");
    title->setStyleSheet("font-size: 13px; font-weight: 600;");

    auto *closeBtn = new FluentToolButton(header);
    closeBtn->setFixedSize(wm.windowButtonWidth, wm.windowButtonHeight);
    closeBtn->setAutoRaise(true);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    // Reuse FluentDialog glyph painting convention.
    closeBtn->setProperty("fluentWindowGlyph", 3);
    closeBtn->setToolTip(tr("Close"));

    connect(closeBtn, &QAbstractButton::clicked, this, &QDialog::reject);

    hl->addWidget(title);
    hl->addStretch(1);
    hl->addWidget(closeBtn);
    root->addWidget(header);

    m_dragHandle = header;
    header->installEventFilter(this);

    auto *contentRow = new QHBoxLayout();
    contentRow->setContentsMargins(0, 0, 0, 0);
    contentRow->setSpacing(10);
    root->addLayout(contentRow);

    // Left: SV panel + alpha + hex
    auto *leftCol = new QVBoxLayout();
    leftCol->setContentsMargins(0, 0, 0, 0);
    leftCol->setSpacing(10);

    auto *sv = new ColorPicker::SvPanel(this);
    sv->setHue(h);
    sv->setSv(s, v);

    m_svPanel = sv;

    // SV + Hue side-by-side (more fluent)
    auto *svRow = new QHBoxLayout();
    svRow->setContentsMargins(0, 0, 0, 0);
    svRow->setSpacing(10);
    auto *hueStrip = new ColorPicker::HueStrip(this);
    hueStrip->setFixedHeight(sv->height());
    hueStrip->setValue(h);
    m_hueStrip = hueStrip;
    svRow->addWidget(sv);
    svRow->addWidget(hueStrip);
    leftCol->addLayout(svRow);

    auto *alphaRow = new QHBoxLayout();
    alphaRow->setContentsMargins(0, 0, 0, 0);
    alphaRow->setSpacing(8);
    auto *alphaLbl = new FluentLabel(QStringLiteral("Alpha"));
    alphaLbl->setStyleSheet("font-size: 12px; opacity: 0.9;");
    auto *alphaStrip = new ColorPicker::AlphaStrip(this);
    alphaStrip->setBaseColor(QColor(m_selected.red(), m_selected.green(), m_selected.blue()));
    alphaStrip->setValue(a);
    alphaStrip->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_alphaStrip = alphaStrip;
    auto *alphaSpin = new FluentSpinBox();
    alphaSpin->setRange(0, 255);
    alphaSpin->setValue(a);
    alphaSpin->setFixedWidth(84);
    m_alphaSpin = alphaSpin;
    alphaRow->addWidget(alphaLbl);
    alphaRow->addWidget(alphaStrip, 1);
    alphaRow->addWidget(alphaSpin);
    leftCol->addLayout(alphaRow);

    auto *hexRow = new QHBoxLayout();
    hexRow->setContentsMargins(0, 0, 0, 0);
    hexRow->setSpacing(8);
    auto *hexLbl = new FluentLabel(QStringLiteral("Hex"));
    hexLbl->setStyleSheet("font-size: 12px; opacity: 0.9;");
    auto *hexEdit = new FluentLineEdit();
    hexEdit->setPlaceholderText(QStringLiteral("#RRGGBB 或 #AARRGGBB"));
    hexEdit->setText(toHexArgb(m_selected));
    m_hexEdit = hexEdit;
    hexRow->addWidget(hexLbl);
    hexRow->addWidget(hexEdit, 1);
    leftCol->addLayout(hexRow);

    contentRow->addLayout(leftCol, 0);

    // Right: preview + eyedropper + swatches
    auto *rightColW = new QWidget(this);
    auto *rightCol = new QVBoxLayout(rightColW);
    rightCol->setContentsMargins(0, 0, 0, 0);
    rightCol->setSpacing(10);

    auto *previewCard = new FluentCard();
    previewCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *pl = new QVBoxLayout(previewCard);
    pl->setContentsMargins(10, 10, 10, 10);
    pl->setSpacing(6);
    auto *previewTitle = new FluentLabel(QStringLiteral("预览"));
    previewTitle->setStyleSheet("font-size: 12px; font-weight: 650; opacity: 0.95;");
    auto *previewSwatch = new ColorPicker::PreviewSwatch(previewCard);
    pl->addWidget(previewTitle);
    pl->addWidget(previewSwatch);
    m_previewSwatch = previewSwatch;
    rightCol->addWidget(previewCard);

    auto *toolRow = new QHBoxLayout();
    toolRow->setContentsMargins(0, 0, 0, 0);
    toolRow->setSpacing(8);
    auto *eyeBtn = new FluentButton(QStringLiteral("吸管取色"));
    auto *resetBtn = new FluentButton(QStringLiteral("重置"));
    toolRow->addWidget(eyeBtn);
    toolRow->addWidget(resetBtn);
    rightCol->addLayout(toolRow);

    QVector<QPair<ColorPicker::ColorSwatchButton *, QColor>> swatchButtons;

    auto makeSwatchSection = [&](const QString &titleText, const QVector<QColor> &colors, FluentFlowLayout **outFlow) {
        auto *lab = new FluentLabel(titleText);
        lab->setStyleSheet("font-size: 12px; font-weight: 650; opacity: 0.95;");
        rightCol->addWidget(lab);
        auto *host = new QWidget(rightColW);
        host->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto *fl = new FluentFlowLayout(host, 0, 6, 6);
        fl->setUniformItemWidthEnabled(false);
        host->setLayout(fl);
        for (const auto &c : colors) {
            auto *swb = new ColorPicker::ColorSwatchButton(c, host);
            swatchButtons.push_back(qMakePair(swb, c));
            fl->addWidget(swb);
        }
        rightCol->addWidget(host);
        if (outFlow) {
            *outFlow = fl;
        }
        return host;
    };

    // Recent colors
    QVector<QColor> recent;
    {
        QSettings s;
        const QStringList lst = s.value(QStringLiteral("QtFluent/FluentColorDialog/recent")).toStringList();
        for (const auto &it : lst) {
            const QColor c = parseHexColor(it);
            if (c.isValid()) {
                recent.push_back(c);
            }
        }
    }

    if (recent.isEmpty()) {
        recent = {ThemeManager::instance().colors().accent, ThemeManager::instance().colors().surface};
    }

    FluentFlowLayout *recentFlow = nullptr;
    auto *recentHost = makeSwatchSection(QStringLiteral("最近"), recent, &recentFlow);
    Q_UNUSED(recentHost)

    makeSwatchSection(QStringLiteral("常用"), commonColors(), nullptr);

    contentRow->addWidget(rightColW);

    // Bottom buttons
    auto *divider = new QWidget(this);
    divider->setFixedHeight(1);
    divider->setStyleSheet(QString("background: %1;").arg(ThemeManager::instance().colors().border.name()));
    root->addWidget(divider);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->setContentsMargins(0, 0, 0, 0);
    buttonRow->setSpacing(8);
    buttonRow->addStretch(1);
    auto *cancel = new FluentButton(tr(u8"取消"));
    auto *ok = new FluentButton(tr(u8"确定"));
    ok->setPrimary(true);
    buttonRow->addWidget(cancel);
    buttonRow->addWidget(ok);
    root->addLayout(buttonRow);

    // Wiring
    auto *svPtr = sv;

    auto applyFromUi = [this](bool emitSignal) { applyColorFromUi(emitSignal); };
    auto applyFromColor = [this](bool emitSignal) { applyUiFromColor(emitSignal); };

    for (const auto &it : swatchButtons) {
        ColorPicker::ColorSwatchButton *btn = it.first;
        const QColor col = it.second;
        QObject::connect(btn, &ColorPicker::ColorSwatchButton::clicked, this, [this, col, applyFromColor]() {
            setSelectedColorInternal(col, true);
            applyFromColor(false);
        });
    }
    QObject::connect(cancel, &QPushButton::clicked, this, [this]() { reject(); });
    QObject::connect(ok, &QPushButton::clicked, this, [this]() {
        // Save to recent
        QSettings s;
        QStringList lst = s.value(QStringLiteral("QtFluent/FluentColorDialog/recent")).toStringList();
        const QString cur = toHexArgb(m_selected);
        lst.removeAll(cur);
        lst.prepend(cur);
        while (lst.size() > 12) {
            lst.removeLast();
        }
        s.setValue(QStringLiteral("QtFluent/FluentColorDialog/recent"), lst);
        accept();
    });

    QObject::connect(hueStrip, &ColorPicker::HueStrip::valueChanged, this, [svPtr, applyFromUi](int v) {
        if (auto *svp = qobject_cast<ColorPicker::SvPanel *>(svPtr)) {
            svp->setHue(v);
        }
        // While dragging, update internal preview but avoid expensive external observers.
        applyFromUi(false);
    });
    QObject::connect(hueStrip, &ColorPicker::HueStrip::valueChangeFinished, this, [svPtr, applyFromUi](int v) {
        if (auto *svp = qobject_cast<ColorPicker::SvPanel *>(svPtr)) {
            svp->setHue(v);
        }
        applyFromUi(true);
    });

    QObject::connect(sv, &ColorPicker::SvPanel::svChanged, this, [applyFromUi](int, int) { applyFromUi(false); });
    QObject::connect(sv, &ColorPicker::SvPanel::svChangeFinished, this, [applyFromUi](int, int) { applyFromUi(true); });

    QObject::connect(alphaStrip, &ColorPicker::AlphaStrip::valueChanged, this, [alphaSpin, applyFromUi](int v) {
        if (auto *sp = qobject_cast<FluentSpinBox *>(alphaSpin)) {
            if (sp->value() != v) {
                const bool prev = sp->blockSignals(true);
                sp->setValue(v);
                sp->blockSignals(prev);
            }
        }
        applyFromUi(false);
    });
    QObject::connect(alphaStrip, &ColorPicker::AlphaStrip::valueChangeFinished, this, [alphaSpin, applyFromUi](int v) {
        if (auto *sp = qobject_cast<FluentSpinBox *>(alphaSpin)) {
            if (sp->value() != v) {
                const bool prev = sp->blockSignals(true);
                sp->setValue(v);
                sp->blockSignals(prev);
            }
        }
        applyFromUi(true);
    });
    QObject::connect(alphaSpin, qOverload<int>(&QSpinBox::valueChanged), this, [alphaStrip, applyFromUi](int v) {
        if (auto *as = qobject_cast<ColorPicker::AlphaStrip *>(alphaStrip)) {
            if (as->value() != v) {
                as->setValue(v);
            }
        }
        applyFromUi(true);
    });

    QObject::connect(hexEdit, &QLineEdit::editingFinished, this, [this, applyFromColor]() {
        if (auto *le = qobject_cast<FluentLineEdit *>(m_hexEdit)) {
            const QColor c = parseHexColor(le->text());
            if (c.isValid()) {
                setSelectedColorInternal(c, true);
                applyFromColor(false);
            } else {
                le->setText(toHexArgb(m_selected));
            }
        }
    });

    QObject::connect(resetBtn, &QPushButton::clicked, this, [this, applyFromColor]() {
        setSelectedColorInternal(m_reset, true);
        applyFromColor(false);
    });

    QObject::connect(eyeBtn, &QPushButton::clicked, this, [this, applyFromColor]() {
        const QColor before = m_selected;

        // Prevent popup-like auto-close while eyedropper (multi-window) is active.
        m_suppressAutoClose = true;

        auto restoreDialog = [this]() {
            m_suppressAutoClose = false;
            raise();
            activateWindow();
        };

        auto *ctl = new Fluent::ColorPicker::EyeDropperController(this);
        // Always restore even if controller closes unexpectedly.
        QObject::connect(ctl, &QObject::destroyed, this, [restoreDialog]() { restoreDialog(); });

        QObject::connect(ctl, &Fluent::ColorPicker::EyeDropperController::hovered, this, [this](const QColor &c) {
            if (c.isValid()) {
                QColor preview = m_selected;
                preview.setRgb(c.red(), c.green(), c.blue(), m_selected.alpha());
                if (auto *ps = qobject_cast<ColorPicker::PreviewSwatch *>(m_previewSwatch)) {
                    ps->setColor(preview);
                }
            }
        });
        QObject::connect(ctl, &Fluent::ColorPicker::EyeDropperController::picked, this, [this, applyFromColor, restoreDialog, ctl](const QColor &c) {
            if (c.isValid()) {
                QColor picked = m_selected;
                picked.setRgb(c.red(), c.green(), c.blue(), m_selected.alpha());
                setSelectedColorInternal(picked, true);
                applyFromColor(false);
            }
            QTimer::singleShot(0, this, [restoreDialog]() { restoreDialog(); });
            ctl->deleteLater();
        });
        QObject::connect(ctl, &Fluent::ColorPicker::EyeDropperController::canceled, this, [this, before, applyFromColor, restoreDialog, ctl]() {
            m_selected = before;
            applyFromColor(false);
            QTimer::singleShot(0, this, [restoreDialog]() { restoreDialog(); });
            ctl->deleteLater();
        });
    });

    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this, divider]() {
        if (isVisible()) {
            m_border.onThemeChanged();
        } else {
            m_border.syncFromTheme();
        }

        const QString next = Theme::dialogStyle(ThemeManager::instance().colors())
            + QStringLiteral("QDialog{background: transparent; border: none; border-radius: 0px;}");
        if (styleSheet() != next) {
            setStyleSheet(next);
        }
        const QString dividerNext = QString("background: %1;").arg(ThemeManager::instance().colors().border.name());
        if (divider->styleSheet() != dividerNext) {
            divider->setStyleSheet(dividerNext);
        }
        applyUiFromColor(false);
    });

    // Initial paint/update
    applyUiFromColor(false);

    // Size
    setMinimumWidth(640);
    setMinimumHeight(520);

    // Initial popup size/position
    resize(640, 500);
    if (parentWidget()) {
        const QPoint c = parentWidget()->mapToGlobal(parentWidget()->rect().center());
        move(c - QPoint(width() / 2, height() / 2));
    } else {
        move(QCursor::pos() - QPoint(width() / 2, height() / 2));
    }
}

void FluentColorDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    m_border.playInitialTraceOnce(0);
}

bool FluentColorDialog::event(QEvent *event)
{
    // Popup-like behavior: close on deactivation unless suppressed (e.g. during eyedropper).
    if (event->type() == QEvent::WindowDeactivate) {
        // Only auto-reject while the dialog is still "active" (exec running) and no explicit
        // accept/reject has happened yet. Otherwise we may override an OK click.
        if (!m_suppressAutoClose && isVisible() && result() == 0) {
            reject();
            return true;
        }
    }
    return QDialog::event(event);
}

void FluentColorDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    if (!p.isActive()) {
        return;
    }

    const auto &tc = ThemeManager::instance().colors();

    FluentFrameSpec frame;
    frame.radius = 10.0;
    frame.maximized = false;
    // Keep previous visual inset (1px) to avoid edge clipping on some platforms.
    frame.borderInset = 1.0;

    m_border.applyToFrameSpec(frame, tc);

    paintFluentFrame(p, rect(), tc, frame);
}

QColor FluentColorDialog::selectedColor() const
{
    return m_selected;
}

void FluentColorDialog::setCurrentColor(const QColor &color)
{
    setSelectedColorInternal(color, true);
    applyUiFromColor(false);
}

QColor FluentColorDialog::currentColor() const
{
    return m_selected;
}

void FluentColorDialog::setResetColor(const QColor &color)
{
    if (color.isValid()) {
        m_reset = color;
    }
}

QColor FluentColorDialog::resetColor() const
{
    return m_reset;
}

void FluentColorDialog::setSelectedColorInternal(const QColor &color, bool emitSignal)
{
    if (!color.isValid()) {
        return;
    }
    const QColor c = color;
    if (m_selected == c) {
        return;
    }
    m_selected = c;
    if (emitSignal) {
        emit colorChanged(m_selected);
    }
}

void FluentColorDialog::applyUiFromColor(bool emitSignal)
{
    if (m_uiUpdating) {
        return;
    }
    m_uiUpdating = true;

    int hh = 0, ss = 0, vv = 0, aa = 0;
    m_selected.getHsv(&hh, &ss, &vv, &aa);
    if (hh < 0) {
        hh = 0;
    }

    if (auto *hs = qobject_cast<ColorPicker::HueStrip *>(m_hueStrip)) {
        hs->setValue(hh);
    }
    if (auto *svp = qobject_cast<ColorPicker::SvPanel *>(m_svPanel)) {
        svp->setHue(hh);
        svp->setSv(ss, vv);
    }
    if (auto *as = qobject_cast<ColorPicker::AlphaStrip *>(m_alphaStrip)) {
        as->setBaseColor(QColor(m_selected.red(), m_selected.green(), m_selected.blue()));
        as->setValue(aa);
    }
    if (auto *sp = qobject_cast<QSpinBox *>(m_alphaSpin)) {
        sp->setValue(aa);
    }
    if (auto *le = qobject_cast<QLineEdit *>(m_hexEdit)) {
        le->setText(toHexArgb(m_selected));
    }
    if (auto *ps = qobject_cast<ColorPicker::PreviewSwatch *>(m_previewSwatch)) {
        ps->setColor(m_selected);
    }

    m_uiUpdating = false;

    // Optionally re-emit to observers if requested.
    if (emitSignal) {
        emit colorChanged(m_selected);
    }
}

void FluentColorDialog::applyColorFromUi(bool emitSignal)
{
    if (m_uiUpdating) {
        return;
    }
    auto *svp = qobject_cast<ColorPicker::SvPanel *>(m_svPanel);
    auto *hs = qobject_cast<ColorPicker::HueStrip *>(m_hueStrip);
    auto *as = qobject_cast<ColorPicker::AlphaStrip *>(m_alphaStrip);
    if (!svp || !hs || !as) {
        return;
    }

    const QColor c = QColor::fromHsv(hs->value(), svp->s(), svp->v(), as->value());
    setSelectedColorInternal(c, emitSignal);

    if (auto *le = qobject_cast<QLineEdit *>(m_hexEdit)) {
        le->setText(toHexArgb(m_selected));
    }
    if (auto *ps = qobject_cast<ColorPicker::PreviewSwatch *>(m_previewSwatch)) {
        ps->setColor(m_selected);
    }
    as->setBaseColor(QColor(m_selected.red(), m_selected.green(), m_selected.blue()));
}

} // namespace Fluent
