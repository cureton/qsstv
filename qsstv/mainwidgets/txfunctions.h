#ifndef TXFUNCTIONS_H
#define TXFUNCTIONS_H

#include <QThread>
#include "appdefs.h"
#include "drmparams.h"
#include "testpatternselection.h"

#define SILENCEDELAY 0.600           // send silence after transmission

extern int templateIndex;
extern bool useTemplate;
extern bool useCW;
extern bool useVOX;

class sstvTx;
class drmTx;
class imageViewer;



class txFunctions  : public QThread
{
  Q_OBJECT
public:
  enum etxState
  {
    TXIDLE,	//!< in idle loop
    TXACTIVE,
    TXSENDTONE,
    TXSENDID,
    TXSENDDRM,
    TXSENDDRMBINARY,
    TXSENDDRMBSR,
    TXSENDDRMFIX,
    TXSENDDRMTXT,
    TXSSTVIMAGE,
    TXSSTVPOST,
    TXRESTART,
    TXTEST

  };
  txFunctions(QObject *parent);
  ~txFunctions();
  void init();
  void run();
  void stopThread();
  void startTX(etxState state);
  void stopAndWait();
  void setToneParam(double duration,double lowerFreq,double upperFreq=0)
  {
    toneDuration=duration;
    toneLowerFrequency=lowerFreq;
    toneUpperFrequency=upperFreq;
  }
  bool prepareFIX(QByteArray bsrByteArray);
  bool prepareBinary(QString fileName);
  void sendBSR(QByteArray *p,drmTxParams dp);
  void applyTemplate(imageViewer *ivPtr,QString templateFilename);
  etxState getTXState() { return txState;}
  void setDRMTxParams(drmTxParams params);
  //  bool initDRMFIX(txSession *sessionPtr);
  void txTestPattern(imageViewer *ivPtr, etpSelect sel);


private:
  void waitTxOn();
  void waitEnd();
  void sendCW();
  void sendFSKID();
  void sendTestPattern();
  void syncBurst();
  void sendFSKChar(int IDChar);
  void switchTxState(etxState newState);
  void startProgress(double duration);
  etxState txState;
  bool started;
  bool abort;
  double toneDuration;
  double toneLowerFrequency;
  double toneUpperFrequency;
  QString binaryFilename;
  sstvTx *sstvTxPtr;
  drmTx *drmTxPtr;
};

#endif // TXFUNCTIONS_H
