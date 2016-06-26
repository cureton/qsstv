/***************************************************************************
 *   Copyright (C) 2000-2008 by Johan Maes                                 *
 *   on4qz@telenet.be                                                      *
 *   http://users.telenet.be/on4qz                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/*!
  The dispatcher is the central system that routes all messages from the different threads.
It also starts, stops and synchronizes the threads.

*/
#include "dispatcher.h"
#include "appglobal.h"
#include "configparams.h"
#include "mainwidgets/rxwidget.h"
#include "mainwidgets/txwidget.h"
#include "mainwidgets/gallerywidget.h"
#include "widgets/spectrumwidget.h"
#include "widgets/vumeter.h"
#include "rxfunctions.h"
#include "mainwindow.h"
#include "utils/ftp.h"
#include "rigcontrol.h"
#include "logbook/logbook.h"
#include "dirdialog.h"
#include <QSettings>
#include <QMessageBox>



/*!
creates dispatcher instance
 */

dispatcher::dispatcher()
{
  mbox=NULL;

  progressFTP=NULL;
  lastFileName.clear();
  prTimerIndex=0;
}

/*!
delete dispatcher instance
 */

dispatcher::~dispatcher()
{

}

void dispatcher::init()
{
  //  if (mbox==NULL)
  //    {
  ////      mbox = new QMessageBox(mainWindowPtr);
  //      mbox = new QMessageBox(0);
  //    }
  editorActive=false;
  infoTextPtr=new textDisplay(mainWindowPtr);
  mainWindowPtr->spectrumFramePtr->init(RXSTRIPE,1,BASESAMPLERATE/SUBSAMPLINGFACTOR);
  infoTextPtr->hide();

}


/*!
  All communication between the threads are passed via this eventhandler.
*/

void dispatcher::customEvent( QEvent * e )
{
  dispatchEventType type;
  QString fn;
  type=(dispatchEventType)e->type();
  addToLog(((baseEvent*)e)->description,LOGDISPATCH);
  switch(type)
  {
  case displayFFT:
    addToLog("dispatcher: displayFFT",LOGDISPATCH);
    mainWindowPtr->spectrumFramePtr->realFFT(((displayFFTEvent*)e)->data());
    rxWidgetPtr->vMeterPtr()->setValue(soundIOPtr->getVolumeDb());
    //      addToLog(QString::number(soundIOPtr->getVolumeDb()),LOGALL);

    break;
  case displaySync:
    // addToLog("dispatcher: displaySync",LOGDISPATCH);
    uint s;
    ((displaySyncEvent*)e)->getInfo(s);
    rxWidgetPtr->sMeterPtr()->setValue((double)s);
    break;
  case rxSSTVStatus:
    rxWidgetPtr->setSSTVStatusText(((statusMsgEvent*)e)->getStr());
    break;

  case startImageRX:
    addToLog("dispatcher: clearing RxImage",LOGDISPATCH);
    //      rxWidgetPtr->getImageViewerPtr()->createImage( ((startImageRXEvent*)e)->getSize(),QColor(0,0,128),imageStretch);
    rxWidgetPtr->getImageViewerPtr()->createImage( ((startImageRXEvent*)e)->getSize(),imageBackGroundColor,imageStretch);
    break;

  case lineDisplay:
  {
    rxWidgetPtr->getImageViewerPtr()->displayImage();
  }
    break;
  case endSSTVImageRX:
    if(autoSave)
    {
      addToLog("dispatcher:endImage savingRxImage",LOGDISPATCH);
      saveRxSSTVImage(((endImageSSTVRXEvent*)e)->getModeName());
    }
    break;


  case rxDRMStatus:
    rxWidgetPtr->setDRMStatusText(((statusMsgEvent*)e)->getStr());

    break;
  case callEditor:
    if(editorActive) break;
    editorActive=true;
    ed=new editor();
    ed->show();
    iv=((callEditorEvent*)e)->getImageViewer();
    addToLog (QString(" callEditorEvent imageViewPtr: %1").arg(QString::number((ulong)iv,16)),LOGDISPATCH);
    addToLog(QString("editor: filename %1").arg(((callEditorEvent*)e)->getFilename()),LOGDISPATCH);
    ed->openFile(((callEditorEvent*)e)->getFilename());
    break;

  case rxDRMNotify:
    rxWidgetPtr->setDRMNotifyText(((rxDRMNotifyEvent*)e)->getStr());
    break;
  case rxDRMNotifyAppend:
    rxWidgetPtr->appendDRMNotifyText(((rxDRMNotifyAppendEvent*)e)->getStr());
    break;
  case txDRMNotify:
    txWidgetPtr->setDRMNotifyText(((txDRMNotifyEvent*)e)->getStr());
    break;
  case txDRMNotifyAppend:
    txWidgetPtr->appendDRMNotifyText(((txDRMNotifyAppendEvent*)e)->getStr());
    break;


  case editorFinished:
    if(!editorActive) break;
    if(((editorFinishedEvent*)e)->isOK())
    {
      addToLog (QString(" editorFinishedEvent imageViewPtr: %1").arg(QString::number((ulong)iv,16)),LOGDISPATCH);
      iv->reload();
    }
    editorActive=false;
    delete ed;
    break;

  case templatesChanged:
    galleryWidgetPtr->changedMatrix(imageViewer::TEMPLATETHUMB);
    txWidgetPtr->setupTemplatesComboBox();
    break;

  case progressTX:
    txTimeCounter=0;
    addToLog(QString("dispatcher: progress duration=%1").arg(((progressTXEvent*)e)->getInfo()),LOGDISPATCH);
    prTimerIndex=startTimer(((progressTXEvent*)e)->getInfo()*10); // time in seconds -> times 1000 for msec,divide by 100 for progress
    break;

  case stoppingTX:
    addToLog("dispatcher: endTXImage",LOGDISPATCH);
    break;

  case endImageTX:
    //addToLog("dispatcher: endTXImage",LOGDISPATCH);
    while(soundIOPtr->isPlaying())
    {
      qApp->processEvents();
    }
    addToLog("dispatcher: endTXImage",LOGDISPATCH);
    startRX();
    break;

  case displayDRMInfo:
    if(!slowCPU)
    {
      rxWidgetPtr->mscWdg()->setConstellation(MSC);
      rxWidgetPtr->facWdg()->setConstellation(FAC);
    }
    rxWidgetPtr->statusWdg()->setStatus();
    break;

  case displayDRMStat:
    DSPFLOAT s1;
    ((displayDRMStatEvent*)e)->getInfo(s1);
    rxWidgetPtr->sMeterPtr()->setValue(s1);
    break;

  case loadRXImage:
  {
    QString fn=((loadRXImageEvent *)e)->getFilename();
    rxWidgetPtr->getImageViewerPtr()->openImage(fn,false,false,false);
  }
    break;
  case moveToTx:
  {
    txWidgetPtr->setImage(((moveToTxEvent *)e)->getFilename());
  }
    break;
  case saveDRMImage:
  {
    ((saveDRMImageEvent*)e)->getFilename(fn);
    if(!rxWidgetPtr->getImageViewerPtr()->openImage(fn,false,false,false))
    {
      if(mbox==NULL) delete mbox;
      mbox = new QMessageBox(mainWindowPtr);
      mbox->setWindowTitle("Received file");
      mbox->setText(QString("Saved file %1").arg(fn));
      mbox->show();
      QTimer::singleShot(4000, mbox, SLOT(hide()));
      break;
    }
    saveImage(fn);
  }
    break;

  case prepareFix:
    addToLog("prepareFix",LOGDISPATCH);
    startDRMFIXTx( ((prepareFixEvent*)e)->getData());
    break;
  case displayText:
    infoTextPtr->clear();
    infoTextPtr->setWindowTitle(QString("Received from %1").arg(drmCallsign));
    infoTextPtr->append(((displayTextEvent*)e)->getStr());
    infoTextPtr->show();
    break;

  case displayMBox:
    if(mbox==NULL) delete mbox;
    mbox = new QMessageBox(mainWindowPtr);
    mbox->setWindowTitle(((displayMBoxEvent*)e)->getTitle());
    mbox->setText(((displayMBoxEvent*)e)->getStr());
    mbox->show();
    QTimer::singleShot(4000, mbox, SLOT(hide()));
    break;

  case displayProgressFTP:
  {
    if(((displayProgressFTPEvent*)e)->getTotal()==0)
    {
      delete progressFTP;
      progressFTP=NULL;
      break;
    }
    if(progressFTP==NULL)
    {
      progressFTP=new QProgressDialog("FTP Transfer","Cancel",0,0,mainWindowPtr);
    }
    progressFTP->show();
    progressFTP->setMaximum(((displayProgressFTPEvent*)e)->getTotal());
    progressFTP->setValue(((displayProgressFTPEvent*)e)->getBytes());
  }
    break;
  default:
    addToLog(QString("unsupported event: %1").arg(((baseEvent*)e)->description), LOGALL);
    break;
  }
  ((baseEvent *)e)->setDone();
}


void dispatcher::idleAll()
{
  if(prTimerIndex>=0)
  {
    killTimer(prTimerIndex);
    prTimerIndex=-1;
    txWidgetPtr->setProgress(0);
  }
  rigControllerPtr->activatePTT(false);
  rxWidgetPtr->functionsPtr()->stopAndWait();
  txWidgetPtr->functionsPtr()->stopAndWait();
}


void dispatcher::startRX()
{
  idleAll();
  soundIOPtr->startCapture();
  rxWidgetPtr->functionsPtr()->startRX();
}

void dispatcher::startTX(txFunctions::etxState state)
{
  idleAll();
  rigControllerPtr->activatePTT(true);
  soundIOPtr->startPlayback();
  txWidgetPtr->functionsPtr()->startTX(state);
}

void dispatcher::startDRMFIXTx(QByteArray ba)
{
  if(!txWidgetPtr->functionsPtr()->prepareFIX(ba)) return;
  startTX(txFunctions::TXSENDDRMFIX);
}

void dispatcher::startDRMTxBinary()
{
  dirDialog d((QWidget *)mainWindowPtr,"Binary File");
  QString filename=d.openFileName("","*");
  if(filename.isEmpty()) return;
  if(!txWidgetPtr->functionsPtr()->prepareBinary(filename)) return;
  startTX(txFunctions::TXSENDDRMBINARY);
}



void dispatcher::logSSTV(QString call,bool fromFSKID)
{
  if(lastFileName.isEmpty())
  {
    return;
  }
  if(fromFSKID)
  {
    QDateTime dt(QDateTime::currentDateTime().toUTC());
    int diffsec=saveTimeStamp.secsTo(dt);
    if(diffsec<2)
    {
      logBookPtr->logQSO(call,"SSTV",lastFileName);
    }
  }
  else
  {
    logBookPtr->logQSO(call,"SSTV","");
  }

}


void dispatcher::saveRxSSTVImage(QString shortModeName)
{
  QString s,fileName;
  QDateTime dt(QDateTime::currentDateTime().toUTC()); //this is compatible with QT 4.6
  dt.setTimeSpec(Qt::UTC);
  if (shortModeName.isEmpty())
  {
    lastFileName.clear();
    return;
  }
  if(!autoSave)
  {
    lastFileName=shortModeName;
  }
  else
  {
    fileName=QString("%1/%2_%3.%4").arg(rxSSTVImagesPath).arg(shortModeName).arg(dt.toString("yyyyMMdd_HHmmss")).arg(defaultImageFormat);
    addToLog(QString("dispatcher: saveRxImage():%1 ").arg(fileName),LOGDISPATCH);
    rxWidgetPtr->getImageViewerPtr()->save(fileName,defaultImageFormat,true,false);
    saveImage(fileName);
    lastFileName=QString("%1_%2.%3").arg(shortModeName).arg(dt.toString("yyyyMMdd_HHmmss")).arg(defaultImageFormat);
    saveTimeStamp= dt;
  }
}

void dispatcher::saveImage(QString fileName)
{
  QFileInfo info(fileName);
  eftpError ftpResult;
  displayMBoxEvent *stmb=0;
  QString fn="/tmp/"+info.baseName()+"."+ftpDefaultImageFormat;
  galleryWidgetPtr->putRxImage(fileName);
  txWidgetPtr->setPreviewWidget(fileName);
  if(enableFTP)
  {
    ftpInterface ftpIntf("Save RX Image");

    if(transmissionModeIndex==TRXSSTV)
    {
      rxWidgetPtr->getImageViewerPtr()->save(fn,ftpDefaultImageFormat,true,false);
      ftpIntf.setupConnection(ftpRemoteHost,ftpPort,ftpLogin,ftpPassword,ftpRemoteSSTVDirectory);
    }
    else
    {
      rxWidgetPtr->getImageViewerPtr()->save(fn,ftpDefaultImageFormat,true,true);
      ftpIntf.setupConnection(ftpRemoteHost,ftpPort,ftpLogin,ftpPassword,ftpRemoteDRMDirectory); // TO CHECK JOMA
    }
    ftpResult=ftpIntf.uploadToRXServer(fn);
    switch(ftpResult)
    {
    case FTPOK:
      break;
    case FTPERROR:
      stmb= new displayMBoxEvent("FTP Error",QString("Host: %1: %2").arg(ftpRemoteHost).arg(ftpIntf.getLastError()));
      break;
    case FTPNAMEERROR:
      stmb= new displayMBoxEvent("FTP Error",QString("Host: %1, Error in filename").arg(ftpRemoteHost));
      break;
    case FTPCANCELED:
      stmb= new displayMBoxEvent("FTP Error",QString("Connection to %1 Canceled").arg(ftpRemoteHost));
      break;
    case FTPTIMEOUT:
      stmb= new displayMBoxEvent("FTP Error",QString("Connection to %1 timed out").arg(ftpRemoteHost));
      break;
    }
    if(ftpResult!=FTPOK)
    {
      QApplication::postEvent( dispatcherPtr, stmb );  // Qt will delete it when done
      return;
    }

  }
}



void dispatcher::timerEvent(QTimerEvent *event)
{
  if(event->timerId()==prTimerIndex)
  {
    txWidgetPtr->setProgress(++txTimeCounter);
    if(txTimeCounter>=100)
    {
      if(prTimerIndex>=0)
      {
        killTimer(prTimerIndex);
        prTimerIndex=-1;
        txWidgetPtr->setProgress(0);
      }
    }
    txWidgetPtr->setProgress(txTimeCounter);
  }
}
