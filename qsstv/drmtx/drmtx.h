#ifndef DRMTX_H
#define DRMTX_H


#include <QObject>

#include "drmtransmitter.h"
#include "reedsolomoncoder.h"
#include "ftp.h"

class imageViewer;

struct txSession
{
  drmTxParams drmParams;
  QByteArray ba;  // contains the image data in jpg, jp2 .... format
  uint transportID;
  QString filename;
  QString extension;
};



class drmTransmitter;

class drmTx : public QObject
{
  Q_OBJECT
public:
  explicit drmTx(QObject *parent = 0);
  ~drmTx();
  void init();
  void start();
  bool initDRMImage(bool binary, QString fileName);
  bool ftpDRMHybridNotifyCheck(QString fn);
  
  void sendBSR(QByteArray *p,drmTxParams dp);
  int processFIX(QByteArray bsrByteArray);
  void initDRMBSR(QByteArray *ba);
  bool initDRMFIX(txSession *sessionPtr);
//  bool initDRMFIX(QString fileName,QString extension,eRSType rsType,int mode);
  txSession *getSessionPtr(uint transportID);
  void applyTemplate(QString templateFilename, bool useTemplate, imageViewer *ivPtr);


  void setTxParams(drmTxParams params)
  {
    drmTxParameters=params;
  }
 double calcTxTime(int overheadTime);

signals:

private slots:
  void rxNotification(QString info);

private:
    void runRx();
    bool ftpDRMHybrid(QString fn);
    drmTransmitter *txDRM;
    QList <txSession> txList;
    drmTxParams drmTxParameters;
    QByteArray baDRM;
    QString ftpErrorStr;
    ftpInterface *notifyIntf;
};

#endif // DRMTX_H
