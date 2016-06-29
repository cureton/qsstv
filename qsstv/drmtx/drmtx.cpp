#include "drmtx.h"
#include "appglobal.h"
#include "soundbase.h"
#include "dispatcher.h"
#include "drmtransmitter.h"
#include "drmparams.h"
#include "reedsolomoncoder.h"
#include "hybridcrypt.h"
#include "configparams.h"
#include "txwidget.h"
#include "ftp.h"
#include "hybridcrypt.h"

#include <QFileInfo>




drmTx::drmTx(QObject *parent) :
  QObject(parent)
{
  txDRM=new drmTransmitter;
  txList.clear();
  notifyIntf=NULL;

}

drmTx::~drmTx()
{
  delete txDRM;
}

void drmTx::init()
{
  if(notifyIntf==NULL)
  {
      notifyIntf = new ftpInterface("HybridTXNotify");
      connect(notifyIntf, SIGNAL(notification(QString)), this, SLOT(rxNotification(QString)));
  }
}

void drmTx::start()
{
  txDRM->start(true);
}



bool drmTx::initDRMImage(bool binary,QString fileName)
{
  eRSType rsType;
  reedSolomonCoder rsd;
  QString fn;
  QString ext;
  QFileInfo finf;
  QFile inf;
  hybridCrypt hc;
  init();
  setTxParams(drmParams);
  // we need to save it as a jpg file
  if(binary)
    {
      finf.setFile(fileName);
    }
  else
    {
      finf.setFile(txWidgetPtr->getImageViewerPtr()->getFilename());
    }
  if(useHybrid)
    {
      fn="de_"+myCallsign+"-1-"+finf.baseName();
    }
  else
    {
      fn=QDateTime::currentDateTime().toUTC().toString("yyyyMMddHHmmss");
      fn+="-"+finf.baseName();
    }
  ext=finf.suffix();
  fixBlockList.clear();
  if(txList.count()>5) txList.removeFirst();
  txList.append(txSession());
  txList.last().filename=fn;
  txList.last().extension=ext;
  if(!useHybrid)
    {
      txList.last().drmParams=drmTxParameters;
      if(binary)
        {
          inf.setFileName(fileName);
          if(!inf.open(QIODevice::ReadOnly))
            {
              return false;
            }
          txList.last().ba=inf.readAll();

        }
      else
        {
          if(!txWidgetPtr->getImageViewerPtr()->copyToBuffer(&(txList.last().ba)))
          {
              return false;
          }
        }

      rsType=(eRSType)txList.last().drmParams.reedSolomon;
      baDRM=txList.last().ba;
      if(rsType!=RSTNONE)
        {
          rsd.encode(baDRM,txList.last().extension,rsType);
          txDRM->init(&baDRM,txList.last().filename,rsTypeStr[rsType],txList.last().drmParams);
        }
      else
        {
          txDRM->init(&baDRM,txList.last().filename,txList.last().extension,txList.last().drmParams);
        }
    }
  else
    {
      txList.last().drmParams.bandwith=1; // bw 2.2
      txList.last().drmParams.robMode=2;  // mode E
      txList.last().drmParams.interleaver=0; // long
      txList.last().drmParams.protection=0; // high
      txList.last().drmParams.qam=0; // 4bit QAM
      txList.last().drmParams.callsign=myCallsign;

      // we have to fill in the body
      txList.last().ba.clear();
      hc.enCrypt(&txList.last().ba);
      txDRM->init(&txList.last().ba,txList.last().filename,txList.last().extension,txList.last().drmParams);
      if(!ftpDRMHybrid(txList.last().filename+"."+finf.suffix()))
      {
        return false;
      }
      ftpDRMHybridNotifyCheck(txList.last().filename+"."+finf.suffix());
    }
  // transportID is set
  txList.last().transportID=txTransportID;
  return true;
}

bool drmTx::ftpDRMHybrid(QString fn)
{
//  char *data;
//  unsigned int ct;
  eftpError ftpResult;
  QByteArray testBA;
  QByteArray ba;
  QTemporaryFile ftmp;
  ftpInterface ftpIntf("HybridTX");
  hybridCrypt hc;
  ftpIntf.setupConnection(hc.host(),hc.port(),hc.user(),hc.passwd(),hc.dir()+"/"+hybridFtpHybridFilesDirectory);
  txWidgetPtr->getImageViewerPtr()->copyToBuffer(&ba);
  if(!ftmp.open()) return false;
  ftmp.write(ba);
  ftmp.close();

  ftpResult=ftpIntf.uploadFile(ftmp.fileName(),fn,true);
  switch(ftpResult)
    {
    case FTPCANCELED:
      ftpErrorStr="Connection Canceled";
    break;
    case FTPOK:
      break;
    case FTPERROR:
      ftpErrorStr=ftpIntf.getLastError();
      break;
    case FTPNAMEERROR:
      ftpErrorStr="Error in filename";
      break;
    case FTPTIMEOUT:
      ftpErrorStr="FTP timed out";
      break;
    }
  addToLog("sendDRMHybrid",LOGDRMTX);
  return ftpResult==FTPOK;
}


bool drmTx::ftpDRMHybridNotifyCheck(QString fn)
{
  txDRMNotifyEvent *txne;
  if (!enableHybridNotify) return false;
  
  if (enableHybridNotifySpecialServer) {
     notifyIntf->setupConnection(hybridNotifyRemoteHost, hybridNotifyPort,
              hybridNotifyLogin, hybridNotifyPassword,
              hybridNotifyRemoteDir+"/"+hybridNotifyDir);
     }
  else {
     // notification to vk4aes.com (EasyPal compatible)
     notifyIntf->setupConnection("vk4aes.com",21,"vk4aes","10mar1936","/RxOkNotifications1");
                    
     // or notification to image server
     //notifyIntf->setupConnection(hc.host(),hc.port(),hc.user(),hc.passwd(),hc.dir()+"/RxOkNotifications1");
     }
     
  txne = new txDRMNotifyEvent("");
  QApplication::postEvent( dispatcherPtr, txne );
  
  notifyIntf->startNotifyCheck(fn, 15, 60/15, true);
  return true;
}

void drmTx::rxNotification(QString info)
{
  if (info != "") {
     txDRMNotifyAppendEvent *txne = new txDRMNotifyAppendEvent(info);
     QApplication::postEvent( dispatcherPtr, txne );
  }
}


double  drmTx::calcTxTime(int overheadTime)
{
  double tim=0;
  //  tim= soundIOPtr->getPlaybackStartupTime();
  tim+=overheadTime;
  tim+=txDRM->getDuration();
  return tim;
}

int drmTx::processFIX(QByteArray bsrByteArray)
{
  int i,j;
  bool inSeries;
//  bool extended; // todo check use of extended
  bool done;
  int block;
  int trID,lastBlock;

  fixBlockList.clear();
  QString str(bsrByteArray);
  str.replace("\r","");
  //  information is in the QByteArray ba
  QStringList sl;
  sl=str.split("\n",QString::SkipEmptyParts);

  if(sl.at(1)!="H_OK")
    {
      return -1;
    }
  trID=sl.at(0).toUInt();
  lastBlock=sl.at(3).toUInt();
  fixBlockList.append(lastBlock++);
  inSeries=false;
  done=false;
//  extended=false;
  for(i=4;(!done)&&i<sl.count();i++)
    {
      block=sl.at(i).toInt();
      if(block==-99)
        {
          done=true;
          i++;
          break;
        }
      if(block<0) inSeries=true;
      else
        {
          if(inSeries)
            {
              inSeries=false;
              for(j=lastBlock;j<block;j++) fixBlockList.append(j);
            }
          fixBlockList.append(block);
          lastBlock=block+1;
        }
    }
  // check if we have a filename beyond -99
  if((i+1)<sl.count()) // we need an additional 2 entries (filename and mode)
    {
//      extended=true;
      //      fileName=sl.at(i++); // not used at this moment
    }
  return trID;
}


void drmTx::initDRMBSR(QByteArray *ba)
{
  baDRM=*ba;
  fixBlockList.clear();
  txDRM->init(&baDRM,"bsr","bin",drmTxParameters);
  addToLog(QString("bsr.bin send %1").arg(baDRM.size()),LOGPERFORM);
}

bool drmTx::initDRMFIX(txSession *sessionPtr)
{
  reedSolomonCoder rsd;
  eRSType rsType;
  rsType=(eRSType)sessionPtr->drmParams.reedSolomon;
  baDRM=sessionPtr->ba;
  if(rsType!=RSTNONE)
    {
      rsd.encode(baDRM,sessionPtr->extension,rsType);
      txDRM->init(&baDRM,sessionPtr->filename,rsTypeStr[rsType],sessionPtr->drmParams);
    }
  else
    {
      txDRM->init(&baDRM,sessionPtr->filename,sessionPtr->extension,sessionPtr->drmParams);
    }
  return true;
}


void drmTx::sendBSR(QByteArray *p,drmTxParams dp)
{
  setTxParams(dp);
  initDRMBSR(p);
  dispatcherPtr->startTX(txFunctions::TXSENDDRMBSR);
  addToLog("sendDRMBSR",LOGDRMTX);
}

txSession *drmTx::getSessionPtr(uint transportID)
{
  int i;
  for(i=0;i<txList.count();i++)
    {
      if(txList.at(i).transportID==transportID)
        {
          return &txList[i];
        }
    }
  return NULL;
}

void drmTx::applyTemplate(QString templateFilename, bool useTemplate, imageViewer *ivPtr)
{
  ivPtr->setParam(templateFilename,useTemplate);
}
