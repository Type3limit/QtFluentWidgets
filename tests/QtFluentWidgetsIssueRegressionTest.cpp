#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentDialog.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentTableWidget.h"
#include "Fluent/FluentTableView.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentWidget.h"

#include <QAbstractItemDelegate>
#include <QDir>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QListView>
#include <QPainter>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QTest>
#include <QVBoxLayout>

using namespace Fluent;

class QtFluentWidgetsIssueRegressionTest : public QObject
{
    Q_OBJECT

private slots:
    void tableFocusBorderCorners_data()
    {
        QTest::addColumn<bool>("tableView");
        QTest::addColumn<bool>("headers");
        QTest::addColumn<int>("scrollBars");
        QTest::addColumn<bool>("dark");
        for (bool view : {false, true}) {
            for (bool headers : {false, true}) {
                for (int bars = 0; bars < 4; ++bars) {
                    for (bool dark : {false, true}) {
                        QTest::newRow(qPrintable(QStringLiteral("view-%1-headers-%2-bars-%3-dark-%4")
                            .arg(view).arg(headers).arg(bars).arg(dark))) << view << headers << bars << dark;
                    }
                }
            }
        }
    }

    void tableFocusBorderCorners()
    {
        QFETCH(bool, tableView);
        QFETCH(bool, headers);
        QFETCH(int, scrollBars);
        QFETCH(bool, dark);
        qApp->setStyleSheet(QString());
        auto &theme = ThemeManager::instance();
        ThemeGuard restoreTheme;
        theme.setAnimationsEnabled(false);
        if (dark) {
            theme.setDarkMode();
        } else {
            theme.setLightMode();
        }
        theme.setAccentColor(QColor(QStringLiteral("#0066B4")));
        QMainWindow window;
        auto *page = new FluentWidget;
        window.setCentralWidget(page);
        auto *layout = new QVBoxLayout(page);
        QTableView *table = nullptr;
        if (tableView) {
            table = new FluentTableView(page);
            table->setModel(new QStandardItemModel(40, 8, table));
        } else {
            table = new FluentTableWidget(40, 8, page);
        }
        layout->addWidget(table);
        table->horizontalHeader()->setVisible(headers);
        table->verticalHeader()->setVisible(headers);
        table->setHorizontalScrollBarPolicy(scrollBars & 2 ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);
        table->setVerticalScrollBarPolicy(scrollBars & 1 ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);
        window.resize(360, 220);
        window.show();
        QCoreApplication::processEvents();
        table->clearFocus();
        QCoreApplication::processEvents();
        QTRY_VERIFY(!table->hasFocus());
        QImage idle(table->size(), QImage::Format_ARGB32_Premultiplied);
        idle.fill(Qt::transparent);
        table->render(&idle);
        table->setFocus(Qt::OtherFocusReason);
        QTRY_VERIFY(table->hasFocus());
        QCoreApplication::processEvents();
        QImage focused(table->size(), QImage::Format_ARGB32_Premultiplied);
        focused.fill(Qt::transparent);
        table->render(&focused);
        const QString output = qEnvironmentVariable("QTFLUENT_ISSUE_REGRESSION_OUTPUT_DIR");
        if (!output.isEmpty()) {
            QDir().mkpath(output);
            idle.save(output + QStringLiteral("/idle-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".png"));
            focused.save(output + QStringLiteral("/focus-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".png"));
        }
        verifyFocusCorners(idle, false);
        verifyFocusCorners(focused, true);

        const QPoint viewportPoint(12, 12);
        const QModelIndex clicked = table->indexAt(viewportPoint);
        QVERIFY(clicked.isValid());
        QCOMPARE(table->childAt(table->viewport()->mapTo(table, viewportPoint)), table->viewport());
        QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, viewportPoint);
        QCOMPARE(table->currentIndex(), clicked);

        table->setEnabled(false);
        QCoreApplication::processEvents();
        verifyFocusCorners(renderTable(table), false);
        table->setEnabled(true);
        table->setFocus(Qt::OtherFocusReason);
        QTRY_VERIFY(table->hasFocus());
        window.resize(410, 260);
        QCoreApplication::processEvents();
        verifyFocusCorners(renderTable(table), true);
    }

    void tableBorderRespectsCustomStyleAndReplacementHeader()
    {
        QCoreApplication::processEvents(); // Drain the previous case's deferred theme restoration.
        qApp->setStyleSheet(QString());
        QMainWindow window;
        auto *table = new FluentTableWidget(0, 0);
        window.setCentralWidget(table);
        window.resize(360, 220);
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        table->setStyleSheet(QStringLiteral(
            "QTableWidget { background: white; border: 3px solid #808080; border-radius: 12px; }"
            "QTableWidget:focus { border-color: #FF0000; }"));
        auto *content = new QLabel(table->viewport());
        content->setStyleSheet(QStringLiteral("background: #00FF00"));
        content->setGeometry(30, 30, 50, 50);
        content->show();
        window.show();
        QCoreApplication::processEvents();
        table->setHorizontalHeader(new QHeaderView(Qt::Horizontal, table));
        QCoreApplication::processEvents();
        table->setFocus(Qt::OtherFocusReason);
        QTRY_VERIFY(table->hasFocus());
        const QImage image = renderTable(table);
        for (const QPoint &point : {QPoint(4, 4), QPoint(image.width() - 5, 4),
                                   QPoint(4, image.height() - 5), QPoint(image.width() - 5, image.height() - 5)}) {
            const QColor color = image.pixelColor(point);
            QVERIFY2(color.red() > color.green() + 60 && color.red() > color.blue() + 60,
                     "Replacement headers must not cover the custom red rounded border");
        }
        const QPoint center = content->mapTo(table, content->rect().center());
        QCOMPARE(image.pixelColor(center).rgba(), QColor(Qt::green).rgba());
    }

    void mainWindowTitleHasVisibleTextGeometry()
    {
        FluentMainWindow window;
        window.resize(640, 240);
        window.setWindowTitle(QStringLiteral("Main window title"));
        window.show();
        auto *label = window.findChild<QLabel *>(QStringLiteral("FluentWindowTitle"));
        QVERIFY(label);
        QTRY_VERIFY(label->isVisible());
        QCOMPARE(label->text(), window.windowTitle());
        QTRY_VERIFY(label->width() >= label->fontMetrics().horizontalAdvance(label->text()));
        verifyTitlePixels(label);

        window.setFluentTitleBarTitle(QStringLiteral("Override title"));
        QTRY_COMPARE(label->text(), QStringLiteral("Override title"));
        QTRY_VERIFY(label->width() >= label->fontMetrics().horizontalAdvance(label->text()));
        window.clearFluentTitleBarTitle();
        QTRY_COMPARE(label->text(), window.windowTitle());
        window.resize(260, 240);
        QTRY_VERIFY(label->width() > 0);
        QVERIFY(label->width() <= label->maximumWidth());
    }

    void dialogTitleHasVisibleTextGeometry()
    {
        FluentDialog dialog;
        dialog.resize(640, 240);
        dialog.setWindowTitle(QStringLiteral("Dialog title"));
        dialog.show();
        auto *label = dialog.findChild<QLabel *>(QStringLiteral("FluentDialogTitle"));
        QVERIFY(label);
        QTRY_VERIFY(label->isVisible());
        QCOMPARE(label->text(), dialog.windowTitle());
        QTRY_VERIFY(label->width() >= label->fontMetrics().horizontalAdvance(label->text()));
        verifyTitlePixels(label);
        dialog.setWindowTitle(QStringLiteral("Updated dialog title"));
        QTRY_COMPARE(label->text(), dialog.windowTitle());
        QTRY_VERIFY(label->width() >= label->fontMetrics().horizontalAdvance(label->text()));
        dialog.resize(260, 240);
        QTRY_VERIFY(label->width() > 0);
        QVERIFY(label->width() <= label->maximumWidth());
    }

    void comboRowsUseTheirOwnTextHeight_data()
    {
        QTest::addColumn<bool>("multilineLast");
        QTest::addColumn<bool>("multiSelection");
        for (bool last : {false, true}) {
            for (bool multi : {false, true}) {
                QTest::newRow(qPrintable(QStringLiteral("last-%1-multi-%2").arg(last).arg(multi))) << last << multi;
            }
        }
    }

    void comboRowsUseTheirOwnTextHeight()
    {
        QFETCH(bool, multilineLast);
        QFETCH(bool, multiSelection);
        FluentComboBox combo;
        combo.resize(260, 36);
        const QString multiline = QStringLiteral("First line\nSecond line\nThird line\nFourth line");
        combo.addItems(multilineLast ? QStringList{QStringLiteral("Short"), multiline}
                                     : QStringList{multiline, QStringLiteral("Short")});
        if (multiSelection) {
            combo.setSelectionMode(FluentComboBox::MultiSelection);
        }
        combo.show();
        combo.showPopup();
        auto *view = combo.view();
        QVERIFY(view);
        QTRY_VERIFY(view->isVisible());
        view->doItemsLayout();
        const QModelIndex longIndex = combo.model()->index(multilineLast ? 1 : 0, combo.modelColumn());
        const QModelIndex shortIndex = combo.model()->index(multilineLast ? 0 : 1, combo.modelColumn());
        const int longHeight = view->visualRect(longIndex).height();
        const int shortHeight = view->visualRect(shortIndex).height();
        QVERIFY2(longHeight >= view->sizeHintForIndex(longIndex).height(), "Multiline row must not clip its delegate height");
        QVERIFY2(longHeight > shortHeight, "A single-line row must keep its own height regardless of item order");
        const QModelIndex lastIndex = combo.model()->index(1, combo.modelColumn());
        QTRY_VERIFY2(view->visualRect(lastIndex).bottom() < view->viewport()->height(),
                     qPrintable(QStringLiteral("Popup must fit both rows: last bottom %1, viewport height %2")
                         .arg(view->visualRect(lastIndex).bottom()).arg(view->viewport()->height())));
        combo.hidePopup();
    }

    void installedTableScrollBarsPaintReservedBackground_data()
    {
        QTest::addColumn<bool>("enabled");
        QTest::newRow("enabled") << true;
        QTest::newRow("disabled") << false;
    }

    void installedTableScrollBarsPaintReservedBackground()
    {
        QFETCH(bool, enabled);
        // Reproduce an application that has never installed FluentMainWindow's global QSS.
        qApp->setStyleSheet(QString());
        QMainWindow window;
        auto *stack = new QStackedWidget;
        window.setCentralWidget(stack);
        auto *oldPage = new QWidget;
        oldPage->setStyleSheet(QStringLiteral("background: magenta"));
        stack->addWidget(oldPage);
        auto *page = new FluentWidget;
        auto *layout = new QVBoxLayout(page);
        auto *table = new FluentTableWidget(50, 8, page);
        layout->addWidget(table);
        stack->addWidget(page);
        table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        window.resize(480, 260);
        window.show();
        QCoreApplication::processEvents();
        stack->setCurrentWidget(page);
        table->setEnabled(enabled);
        QCoreApplication::processEvents();
        const QColor background = table->viewport()->palette().color(table->viewport()->backgroundRole());
        for (QScrollBar *bar : {table->verticalScrollBar(), table->horizontalScrollBar()}) {
            QVERIFY(bar->isVisible());
            QVERIFY(bar->parentWidget() != table); // Qt reparents installed bars into an internal container.
            QImage image(bar->size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::magenta);
            // Skip automatic background clearing to check the bar's paint path itself.
            bar->render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
            QCOMPARE(image.pixelColor(0, image.height() - 1).rgba(), background.rgba());
            bar->setRange(0, 0);
            image.fill(Qt::magenta);
            bar->render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
            QCOMPARE(image.pixelColor(image.width() / 2, image.height() / 2).rgba(), background.rgba());
        }
    }

private:
    struct ThemeGuard {
        ThemeManager &theme = ThemeManager::instance();
        ThemeManager::ThemeMode mode = theme.themeMode();
        ThemeColors colors = theme.colors();
        bool animations = theme.animationsEnabled();
        ~ThemeGuard()
        {
            theme.setThemeMode(mode);
            theme.setColors(colors);
            theme.setAnimationsEnabled(animations);
        }
    };

    static QImage renderTable(QTableView *table)
    {
        QImage image(table->size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        table->render(&image);
        return image;
    }

    static void verifyFocusCorners(const QImage &image, bool accent)
    {
        for (const QPoint &point : {QPoint(2, 2), QPoint(image.width() - 3, 2),
                                   QPoint(2, image.height() - 3), QPoint(image.width() - 3, image.height() - 3)}) {
            const QColor color = image.pixelColor(point);
            if (accent) {
                QVERIFY2(color.blue() > color.red() + 25 && color.blue() > color.green() + 10,
                         "The rounded focus arc must remain visible above table children");
            } else {
                QVERIFY2(qAbs(color.blue() - color.red()) < 20 && qAbs(color.blue() - color.green()) < 20,
                         "Unfocused and disabled tables must use a neutral border");
            }
        }
    }

    static void verifyTitlePixels(QLabel *label)
    {
        QImage image(label->size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        label->render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
        int painted = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                painted += image.pixelColor(x, y).alpha() > 0;
            }
        }
        QVERIFY2(painted > 10, "Title label must actually paint text pixels");
    }
};

QTEST_MAIN(QtFluentWidgetsIssueRegressionTest)
#include "QtFluentWidgetsIssueRegressionTest.moc"
