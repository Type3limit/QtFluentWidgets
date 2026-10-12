#include "Fluent/FluentComboBox.h"
#include "Fluent/FluentDialog.h"
#include "Fluent/FluentMainWindow.h"
#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentTableWidget.h"
#include "Fluent/FluentWidget.h"

#include <QAbstractItemDelegate>
#include <QImage>
#include <QLabel>
#include <QListView>
#include <QPainter>
#include <QStackedWidget>
#include <QTest>
#include <QVBoxLayout>

using namespace Fluent;

class QtFluentWidgetsIssueRegressionTest : public QObject
{
    Q_OBJECT

private slots:
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
