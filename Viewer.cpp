# include	"Viewer.h"
# include	<QMatrix>
# include	<unistd.h>
# include	"Exif.h"
# include	"clickablelabel.h"

extern void update_index(float resolver_delay, unsigned int maxrequests);
extern float resolver_delay;
extern QMap<QString,QString> *locationmap;

/*
 * NAME: Viewer
 * PURPOSE: Constructor of the Viewer class
 * ARGUMENTS: names: QStringlist of file names
 *	locationmap: QMap<QString,QString> mapping file names to locations (street, place, ...)
 *	new_settings: QSettings for this program
 * RETURNS: Nothing
 */
Viewer::Viewer(QStringList names, QMap<QString,QString> *locationmap, QSettings *new_settings)
{
    // qDebug() << "new_settings->directory" << new_settings->value("directory", ".").toString();
    settings = new_settings;
    createMenu();
    createBox(names, locationmap);

    // Create the top window that contains the menubar and the subwindow
    mainLayout = new QVBoxLayout;
    mainLayout->setMenuBar(menuBar);
    mainLayout->addWidget(groupbox);

    setLayout(mainLayout);
}

/*
 * NAME: ~Viewer
 * PURPOSE: Desctructor of the Viewer class
 * ARGUMENTS: None
 * RETURNS: Nothing
 * NOTE: This destructor does nothing
 */
Viewer::~Viewer()
{
}

/*
 * NAME: createMenu
 * PURPOSE: To create the menu bar of this program
 * ARGUMENTS: None
 * RETURNS: Nothing
 */
void
Viewer::createMenu()
{
    menuBar = new QMenuBar;
    fileMenu = new QMenu(tr("&File"), this);
    openAction = fileMenu->addAction(tr("O&pen"));
    exitAction = fileMenu->addAction(tr("E&xit"));
    menuBar->addMenu(fileMenu);

    /* "File" menu */
    connect(exitAction, SIGNAL(triggered()), this, SLOT(accept()));
    connect(openAction, SIGNAL(triggered()), this, SLOT(openDir()));
}

/*
 * NAME: createBox
 * PURPOSE: To create the main window displaying the locations/thumbnails
 * ARGUMENTS: names: QStringlist of file names
 *	locationmap: QMap<QString,QString> mapping file names to locations (street, place, ...)
 * RETURNS: Nothing
 */
void
Viewer::createBox(QStringList names, QMap<QString,QString> *locationmap)
{
    int row, col;
    QString currentLocation("");
    QString currentDirectory(QDir().canonicalPath());

    // Create the subwindow that contains the thumbnails and the descriptions

    // qDebug() << "currentDirectory" << currentDirectory;
    groupbox = new QGroupBox(currentDirectory);
    settings->setValue("directory", currentDirectory);
    QVBoxLayout *layout = new QVBoxLayout();

    QFrame *f = new QFrame(groupbox);
    f->show();
    QGridLayout *frame_layout = new QGridLayout();
    f->setLayout(frame_layout);

    row = -1;
    col = 0;
    for (QStringList::iterator name = names.begin(); name != names.end(); name++)
    {
	QString location(locationmap->value(*name));
	Exif exif(*name);

	// qDebug() << *name << ": location=" << location << " - current=" << currentLocation;
	if (location != currentLocation)
	{
	    QFrame *descrFrame = new QFrame();
	    QHBoxLayout *descrLayout = new QHBoxLayout();
	    descrFrame->setLayout(descrLayout);

	    row++;
	    // qDebug() << "New label row=" << row;
	    QLabel *loc = new QLabel(QString("<b>") + locationmap->value(*name) + QString("</b>"), f);
	    loc->setTextInteractionFlags(Qt::TextSelectableByMouse);
	    descrLayout->addWidget(loc, 1, Qt::AlignLeft);

	    QLabel *date = new QLabel(exif.Date(), f);
	    descrLayout->addWidget(date, 0, Qt::AlignRight);

	    frame_layout->addWidget(descrFrame, row, 0, 1, -1);
	    descrFrame->show();
	    row++;
	    col = 0;
	}
	else
	{
	    // qDebug() << "Same label";
	}

	QPixmap *image = new QPixmap(".thumbnails/" + *name);
	QPixmap rotated;
	if (exif.Orientation() != "Top-left")
	{
	    QMatrix rm;

	    if (exif.Orientation() == "Right-top")
		rm.rotate(90);
	    else if (exif.Orientation() == "Left-bottom")
	        rm.rotate(-90);
	    else
	        rm.rotate(0);
	    rotated = image->transformed(rm);
	    image = &rotated;
	}
	ClickableLabel *imageLabel = new ClickableLabel(f);
	imageLabel->setImageName(*name);
	imageLabel->setToolTip(*name);
	imageLabel->setPixmap(*image);
	// qDebug() << "row=" << row << "col=" << col;
	frame_layout->addWidget(imageLabel, row, col);
	imageLabel->show();
	col++;
	if (col >= 4)
	{
	    row++;
	    col = 0;
	}

        currentLocation = location;
    }

    QScrollArea *scroll = new QScrollArea(groupbox);
    scroll->setWidget(f);
    layout->addWidget(scroll);

    groupbox->setLayout(layout);
}

/*
 * NAME: openDir
 * PURPOSE: To open (ie change into) a different directory
 * ARGUMENTS: None
 * RETURNS: Nothing
 * NOTE: The new directory path name is obtained through a QFileDialog
 */
void
Viewer::openDir()
{
    // qDebug() << "openDir";
    QFileDialog dialog;

    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly);
    dialog.setDirectory(QDir());

    QString dirname = dialog.getExistingDirectory(this, "Wähle neues Verzeichnis");

    // qDebug() << "Selected" << dirname;
    if (dirname.length() && chdir(dirname.toStdString().c_str()) != -1)
    {
        mainLayout->removeWidget(groupbox);
	delete groupbox;
	groupbox = NULL;

	// First step: load and update the location map
	delete locationmap;
	locationmap = NULL;
	update_index(resolver_delay, 100);

	QStringList filenames(locationmap->keys());
	filenames.sort(Qt::CaseInsensitive);

	createBox(filenames, locationmap);
	mainLayout->addWidget(groupbox);
    }
}
