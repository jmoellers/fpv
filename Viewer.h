# ifndef	VIEWER_H
# define	VIEWER_H

# include	<QDialog>
# include       <QtWidgets>
# include	<QStringList>

using namespace std;

class Viewer : public QDialog {
    Q_OBJECT
public slots:
    void openDir();
public:
    Viewer(QStringList, QMap<QString,QString> *, QSettings *);
    ~Viewer();
private:
    void createMenu();
    void createBox(QStringList, QMap<QString,QString> *);
    QMenuBar *menuBar;
    QVBoxLayout *mainLayout;
    QMenu *fileMenu;
    QAction *exitAction,
    	*openAction;
    QGroupBox *groupbox;
    QSettings *settings;
};
# endif // VIEWER_H
