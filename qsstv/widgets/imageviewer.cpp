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
#include "imageviewer.h"
#include "appglobal.h"
#include "utils/logging.h"
#include "configparams.h"
#include "dispatcher.h"
#include "dirdialog.h"
#include "extviewer.h"
#include "jp2io.h"
#include <configdialog.h>


#include <QtGui>
#include <QMenu>

#define RATIOSCALE 1.


/**
  \class imageViewer

  The image is stored in it's original format and size.
  All interactions are done on the original image.
  A scaled version is used to display the contents.
*/	


imageViewer::imageViewer(QWidget *parent): QLabel(parent)
{
  addToLog("image creation",LOGIMAG);
  validImage=false;
  setFrameStyle(QFrame::Sunken | QFrame::Panel);
  QBrush b;
  QPalette palette;
  b.setTexture(QPixmap::fromImage(QImage(":/icons/transparency.png")));
  palette.setBrush(QPalette::Active,QPalette::Base,b);
  palette.setBrush(QPalette::Inactive,QPalette::Base,b);
  palette.setBrush(QPalette::Disabled,QPalette::Base,b);
  setPalette(palette);
  setBackgroundRole(QPalette::Base);
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

  setBackgroundRole(QPalette::Dark);

  popup=new QMenu (this);
  newAct = new QAction(tr("&New"),this);
  connect(newAct, SIGNAL(triggered()), this, SLOT(slotNew()));
  loadAct = new QAction(tr("&Load"), this);
  connect(loadAct, SIGNAL(triggered()), this, SLOT(slotLoad()));
  toTXAct = new QAction(tr("&To TX"), this);
  connect(toTXAct, SIGNAL(triggered()), this, SLOT(slotToTX()));
  editAct = new QAction(tr("&Edit"), this);
  connect(editAct, SIGNAL(triggered()), this, SLOT(slotEdit()));
  printAct = new QAction(tr("&Print"), this);
  connect(printAct, SIGNAL(triggered()), this, SLOT(slotPrint()));
  deleteAct = new QAction(tr("&Delete"), this);
  connect(deleteAct, SIGNAL(triggered()), this, SLOT(slotDelete()));
  viewAct = new QAction(tr("&View"), this);
  connect(viewAct, SIGNAL(triggered()), this, SLOT(slotView()));
  propertiesAct = new QAction(tr("Propert&ies"), this);
  connect(propertiesAct, SIGNAL(triggered()), this, SLOT(slotProperties()));
  connect(configDialogPtr,SIGNAL(bgColorChanged()), SLOT(slotBGColorChanged()));

  init(RXIMG);
  activeMovie=false;
  //
}

imageViewer::~imageViewer()
{
}

void imageViewer::init(thumbType tp)
{
  setScaledContents(false);
  setAlignment(Qt::AlignCenter);
  setAutoFillBackground(true);
  slotBGColorChanged();
  addToLog(QString("image creation %1").arg(tp),LOGIMAG);
  setType(tp);
  setPixmap(QPixmap());
  clear();
}

bool imageViewer::openImage(QString &filename,QString start,bool ask,bool showMessage,bool emitSignal,bool fromCache)
{
  QImage tempImage;
  QFile fi(filename);
  QFileInfo finf(filename);
  QString cacheFileName;
  jp2IO jp2;
  displayMBoxEvent *stmb=0;
  editorScene ed;
  bool success=false;
  bool cacheHit=false;
  cacheFileName=finf.absolutePath()+"/cache/"+finf.baseName()+finf.created().toString()+".png";

  if(activeMovie)
  {
    activeMovie=false;
    qm.stop();
  }
  if (filename.isEmpty()&&!ask) return false;
  if(ask)
  {
    dirDialog dd((QWidget *)this,"Browse");
    filename=dd.openFileName(start,"*");
  }
  if(filename.isEmpty())
  {
    imageFileName="";
    return false;
  }

  if(fromCache)
  {
    if(tempImage.load(cacheFileName))
    {
      cacheHit=true;
      success=true;
      orgWidth=tempImage.text("orgWidth").toInt();
      orgHeight=tempImage.text("orgHeight").toInt();
    }
  }
  if(!success)
  {
    if(tempImage.load(filename))
    {
      success=true;
    }
    else if(ed.load(fi))
    {
      success=true;
      tempImage=QImage(ed.renderImage(0,0)->copy());
    }
    else if(jp2.check(filename))
    {
      tempImage=jp2.decode(filename);
      if(!tempImage.isNull())
      {
        success=true;
      }
    }
  }

  if(!success)
  {
    if(showMessage)
    {
      stmb= new displayMBoxEvent("Image Loader",QString("Unable to load image:\n%1").arg(filename));
      QApplication::postEvent( dispatcherPtr, stmb );  // Qt will delete it when done
    }
    validImage=false;
    imageFileName="";
    return false;
  }

  if(fromCache)
  {
    sourceImage=QImage();

    if(!cacheHit)
    {
      orgWidth=tempImage.width();
      orgHeight=tempImage.height();
      tempImage=tempImage.scaledToWidth(120, Qt::FastTransformation);
      // save cacheImage for next time
      tempImage.setText("orgWidth",QString::number(orgWidth));
      tempImage.setText("orgHeight",QString::number(orgHeight));
      tempImage.save(cacheFileName,"PNG");
    }
    displayedImage=tempImage;
  }
  else
  {
    orgWidth=tempImage.width();
    orgHeight=tempImage.height();
    displayedImage=tempImage;
    sourceImage=tempImage;
  }

  imageFileName=filename;

  QFileInfo finfo(filename);
  if (finfo.suffix().toLower()=="gif")
  {
    //we will try a animated gif
    qm.setFileName(filename);
    if(qm.isValid())
    {
      if(qm.frameCount()>1)
      {
        activeMovie=true;
        setMovie(&qm);
        qm.start();
        displayedImage=QImage();
      }
      else
      {
        displayImage();  // we have a single image gif
      }
    }
  }
  else
  {
    displayImage();
  }
  validImage=true;
  if (emitSignal) emit imageChanged();
  return true;
}

bool imageViewer::openImage(QString &filename,bool showMessage,bool emitSignal,bool fromCache)
{
  return openImage(filename,"",false,showMessage,emitSignal,fromCache);
}

bool imageViewer::openImage(QImage im)
{
  imageFileName="";
  if(!im.isNull())
  {
    validImage=true;
    sourceImage=im;
    displayedImage=im;
    displayImage();
    return true;
  }
  validImage=false;
  return false;
}

bool imageViewer::openImage(QByteArray *ba)
{
  QImage tempImage;
  QBuffer buffer(ba);
  buffer.open(QIODevice::ReadOnly);
  if(tempImage.load(&buffer,NULL))
  {
    return  openImage(tempImage.convertToFormat(QImage::Format_ARGB32_Premultiplied));
  }
  validImage=false;
  return false;
}

void imageViewer::clear()
{
  validImage=false;
  imageFileName.clear();
  sourceImage=QImage();
  displayedImage=QImage();
  setPixmap(QPixmap());
  targetWidth=0;
  targetHeight=0;
  templateFileName.clear();
  useTemplate=false;
}

bool imageViewer::hasValidImage()
{
  return validImage;
}

void imageViewer::createImage(QSize sz,QColor fill,bool scale)
{
  clear();
  displayedImage=QImage(sz,QImage::Format_ARGB32_Premultiplied);
  if(!displayedImage.isNull())
  {
    displayedImage.fill(fill.rgb());
    useCompression=false;
  }
  stretch=scale;
  displayImage();
  emit imageChanged();
}

void imageViewer::copy(imageViewer *src)
{
  imageFileName=src->imageFileName;
  ttype=src->ttype;
  openImage(imageFileName,false,false,false);
}



QRgb *imageViewer::getScanLineAddress(int line)
{
  return (QRgb *)displayedImage.scanLine(line);
}



void imageViewer::displayImage()
{
  if(displayedImage.isNull())
  {
    return;
  }
  if(hasScaledContents() || (displayedImage.width()>width()) || (displayedImage.height()>height()) || stretch)
  {
    setPixmap(QPixmap::fromImage(displayedImage.scaled(width()-2,height()-2,Qt::KeepAspectRatio,Qt::SmoothTransformation)));

  }
  else
  {
    setPixmap(QPixmap::fromImage(displayedImage));
  }

}



void imageViewer::setType(thumbType tp)
{
  ttype=tp;
  switch(ttype)
  {
  case RXIMG:
  case PREVIEW:
  case RXSSTVTHUMB:
    imageFilePath=rxSSTVImagesPath;
    break;
  case RXDRMTHUMB:
    imageFilePath=rxDRMImagesPath;
    break;

  case TXSSTVTHUMB:
    imageFilePath=txSSTVImagesPath;
    break;
  case TXDRMTHUMB:
    imageFilePath=txDRMImagesPath;
    break;
  case TXIMG:
  case TXSTOCKTHUMB:
    imageFilePath=txStockImagesPath;
    break;
  case TEMPLATETHUMB:
    imageFilePath=templatesPath;
    break;

  }
  if((tp==RXSSTVTHUMB) || (tp==RXDRMTHUMB) || (tp==TXSSTVTHUMB) || (tp==TXDRMTHUMB) ||(tp==TXSTOCKTHUMB) ||(tp==TEMPLATETHUMB))
  {
    setScaledContents(false);
    setAlignment(Qt::AlignCenter);
  }
  popup->removeAction(newAct);
  popup->removeAction(loadAct);
  popup->removeAction(toTXAct);
  popup->removeAction(editAct);
  popup->removeAction(printAct);
  popup->removeAction(deleteAct);
  popup->removeAction(viewAct);
  popup->removeAction(propertiesAct);
  switch(tp)
  {
  case RXIMG:
    popup->addAction(viewAct);
    popup->addAction(propertiesAct);
    break;

  case TXIMG:
    popup->addAction(newAct);
    popup->addAction(loadAct);
    popup->addAction(editAct);
    popup->addAction(printAct);
    popup->addAction(viewAct);
    popup->addAction(propertiesAct);
    break;

  case PREVIEW:
    popup->addAction(loadAct);
    popup->addAction(toTXAct);
    popup->addAction(viewAct);
    popup->addAction(propertiesAct);
    break;

  case RXSSTVTHUMB:
  case RXDRMTHUMB:
  case TXSSTVTHUMB:
  case TXDRMTHUMB:
    popup->addAction(printAct);
    popup->addAction(deleteAct);
    popup->addAction(viewAct);
    popup->addAction(propertiesAct);
    break;

  case TXSTOCKTHUMB:
  case TEMPLATETHUMB:
    popup->addAction(newAct);
    popup->addAction(loadAct);
    popup->addAction(toTXAct);
    popup->addAction(editAct);
    popup->addAction(printAct);
    popup->addAction(deleteAct);
    popup->addAction(viewAct);
    popup->addAction(propertiesAct);
    break;
  }
  popupEnabled=true;

}

void imageViewer::mousePressEvent( QMouseEvent *e )
{
  QString temp;
  QString fn;
  if (e->button() == Qt::RightButton)
  {
    if(popupEnabled)
    {
      popup->popup(QCursor::pos());
    }
  }
}

void imageViewer::slotDelete()
{
  if(imageFileName.isEmpty()) return;
  if(QMessageBox::question(this,"Delete file","Do you want to delete the file and\n move it to the trash folder?",QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes)
  {
    trash(imageFileName,true);
  }
  imageFileName="";
  emit layoutChanged();
}

void imageViewer::slotEdit()
{
  if(imageFileName.isEmpty())
  {
    slotLoad();
    if (imageFileName.isEmpty()) return;
  }
  callEditorEvent *ce = new callEditorEvent( this,imageFileName );
  QApplication::postEvent(dispatcherPtr, ce );  // Qt will delete it when done
}


void imageViewer::slotLoad()
{
  QString fileNameTmp;
  dirDialog dd((QWidget *)this,"Browse");
  fileNameTmp=dd.openFileName(imageFilePath);
  if(openImage(fileNameTmp,true,false,false))
  {
    imageFileName=fileNameTmp;
    if(ttype==TEMPLATETHUMB)
    {
      templatesChangedEvent *ce = new templatesChangedEvent( );
      QApplication::postEvent(dispatcherPtr, ce );  // Qt will delete it when done
    }
    else if((ttype==TXIMG) ||(ttype==PREVIEW))
    {
      emit imageChanged();
    }

  }
}



void imageViewer::slotNew()
{
  callEditorEvent *ce = new callEditorEvent( this,NULL);
  QApplication::postEvent(dispatcherPtr, ce );  // Qt will delete it when done
}


void imageViewer::slotPrint()
{
}


void imageViewer::slotView()
{
  extViewer vm(this);
  vm.setup(imageFileName);
  vm.exec();
}


void imageViewer::slotBGColorChanged()
{
  QPalette mpalette;
  mpalette.setColor(QPalette::Window,backGroundColor);
  setBackgroundRole(QPalette::Window);
  mpalette.setColor(QPalette::WindowText, Qt::yellow);
  setPalette(mpalette);
}

void imageViewer::slotProperties()
{
  QFileInfo fi(imageFileName);
  if(fi.exists())
  {
    QMessageBox::information(this,"Image Properties",
                             "File: " + imageFileName
                             + "\n File size:     " + QString::number(fi.size())
                             + "\n Image width:   " + QString::number(orgWidth)
                             + "\n Image heigth:  " + QString::number(orgHeight)
                             + "\n Last Modified: " + fi.lastModified().toString()
                             ,QMessageBox::Ok);
  }
  else
  {
    QMessageBox::information(this,"Image Properties",
                             " Image width:   " + QString::number(orgWidth)
                             + "\n Image heigth:  " + QString::number(orgHeight)
                             ,QMessageBox::Ok);
  }

}

void imageViewer::slotToTX()
{
  moveToTxEvent *mt=0;
  mt=new moveToTxEvent(imageFileName);
  QApplication::postEvent(dispatcherPtr, mt); // Qt will delete it when done
}


void imageViewer::save(QString fileName,QString fmt,bool convertRGB, bool source)
{
  QImage im;
  if(source)
  {
    if(sourceImage.isNull()) return;
  }
  else
  {
    if(displayedImage.isNull()) return;
  }
  if(!convertRGB)
  {
    if(source) im=sourceImage;
    else im=displayedImage;
  }
  else
  {
    if(source) im=sourceImage.convertToFormat(QImage::Format_RGB32);
    else im=displayedImage.convertToFormat(QImage::Format_RGB32);;
  }
  im.save(fileName,fmt.toUpper().toLatin1().data());
}

bool imageViewer::copyToBuffer(QByteArray *ba)
{
  QImage im;
  jp2IO jp2;
  int fileSize;
  if(displayedImage.isNull())
  {
    return false;
  }
  *ba=jp2.encode(displayedImage.convertToFormat(QImage::Format_RGB32),im,fileSize,compressionRatio);
  if(fileSize==0)
  {
    return false;
  }
  return true;
}


uint  imageViewer:: setSizeRatio(int sizeRatio,bool usesCompression)
{
  if(!usesCompression) return sourceImage.byteCount();
  compressionRatio=sizeRatio;
  return applyTemplate();
}

bool imageViewer::reload()
{
  return openImage(imageFileName ,true,false,false);
}


void imageViewer::setParam(QString templateFn,bool usesTemplate,int width,int height)
{
  targetWidth=width;
  targetHeight=height;
  templateFileName=templateFn;
  useTemplate=usesTemplate;
  applyTemplate();
  displayImage();
}


int imageViewer::applyTemplate()
{
  QImage *resultImage;
  jp2IO jp2;
  QImage overlayedImage;
  if(sourceImage.isNull()) return 0;
  QFile fi(templateFileName);
  if(ttype!=TXIMG) return 0;
  editorScene tscene(0);
  resultImage=&sourceImage;
  if(transmissionModeIndex==TRXDRM) useCompression=true;
  else useCompression=false;
  if((fi.fileName().isEmpty())  || (!useTemplate))
  {
    if(targetWidth!=0 && targetHeight!=0)
    {
      displayedImage= QImage(sourceImage.scaled(targetWidth,targetHeight,Qt::IgnoreAspectRatio,Qt::SmoothTransformation)); // same resolution as sstv mode

    }
    else
    {
      displayedImage=sourceImage;
    }
    resultImage=&displayedImage;
  }
  else
  {
    addToLog("apply temlate",LOGIMAG);
    //  sconvert cnv;

    if((!fi.fileName().isEmpty())  && (useTemplate))
    {
      tscene.load(fi);
      tscene.addConversion('c',toCall,true);
      tscene.addConversion('r',rsv);
      tscene.addConversion('o',toOperator);
      tscene.addConversion('t',QDateTime::currentDateTime().toUTC().toString("hh:mm"));
      tscene.addConversion('d',QDateTime::currentDateTime().toUTC().toString("yyyy/MM/dd"));
      tscene.addConversion('m',myCallsign);
      tscene.addConversion('q',myQth);
      tscene.addConversion('l',myLocator);
      tscene.addConversion('n',myLastname);
      tscene.addConversion('f',myFirstname);
      tscene.addConversion('v',qsstvVersion);
      tscene.addConversion('x',comment1);
      tscene.addConversion('y',comment2);
      tscene.addConversion('z',comment3);
      addToLog(QString("applyTemplate size=%1,%2").arg(tscene.width()).arg(tscene.height()),LOGIMAG);
      if(targetWidth!=0 && targetHeight!=0)
      {
        overlayedImage= QImage(sourceImage.scaled(targetWidth,targetHeight));
        tscene.overlay(&overlayedImage);
      }
      else
      {
        tscene.overlay(&sourceImage);
      }
      resultImage=tscene.getImagePtr();
    }
  }
  if(useCompression)
  {
    int fileSize;
    jp2.encode(resultImage->convertToFormat(QImage::Format_RGB32),compressedImage,fileSize,compressionRatio);
    displayedImage=compressedImage;
    return fileSize;
  }
  else
  {
    displayedImage=*resultImage;
    return displayedImage.byteCount();
  }
}


void imageViewer::resizeEvent(QResizeEvent *)
{
  displayImage();
}


