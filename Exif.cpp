# include	<sys/stat.h>
# include	<sys/types.h>
# include	<stdio.h>
# include	<unistd.h>
# include	<QFile>
# include	<QDebug>
# include	<QDataStream>
# include	<QString>
# include	"Exif.h"

static double exif_convert_latlon(char *s);

Exif::Exif(QString pn)
{
    pathname = pn;

    longitude = 0.0;
    latitude = 0.0;
    ed = NULL;
    orientation = -1;
}

Exif::~Exif()
{
    if (ed != NULL)
        exif_data_free(ed);
}

QString
Exif::Orientation()
{
    char buffer[128];

    if (orientation == -1)
    { 
	if (ed == NULL)
	    if ((ed = load_exif_data()) == NULL)
	        return 0;

        if (exif_content_get_value(ed->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION, buffer, sizeof(buffer)) != NULL)
	    return QString(buffer);
    }

    return QString("");
}

double
Exif::Longitude()
{
    char buffer[128];

    if (longitude == 0.0)
    {
	if (ed == NULL)
	    if ((ed = load_exif_data()) == NULL)
	        return 0.0;

	if (exif_content_get_value(ed->ifd[EXIF_IFD_GPS], (ExifTag) EXIF_TAG_GPS_LONGITUDE, buffer, sizeof(buffer)) != NULL)
	{
	    longitude = exif_convert_latlon(buffer);
	    if (exif_content_get_value(ed->ifd[EXIF_IFD_GPS], (ExifTag) EXIF_TAG_GPS_LONGITUDE_REF, buffer, sizeof(buffer)) == NULL)
		longitude = 0.0;
	    else if (buffer[0] == 'W')
		longitude = -longitude;
	}
    }

    return longitude;
}

double
Exif::Latitude()
{
    char buffer[128];

    if (latitude == 0.0)
    {
	if (ed == NULL)
	    if ((ed = load_exif_data()) == NULL)
	        return 0.0;

	if (exif_content_get_value(ed->ifd[EXIF_IFD_GPS], (ExifTag) EXIF_TAG_GPS_LATITUDE, buffer, sizeof(buffer)) != NULL)
	{
	    latitude = exif_convert_latlon(buffer);
	    if (exif_content_get_value(ed->ifd[EXIF_IFD_GPS], (ExifTag) EXIF_TAG_GPS_LATITUDE_REF, buffer, sizeof(buffer)) == NULL)
		latitude = 0.0;
	    else if (buffer[0] == 'S')
		latitude = -latitude;
	}
    }

    return latitude;
}

QString
Exif::Date()
{
    char buffer[128];
    QString date("");

    if (ed == NULL)
	if ((ed = load_exif_data()) == NULL)
	    return date;

    // The format is "YYYY:MM:DD HH:MM:SS"
    // [https://www.awaresystems.be/imaging/tiff/tifftags/privateifd/exif/datetimeoriginal.html]
    if (exif_content_get_value(ed->ifd[EXIF_IFD_EXIF], (ExifTag) EXIF_TAG_DATE_TIME_ORIGINAL, buffer, sizeof(buffer)) != NULL)
    {
	// YYYY:MM:DD HH:MM:SS
	// 0123456789
        if (buffer[8] != '0')
	    date.append(buffer[8]);
	date.append(buffer[9]);
	date.append('.');
	if (buffer[5] != '0')
	    date.append(buffer[5]);
	date.append(buffer[6]);
	date.append('.');
	for (int i = 0; i < 4; i++)
	    date.append(buffer[i]);
    }

    return date;
}

void
Exif::SaveThumbnail(QString directory, QString filename)
{
    QString pathname(directory + "/" + filename);

    if (ed == NULL)
        if ((ed = load_exif_data()) == NULL)
	    return;
        
    // Save the thumbnail if there is one
    if (ed->data != NULL && ed->size != 0)
    {
	QFile thumbnailFile(pathname);
	const char *dirname = directory.toStdString().c_str();

	if (!thumbnailFile.exists())
	{
	    if (access(dirname, W_OK) == -1)
	    {
		if (errno == ENOENT)
		    mkdir(dirname, 0755);
		else
		    return;
	    }

	    // qDebug() << "Saving thumbnail";
	    if (thumbnailFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	    {
		QDataStream stream(&thumbnailFile);
		stream.writeRawData((char *) ed->data, ed->size);
	    }
	    // qDebug() << "Done saving";
	}
    }
}

ExifData *
Exif::load_exif_data()
{
    ExifLoader *el;

    // Get a new Exif Loader instance
    if ((el = exif_loader_new()) == NULL)
	return NULL;

    exif_loader_write_file(el, pathname.toStdString().c_str());
    ed = exif_loader_get_data(el);
    exif_loader_unref(el);	// The loader is no longer needed--free it

    return ed;
}
	    
# include	<string.h>
/*
 * NAME: exif_convert_latlon
 * PURPOSE: To convert a longitude or latitude in the form "degrees, minutes, seconds"
 *	into a double
 * ARGUMENTS: s: string containing degrees, minutes, seconds
 * RETURNS: double
 */
static double
exif_convert_latlon(char *s)
{
    double latlon;
    char *next;

    latlon = strtod(s, &next);
    latlon += strtod(next+1, &next) / 60.0;
    latlon += strtod(next+1, &next) / 3600.0;

    return latlon;
}
