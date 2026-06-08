#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QGraphicsEffect>
#include <QImage>
#include <QList>
#include <QLocale>
#include <QMouseEvent>
#include <QPointer>
#include <QRegularExpression>
#include <QScreen>
#include <QSet>
#include <QStringList>
#include <QTextStream>
#include <QTimer>

#include <cstdlib>
#include <utility>

#if defined(Q_OS_WIN)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "Fluent/FluentTheme.h"
#include "Fluent/FluentAnnotatedScrollBar.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentNavigationView.h"

#include "DemoHelpers.h"
#include "DemoWindow.h"

namespace {

QString safeBaselineName(QString text)
{
    text.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]+")), QStringLiteral("_"));
    return text.left(120);
}

class ScopedGraphicsEffectDisabler final {
public:
    explicit ScopedGraphicsEffectDisabler(QWidget *root)
    {
        if (!root) {
            return;
        }

        QList<QWidget *> widgets{root};
        widgets.append(root->findChildren<QWidget *>(QString(), Qt::FindChildrenRecursively));
        for (QWidget *widget : std::as_const(widgets)) {
            if (!widget) {
                continue;
            }
            if (auto *effect = widget->graphicsEffect(); effect && effect->isEnabled()) {
                m_effects.append({effect, true});
                effect->setEnabled(false);
            }
        }
    }

    ~ScopedGraphicsEffectDisabler()
    {
        for (const EffectState &state : std::as_const(m_effects)) {
            if (state.effect) {
                state.effect->setEnabled(state.wasEnabled);
            }
        }
    }

private:
    struct EffectState {
        QPointer<QGraphicsEffect> effect;
        bool wasEnabled = false;
    };

    QList<EffectState> m_effects;
};

QStringList splitCsv(const QString &text)
{
    QStringList trimmed;
    const QStringList parts = text.split(QLatin1Char(','), Qt::SkipEmptyParts);
    trimmed.reserve(parts.size());
    for (QString part : parts) {
        part = part.trimmed();
        if (!part.isEmpty()) {
            trimmed << part;
        }
    }
    return trimmed;
}

QStringList renderPages(const QString &overrideValue = QString())
{
    const QString renderEnv = qEnvironmentVariable("QTFLUENT_DEMO_RENDER_PAGES");
    const QString screenshotEnv = qEnvironmentVariable("QTFLUENT_DEMO_SCREENSHOT_PAGES");
    const QString env = renderEnv.isEmpty() ? screenshotEnv : renderEnv;
    if (!overrideValue.isEmpty()) {
        return splitCsv(overrideValue);
    }
    if (!env.isEmpty()) {
        return splitCsv(env);
    }
    return {
        QStringLiteral("overview"),
        QStringLiteral("basic_input"),
        QStringLiteral("inputs"),
        QStringLiteral("buttons"),
        QStringLiteral("icons"),
        QStringLiteral("motion"),
        QStringLiteral("pickers"),
        QStringLiteral("angles"),
        QStringLiteral("dataviews"),
        QStringLiteral("containers"),
        QStringLiteral("windows"),
        QStringLiteral("settings"),
    };
}

struct RenderVariant {
    QString name;
    bool dark = false;
    QColor accent;
};

QList<RenderVariant> renderVariants(const QString &overrideValue = QString())
{
    const QColor defaultAccent(QStringLiteral("#0066B4"));
    const QColor brightAccent(QStringLiteral("#43FF43"));
    const QString renderEnv = qEnvironmentVariable("QTFLUENT_DEMO_RENDER_VARIANTS");
    const QString screenshotEnv = qEnvironmentVariable("QTFLUENT_DEMO_SCREENSHOT_VARIANTS");
    const QString env = renderEnv.isEmpty() ? screenshotEnv : renderEnv;
    const QStringList names = (!overrideValue.isEmpty())
        ? splitCsv(overrideValue)
        : (env.isEmpty()
        ? QStringList{QStringLiteral("light"), QStringLiteral("dark"), QStringLiteral("accent")}
        : splitCsv(env));

    QList<RenderVariant> variants;
    for (QString name : names) {
        name = name.trimmed().toLower();
        if (name == QLatin1String("dark")) {
            variants.push_back({QStringLiteral("dark"), true, defaultAccent});
        } else if (name == QLatin1String("accent") || name == QLatin1String("light-accent")) {
            variants.push_back({QStringLiteral("light-accent"), false, brightAccent});
        } else if (name == QLatin1String("dark-accent")) {
            variants.push_back({QStringLiteral("dark-accent"), true, brightAccent});
        } else {
            variants.push_back({QStringLiteral("light"), false, defaultAccent});
        }
    }
    return variants;
}

QString firstAvailableFontFamily(const QStringList &candidates)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QStringList installedFamilies = QFontDatabase::families();
#else
    const QFontDatabase fontDatabase;
    const QStringList installedFamilies = fontDatabase.families();
#endif
    for (const QString &candidate : candidates) {
        for (const QString &family : installedFamilies) {
            if (family.compare(candidate, Qt::CaseInsensitive) == 0) {
                return family;
            }
        }
    }
    return QString();
}

QString preferredDemoFontFamily()
{
    const bool chinese = QLocale::system().language() == QLocale::Chinese;
    const QStringList candidates = chinese
        ? QStringList{QStringLiteral("Microsoft YaHei"),
                      QStringLiteral("Microsoft YaHei UI"),
                      QStringLiteral("Segoe UI")}
        : QStringList{QStringLiteral("Segoe UI"),
                      QStringLiteral("Microsoft YaHei"),
                      QStringLiteral("Microsoft YaHei UI")};
    return firstAvailableFontFamily(candidates);
}

void configureDemoFontRendering(QApplication &app)
{
    QFont font = app.font();
    const QString overrideFamily = qEnvironmentVariable("QTFLUENT_DEMO_FONT_FAMILY").trimmed();
    const QString family = overrideFamily.isEmpty() ? preferredDemoFontFamily() : overrideFamily;
    if (!family.isEmpty()) {
        font.setFamily(family);
    }
    font.setHintingPreference(QFont::PreferFullHinting);
    font.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(font);

    qInfo().noquote() << QStringLiteral("[DemoStartup] font family=\"%1\" hinting=PreferFullHinting")
                             .arg(app.font().family());
}

int currentScalePercent()
{
    const QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return 100;
    }
    return qMax(1, qRound(screen->devicePixelRatio() * 100.0));
}

qreal currentDevicePixelRatio()
{
    const QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return 1.0;
    }
    return qMax<qreal>(1.0, screen->devicePixelRatio());
}

QImage renderWidgetToImage(QWidget *widget, qreal devicePixelRatio)
{
    const QSize pixelSize(qMax(1, qRound(widget->width() * devicePixelRatio)),
                          qMax(1, qRound(widget->height() * devicePixelRatio)));
    QImage image(pixelSize, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(devicePixelRatio);
    image.fill(Qt::transparent);
    const ScopedGraphicsEffectDisabler effectDisabler(widget);
    widget->render(&image);
    return image;
}

QString renderHealthError(const QImage &image, const QString &label)
{
    if (image.isNull()) {
        return QStringLiteral("%1 rendered a null image").arg(label);
    }
    if (image.width() < 320 || image.height() < 240) {
        return QStringLiteral("%1 rendered at unexpected size %2x%3")
            .arg(label)
            .arg(image.width())
            .arg(image.height());
    }

    const QImage readable = image.convertToFormat(QImage::Format_ARGB32);
    QSet<QRgb> sampleColors;
    int totalSamples = 0;
    int visibleSamples = 0;
    const int stepX = qMax(1, readable.width() / 18);
    const int stepY = qMax(1, readable.height() / 18);
    for (int y = stepY / 2; y < readable.height(); y += stepY) {
        for (int x = stepX / 2; x < readable.width(); x += stepX) {
            const QRgb pixel = readable.pixel(x, y);
            sampleColors.insert(pixel);
            ++totalSamples;
            if (qAlpha(pixel) > 16) {
                ++visibleSamples;
            }
        }
    }

    if (visibleSamples < qMax(1, totalSamples / 4)) {
        return QStringLiteral("%1 rendered mostly transparent pixels").arg(label);
    }
    if (sampleColors.size() < 3) {
        return QStringLiteral("%1 rendered a blank or nearly blank image (%2 sampled colors)")
            .arg(label)
            .arg(sampleColors.size());
    }
    return {};
}

void settleUi(int milliseconds = 80)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec(QEventLoop::AllEvents);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
}

void appendRenderLog(const QDir &outputDir, const QString &message)
{
    QFile file(outputDir.filePath(QStringLiteral("_render.log")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }
    QTextStream stream(&file);
    stream << message << Qt::endl;
}

QString argumentValue(const QStringList &arguments, const QString &optionName)
{
    for (int i = 1; i < arguments.size(); ++i) {
        const QString argument = arguments.at(i);
        if (argument == optionName && i + 1 < arguments.size()) {
            return arguments.at(i + 1);
        }
        const QString prefix = optionName + QLatin1Char('=');
        if (argument.startsWith(prefix)) {
            return argument.mid(prefix.size());
        }
    }
    return {};
}

int runVisualRenderBatch(const QString &outputPath,
                         const QString &pagesOverride = QString(),
                         const QString &variantsOverride = QString())
{
    QDir outputDir(outputPath);
    if (!outputDir.exists() && !QDir().mkpath(outputDir.absolutePath())) {
        qWarning().noquote() << QStringLiteral("[DemoVisualRender] cannot create output directory: %1")
                                    .arg(outputDir.absolutePath());
        return 1;
    }

    const QStringList pages = renderPages(pagesOverride);
    const QList<RenderVariant> variants = renderVariants(variantsOverride);
    const qreal devicePixelRatio = currentDevicePixelRatio();
    const int scale = currentScalePercent();
    int failures = 0;

    Fluent::ThemeManager::instance().setAnimationsEnabled(false);
    settleUi();

    for (const RenderVariant &variant : variants) {
        Fluent::ThemeManager::instance().setThemeMode(variant.dark
                                                          ? Fluent::ThemeManager::ThemeMode::Dark
                                                          : Fluent::ThemeManager::ThemeMode::Light);
        Demo::applyAccent(variant.accent);
        settleUi();

        for (const QString &page : pages) {
            appendRenderLog(outputDir, QStringLiteral("begin %1 %2").arg(variant.name, page));
            auto *window = new Demo::DemoWindow(nullptr, page);
            appendRenderLog(outputDir, QStringLiteral("constructed %1 %2").arg(variant.name, page));
            window->setAttribute(Qt::WA_DontShowOnScreen, true);
            window->resize(1260, 780);
            window->ensurePolished();
            settleUi(40);
            appendRenderLog(outputDir, QStringLiteral("prepared %1 %2").arg(variant.name, page));

            const QString fileName = QStringLiteral("%1-scale%2-%3.png")
                                         .arg(variant.name)
                                         .arg(scale)
                                         .arg(safeBaselineName(page));
            const QString filePath = outputDir.filePath(fileName);
            const QImage image = renderWidgetToImage(window, devicePixelRatio);
            appendRenderLog(outputDir, QStringLiteral("rendered %1 %2").arg(variant.name, page));
            const QString healthError = renderHealthError(image, QStringLiteral("%1 %2").arg(variant.name, page));
            if (!healthError.isEmpty()) {
                ++failures;
                appendRenderLog(outputDir, healthError);
                qWarning().noquote() << QStringLiteral("[DemoVisualRender] %1").arg(healthError);
                continue;
            }
            if (!image.save(filePath)) {
                ++failures;
                qWarning().noquote() << QStringLiteral("[DemoVisualRender] failed to save %1").arg(filePath);
            } else {
                qInfo().noquote() << QStringLiteral("[DemoVisualRender] %1").arg(filePath);
            }
            appendRenderLog(outputDir, QStringLiteral("saved %1 %2").arg(variant.name, page));
        }
    }

    const int exitCode = failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
#if defined(Q_OS_WIN)
    TerminateProcess(GetCurrentProcess(), static_cast<UINT>(exitCode));
#else
    std::_Exit(exitCode);
#endif
    return exitCode;
}

void sendSyntheticClick(QWidget *target, const QPoint &pos)
{
    if (!target) {
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMouseEvent press(QEvent::MouseButtonPress,
                      QPointF(pos),
                      QPointF(target->mapToGlobal(pos)),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease,
                        QPointF(pos),
                        QPointF(target->mapToGlobal(pos)),
                        Qt::LeftButton,
                        Qt::NoButton,
                        Qt::NoModifier);
#else
    QMouseEvent press(QEvent::MouseButtonPress,
                      pos,
                      target->mapToGlobal(pos),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease,
                        pos,
                        target->mapToGlobal(pos),
                        Qt::LeftButton,
                        Qt::NoButton,
                        Qt::NoModifier);
#endif
    QCoreApplication::sendEvent(target, &press);
    QCoreApplication::sendEvent(target, &release);
}

QString annotatedScrollBarHealthError(QWidget *root, const QString &context)
{
    const auto bars = root ? root->findChildren<Fluent::FluentAnnotatedScrollBar *>() : QList<Fluent::FluentAnnotatedScrollBar *>();
    QList<Fluent::FluentAnnotatedScrollBar *> visibleBars;
    QList<Fluent::FluentAnnotatedScrollBar *> pageBars;
    for (Fluent::FluentAnnotatedScrollBar *bar : bars) {
        if (bar && bar->isVisible()) {
            visibleBars.append(bar);
            const QString objectName = bar->objectName();
            if (objectName == QLatin1String("DemoPageAnnotatedScrollBar")
                || objectName == QLatin1String("DemoSettingsAnnotatedScrollBar")) {
                pageBars.append(bar);
            }
        }
    }

    if (visibleBars.isEmpty()) {
        return QStringLiteral("%1 has no visible FluentAnnotatedScrollBar").arg(context);
    }

    const QList<Fluent::FluentAnnotatedScrollBar *> barsToCheck = pageBars.isEmpty() ? visibleBars : pageBars;
    for (Fluent::FluentAnnotatedScrollBar *bar : barsToCheck) {
        const QVector<Fluent::FluentAnnotatedScrollBarSource> sources = bar->sources();
        const QVector<Fluent::FluentAnnotatedScrollBarRange> ranges = bar->annotatedRanges();
        const int sourceOrRangeCount = !sources.isEmpty() ? sources.size() : ranges.size();
        if (sourceOrRangeCount < 2) {
            return QStringLiteral("%1 annotated scrollbar %2 has only %3 source/range entries")
                .arg(context, bar->objectName().isEmpty() ? QStringLiteral("<unnamed>") : bar->objectName())
                .arg(sourceOrRangeCount);
        }

        const bool hasCurrent = !sources.isEmpty() ? bar->currentSourceIndex() >= 0
                                                   : bar->currentRangeIndex() >= 0;
        if (!hasCurrent) {
            return QStringLiteral("%1 annotated scrollbar %2 has no current source/range")
                .arg(context, bar->objectName().isEmpty() ? QStringLiteral("<unnamed>") : bar->objectName());
        }

        if (!sources.isEmpty()) {
            int shortSourceCount = 0;
            for (const Fluent::FluentAnnotatedScrollBarSource &source : sources) {
                if (source.end - source.start <= 24) {
                    ++shortSourceCount;
                }
            }
            if (shortSourceCount >= qMax(1, sources.size() / 2)) {
                return QStringLiteral("%1 annotated scrollbar %2 sources look like anchors instead of content ranges")
                    .arg(context, bar->objectName().isEmpty() ? QStringLiteral("<unnamed>") : bar->objectName());
            }
        }
    }

    return {};
}

QStringList expectedStateMatrixTitles(const QString &key)
{
    if (key == QLatin1String("inputs")) {
        return {QStringLiteral("P0 Input State Matrix")};
    }
    if (key == QLatin1String("buttons")) {
        return {QStringLiteral("P0 Button State Matrix")};
    }
    if (key == QLatin1String("icons")) {
        return {QStringLiteral("Icon State Matrix")};
    }
    if (key == QLatin1String("motion")) {
        return {QStringLiteral("Motion Role Matrix")};
    }
    if (key == QLatin1String("pickers")) {
        return {QStringLiteral("Picker State Matrix")};
    }
    if (key == QLatin1String("angles")) {
        return {QStringLiteral("Angle State Matrix")};
    }
    if (key == QLatin1String("dataviews")) {
        return {QStringLiteral("Data View State Matrix")};
    }
    if (key == QLatin1String("containers")) {
        return {QStringLiteral("Navigation / Tab State Matrix")};
    }
    if (key == QLatin1String("windows")) {
        return {QStringLiteral("Feedback State Matrix")};
    }
    return {};
}

QString demoMatrixHealthError(QWidget *root, const QString &key, const QString &context)
{
    const QStringList titles = expectedStateMatrixTitles(key);
    if (titles.isEmpty()) {
        return {};
    }
    if (!root) {
        return QStringLiteral("%1 has no root widget for state matrix checks").arg(context);
    }

    const QList<Fluent::FluentCard *> cards = root->findChildren<Fluent::FluentCard *>();
    const QRect viewport = root->rect();
    for (const QString &title : titles) {
        Fluent::FluentCard *matched = nullptr;
        for (Fluent::FluentCard *card : cards) {
            if (!card) {
                continue;
            }
            const QString explicitTitle = card->property("_demoSectionTitle").toString().trimmed();
            const QString cardTitle = !explicitTitle.isEmpty() ? explicitTitle : card->title().trimmed();
            if (cardTitle == title) {
                matched = card;
                break;
            }
        }

        if (!matched) {
            return QStringLiteral("%1 is missing state matrix card '%2'").arg(context, title);
        }
        if (!matched->isVisible()) {
            return QStringLiteral("%1 state matrix '%2' is not visible").arg(context, title);
        }
        if (matched->isCollapsed()) {
            return QStringLiteral("%1 state matrix '%2' is collapsed").arg(context, title);
        }

        QWidget *content = matched->contentWidget();
        if (!content) {
            return QStringLiteral("%1 state matrix '%2' has no content widget").arg(context, title);
        }

        const int contentHeight = qMax(content->height(), content->sizeHint().height());
        if (contentHeight < 64) {
            return QStringLiteral("%1 state matrix '%2' content is too small (%3 px)")
                .arg(context, title)
                .arg(contentHeight);
        }

        const QRect cardRect(matched->mapTo(root, QPoint(0, 0)), matched->size());
        const QRect visibleRect = viewport.intersected(cardRect);
        if (visibleRect.height() < 72 || visibleRect.width() < qMin(240, qMax(1, cardRect.width() / 2))) {
            return QStringLiteral("%1 state matrix '%2' is not visible in the first viewport")
                .arg(context, title);
        }
    }

    return {};
}

int runInteractionSmoke(const QString &outputPath)
{
    QDir outputDir(outputPath);
    if (!outputDir.exists() && !QDir().mkpath(outputDir.absolutePath())) {
        qWarning().noquote() << QStringLiteral("[DemoInteractionSmoke] cannot create output directory: %1")
                                    .arg(outputDir.absolutePath());
        return EXIT_FAILURE;
    }

    Fluent::ThemeManager::instance().setThemeMode(Fluent::ThemeManager::ThemeMode::Light);
    Demo::applyAccent(QColor(QStringLiteral("#0066B4")));
    Fluent::ThemeManager::instance().setAnimationsEnabled(true);
    settleUi(80);

    Demo::DemoWindow window;
    window.setAttribute(Qt::WA_DontShowOnScreen, true);
    window.resize(1260, 780);
    window.show();
    settleUi(160);

    auto *nav = window.findChild<Fluent::FluentNavigationView *>();
    if (!nav) {
        appendRenderLog(outputDir, QStringLiteral("navigation view not found"));
        return EXIT_FAILURE;
    }

    const qreal devicePixelRatio = currentDevicePixelRatio();
    const int settleMs = qMax(120, Fluent::FluentMotion::duration(Fluent::FluentMotionRole::Page) + 90);
    const Demo::DemoLanguage originalLanguage = Demo::currentLanguage();
    const Demo::DemoLanguage alternateLanguage = originalLanguage == Demo::DemoLanguage::English
                                                     ? Demo::DemoLanguage::Chinese
                                                     : Demo::DemoLanguage::English;
    appendRenderLog(outputDir, QStringLiteral("language/theme rebuild smoke begin"));
    window.switchLanguage(alternateLanguage);
    settleUi(settleMs + 80);
    Fluent::ThemeManager::instance().setDarkMode();
    settleUi(160);
    Fluent::ThemeManager::instance().setLightMode();
    settleUi(160);
    window.switchLanguage(originalLanguage);
    settleUi(settleMs + 80);
    nav = window.findChild<Fluent::FluentNavigationView *>();
    if (!nav) {
        appendRenderLog(outputDir, QStringLiteral("navigation view not found after language/theme rebuild"));
        return EXIT_FAILURE;
    }
    const QImage rebuiltImage = renderWidgetToImage(&window, devicePixelRatio);
    const QString rebuildHealthError = renderHealthError(rebuiltImage, QStringLiteral("language/theme rebuild"));
    if (!rebuildHealthError.isEmpty()) {
        appendRenderLog(outputDir, rebuildHealthError);
        qWarning().noquote() << QStringLiteral("[DemoInteractionSmoke] %1").arg(rebuildHealthError);
        return EXIT_FAILURE;
    }
    appendRenderLog(outputDir, QStringLiteral("language/theme rebuild smoke ok"));

    const QStringList keys = {
        QStringLiteral("overview"),
        QStringLiteral("basic_input"),
        QStringLiteral("inputs"),
        QStringLiteral("buttons"),
        QStringLiteral("icons"),
        QStringLiteral("motion"),
        QStringLiteral("pickers"),
        QStringLiteral("angles"),
        QStringLiteral("dataviews"),
        QStringLiteral("containers"),
        QStringLiteral("windows"),
        QStringLiteral("settings"),
        QStringLiteral("overview"),
    };

    int failures = 0;
    for (int i = 0; i < keys.size(); ++i) {
        const QString &key = keys.at(i);
        appendRenderLog(outputDir, QStringLiteral("select %1").arg(key));
        nav->setSelectedKey(key);
        sendSyntheticClick(&window, QPoint(24, 24));
        settleUi(settleMs);

        const QImage image = renderWidgetToImage(&window, devicePixelRatio);
        const QString fileName = QStringLiteral("interaction-%1-%2.png")
                                     .arg(i, 2, 10, QLatin1Char('0'))
                                     .arg(safeBaselineName(key));
        const QString healthError = renderHealthError(image, QStringLiteral("interaction %1").arg(key));
        if (!healthError.isEmpty()) {
            ++failures;
            appendRenderLog(outputDir, healthError);
            qWarning().noquote() << QStringLiteral("[DemoInteractionSmoke] %1").arg(healthError);
            continue;
        }

        const QString annotationError = annotatedScrollBarHealthError(&window, QStringLiteral("interaction %1").arg(key));
        if (!annotationError.isEmpty()) {
            ++failures;
            appendRenderLog(outputDir, annotationError);
            qWarning().noquote() << QStringLiteral("[DemoInteractionSmoke] %1").arg(annotationError);
            continue;
        }

        const QString matrixError = demoMatrixHealthError(&window, key, QStringLiteral("interaction %1").arg(key));
        if (!matrixError.isEmpty()) {
            ++failures;
            appendRenderLog(outputDir, matrixError);
            qWarning().noquote() << QStringLiteral("[DemoInteractionSmoke] %1").arg(matrixError);
            continue;
        }

        if (!image.save(outputDir.filePath(fileName))) {
            ++failures;
            appendRenderLog(outputDir, QStringLiteral("save failed %1").arg(fileName));
        } else {
            appendRenderLog(outputDir, QStringLiteral("saved %1").arg(fileName));
        }
    }

    window.close();
    settleUi(80);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // namespace

int main(int argc, char *argv[])
{
    QElapsedTimer startupTimer;
    startupTimer.start();

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    qInfo().noquote() << QStringLiteral("[DemoStartup] QApplication constructed at %1 ms").arg(startupTimer.elapsed());
    configureDemoFontRendering(app);

    QCoreApplication::setOrganizationName(QStringLiteral("QtFluent"));
    QCoreApplication::setApplicationName(QStringLiteral("QtFluentDemo"));
    Demo::initializeLanguage();
    qInfo().noquote() << QStringLiteral("[DemoStartup] language initialized at %1 ms").arg(startupTimer.elapsed());

    const QStringList arguments = QCoreApplication::arguments();
    const QString renderArg = argumentValue(arguments, QStringLiteral("--render-baselines"));
    const QString screenshotArg = argumentValue(arguments, QStringLiteral("--screenshots"));
    const QString renderDir = !renderArg.isEmpty()
        ? renderArg
        : (!screenshotArg.isEmpty()
               ? screenshotArg
               : (qEnvironmentVariable("QTFLUENT_DEMO_RENDER_DIR").isEmpty()
                      ? qEnvironmentVariable("QTFLUENT_DEMO_SCREENSHOT_DIR")
                      : qEnvironmentVariable("QTFLUENT_DEMO_RENDER_DIR")));
    if (!renderDir.isEmpty()) {
        const QString renderPages = argumentValue(arguments, QStringLiteral("--render-pages"));
        const QString renderVariants = argumentValue(arguments, QStringLiteral("--render-variants"));
        return runVisualRenderBatch(renderDir,
                                    renderPages.isEmpty() ? argumentValue(arguments, QStringLiteral("--screenshot-pages")) : renderPages,
                                    renderVariants.isEmpty() ? argumentValue(arguments, QStringLiteral("--screenshot-variants")) : renderVariants);
    }

    const QString interactionSmokeDir = argumentValue(arguments, QStringLiteral("--interaction-smoke"));
    if (!interactionSmokeDir.isEmpty()) {
        return runInteractionSmoke(interactionSmokeDir);
    }

    Demo::DemoWindow w;
    qInfo().noquote() << QStringLiteral("[DemoStartup] DemoWindow constructed at %1 ms").arg(startupTimer.elapsed());

    w.resize(1260, 780);
    qInfo().noquote() << QStringLiteral("[DemoStartup] DemoWindow resized at %1 ms").arg(startupTimer.elapsed());

    w.show();
    qInfo().noquote() << QStringLiteral("[DemoStartup] DemoWindow show() returned at %1 ms").arg(startupTimer.elapsed());

    QTimer::singleShot(0, &app, [&startupTimer]() {
        qInfo().noquote() << QStringLiteral("[DemoStartup] first event loop tick at %1 ms").arg(startupTimer.elapsed());
    });

    return app.exec();
}
