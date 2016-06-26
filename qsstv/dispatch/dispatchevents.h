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
#ifndef DISPATCHEVENT_H
#define DISPATCHEVENT_H
#include <QEvent>
#include "widgets/imageviewer.h"
#include "appdefs.h"
#include <unistd.h>


/** dispatch events are used to communicate with the different threads */
enum dispatchEventType
{
    info = QEvent::User, //!< send when dsp stops running
    soundcardIdle, //!< send when soundcard stops running
    displayFFT,
    displaySync,
    displayDRMStat,
    displayDRMInfo,
    syncDisp,				//!< synchro display event
    lineDisplay,				//!< display 1 line
    eraseDisp,
    createMode,
    startImageRX,
    endSSTVImageRX,
    endImageTX,
    stoppingTX,
    progressTX,
    //  verticalRetrace,
    //  syncLost,
    outOfSync,
    statusMsg,  	//!<  display status message
    rxSSTVStatus,     //! shows message in sstv tab
    rxDRMStatus,     //! shows message in drm tab
    rxDRMNotify,	//! shows text in rx notifications box
    rxDRMNotifyAppend,
    txDRMNotify,  //! shows text in tx notifications box
    txDRMNotifyAppend,
    closeWindows,
    callEditor,
    templatesChanged,
    editorFinished,
    changeRXFilter,
    startAutoRepeater,
    startRepeater,
    stopRxTx,
    loadRXImage,
    saveDRMImage,
    prepareFix,
    displayText,
    displayMBox,
    displayProgressFTP,
    moveToTx
};

class baseEvent: public QEvent
{
public:
    baseEvent(QEvent::Type t):QEvent(t) {doneIt=NULL;}
    void waitFor(bool *d) {doneIt=d;}
    void setDone()
    {
        if(doneIt!=NULL) *doneIt=true;
    }
    QString description;
private:
    bool *doneIt;

};

/**
  this event is send when the dspfunc thread stops running
*/
class infoEvent : public  baseEvent
{
public:
    /** create event */
    infoEvent(QString t):baseEvent( (QEvent::Type) info ), str(t)
    {
        description="infoEvent";
    }
    /** returns info string from the event */
    QString getStr() const { return str; }
private:
    QString str;
};

/**
  this event is send when the soundcard thread goes to idle
*/
class soundcardIdleEvent : public baseEvent
{
public:
    /** create event */
    soundcardIdleEvent():baseEvent( (QEvent::Type) soundcardIdle )
    {
        {
            description=" soudcardIdleEvent";
        }
    }
};


//class rxDataAvailableEvent : public baseEvent
//{
//public:
//	/** create event */
//	rxDataAvailableEvent(uint idx,uint numSamples):baseEvent( (QEvent::Type)rxData ), index(idx),len(numSamples) {}
//	/** returns length and pointer  from the event */
//  uint getIndex(uint &idx) const { idx=index; return len;}

//private:
//  uint index;
//	uint len;
//};


/**
  this event is send with teh sync quality info and the signal volume
*/
class displaySyncEvent : public baseEvent
{
public:
    /** create event */
    displaySyncEvent(uint s):baseEvent( (QEvent::Type) displaySync), sync(s)
    {
        description=" displaySyncEvent";
    }
    /** returns int sync value */
    void getInfo(uint &s)  {s=sync;}

private:
    uint sync;
    DSPFLOAT vol;
};

class displayDRMStatEvent  : public baseEvent
{
public:
    /** create event */
    displayDRMStatEvent(uint s):baseEvent( (QEvent::Type) displayDRMStat), snr(s)
    {
        description=" displayDRMStatEvent";
    }
    /** returns length and pointer  from the event */
    void getInfo(DSPFLOAT &s)  {s=snr;}

private:
    DSPFLOAT snr;
};

class statusMsgEvent : public baseEvent
{
public:
    /** create event */
    statusMsgEvent(QString t):baseEvent( (QEvent::Type)statusMsg ), str(t)
    {
        description="statusMsgEvent";
    }
    /** returns info string from the event */
    QString getStr() const { return str; }
private:
    QString str;
};



class rxSSTVStatusEvent : public baseEvent
{
public:
    /** create event */
    rxSSTVStatusEvent(QString t):baseEvent( (QEvent::Type)rxSSTVStatus ), str(t)
    {
        description="rxSSTVStatusEvent";
    }
    /** returns info string from the event */
    QString getStr() const { return str; }
private:
    QString str;
};

class rxDRMStatusEvent : public baseEvent
{
public:
    /** create event */
    rxDRMStatusEvent(QString t):baseEvent( (QEvent::Type)rxDRMStatus ), str(t)
    {
        description="rxDRMStatusEvent";
    }
    /** returns info string from the event */
    QString getStr() const { return str; }
private:
    QString str;
};

class rxDRMNotifyEvent : public baseEvent
{
public:
  /** create event */
  rxDRMNotifyEvent(QString t):baseEvent( (QEvent::Type)rxDRMNotify ), str(t)
  {
    description="rxDRMNotifyEvent";
  }
  /** returns info string from the event */
  QString getStr() const { return str; }
private:
  QString str;
};

class rxDRMNotifyAppendEvent : public baseEvent
{
public:
  /** create event */
  rxDRMNotifyAppendEvent(QString t):baseEvent( (QEvent::Type)rxDRMNotifyAppend ), str(t)
  {
    description="rxDRMNotifyAppendEvent";
  }
  /** returns info string from the event */
  QString getStr() const { return str; }
private:
  QString str;
};

class txDRMNotifyEvent : public baseEvent
{
public:
  /** create event */
  txDRMNotifyEvent(QString t):baseEvent( (QEvent::Type)txDRMNotify ), str(t)
  {
    description="txDRMNotifyEvent";
  }
  /** returns info string from the event */
  QString getStr() const { return str; }
private:
  QString str;
};

class txDRMNotifyAppendEvent : public baseEvent
{
public:
  /** create event */
  txDRMNotifyAppendEvent(QString t):baseEvent( (QEvent::Type)txDRMNotifyAppend ), str(t)
  {
    description="txDRMNotifyAppendEvent";
  }
  /** returns info string from the event */
  QString getStr() const { return str; }
 private:
   QString str;
 };


class lineDisplayEvent : public baseEvent
{
public:
    /** create event */
    lineDisplayEvent(uint lineNbr):baseEvent( (QEvent::Type)lineDisplay ), lineNumber(lineNbr)
    {
        description="lineDisplayEvent";
    }
    /** returns length and pointer  from the event */
    void getInfo(uint &lineNbr) const { lineNbr=lineNumber;}

private:
    uint lineNumber;
};

class eraseDisplayEvent : public baseEvent
{
public:
    /** create event */
    eraseDisplayEvent():baseEvent( (QEvent::Type)eraseDisp )
    {
        description="eraseDisplayEvent";
    }
};



class displayDRMInfoEvent : public baseEvent
{
public:
    /** create event */
    displayDRMInfoEvent():baseEvent( (QEvent::Type)displayDRMInfo)
    {
        description="displayDRMInfo";
    }
};

class startAutoRepeaterEvent: public baseEvent
{
public:
    /** create event */
    startAutoRepeaterEvent():baseEvent( (QEvent::Type)startAutoRepeater )
    {
        description="startAutoRepeaterEvent";
    }
};

class startRepeaterEvent: public baseEvent
{
public:
    /** create event */
    startRepeaterEvent():baseEvent( (QEvent::Type)startRepeater )
    {
        description="startRepeaterEvent";
    }
};


class createModeEvent : public baseEvent
{
public:
    /** create event */
    createModeEvent(uint m,QString t):baseEvent( (QEvent::Type)createMode ), mode(m) ,str(t)
    {
        description="createModeEvent";
    }
    /** returns info string from the event */
    void getMode(uint &m,QString &s) const { m=mode;s=str; }
private:
    uint mode;
    QString str;
};

class loadRXImageEvent : public baseEvent
{
public:
    loadRXImageEvent(QString fn):baseEvent( (QEvent::Type)loadRXImage),fileName(fn)
    {
        description="loadRXImageEvent";
    }
    QString getFilename() {return fileName;}
private:
    QString fileName;
};


class moveToTxEvent : public baseEvent
{
public:
    moveToTxEvent(QString fn):baseEvent( (QEvent::Type)moveToTx),fileName(fn)
    {
        description="moveToTxEvent";
    }
    QString getFilename() {return fileName;}
private:
    QString fileName;
};



class saveDRMImageEvent : public baseEvent
{
public:
    saveDRMImageEvent(QString fn):baseEvent( (QEvent::Type)saveDRMImage),fileName(fn)
    {
        description="saveDRMImageEvent";
    }
    void getFilename(QString &fn) {fn=fileName;}
private:
    QString fileName;
};



class startImageRXEvent : public baseEvent
{
public:
    /** create event */
    startImageRXEvent(QSize ims):baseEvent( (QEvent::Type)startImageRX ),imSize(ims)
    {
        description="startImageRXEvent";
    }
    QSize getSize()  {return imSize;}
private:
    QSize imSize;

};

class endImageSSTVRXEvent : public baseEvent
{
public:
    /** create event */
    endImageSSTVRXEvent(QString mn):baseEvent( (QEvent::Type)endSSTVImageRX ),modeName(mn)
    {
        description="endImageSSTVRXEvent";
    }
    QString getModeName() {return modeName;}
private:
    QString modeName;
};

class endImageTXEvent : public baseEvent
{
public:
    /** create event */
    endImageTXEvent():baseEvent( (QEvent::Type)endImageTX )
    {
        description="endImageTXEvent";
    }
};


class stopTXEvent : public baseEvent
{
public:
    /** create event */
    stopTXEvent():baseEvent( (QEvent::Type)stoppingTX )
    {
        description="stopTXEvent";
    }
};

//class verticalRetraceEvent : public baseEvent
//{
//public:
//  /** create event */
//  verticalRetraceEvent():baseEvent( (QEvent::Type) verticalRetrace )
//  {
//    description="verticalRetraceEvent";
//  }
//};

//class syncLostEvent : public baseEvent
//{
//public:
//  /** create event */
//  syncLostEvent():baseEvent( (QEvent::Type) syncLost )
//  {
//    description="syncLostEvent";
//  }
//};



class outOfSyncEvent : public baseEvent
{
public:
    /** create event */
    outOfSyncEvent():baseEvent( (QEvent::Type)outOfSync )
    {
        description="outOfSyncEvent";
    }
};



class progressTXEvent : public baseEvent
{
public:
    /** create event */
    progressTXEvent(double tim):baseEvent( (QEvent::Type)progressTX ), txTime(tim)
    {
        description="progressTXEvent";
    }
    /** returns length and pointer  from the event */
    double getInfo() { return txTime;}

private:
    double txTime;
};

class closeWindowsEvent : public baseEvent
{
public:
    /** create event */
    closeWindowsEvent():baseEvent( (QEvent::Type)closeWindows)
    {
        description="closeWindowEvent";
    }
    /** returns length and pointer  from the event */
};



class callEditorEvent : public baseEvent
{
public:
    /** create event */
    callEditorEvent(imageViewer *iv,QString fn):baseEvent( (QEvent::Type) callEditor ), filename(fn),imviewer(iv)
    {
        description="callEditorEvent";
    }
    /** returns info string from the event */
    QString getFilename() const { return filename; }
    imageViewer *getImageViewer() { return imviewer; }
private:
    QString filename;
    imageViewer *imviewer;
};


class templatesChangedEvent : public baseEvent
{
public:
    /** create event */
    templatesChangedEvent():baseEvent( (QEvent::Type) templatesChanged )
    {
        description="templateChangeEvent";
    }
};

class editorFinishedEvent : public baseEvent
{
public:
    /** create event */
    editorFinishedEvent(bool b,QString fn):baseEvent( (QEvent::Type)editorFinished),ok(b),filename(fn)
    {
        description="editorFinishedEvent";
    }
    bool isOK() { return ok;}
    QString getFilename() const { return filename; }

private:
    bool ok;
    QString filename;

};


class displayFFTEvent : public baseEvent
{
public:
    /** create event */
    displayFFTEvent(DSPFLOAT *buf):baseEvent( (QEvent::Type)displayFFT),buffer(buf)
    {
        description="displayFFTEvent";
    }
    DSPFLOAT *data() { return buffer;}
private:
    DSPFLOAT *buffer;

};

class filterRXChangedEvent: public baseEvent
{
public:
    /** create event */
    filterRXChangedEvent(int fIndex):baseEvent( (QEvent::Type)changeRXFilter),filterIndex(fIndex)
    {
        description="filterChangedEvent";
    }
    int index() { return filterIndex;}
private:
    int filterIndex;
};

class stopRxTxEvent : public baseEvent
{
public:
    /** create event */
    stopRxTxEvent():baseEvent( (QEvent::Type)stopRxTx)
    {
        description="stopRxTxEvent";
    }
};


class prepareFixEvent: public baseEvent
{
public:
    prepareFixEvent(QByteArray ba):baseEvent( (QEvent::Type)prepareFix),data(ba)
    {
        description="filterChangedEvent";
    }
    QByteArray &getData() {return data;}
private:
    QByteArray data;
};


/**
  this event is send when the dspfunc thread stops running
*/
class displayTextEvent : public  baseEvent
{
public:
    /** create event */
    displayTextEvent(QString t):baseEvent( (QEvent::Type) displayText ), str(t)
    {
        description="displayTextEvent";
    }
    /** returns info string from the event */
    QString getStr() const { return str; }
private:
    QString str;
};

class displayMBoxEvent : public  baseEvent
{
public:
    /** create event */
    displayMBoxEvent(QString title,QString text):baseEvent( (QEvent::Type) displayMBox ), str(text), title(title)
    {
        description="displayMBoxEvent";
    }
    /** returns info string from the event */
    QString getStr() const { return str; }
    QString getTitle() const { return title; }

private:
    QString str;
    QString title;
};


class displayProgressFTPEvent : public  baseEvent
{
public:
    /** create event */
    displayProgressFTPEvent(quint64 byts,quint64 tot):baseEvent( (QEvent::Type) displayProgressFTP ),  bytes(byts),total(tot)
    {
        description="displayMBoxEvent";
    }
    /** returns info string from the event */
    quint64 getTotal() const { return total; }
    quint64 getBytes() const { return bytes; }

private:
    quint64 bytes;
    quint64 total;

};

#endif
