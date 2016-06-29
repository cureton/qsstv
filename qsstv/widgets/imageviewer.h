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
#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H
#include <QLabel>
#include <QSettings>
#include <QEvent>
#include <QMovie>
#include "editor/editor.h"
#include "editor/editorscene.h"


class QMenu;
class QAction;
class editor;

class imageViewer : public QLabel
{
  Q_OBJECT
  /*! thumbnail type */
public:
  enum thumbType
    {
    RXIMG, /*!< just for receiver */
    TXIMG, /*!< just for transmitter */
    RXSSTVTHUMB, /*!< thumbnail for receiver. */
    RXDRMTHUMB, /*!< thumbnail for receiver. */
    TXSSTVTHUMB,/*!< thumbnail for transmitter. */
    TXDRMTHUMB,/*!< thumbnail for transmitter. */
    TXSTOCKTHUMB,/*!< thumbnail for transmitter. */
    TEMPLATETHUMB, /*!< thumbnail for template. */
    PREVIEW /*!< preview tx. */
    };
  imageViewer(QWidget *parent=0);
  ~imageViewer();


  void init(thumbType tp);
  bool openImage(QString &filename, QString start, bool ask, bool showMessage, bool emitSignal,bool fromCache);
  bool openImage(QString &filename, bool showMessage, bool emitSignal, bool fromCache);
  bool openImage(QImage im);
  bool openImage(QByteArray *ba);
  void setParam(QString templateFn,bool usesTemplate,int width=0,int height=0);
  void clear();
  bool reload();


//  void scale( int w, int h);
  QImage * getImagePtr() {return &sourceImage;}
  bool hasValidImage();
  void setValidImage(bool v)
  {
    validImage=v;
  }

  void createImage(QSize sz, QColor fill, bool scale);
  QRgb *getScanLineAddress(int line);
  void copy(imageViewer *src);
  void setType(thumbType t);
  QString getFilename() {return imageFileName;}
  void enablePopup(bool en) {popupEnabled=en;}
  void displayImage();
  void save(QString fileName, QString fmt, bool convertRGB, bool source);
  bool copyToBuffer(QByteArray *ba);
//  int calcSize(int &sizeRatio);
  uint setSizeRatio(int sizeRatio,bool usesCompression);
  int getFileSize(){return fileSize;}
  QString toCall;
  QString toOperator;
  QString rsv;
  QString comment1;
  QString comment2;
  QString comment3;
  bool stretch;
  void getOrgSize(int &w,int &h) {w=orgWidth; h=orgHeight;}


  int applyTemplate();


protected:
  void resizeEvent(QResizeEvent *);


private slots:
  void slotDelete();
  void slotEdit();
  void slotLoad();
  void slotNew();
  void slotPrint();
  void slotProperties();
  void slotToTX();
  void slotView();
  void slotBGColorChanged();


signals:
  void layoutChanged();
  void imageChanged();

private:
  QImage displayedImage;
  QImage sourceImage;
  QImage compressedImage;

  void mousePressEvent( QMouseEvent *e );
  bool validImage;
  QString imageFileName;
  QString imageFilePath;
  thumbType ttype;
  bool popupEnabled;

  QMenu *popup;
  QAction *newAct;
  QAction *loadAct;
  QAction *toTXAct;
  QAction *editAct;
  QAction *printAct;
  QAction *deleteAct;
  QAction *viewAct;
  QAction *propertiesAct;

//  double psizeRatio;
  int compressionRatio; // 0=lossless 99 is max compression
  int fileSize;
  QString format;
  QMovie qm;
  bool activeMovie;
  bool useCompression;
  QString templateFileName;
  bool useTemplate;
  int targetWidth;
  int targetHeight;
  int orgWidth;
  int orgHeight;

};

#endif
