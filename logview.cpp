#include "logview.h"

#include <QDesktopServices>

LogView::LogView(QWidget *parent) : QPlainTextEdit(parent)
{
    this->setMouseTracking(true);
    click = false;
}

void LogView::mouseMoveEvent(QMouseEvent *e)
{
    QPlainTextEdit::mouseMoveEvent(e);

    if ( !anchorAt( e->pos() ).isEmpty() )
    {
        viewport()->setCursor(Qt::PointingHandCursor);
    }
    else
    {
        click = false;
        viewport()->setCursor(Qt::IBeamCursor);
    }
}

void LogView::mousePressEvent(QMouseEvent *e)
{
    QPlainTextEdit::mousePressEvent(e);

    if ( !anchorAt( e->pos() ).isEmpty() )
    {
        click = true;
    }
}

void LogView::mouseReleaseEvent(QMouseEvent *e)
{
    QPlainTextEdit::mouseReleaseEvent(e);

    if (click)
    {
        click = false;

        QString anchor = anchorAt( e->pos() );
        if ( !anchor.isEmpty() )
        {
            QDesktopServices::openUrl( QUrl(anchor) );
        }
    }
}
