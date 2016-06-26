/***************************************************************************
 *   Copyright (C) 2004 by Johan Maes                                      *
 *   on4qz@telenet.be                                                      *
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
#include "ftp.h"
#include "appglobal.h"
#include "dispatch/dispatcher.h"

//#include <qfiledialog.h>

#include <qapplication.h>
#include "configparams.h"

#include <QDebug>
#include <QTemporaryFile>
#include <string.h>
#include <errno.h>

#define FTPTIMEOUTTIME 12000

ftpInterface::ftpInterface(QString id)
{
  ftp=NULL;
  addToLog(id, LOGFTP);
  name=id;
  sourceFn=NULL;
  init();
}

ftpInterface::~ftpInterface()
{
  addToLog(QString("'%1' FTP destroy in delete").arg(name),LOGFTP);
  destroy();
  if (ftp) delete ftp;
}

void ftpInterface::init()
{
  destroy();
  if(ftp) delete ftp;
  addToLog(QString("FTP init '%1'").arg(name), LOGFTP);
  ftp = new QFtp( 0);
  connect( ftp, SIGNAL(commandStarted(int)),SLOT(ftp_commandStarted(int)) );
  connect( ftp, SIGNAL(commandFinished(int,bool)),SLOT(ftp_commandFinished(int,bool)) );
  connect( ftp, SIGNAL(done(bool)),SLOT(ftp_done(bool)) );
  connect( ftp, SIGNAL(stateChanged(int)),SLOT(ftp_stateChanged(int)) );
  connect( ftp, SIGNAL(listInfo(const QUrlInfo &)),SLOT(ftp_listInfo(const QUrlInfo &)) );
  connect( ftp, SIGNAL(rawCommandReply(int, const QString &)),SLOT(ftp_rawCommandReply(int, const QString &)) );
  connect( ftp, SIGNAL(dataTransferProgress(qint64,qint64)),SLOT(slotProgress(qint64,qint64)) );
}

void ftpInterface::destroy()
{
  mremove_listids.clear();
  listingResults.clear();
  notifyId=-1;
  ftpDone=true;
  if(ftp)
    {
      addToLog("FTP show state in destroy",LOGFTP);
      ftp_stateChanged(ftp->state());
      if( ftp->state() != QFtp::Unconnected ) 
        {
          addToLog(QString("Closing '%1' to %2").arg(name).arg(host),LOGFTP);
          ftp->close();
        }
    }
}

void ftpInterface::setupConnection(QString tHost,int tPort,QString tUser,QString tPasswd,QString tDirectory)
{
    addToLog(QString("'%1' host %2").arg(name).arg(tHost), LOGFTP);
    if(host!=tHost || user!=tUser || passwd!=tPasswd || port!=tPort || directory!=tDirectory)
      {
        if (ftp) destroy();
      }
    host=tHost;
    user=tUser;
    passwd=tPasswd;
    directory=tDirectory;
    port=tPort;
}

eftpError ftpInterface::doConnect()
{
  aborting=false;
  addToLog(QString("'%1' to %2").arg(name).arg(host),LOGFTP);
  //dumpState();
  
  // if !ftpDone then we've had at least one command issued, which means
  // that first command caused doConnect() to run and call connectToHost()
  // if !isUnconnected() then the connection has already been established.
  if(isUnconnected() && ftpDone)
    {
      addToLog(QString("FTP connect to host %1").arg(host),LOGFTP);
      connectToHost();
    }
  else
    {
      addToLog(QString("FTP already connected to host %1").arg(host),LOGFTP);
    }
  return FTPOK;
}


eftpError ftpInterface::uploadFile(QString fileName,QString targetFilename,bool reconnect)
{
//  int id;
  eftpError result;
  addToLog("uploadFile",LOGFTP);
  if ( fileName.isNull() )  return FTPNAMEERROR;
  if(reconnect)
    {
      result=doConnect();
      if(result!=FTPOK) return result;
    }
  sourceFn=new QFile(fileName);
  if ( !sourceFn->open( QIODevice::ReadOnly ) )
    {
      //		   QMessageBox::critical( 0, tr("Upload error"),
      //                tr("Can't open file '%1' for reading.").arg(fileName) );
      sourceFn=NULL;
      return FTPNAMEERROR;
    }
  QFileInfo fi( fileName );
  QFileInfo fin(targetFilename);
  addToLog(QString("'%1' put '%2', %3 bytes").arg(name).arg(targetFilename).arg(sourceFn->size()),LOGFTP);

  ftpDone=false;
  if(fin.fileName().isEmpty())
    {
      ftp->put( sourceFn, fi.fileName(),QFtp::Binary);
    }
  else
    {
      ftp->put( sourceFn, fin.fileName(),QFtp::Binary);
    }
//  addToLog(QString("FTP put file id: %1").arg(id),LOGFTP);
  result = wait(-3);
  if (result!=FTPOK) return result;
  addToLog("FTP starting progress",LOGFTP);
  return FTPOK;
}

eftpError ftpInterface::uploadData(const QByteArray & data,QString targetFilename)
{
  int id;
  Q_UNUSED(id)
  eftpError result;

  QFileInfo fin(targetFilename);
  if(fin.fileName().isEmpty()) return FTPNAMEERROR;

  result=doConnect();
  if(result!=FTPOK) return result;

  ftpDone=false;
  id=ftp->put( data,fin.fileName(),QFtp::Binary);
  addToLog(QString("'%1' '%2', %3 bytes, id:%4").arg(name).arg(targetFilename).arg(data.size()).arg(id),LOGFTP);
  return FTPOK;
}

eftpError ftpInterface::downloadFile(QString sourceFileName,QString destinationFilename)
{
  eftpError result;
  addToLog("FTP downloadFile",LOGFTP);
  QFile *destFn;
  destFn=new QFile(destinationFilename);
  if(!destFn->open(QIODevice::WriteOnly))
    {
      addToLog(QString("FTP unable to open destinationFilename %1").arg(destinationFilename),LOGFTP);
      return FTPNAMEERROR;
    }

  if (sourceFileName.isNull() )  return FTPNAMEERROR;
  result=doConnect();
  if(result!=FTPOK) return result;
  ftpDone=false;
  addToLog(QString("'%1' get '%2' destination '%3'").arg(name).arg(sourceFileName).arg(destFn->fileName()),LOGFTP);
  ftp->get( sourceFileName, destFn,QFtp::Binary);
  result = wait(-3);
  if(result!=FTPOK) return result;
  addToLog(QString("FTP file: %1 bytes: %2").arg(destinationFilename).arg(QFile(destinationFilename).size()),LOGFTP);
  return FTPOK;
}


eftpError ftpInterface::wait(int timeout)
{
  if (timeout < 0) timeout = FTPTIMEOUTTIME - (0-timeout);
  if (timeout) {
     tim.setSingleShot(true);
     tim.setInterval(timeout);
     tim.start();
     }
     
  while (!ftpDone)
    {
      if(aborting) return FTPCANCELED;
      qApp->processEvents();
      if(timeout && !tim.isActive())
        {
          slotAbort();
          addToLog(QString("'%1' Timeout (%2ms) Host:%3 ").arg(name).arg(timeout).arg(host),LOGALL);
          return FTPTIMEOUT;
        }
    }
  if (timeout) tim.stop();
  if(!ftpCommandSuccess) return FTPERROR;
  return FTPOK;
}

eftpError ftpInterface::remove(QString path)
{
  int id;
  Q_UNUSED(id)
  eftpError result=doConnect();
  if(result!=FTPOK) return result;

  ftpDone=false;
  id = ftp->remove(path);
  addToLog(QString("'%1' Name='%2'  id:%3").arg(name).arg(path).arg(id),LOGFTP);
  return FTPOK;
}

eftpError ftpInterface::mremove(QString path)
{
  int id;
  Q_UNUSED(id)
  eftpError result;
  addToLog(QString("FTP mremove"),LOGFTP);

  result=doConnect();
  if(result!=FTPOK) return result;

  ftpDone=false;
  id = ftp->list(path);
  if (id>0) mremove_listids.append(id);
  addToLog(QString("'%1' Path='%2'  id=%3").arg(name).arg(path).arg(id),LOGFTP);
  return FTPOK;  
}

eftpError ftpInterface::getListing(QString path)
{
  int id;
  Q_UNUSED(id)
  eftpError result;
  addToLog("FTP getListing",LOGFTP);

  result=doConnect();
  if(result!=FTPOK) return result;
  ftpDone=false;
  id = ftp->list(path);
  addToLog(QString("'%1' Path='%2'  id:%3").arg(name).arg(path).arg(id),LOGFTP);
  return FTPOK;  
}

QList <QUrlInfo> ftpInterface::getListingResults()
{
  return listingResults;
}

void ftpInterface::clearListingResults()
{
  listingResults.clear();
}


eftpError ftpInterface::startNotifyCheck(QString fn, int interval, int repeats, bool rm)
{
  addToLog(QString("'%1' startNotifyCheck(%2,%3,%4,%5)").arg(name).arg(fn).arg(interval).arg(repeats).arg(rm),LOGNOTIFY);
  notifyMask    = "Dummy"+fn+"+++*";
  notifyTicks   = -1;
  notifyRepeats = repeats;
  notifyRemove  = rm;
  
  notifyList.clear();
  
  notifyTimer.setSingleShot(false);
  notifyTimer.setInterval(interval*1000);
  connect(&notifyTimer, SIGNAL(timeout()), this, SLOT(notifyTick()));
  notifyTimer.start();
  notifyTick();  
  return FTPOK;
}

void ftpInterface::notifyTick()
{
   // runs once every interval seconds
   notifyTicks++;
   addToLog(QString("'%1': ticks=%2  notifyid=%3").arg(name).arg(notifyTicks).arg(notifyId), LOGNOTIFY);
   if (notifyTicks>notifyRepeats)
   {
      notifyTimer.stop();
   }
   else if (notifyId<0)
   {
      eftpError result=doConnect();
      if (result==FTPOK)
      {
         ftpDone=false;
         notifyId = ftp->list(notifyMask);
      }
   }
}

void ftpInterface::connectToHost()
{
  addToLog(QString("'%1' connectToHost %2").arg(name).arg(host), LOGFTP);
  destroy();
  ftpDone=false;
  ftp->connectToHost(host,port);
  ftp->login( user, passwd );
  if(!directory.isEmpty()) changePath(directory);
}

// This slot is connected to the QComboBox::activated() signal of the
// remotePath.
void ftpInterface::changePath( const QString &newPath )
{
  directory=newPath;
  ftp->cd( newPath );
}


/****************************************************************************
**
** Slots connected to signals of the QFtp class
**
*****************************************************************************/

void ftpInterface::ftp_commandStarted(int id)
{
  Q_UNUSED(id);
  addToLog(QString("'%1' id:%2, %3").arg(name).arg(id).arg(ftp->currentCommand()),LOGFTP);
  if ( ftp->currentCommand() == QFtp::List )
    {
    }
  if (tim.isActive()) tim.start();
}

void ftpInterface::ftp_commandFinished(int id,bool err)
{
  Q_UNUSED(id);
  QIODevice *p;

  slotProgress(0,0);

  addToLog(QString("'%1' id:%2, error:%3").arg(name).arg(id).arg(err),LOGFTP);
  if(err)
    {
      addToLog(QString("FTP error:%1").arg(ftp->errorString()),LOGFTP);
      ftpCommandSuccess=false;
    }
  else
    {
      ftpCommandSuccess=true;
    }

  mremove_listids.removeOne(id);
  if (id==notifyId) notifyId=-1;
  
  p=ftp->currentDevice();
  if(p)
    {
      delete ftp->currentDevice();
    }

}

void ftpInterface::ftp_done( bool error )
{

  if ( error )
    {

      //        QMessageBox::critical( 0, tr("FTP Error"), ftp->errorString() );

      // If we are connected, but not logged in, it is not meaningful to stay
      // connected to the server since the error is a really fatal one (login
      // failed).
      if(!isLoggedIn())
        {
          addToLog(QString("'%1': error and not logged in-> disconnecting").arg(host),LOGFTP);
          destroy();
          ftpDone=true;
          return;
        }
      addToLog(QString("'%1': error").arg(host),LOGFTP);
    }
  else
    {
      addToLog(QString("'%1': OK").arg(host),LOGFTP);
    }
  mremove_listids.clear();
  ftpDone=true;
}

bool ftpInterface::isLoggedIn()
{
  return ftp->state() == QFtp::LoggedIn;
}

bool ftpInterface::isUnconnected()
{
  return ftp->state() == QFtp::Unconnected;
}

bool ftpInterface::isBusy()
{
  return !ftpDone;
}

void ftpInterface::ftp_stateChanged( int )
{
  dumpState();
}

void ftpInterface::dumpState()
{
  switch (ftp->state() )
    {
    case QFtp::Unconnected:
      addToLog(QString("FTPss Unconnected name:=%1 :host=%2").arg(name).arg(host),LOGFTP);
      break;
    case QFtp::HostLookup:
      addToLog(QString("FTPss Host lookup name:=%1 :host=%2").arg(name).arg(host),LOGFTP);
      break;
    case QFtp::Connecting:
      addToLog(QString("FTPss Connecting name:=%1 :host=%2").arg(name).arg(host),LOGFTP);
      break;
    case QFtp::Connected:
      addToLog(QString("FTPss Connected name:=%1 :host=%2").arg(name).arg(host),LOGFTP);
      break;
    case QFtp::LoggedIn:
      addToLog(QString("FTPss Logged In name:=%1 :host=%2").arg(name).arg(host),LOGFTP);
      break;
    case QFtp::Closing:
      addToLog(QString("FTPss Closing name:=%1 :host=%2").arg(name).arg(host),LOGFTP);
      break;
    default:
      addToLog(QString("FTPss uknown %1 name:=%2 host=%3").arg(ftp->state()).arg(name).arg(host),LOGFTP);
      break;
    }
}


void ftpInterface::ftp_listInfo( const QUrlInfo & ent)
{
  if (ent.isFile()) {
     addToLog(QString("currentId=%1, name=%2").
             arg(ftp->currentId()).arg(ent.name()),
             LOGFTP);
             
     if (ftp->currentId()==notifyId) {
        QString name = ent.name().mid(notifyMask.length());

        if (!notifyList.contains(name)) {
            addToLog(QString("New Notification:'%1'").arg(name), LOGALL);
            notifyList.append(name);
            emit notification(name);
            }
        
        if (notifyRemove) remove(ent.name());
     }
     else if (mremove_listids.contains(ftp->currentId())) {
        remove(ent.name());
     }
     else {
        listingResults.append(ent);
        addToLog(QString("List Entry: %1").arg(ent.name()), LOGFTP);
     }
  }
}

void ftpInterface::ftp_rawCommandReply( int code, const QString &text )
{
  Q_UNUSED(code);
  Q_UNUSED(text);
  addToLog(QString("FTP Raw Command Reply: code=%1 , %2").arg(code).arg(text),LOGFTP);
}



void ftpInterface::slotAbort()
{
  aborting=true;
  ftp->abort();
}

void ftpInterface::slotProgress(qint64 bytes ,qint64 total)
{
  displayProgressFTPEvent *stmb;
  stmb=new displayProgressFTPEvent(bytes,total);
  QApplication::postEvent( dispatcherPtr, stmb );  // Qt will delete it when done
  if (tim.isActive()) tim.start();
}

eftpError ftpInterface::uploadToRXServer(QString fn)
{
  int i;
  eftpError result;
  addToLog("FTP show state",LOGFTP);
  ftp_stateChanged(ftp->state());
  result=doConnect();

  if(result!=FTPOK) return result;
  if(ftpSaveFormat==FTPIM)
    {
      ftpDone=false;
      ftp->remove(QString("image%1").arg(ftpNumImages));
      while(!ftpDone)
        {
          qApp->processEvents();
        }

      for(i=0;i<ftpNumImages-1;i++)
        //        for(i=0;i<1;i++)
        {
          ftpDone=false;
          ftp->rename(QString("image%1").arg(ftpNumImages-1-i),QString("image%1").arg(ftpNumImages-i));
          while(!ftpDone)
            {
              qApp->processEvents();
            }
          addToLog("FTP done:  in rename",LOGFTP);
        }
      ftpDone=false;
      return uploadFile(fn,"image1",false);
    }
  else
    {
      ftpDone=false;
      return uploadFile(fn,"",false);
    }
}


QString ftpInterface::execFTPTest()
{
  QString ret;
  QString fn;
  QString rfn;
  eftpError ftpResult;

  addToLog(QString("%1 execFTPTest").arg(name), LOGFTP);

  QTemporaryFile tst;
  if (!tst.open())
    {
      ret=QString("Error writing temp file: %1, %2").arg(tst.fileName()).arg(strerror(errno));
    }
  else
    {
      fn=tst.fileName();
      tst.write("connection test\n");
      tst.close();
      
      rfn = QString("test_%1.txt").arg( myCallsign);
#if 1
      ftpResult=uploadFile(fn,rfn,true);
#else
      mremove("test_"+myCallsign+".*");
      uploadData(QByteArray("Dummy\r\n"), rfn);
      ftpResult = wait(-1);
#endif
      
      switch(ftpResult)
        {
        case FTPCANCELED:
          ret="Connection Canceled";
          break;
        case FTPOK:
          ret="Connection OK";
          break;
        case FTPERROR:
          ret=getLastError();
          break;
        case FTPNAMEERROR:
          ret="Error in filename";
          break;
        case FTPTIMEOUT:
          ret="FTP timed out";
          break;
        default:
          ret="Unknown error";
          break;
        }
    if (ftpResult==FTPOK) {
        remove(rfn);
        ftpResult=wait(-1);
        switch(ftpResult)
          {
          case FTPCANCELED:
            ret="mremove: Connection Canceled";
            break;
          case FTPOK:
            ret="Connection OK";
            break;
          case FTPERROR:
            ret=getLastError();
            break;
          case FTPNAMEERROR:
            ret="mremove: Error in filename";
            break;
          case FTPTIMEOUT:
            ret="mremove: FTP timed out";
            break;
          default:
            ret="mremove: Unknown error";
            break;
          }

       }
    }
  return ret;
}

