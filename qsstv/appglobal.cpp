#include "appglobal.h"
#include "logging.h"
#include "soundbase.h"
#include <QPixmap>
#include <QCursor>


const QString MAJORVERSION  = "9.1";
const QString CONFIGVERSION = "9.0";
const QString MINORVERSION  = ".7";
const QString LOGVERSION = ("qsstv."+MAJORVERSION+MINORVERSION+".log");
const QString ORGANIZATION = "ON4QZ";
const QString APPLICATION  = ("qsstv_" +CONFIGVERSION);
const QString qsstvVersion=QString("QSSTV " + MAJORVERSION+MINORVERSION);
const QString shortName=QString("QSSTV");


QSplashScreen *splashPtr;
QString splashStr;

mainWindow *mainWindowPtr;
soundBase *soundIOPtr;
logFile *logFilePtr;
configDialog *configDialogPtr;



dispatcher *dispatcherPtr;
QStatusBar *statusBarPtr;
rxWidget *rxWidgetPtr;
txWidget *txWidgetPtr;
galleryWidget *galleryWidgetPtr;
waterfallText *waterfallPtr;
rigControl *rigControllerPtr;
xmlInterface *xmlIntfPtr;
logBook *logBookPtr;

bool useHybrid;





etransmissionMode transmissionModeIndex;  // SSTV , DRM



QPixmap *greenPXMPtr;
QPixmap *redPXMPtr;

#ifndef QT_NO_DEBUG
scopeView *scopeViewerData;
scopeView *scopeViewerSyncNarrow;
scopeView *scopeViewerSyncWide;
#endif


void globalInit()
{
  logFilePtr=new logFile();
  logFilePtr->open(LOGVERSION);
  QSettings qSettings;
  qSettings.beginGroup("MAIN");
  logFilePtr->readSettings();
  greenPXMPtr=new QPixmap(16,16);
  greenPXMPtr->fill(Qt::green);
  redPXMPtr=new QPixmap(16,16);
  redPXMPtr->fill(Qt::red);
  qSettings.endGroup();
}

void globalEnd(void)
{
  logFilePtr->close();
}

