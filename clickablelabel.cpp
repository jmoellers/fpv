#include "clickablelabel.h"
# include	<QString>
# include	<QDebug>
# include	<QMouseEvent>

// https://wiki.qt.io/Clickable_QLabel
ClickableLabel::ClickableLabel(QWidget* parent, Qt::WindowFlags f)
    : QLabel(parent, f) {
    
}

ClickableLabel::~ClickableLabel() {}

void ClickableLabel::mousePressEvent(QMouseEvent* event) {
    emit clicked();
qDebug() << "Clicked" << ImageName() << (unsigned int) event->button();

    // Start the external viewer if the left button is clicked
    if (event->button() == Qt::LeftButton)
    {
	QString cmd("gwenview");
	cmd.append(" "); cmd.append(ImageName());
	(void) system(cmd.toStdString().c_str());
    }
}

void
ClickableLabel::setImageName(QString imageName)
{
    imgname = imageName;
}

QString
ClickableLabel::ImageName()
{
    return imgname;
}
