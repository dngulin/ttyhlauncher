#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <QPlainTextEdit>

class LogView : public QPlainTextEdit
{
    Q_OBJECT

public:
    LogView(QWidget *parent = 0);

protected:
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);

private:
    bool click;
};

#endif // LOGVIEW_H
