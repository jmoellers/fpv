# include	<stdio.h>
# include	<curl/curl.h>
# include	<iostream>
# include	<QRegularExpressionMatch>
# include	<QDebug>
# include	"Resolver.h"

using namespace std;

struct MemoryStruct {
    char *memory;
    size_t size;
};

/*
 * See also
 * https://developer.mapquest.com/documentation/open/nominatim-search/
 * https://opencagedata.com/
 * https://locationiq.com/
 */
Resolver::Resolver()
{
}

Resolver::~Resolver()
{
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
QString resolve_pattern(QString pat, QString s);

/*
 * The following is a list of specifications which fields from the
 * response of the reverse geolocator we want and how they are to be composed
 * The names in brackets are field names in the XML.
 * A specification is used only if ALL of the fields are
 * present.
 * alternatives can be specified by "or-ing" them.
 * NOTE that the list is used sequentially, so put longer patterns first
 */
static QString pattern[] = {
    "<road> <house_number?>, <postcode?> <city|town|village|city_district>, <country>",
    "<pedestrian>, <postcode?> <city|town|village|city_district>, <country>",
    "<footway|path|locality|cycleway|suburb>, <postcode?> <city|town|village|city_district>, <country>",
    // "<path>, <postcode?> <city|town|village|city_district>, <country>",
    // "<locality>, <postcode?> <city|town|village|city_district>, <country>",
    // "<cycleway>, <postcode?> <city|town|village|city_district>, <country>",
    // "<suburb>, <postcode?> <city|town|village|city_district>, <country>",
    // "<suburb>, <city|town|village|city_district>, <country>",
    "<city|town|village|city_district>, <country>",
    ""
};

/*
 * NAME: Location
 * PURPOSE: To reverse geoencode a location given its longitude and latitude
 * ARGUMENTS: lon: the location's longitude
 *	lat: the location's latitude
 * RETURNS: a string describing the location
 */
QString
Resolver::Location(double lon, double lat)
{
    CURL *curl = curl_easy_init();
    char url[1024];
    struct MemoryStruct membuffer;
    int i;
    QString location;

    if (!curl)
	return QString("");

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Fruity Picture Viewer/0.1");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    membuffer.memory = NULL;
    membuffer.size = 0;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &membuffer);
    snprintf(url, sizeof(url), "http://nominatim.openstreetmap.org/reverse?format=xml&lat=%f&lon=%f&zoom=18&addressdetails=1", lat, lon);
    qDebug() << "URL=" << url;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_perform(curl);
    // fprintf(stderr, "%s", membuffer.memory);

    QString s(membuffer.memory);
    for (i = 0; pattern[i].length() != 0; i++)
    {
	location = resolve_pattern(pattern[i], s);

	if (location.length())
	{
	    // remove multiple white space that may have been a result
	    // of an optional tag (eg postcode).
	    location.replace(QRegExp("\\s+"), " ");
	    // remove white specae preceding a comma
	    // that may have been a result of an optional tag (eg house_number)
	    location.replace(QRegExp("\\s+,"), ",");
	    break;
	}
    }

    if (location.length() == 0)
    {
	cerr << "********\n" << s.toStdString().c_str() << "\n********\n";
	QString lons, lats;
	lons.setNum(lon);
	lats.setNum(lat);
	location = QString("Unbekannt") + " (" + lons + "/" + lats + ")";
    }

    free(membuffer.memory);

    curl_easy_cleanup(curl);

    return location;
}

/*
 * NAME: WriteMemoryCallback
 * PURPOSE: To append a string to a given buffer, extending the buffer
 * ARGUMENTS: contents: data to append
 *	size: size of portion to append
 *	nmemb: number of <size> portions
 *	userp: address of struct MemoryStruct describing the buffer
 * RETURNS: size * nmemb on success, 0 on failure
 * NOTE: This is the WRITEFUNCTION callback of curl
 */
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = (char *) realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
	/* out of memory! */ 
	printf("not enough memory (realloc returned NULL)\n");
	return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

/*
 * NAME: resolve_pattern
 * PURPOSE: To resolve a given pattern using data from an XML structure
 * ARGUMENTS: pat: pattern, eg "<pedestrian>, <postcode?> <city>, <country>"
 *	xml: XML string to use
 * RETURNS: resolved string if ALL fields in the pattern can be resolved
 *	else returns empty string
 */
QString
resolve_pattern(QString pat, QString xml)
{
    //     "<road> <house_number>, <postcode?> <city>, <country>",
    int len = pat.length(), i = 0;
    QString result;
    // qDebug() << "resolve_pattern(\"" << pat << "\",\"" << "..." << "\")";
    // qDebug() << "xml=" << xml;

    while (i < len)
    {
        QString c, tag;
	QRegularExpression re;
	int end;
	bool match, optional;

        c = pat[i];
	if (c != "<") {
	    // qDebug() << "  Appending '" << c << "'";
	    result += c;
	    i++;
	    continue;
	}
	end = pat.indexOf('>', i);
	if (end == -1)
	    break;

	tag = pat.mid(i+1, end-i-1);
	// qDebug() << "tag=" << tag;
	// If the tag ends with a '?', this tag is optional
	if (optional = tag.endsWith('?')) {
	    // qDebug() << "    is optional";
	    tag.chop(1);	// peel off the '?'
	}

	QStringList alternatives(tag.split('|'));
	match = false;
	for (QStringList::iterator alternative = alternatives.begin(); alternative != alternatives.end(); alternative++)
	{
	    // qDebug() << "    checking alternative" << *alternative;
	    re = QRegularExpression("<" + *alternative + ">([^>]+)</" + *alternative + ">");
	    if (match = re.match(xml).hasMatch())
	    {
		// qDebug() << "        has a match" << re.match(xml).captured(1);
		break;
	    }
	}
	if (!optional && !match)
	    break;

	// optional || match
	if (match)
	    result += re.match(xml).captured(1);

	i = end + 1;
    }

    return (i >= len) ? result : QString("");
}
