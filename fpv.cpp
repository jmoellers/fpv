# include	<sys/stat.h>
# include	<sys/types.h>
# include	<iostream>
# include	<fstream>
# include	<QApplication>
# include	<QCommandLineParser>
# include	<QDebug>
# include	<QDirIterator>
# include	<QFile>
# include	<QDataStream>
# include	<unistd.h>
# include	<errno.h>
# include	<string.h>
# include	<stdlib.h>

# include	"Exif.h"
# include	"Viewer.h"
# include	"Resolver.h"

using namespace std;

void update_index(float resolver_delay, unsigned int maxrequests);
static void saveMap(QMap<QString,QString> *map, QString filename);

int debug;
QMap<QString,QString> *locationmap;
float resolver_delay = 0.0;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCommandLineParser commandline_parser;
    QString delay_s;
    char *pname;

    if ((pname = strrchr(argv[0], '/')) != NULL)
        pname++;
    else
        pname = argv[0];

    QSettings settings("Josef Möllers", pname);

    setlocale(LC_ALL, "en_US.UTF-8");
    QCommandLineOption debugOption("D", QCoreApplication::translate("main", "Show debug output"));
    commandline_parser.addOption(debugOption);
    QCommandLineOption delayOption("delay", QCoreApplication::translate("main", "Specify delay between reverse geocoding"));
    commandline_parser.addOption(delayOption);
    commandline_parser.process(app);

    debug = commandline_parser.isSet(debugOption);
    delay_s = commandline_parser.value(delayOption);
    if (delay_s.length() > 0)
        resolver_delay = delay_s.toFloat();

    const QStringList args = commandline_parser.positionalArguments();
    QString savedDir(settings.value("directory", ".").toString());

    switch (args.length())
    {
    case 1:
        if (chdir(args[0].toStdString().c_str()) == -1)
	{
	    cerr << argv[0] << ": Cannot chdir to " << args[0].toStdString().c_str() << endl;
	    exit(1);
	}
	/* FALLTHROUGH */
    case 0:
	if (chdir(savedDir.toStdString().c_str()) == -1)
	{
	    perror(savedDir.toStdString().c_str());
	    chdir(getenv("HOME"));
	}

    	break;
    default:
        cerr << "usage: " << argv[0] << " [directory]" << endl;
	exit(255);
    }

    // First step: load and update the location map
    update_index(resolver_delay, 100);

    QStringList filenames(locationmap->keys());
    filenames.sort(Qt::CaseInsensitive);

    /*
     * We now have:
     * - a list of locations for some of the images
     * - a directory with thumbnails for some (all?) of the images
     */

    // Next step: build viewer
    Viewer *v = new Viewer(filenames, locationmap, &settings);
    v->show();
    app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

    app.exec();
    return 0;
}

/*
 * NAME: update_index
 * PURPOSE: To update the location database of the current directory
 *	and create thumbnails
 * ARGUMENTS: resolver_delay: delay between requests to reverse geocoder
 *	max_requests: max number of requests this time
 * RETURNS: Nothing
 */
void
update_index(float resolver_delay, unsigned int max_requests)
{
    QFile inputFile(".location.csv");
    bool isModified = false;
    unsigned int count;
    locationmap = new QMap<QString,QString>;

    // Read current contents of location file
    if (inputFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
	QTextStream in(&inputFile);
	while (!in.atEnd())
	{
	    QString line = in.readLine();

	    while (1)
	        if (line.endsWith('\r') || line.endsWith('\n'))
		    line.remove(line.length()-1, 1);
		else
		    break;

	    QStringList fields = line.split(',');
	    fields.replaceInStrings("%2c", ",");
	    fields.replaceInStrings("%22", "\"");
	    fields.replaceInStrings("%25", "%");
	    for (QStringList::iterator e = fields.begin(); e != fields.end(); e++)
	    {
	        if (e->startsWith('"') && e->endsWith('"'))
		{
		    e->remove(0, 1);
		    e->remove(e->size()-1, 1);
		}
	        // qDebug() << "After replacement: \"" << e->toStdString().c_str() << "\"";
	    }

	    // Check if file still exists
	    if (access(fields[0].toStdString().c_str(), F_OK) == -1)
	    {
		isModified = 1;
	        continue;
	    }
	    locationmap->insert(fields[0], fields[1]);
	}
	inputFile.close();
    }

    // Now work through the list of files and augment the map
    QStringList wantedFiles;
    wantedFiles << "*.jpg";
    wantedFiles << "*.mov";

    QDirIterator it(".", wantedFiles, QDir::Files, 0);	// don't descend! QDirIterator::Subdirectories);

    count = 0;
    while (it.hasNext())
    {
	QString filename = it.next();
	// qDebug() << "File                 : " << filename;
	Exif exif(filename);
	// qDebug() << "Orientation          : " << exif.Orientation();

	// We deal with JPEG files only
	if (!filename.endsWith(".jpg", Qt::CaseInsensitive) && !filename.endsWith(".jpeg", Qt::CaseInsensitive))
	    continue;

	// Strip any leading "./"
	if (filename.startsWith("./"))
	    filename.remove(0, 2);

	exif.SaveThumbnail(QString(".thumbnails"), filename);

	// No need to go further if we already have a location
	if (locationmap->value(filename, QString()).length() != 0)
	    continue;
	    
	// Use "Unbekannt" as the location if we do not have any coodinates
	if (exif.Latitude() == 0.0 && exif.Longitude() == 0.0)
	{
	    locationmap->insert(filename, "Unbekannt");
	    isModified = true;
	}
	else	// Resolve coordinates into a location
	{
	    Resolver res;

	    // qDebug() << "Latitude             : " << exif.Latitude();
	    // qDebug() << "Longitude            : " << exif.Longitude();
	    // qDebug() << "Date/Time Original   : " << exif.Date();

	    if (count > 0 && resolver_delay > 0.0)
	        usleep(resolver_delay * (useconds_t) 1000000);
	        
	    QString location(res.Location(exif.Longitude(), exif.Latitude()));
	    if (location.length() != 0)
	    {
		isModified = true;
		locationmap->insert(filename, location);
	    }
	    if (count++ > max_requests)
	        break;
	}
    }
    if (isModified)
    {
        // qDebug() << "Map was modified, saving";
        saveMap(locationmap, ".location.csv");
    }

    return;
}

static QString
encode(QString s)
{
    QString result(s);

    result.replace("%", "%25");
    result.replace("\"", "%22");
    result.replace(",", "%2c");

    return result;
}

static void
saveMap(QMap<QString,QString> *map, QString filename)
{
    QFile outputFile(filename);

    if (outputFile.exists())
    {
        // qDebug() << "Location file exists, backing up";
	outputFile.rename(filename + "~");
    }

    if (outputFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
	QTextStream stream(&outputFile);
        // qDebug() << "Location database opened for writing";

	for (QMap<QString,QString>::iterator elem = map->begin(); elem != map->end(); elem++)
	    stream << '"' << encode(elem.key()) << "\",\"" << encode(elem.value()) << '"' << endl;
    }
}
