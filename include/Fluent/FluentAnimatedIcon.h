#pragma once

#include "Fluent/FluentExport.h"

#include "Fluent/FluentLottieWidget.h"
#include "Fluent/FluentQtCompat.h"

class QMouseEvent;

namespace Fluent {

class FLUENT_EXPORT FluentAnimatedIcon final : public FluentLottieWidget
{
    Q_OBJECT
    Q_PROPERTY(QString state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(bool interactive READ isInteractive WRITE setInteractive)
public:
    explicit FluentAnimatedIcon(QWidget *parent = nullptr);

    QString state() const;
    void setState(const QString &state);

    Q_INVOKABLE void setState(const QString &state, bool animated);

    bool isInteractive() const;
    void setInteractive(bool interactive);

signals:
    void stateChanged(const QString &state);

protected:
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    int resolvedStateFrame(const QString &previousState, const QString &nextState) const;
    bool playResolvedTransition(const QString &previousState, const QString &nextState);

    QString m_state = QStringLiteral("Normal");
    bool m_interactive = false;
};

} // namespace Fluent
