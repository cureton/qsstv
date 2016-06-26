#include "extviewer.h"
#include "ui_extviewer.h"
#include <QFileInfo>
#include <QDebug>

extViewer::extViewer(QWidget *parent) :   QDialog(parent),   ui(new Ui::extViewer)
{
  ui->setupUi(this);
  activeMovie=false;
  setModal(false);
}

extViewer::~extViewer()
{
  delete ui;
}


void extViewer::setup(QString fn)
{
  int fw,fh;

  // we want the original image
  ui->imViewer->openImage(fn,false,false,false);
  fileName=fn;
  QFileInfo fi(fn);
  fw=ui->imViewer-> getImagePtr()->width();
  fh=ui->imViewer->getImagePtr()->height();
  ui->lineEdit->setText(QString("%1 %2x%3").arg(fi.fileName()).arg(fw).arg(fh));
}

