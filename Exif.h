# ifndef	EXIF_H
# define	EXIF_H
# include	<libexif/exif-loader.h>
# include	<QString>

using namespace std;

class Exif {
    QString pathname;
    double longitude;
    double latitude;
    int orientation;
    ExifData *ed;
    ExifData *load_exif_data(void);
public:
    Exif(QString);
    ~Exif();
    double Longitude();
    double Latitude();
    QString Date();
    QString Orientation();
    void SaveThumbnail(QString directory, QString filename);
};
# endif // EXIF_H
