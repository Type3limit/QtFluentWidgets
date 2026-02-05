#include <QtGlobal>

#include <cstdio>

#include <QByteArray>
#include <QString>

namespace {

static bool isTruthyEnv(const QByteArray &raw, bool defaultValue)
{
    if (raw.isEmpty()) {
        return defaultValue;
    }
    const QByteArray v = raw.trimmed().toLower();
    if (v == "0" || v == "false" || v == "off" || v == "no") {
        return false;
    }
    if (v == "1" || v == "true" || v == "on" || v == "yes") {
        return true;
    }
    return defaultValue;
}

static bool shouldSuppressKnownPaintWarnings()
{
    // Default OFF for library users; demos can enable via env var.
    return isTruthyEnv(qgetenv("QTFLUENT_SUPPRESS_KNOWN_PAINT_WARNINGS"), false);
}

static bool isKnownPaintWarning(const QString &msg)
{
    return msg.contains(QStringLiteral("QWidget::paintEngine: Should no longer be called"))
        || msg.contains(QStringLiteral("QPainter::begin: Paint device returned engine == 0"));
}

static QtMessageHandler g_prevHandler = nullptr;

static void fluentMessageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    if (type == QtWarningMsg && isKnownPaintWarning(msg) && shouldSuppressKnownPaintWarnings()) {
        return;
    }

    if (g_prevHandler) {
        g_prevHandler(type, ctx, msg);
    } else {
        // Fallback to default formatting.
        const QByteArray m = msg.toLocal8Bit();
        fprintf(stderr, "%s\n", m.constData());
    }
}

struct FluentDiagnosticsInstaller {
    FluentDiagnosticsInstaller()
    {
        if (!shouldSuppressKnownPaintWarnings()) {
            return;
        }
        // Install once; chain previous handler.
        g_prevHandler = qInstallMessageHandler(fluentMessageHandler);
    }
};

static const FluentDiagnosticsInstaller g_installer;

} // namespace
