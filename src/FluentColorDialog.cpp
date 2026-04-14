#include "Fluent/FluentColorDialog.h"

#include "Fluent/FluentButton.h"
#include "Fluent/FluentAngleSelector.h"
#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentFlowLayout.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentLineEdit.h"
#include "Fluent/FluentSpinBox.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentToolButton.h"

#include "colorPicker/EyeDropper.h"
#include "colorPicker/ColorPickerWidgets.h"

#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QCursor>
#include <QFontDatabase>
#include <QGradient>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QEnterEvent>
#endif

namespace Fluent {

namespace {

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
    if (!c.isValid()) return QString();
    const auto a = QString::number(c.alpha(), 16).rightJustified(2, QLatin1Char('0')).toUpper();
    const auto r = QString::number(c.red(),   16).rightJustified(2, QLatin1Char('0')).toUpper();
    const auto g = QString::number(c.green(), 16).rightJustified(2, QLatin1Char('0')).toUpper();
    const auto b = QString::number(c.blue(),  16).rightJustified(2, QLatin1Char('0')).toUpper();
    return QStringLiteral("#%1%2%3%4").arg(a, r, g, b);
}

static QColor parseHexColor(QString s)
{
    s = s.trimmed();
    if (s.startsWith('#')) s.remove(0, 1);
    if (s.size() == 6) return QColor(QStringLiteral("#%1").arg(s));
    if (s.size() == 8) {
        bool ok = false;
        const int a = s.mid(0, 2).toInt(&ok, 16); if (!ok) return QColor();
        const int r = s.mid(2, 2).toInt(&ok, 16); if (!ok) return QColor();
        const int g = s.mid(4, 2).toInt(&ok, 16); if (!ok) return QColor();
        const int b = s.mid(6, 2).toInt(&ok, 16); if (!ok) return QColor();
        return QColor(r, g, b, a);
    }
    return QColor();
}

// Comprehensive preset palette: grays + 10 hue families × 7 shades
static QVector<QColor> presetColorPalette()
{
    QVector<QColor> result;
    // 14-step grayscale
    for (int i = 0; i < 14; ++i)
        result.push_back(QColor(255 * i / 13, 255 * i / 13, 255 * i / 13));

    struct HueRow { int hue; };
    const HueRow rows[] = { {354},{15},{40},{65},{120},{170},{205},{255},{290},{330} };
    for (const auto &row : rows) {
        for (int i = 0; i < 7; ++i) {
            const qreal t = i / 6.0;
            const int s = qRound(50  + t * 200);   // 50 → 250
            const int v = qRound(255 - t * 100);   // 255 → 155
            result.push_back(QColor::fromHsv(row.hue, s, v));
        }
    }
    return result;
}


} // namespace

// ────────────────────────────────────────────────────────────────────────────
// eventFilter (drag handle)
// ────────────────────────────────────────────────────────────────────────────
bool FluentColorDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_dragHandle || watched != m_dragHandle.data())
        return QDialog::eventFilter(watched, event);

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            m_dragging   = true;
            m_dragOffset = mouseGlobalPos(me) - frameGeometry().topLeft();
            return true;
        }
        break;
    }
    case QEvent::MouseMove: {
        if (!m_dragging) break;
        auto *me = static_cast<QMouseEvent *>(event);
        move(mouseGlobalPos(me) - m_dragOffset);
        return true;
    }
    case QEvent::MouseButtonRelease: {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) { m_dragging = false; return true; }
        break;
    }
    default: break;
    }
    return QDialog::eventFilter(watched, event);
}

// ────────────────────────────────────────────────────────────────────────────
// Constructor
// ────────────────────────────────────────────────────────────────────────────
FluentColorDialog::FluentColorDialog(const QColor &initial, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr(u8"选择颜色"));
    setModal(false);
    setSizeGripEnabled(false);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet(Theme::dialogStyle(ThemeManager::instance().colors())
        + QStringLiteral("QDialog{background: transparent; border: none; border-radius: 0px;}"));

    m_border.syncFromTheme();

    m_reset    = initial.isValid() ? initial : QColor("#0067C0");
    m_selected = m_reset;
    int h = 0, initS = 0, initV = 0;
    m_selected.getHsv(&h, &initS, &initV);
    if (h < 0) h = 0;
    const int a = m_selected.alpha();

    // ── Root layout ────────────────────────────────────────────────────────
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 8, 10, 10);
    root->setSpacing(6);

    // ── Title bar (draggable) ─────────────────────────────────────────────
    const auto wm = Style::windowMetrics();
    auto *header = new QWidget(this);
    header->setObjectName(QStringLiteral("fluentColorHeader"));
    header->setFixedHeight(wm.titleBarHeight);
    auto *hl = new QHBoxLayout(header);
    hl->setContentsMargins(wm.titleBarPaddingX, wm.titleBarPaddingY,
                           wm.titleBarPaddingX, wm.titleBarPaddingY);
    hl->setSpacing(8);

    auto *title = new QLabel(windowTitle(), header);
    title->setObjectName("FluentColorDialogTitle");
    title->setStyleSheet("font-size: 13px; font-weight: 600;");

    // Mode selector ComboBox (replaces the old three QPushButtons)
    auto *modeCombo = new FluentComboBox(header);
    modeCombo->addItem(tr(u8"纯色"),    static_cast<int>(ColorPickerMode::Solid));
    modeCombo->addItem(tr(u8"线性渐变"), static_cast<int>(ColorPickerMode::LinearGradient));
    modeCombo->addItem(tr(u8"径向渐变"), static_cast<int>(ColorPickerMode::RadialGradient));
    modeCombo->setCurrentIndex(0);
    modeCombo->setFixedWidth(148);
    modeCombo->setVisible(m_gradientEnabled);
    m_modeCombo = modeCombo;

    auto *closeBtn = new FluentToolButton(header);
    closeBtn->setFixedSize(wm.windowButtonWidth, wm.windowButtonHeight);
    closeBtn->setAutoRaise(true);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    closeBtn->setProperty("fluentWindowGlyph", 3);
    closeBtn->setToolTip(tr("Close"));
    connect(closeBtn, &QAbstractButton::clicked, this, &QDialog::reject);

    hl->addWidget(title);
    hl->addStretch(1);
    hl->addWidget(modeCombo);
    hl->addWidget(closeBtn);
    root->addWidget(header);

    m_dragHandle = header;
    header->installEventFilter(this);

    // ── Main content (single-column VBox) ─────────────────────────────────
    auto *content = new QWidget(this);
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *col = new QVBoxLayout(content);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(8);
    root->addWidget(content, 1);

    // SV panel – full width
    auto *sv = new ColorPicker::SvPanel(content);
    sv->setHue(h);
    sv->setSv(initS, initV);
    m_svPanel = sv;
    col->addWidget(sv);

    // Hue row: [hue strip (horizontal)] [eyedropper]
    auto *eyeDropBtn = new ColorPicker::EyeDropperButton(content);
    {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(4);

        auto *hueStrip = new ColorPicker::HueStrip(content);
        hueStrip->setOrientation(Qt::Horizontal);
        hueStrip->setValue(h);
        m_hueStrip = hueStrip;

        row->addWidget(hueStrip, 1);
        row->addWidget(eyeDropBtn);   // eyedropper on the RIGHT (same side as alpha spin)
        col->addLayout(row);
    }

    // Alpha row: [alpha strip] [spin]  — unchanged, already on the right
    {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(4);

        auto *alphaStrip = new ColorPicker::AlphaStrip(content);
        alphaStrip->setBaseColor(QColor(m_selected.red(), m_selected.green(), m_selected.blue()));
        alphaStrip->setValue(a);
        alphaStrip->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_alphaStrip = alphaStrip;

        auto *alphaSpin = new FluentSpinBox(content);
        alphaSpin->setRange(0, 255);
        alphaSpin->setValue(a);
        alphaSpin->setFixedWidth(72);
        m_alphaSpin = alphaSpin;

        row->addWidget(alphaStrip, 1);
        row->addWidget(alphaSpin);
        col->addLayout(row);
    }

    // ── Thin separator — visually groups picker (SV/hue/alpha) from info ──
    {
        auto *sep = new QWidget(content);
        sep->setFixedHeight(1);
        sep->setStyleSheet(QStringLiteral("background: %1;").arg(
            ThemeManager::instance().colors().border.name()));
        col->addWidget(sep);
    }

    // ── Combined info row: [■ preview] [# hex edit] [Reset] ─────────────
    // Merges the old separate "preview" row and "hex" row into one compact line.
    auto *resetBtn = new FluentButton(tr(u8"重置"), content);
    resetBtn->setFixedWidth(72);
    {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(4);

        // Small colour swatch (replaces the full-width preview row)
        auto *preview = new ColorPicker::PreviewSwatch(content);
        preview->setFixedSize(44, 34);
        preview->setColor(m_selected);
        m_previewSwatch = preview;

        auto *hashLbl = new FluentLabel(QStringLiteral("#"), content);
        hashLbl->setStyleSheet("font-size: 12px;");
        hashLbl->setFixedWidth(14);

        auto *hexEdit = new FluentLineEdit(content);
        hexEdit->setPlaceholderText(QStringLiteral("RRGGBB / AARRGGBB"));
        hexEdit->setText(toHexArgb(m_selected).mid(1));
        m_hexEdit = hexEdit;

        row->addWidget(preview);
        row->addWidget(hashLbl);
        row->addWidget(hexEdit, 1);
        row->addWidget(resetBtn);
        col->addLayout(row);
    }

    // Gradient container (hidden in solid mode)
    auto *gradContainer = new QWidget(content);
    gradContainer->hide();
    m_gradientContainer = gradContainer;
    auto *gradVBox = new QVBoxLayout(gradContainer);
    gradVBox->setContentsMargins(0, 0, 0, 0);
    gradVBox->setSpacing(4);

    auto *angleEditor = new FluentAngleSelector(gradContainer);
    angleEditor->hide();
    angleEditor->setRange(0, 359);
    angleEditor->setWrapping(true);
    angleEditor->setValue(m_gradientAngle);
    m_angleEditor = angleEditor;
    gradVBox->addWidget(angleEditor);

    auto *gradBar = new ColorPicker::GradientStopBar(gradContainer);
    m_gradientStopBar = gradBar;
    gradVBox->addWidget(gradBar);

    auto *gradHint = new FluentLabel(tr(u8"单击渐变条添加色标，右键删除"), gradContainer);
    gradHint->setStyleSheet("font-size: 11px; opacity: 0.7;");
    gradVBox->addWidget(gradHint);

    col->addWidget(gradContainer);

    // Mid divider
    {
        auto *div = new QWidget(content);
        div->setFixedHeight(1);
        div->setStyleSheet(QStringLiteral("background: %1;").arg(
            ThemeManager::instance().colors().border.name()));
        col->addWidget(div);
    }


    // ── Swatch sections ────────────────────────────────────────────────────
    QVector<QPair<ColorPicker::ColorSwatchButton *, QColor>> swatchButtons;

    auto makeSwatchSection = [&](const QString &labelText, const QVector<QColor> &colors) {
        auto *lab = new FluentLabel(labelText, content);
        lab->setStyleSheet("font-size: 12px; font-weight: 650; opacity: 0.95;");
        col->addWidget(lab);

        auto *host = new QWidget(content);
        host->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto *fl = new FluentFlowLayout(host, 0, 3, 3);
        fl->setUniformItemWidthEnabled(false);
        for (const auto &c : colors) {
            auto *swb = new ColorPicker::ColorSwatchButton(c, host);
            swatchButtons.push_back(qMakePair(swb, c));
            fl->addWidget(swb);
        }
        col->addWidget(host);
    };

    // Recent colors from settings
    QVector<QColor> recent;
    {
        QSettings s;
        const QStringList lst = s.value(QStringLiteral("QtFluent/FluentColorDialog/recent")).toStringList();
        for (const auto &it : lst) {
            const QColor c = parseHexColor(it);
            if (c.isValid()) recent.push_back(c);
        }
    }
    if (recent.isEmpty())
        recent = { ThemeManager::instance().colors().accent, ThemeManager::instance().colors().surface };

    makeSwatchSection(tr(u8"预置颜色"), presetColorPalette());
    makeSwatchSection(tr(u8"最近使用"), recent);

    // ── Bottom divider + OK/Cancel ─────────────────────────────────────────
    auto *bottomDiv = new QWidget(this);
    bottomDiv->setFixedHeight(1);
    bottomDiv->setStyleSheet(QStringLiteral("background: %1;").arg(
        ThemeManager::instance().colors().border.name()));
    root->addWidget(bottomDiv);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->setContentsMargins(0, 0, 0, 0);
    buttonRow->setSpacing(8);
    buttonRow->addStretch(1);
    auto *cancelBtn = new FluentButton(tr(u8"取消"), this);
    auto *okBtn     = new FluentButton(tr(u8"确定"), this);
    okBtn->setPrimary(true);
    buttonRow->addWidget(cancelBtn);
    buttonRow->addWidget(okBtn);
    root->addLayout(buttonRow);

    // ── Wiring ────────────────────────────────────────────────────────────
    auto *svPtr      = sv;
    auto *alphaPtr   = m_alphaStrip;
    auto *alphaSpPtr = m_alphaSpin;

    auto applyFromUi    = [this](bool sig) { applyColorFromUi(sig); };
    auto applyFromColor = [this](bool sig) { applyUiFromColor(sig); };

    // Swatch clicks
    for (const auto &it : swatchButtons) {
        auto *btn          = it.first;
        const QColor swatchCol = it.second;
        connect(btn, &ColorPicker::ColorSwatchButton::clicked, this,
            [this, swatchCol, applyFromColor]() {
                setSelectedColorInternal(swatchCol, true);
                applyFromColor(false);
                // Also update gradient selected stop in gradient modes
                if (m_mode != ColorPickerMode::Solid) {
                    if (auto *gsb = qobject_cast<ColorPicker::GradientStopBar *>(m_gradientStopBar))
                        gsb->setSelectedColor(m_selected);
                }
            });
    }

    // Cancel / OK
    connect(cancelBtn, &QPushButton::clicked, this, [this]() { reject(); });
    connect(okBtn, &QPushButton::clicked, this, [this]() {
        QSettings s;
        QStringList lst = s.value(QStringLiteral("QtFluent/FluentColorDialog/recent")).toStringList();
        const QString cur = toHexArgb(m_selected);
        lst.removeAll(cur);
        lst.prepend(cur);
        while (lst.size() > 12) lst.removeLast();
        s.setValue(QStringLiteral("QtFluent/FluentColorDialog/recent"), lst);
        accept();
    });

    // Hue strip → SV + UI
    connect(qobject_cast<ColorPicker::HueStrip *>(m_hueStrip), &ColorPicker::HueStrip::valueChanged, this,
        [svPtr, applyFromUi](int hv) {
            if (auto *svp = qobject_cast<ColorPicker::SvPanel *>(svPtr))
                svp->setHue(hv);
            applyFromUi(false);
        });
    connect(qobject_cast<ColorPicker::HueStrip *>(m_hueStrip), &ColorPicker::HueStrip::valueChangeFinished, this,
        [svPtr, applyFromUi](int hv) {
            if (auto *svp = qobject_cast<ColorPicker::SvPanel *>(svPtr))
                svp->setHue(hv);
            applyFromUi(true);
        });

    // SV panel
    connect(sv, &ColorPicker::SvPanel::svChanged,       this, [applyFromUi](int,int) { applyFromUi(false); });
    connect(sv, &ColorPicker::SvPanel::svChangeFinished, this, [applyFromUi](int,int) { applyFromUi(true);  });

    // Alpha strip ↔ spin (keep in sync)
    connect(qobject_cast<ColorPicker::AlphaStrip *>(m_alphaStrip), &ColorPicker::AlphaStrip::valueChanged, this,
        [alphaSpPtr, applyFromUi](int val) {
            if (auto *sp = qobject_cast<FluentSpinBox *>(alphaSpPtr)) {
                if (sp->value() != val) { const bool b = sp->blockSignals(true); sp->setValue(val); sp->blockSignals(b); }
            }
            applyFromUi(false);
        });
    connect(qobject_cast<ColorPicker::AlphaStrip *>(m_alphaStrip), &ColorPicker::AlphaStrip::valueChangeFinished, this,
        [alphaSpPtr, applyFromUi](int val) {
            if (auto *sp = qobject_cast<FluentSpinBox *>(alphaSpPtr)) {
                if (sp->value() != val) { const bool b = sp->blockSignals(true); sp->setValue(val); sp->blockSignals(b); }
            }
            applyFromUi(true);
        });
    connect(qobject_cast<QSpinBox *>(m_alphaSpin), qOverload<int>(&QSpinBox::valueChanged), this,
        [alphaPtr, applyFromUi](int val) {
            if (auto *as = qobject_cast<ColorPicker::AlphaStrip *>(alphaPtr)) {
                if (as->value() != val) as->setValue(val);
            }
            applyFromUi(true);
        });

    // Hex edit
    connect(qobject_cast<QLineEdit *>(m_hexEdit), &QLineEdit::editingFinished, this, [this, applyFromColor]() {
        if (auto *le = qobject_cast<FluentLineEdit *>(m_hexEdit)) {
            // Accept both with and without '#'
            const QColor c = parseHexColor(le->text());
            if (c.isValid()) {
                setSelectedColorInternal(c, true);
                applyFromColor(false);
                if (m_mode != ColorPickerMode::Solid) {
                    if (auto *gsb = qobject_cast<ColorPicker::GradientStopBar *>(m_gradientStopBar))
                        gsb->setSelectedColor(m_selected);
                }
            } else {
                le->setText(toHexArgb(m_selected).mid(1));
            }
        }
    });

    // Reset
    connect(resetBtn, &QPushButton::clicked, this, [this, applyFromColor]() {
        setSelectedColorInternal(m_reset, true);
        applyFromColor(false);
        if (m_mode != ColorPickerMode::Solid) {
            if (auto *gsb = qobject_cast<ColorPicker::GradientStopBar *>(m_gradientStopBar))
                gsb->setSelectedColor(m_selected);
        }
    });

    // Mode ComboBox (in title bar)
    connect(modeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
        [this](int idx) {
            const ColorPickerMode modes[] = {
                ColorPickerMode::Solid, ColorPickerMode::LinearGradient, ColorPickerMode::RadialGradient
            };
            if (idx >= 0 && idx < 3) setMode(modes[idx]);
        });

    // Gradient angle editor
    connect(qobject_cast<FluentAngleSelector *>(m_angleEditor), &FluentAngleSelector::valueChanged, this,
        [this](int v) {
            m_gradientAngle = v;
            if (auto *ps = qobject_cast<ColorPicker::PreviewSwatch *>(m_previewSwatch))
                ps->setGradientAngle(v);
            if (m_mode != ColorPickerMode::Solid)
                emit gradientResultChanged(gradientResult());
        });

    // Gradient stop bar
    connect(gradBar, &ColorPicker::GradientStopBar::stopSelected, this,
        [this, applyFromColor](int /*idx*/) {
            if (auto *gsb = qobject_cast<ColorPicker::GradientStopBar *>(m_gradientStopBar)) {
                const QColor c = gsb->selectedColor();
                if (c.isValid()) {
                    setSelectedColorInternal(c, false);
                    applyFromColor(false);
                }
            }
        });
    connect(gradBar, &ColorPicker::GradientStopBar::stopsChanged, this,
        [this, gradBar]() {
            if (auto *ps = qobject_cast<ColorPicker::PreviewSwatch *>(m_previewSwatch)) {
                QGradientStops gs;
                for (const auto &s : gradBar->stops())
                    gs.append({s.pos, s.color});
                ps->setGradientPreview(gs);
            }
            emit gradientResultChanged(gradientResult());
        });

    // Eyedropper (icon button in hue row)
    connect(eyeDropBtn, &ColorPicker::EyeDropperButton::clicked, this, [this, applyFromColor]() {
        const QColor before = m_selected;
        m_suppressAutoClose = true;

        auto restoreDialog = [this]() {
            m_suppressAutoClose = false;
            raise();
            activateWindow();
        };

        auto *ctl = new Fluent::ColorPicker::EyeDropperController(this);
        connect(ctl, &QObject::destroyed, this, [restoreDialog]() { restoreDialog(); });

        connect(ctl, &ColorPicker::EyeDropperController::hovered, this,
            [this](const QColor &c) {
                if (c.isValid()) {
                    QColor preview = m_selected;
                    preview.setRgb(c.red(), c.green(), c.blue(), m_selected.alpha());
                    if (auto *ps = qobject_cast<ColorPicker::PreviewSwatch *>(m_previewSwatch)) {
                        // Temporarily show solid hover color (gradient restored on pick/cancel)
                        ps->clearGradientPreview();
                        ps->setColor(preview);
                    }
                }
            });
        connect(ctl, &ColorPicker::EyeDropperController::picked, this,
            [this, applyFromColor, restoreDialog, ctl](const QColor &c) {
                if (c.isValid()) {
                    QColor picked = m_selected;
                    picked.setRgb(c.red(), c.green(), c.blue(), m_selected.alpha());
                    setSelectedColorInternal(picked, true);
                    applyFromColor(false);
                    if (m_mode != ColorPickerMode::Solid) {
                        if (auto *gsb = qobject_cast<ColorPicker::GradientStopBar *>(m_gradientStopBar))
                            gsb->setSelectedColor(m_selected);
                    }
                }
                QTimer::singleShot(0, this, [restoreDialog]() { restoreDialog(); });
                ctl->deleteLater();
            });
        connect(ctl, &ColorPicker::EyeDropperController::canceled, this,
            [this, before, applyFromColor, restoreDialog, ctl]() {
                m_selected = before;
                applyFromColor(false);
                // Restore gradient preview if in gradient mode
                if (m_mode != ColorPickerMode::Solid) {
                    if (auto *gsb = qobject_cast<ColorPicker::GradientStopBar *>(m_gradientStopBar)) {
                        if (auto *ps = qobject_cast<ColorPicker::PreviewSwatch *>(m_previewSwatch)) {
                            QGradientStops gs;
                            for (const auto &s : gsb->stops())
                                gs.append({s.pos, s.color});
                            ps->setGradientPreview(gs);
                        }
                    }
                }
                QTimer::singleShot(0, this, [restoreDialog]() { restoreDialog(); });
                ctl->deleteLater();
            });
    });

    // Theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
        [this, bottomDiv]() {
            if (isVisible()) m_border.onThemeChanged();
            else             m_border.syncFromTheme();

            const QString next = Theme::dialogStyle(ThemeManager::instance().colors())
                + QStringLiteral("QDialog{background: transparent; border: none; border-radius: 0px;}");
            if (styleSheet() != next) setStyleSheet(next);

            const QString divStyle = QStringLiteral("background: %1;").arg(
                ThemeManager::instance().colors().border.name());
            bottomDiv->setStyleSheet(divStyle);

            applyUiFromColor(false);
        });

    // Initial sync
    applyUiFromColor(false);

    // Dialog size — compute height from content rather than hard-coding it
    setMinimumWidth(380);
    {
        // Fix width first so heightForWidth queries are meaningful
        const int w = 400;
        resize(w, 100);
        layout()->activate();
        int h = layout()->hasHeightForWidth() ? layout()->heightForWidth(w) : layout()->sizeHint().height();
        if (h < 200) h = layout()->sizeHint().height(); // fallback
        resize(w, h);
    }

    if (parentWidget()) {
        const QPoint c = parentWidget()->mapToGlobal(parentWidget()->rect().center());
        move(c - QPoint(width() / 2, height() / 2));
    } else {
        move(QCursor::pos() - QPoint(width() / 2, height() / 2));
    }
}

// ────────────────────────────────────────────────────────────────────────────
// setMode
// ────────────────────────────────────────────────────────────────────────────
void FluentColorDialog::setMode(ColorPickerMode mode)
{
    if (m_mode == mode) return;
    m_mode = mode;

    auto *gsb = qobject_cast<ColorPicker::GradientStopBar *>(m_gradientStopBar);
    auto *ps  = qobject_cast<ColorPicker::PreviewSwatch  *>(m_previewSwatch);

    const bool isGradient = (mode != ColorPickerMode::Solid);
    const bool isLinear   = (mode == ColorPickerMode::LinearGradient);
    const bool isRadial   = (mode == ColorPickerMode::RadialGradient);

    // ── Show / hide widgets ───────────────────────────────────────────────
    if (m_gradientContainer) m_gradientContainer->setVisible(isGradient);
    if (m_angleEditor)       m_angleEditor->setVisible(isLinear);   // angle only relevant for linear

    // ── Update PreviewSwatch display mode ─────────────────────────────────
    if (ps) {
        ps->setRadialMode(isRadial);
        ps->setGradientAngle(m_gradientAngle);
        if (!isGradient) {
            ps->clearGradientPreview();
            ps->setColor(m_selected);
        }
    }

    if (isGradient) {
        if (gsb) {
            // Initialise gradient stops on first switch
            if (gsb->stops().size() < 2) {
                const QVector<ColorPicker::GradientStop> stops = {
                    {0.0, m_selected},
                    {1.0, Qt::white}
                };
                gsb->setStops(stops);
            }
            // Load selected stop into colour controls
            setSelectedColorInternal(gsb->selectedColor(), false);
            applyUiFromColor(false);
        }
        // Populate gradient preview
        if (ps && gsb) {
            QGradientStops gs;
            for (const auto &s : gsb->stops())
                gs.append({s.pos, s.color});
            ps->setGradientPreview(gs);
        }
    }

    // ── Recompute dialog height from layout after visibility changes ───────
    const QPoint savedPos = pos();
    const int w = width();
    layout()->activate();
    int h = layout()->hasHeightForWidth() ? layout()->heightForWidth(w) : layout()->sizeHint().height();
    if (h < 200) h = layout()->sizeHint().height();
    resize(w, h);
    move(savedPos);

    emit gradientResultChanged(gradientResult());
}

// ────────────────────────────────────────────────────────────────────────────
// gradientResult
// ────────────────────────────────────────────────────────────────────────────
FluentGradientResult FluentColorDialog::gradientResult() const
{
    FluentGradientResult res;
    res.mode       = m_mode;
    res.solidColor = m_selected;
    res.angle      = m_gradientAngle;
    if (m_mode != ColorPickerMode::Solid) {
        if (auto *gsb = qobject_cast<ColorPicker::GradientStopBar *>(m_gradientStopBar)) {
            for (const auto &s : gsb->stops())
                res.stops.append({s.pos, s.color});
        }
    }
    return res;
}

// ────────────────────────────────────────────────────────────────────────────
// Show / event / paint
// ────────────────────────────────────────────────────────────────────────────
void FluentColorDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    m_border.playInitialTraceOnce(0);
}

bool FluentColorDialog::event(QEvent *event)
{
    if (event->type() == QEvent::WindowDeactivate) {
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
    if (!p.isActive()) return;

    const auto &tc = ThemeManager::instance().colors();
    FluentFrameSpec frame;
    frame.radius      = 10.0;
    frame.maximized   = false;
    frame.borderInset = 1.0;
    m_border.applyToFrameSpec(frame, tc);
    paintFluentFrame(p, rect(), tc, frame);
}

// ────────────────────────────────────────────────────────────────────────────
// Public API
// ────────────────────────────────────────────────────────────────────────────
QColor FluentColorDialog::selectedColor() const { return m_selected; }

void FluentColorDialog::setCurrentColor(const QColor &color)
{
    setSelectedColorInternal(color, true);
    applyUiFromColor(false);
}

QColor FluentColorDialog::currentColor() const { return m_selected; }

void FluentColorDialog::setResetColor(const QColor &color)
{
    if (color.isValid()) m_reset = color;
}

QColor FluentColorDialog::resetColor() const { return m_reset; }

void FluentColorDialog::setGradientEnabled(bool enabled)
{
    if (m_gradientEnabled == enabled) return;
    m_gradientEnabled = enabled;

    if (auto *cb = qobject_cast<QComboBox *>(m_modeCombo))
        cb->setVisible(enabled);

    // If gradient is disabled while a gradient mode is active, revert to solid
    if (!enabled && m_mode != ColorPickerMode::Solid) {
        if (auto *cb = qobject_cast<QComboBox *>(m_modeCombo))
            cb->setCurrentIndex(0);  // triggers setMode(Solid) via signal
        else
            setMode(ColorPickerMode::Solid);
    }
}

// ────────────────────────────────────────────────────────────────────────────
// Private helpers
// ────────────────────────────────────────────────────────────────────────────
void FluentColorDialog::setSelectedColorInternal(const QColor &color, bool emitSignal)
{
    if (!color.isValid() || m_selected == color) return;
    m_selected = color;
    if (emitSignal) emit colorChanged(m_selected);
}

void FluentColorDialog::applyUiFromColor(bool emitSignal)
{
    if (m_uiUpdating) return;
    m_uiUpdating = true;

    int hh = 0, ss = 0, vv = 0, aa = 0;
    m_selected.getHsv(&hh, &ss, &vv, &aa);
    if (hh < 0) hh = 0;

    if (auto *hs  = qobject_cast<ColorPicker::HueStrip *>(m_hueStrip))  hs->setValue(hh);
    if (auto *svp = qobject_cast<ColorPicker::SvPanel  *>(m_svPanel))   { svp->setHue(hh); svp->setSv(ss, vv); }
    if (auto *as  = qobject_cast<ColorPicker::AlphaStrip *>(m_alphaStrip)) {
        as->setBaseColor(QColor(m_selected.red(), m_selected.green(), m_selected.blue()));
        as->setValue(aa);
    }
    if (auto *sp  = qobject_cast<QSpinBox  *>(m_alphaSpin))  sp->setValue(aa);
    if (auto *le  = qobject_cast<QLineEdit *>(m_hexEdit))    le->setText(toHexArgb(m_selected).mid(1));
    // Only update the preview swatch with solid color in solid mode;
    // in gradient mode the preview is kept as the gradient (updated via stopsChanged).
    if (m_mode == ColorPickerMode::Solid) {
        if (auto *ps = qobject_cast<ColorPicker::PreviewSwatch *>(m_previewSwatch)) ps->setColor(m_selected);
    }

    m_uiUpdating = false;
    if (emitSignal) emit colorChanged(m_selected);
}

void FluentColorDialog::applyColorFromUi(bool emitSignal)
{
    if (m_uiUpdating) return;

    auto *svp = qobject_cast<ColorPicker::SvPanel   *>(m_svPanel);
    auto *hs  = qobject_cast<ColorPicker::HueStrip  *>(m_hueStrip);
    auto *as  = qobject_cast<ColorPicker::AlphaStrip*>(m_alphaStrip);
    if (!svp || !hs || !as) return;

    const QColor c = QColor::fromHsv(hs->value(), svp->s(), svp->v(), as->value());
    setSelectedColorInternal(c, emitSignal);

    if (auto *le = qobject_cast<QLineEdit *>(m_hexEdit))
        le->setText(toHexArgb(m_selected).mid(1));
    // Only update preview with solid color in solid mode
    if (m_mode == ColorPickerMode::Solid) {
        if (auto *ps = qobject_cast<ColorPicker::PreviewSwatch *>(m_previewSwatch))
            ps->setColor(m_selected);
    }
    as->setBaseColor(QColor(m_selected.red(), m_selected.green(), m_selected.blue()));

    // Sync gradient stop colour in gradient modes
    if (m_mode != ColorPickerMode::Solid) {
        if (auto *gsb = qobject_cast<ColorPicker::GradientStopBar *>(m_gradientStopBar))
            gsb->setSelectedColor(m_selected);
    }
}

} // namespace Fluent
