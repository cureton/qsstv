#include "txwidget.h"
#include "appglobal.h"
#include "drmtx/drmparams.h"
#include "rxwidget.h"
#include "rxfunctions.h"
#include "ui_txwidget.h"
#include <QTimer>
#include "configparams.h"
#include "dispatcher.h"
#include "supportfunctions.h"
#include "gallerywidget.h"
#include "waterfalltext.h"

#include "ui_freqform.h"
#include "ui_sweepform.h"
#include "cameradialog.h"
#include "soundbase.h"
#include "waterfallform.h"
#include "bsrform.h"
#include "mainwindow.h"
#include "rigcontrol.h"
#include "imageviewer.h"
#include "testpatternselection.h"






txWidget::txWidget(QWidget *parent) :  QWidget(parent), ui(new Ui::txWidget)
{
  int i;
  QString tmp;
  ui->setupUi(this);
  ui->previewWidget->setType(imageViewer::PREVIEW);
  txFunctionsPtr=new txFunctions(this);
  imageViewerPtr=ui->imageFrame;

  imageViewerPtr->displayImage();
  for(i=0;i<NUMSSTVMODES;i++)
    {
      ui->sstvModeComboBox->addItem(getSSTVModeNameLong((esstvMode)i));
    }


  connect(ui->sstvModeComboBox,SIGNAL(activated(int)),SLOT(slotModeChanged(int )));


  connect(ui->templatesComboBox,SIGNAL(currentIndexChanged(int)),SLOT(slotGetParams()));
  connect(ui->templateCheckBox,SIGNAL(toggled(bool)),SLOT(slotGetParams()));
  connect(ui->refreshPushButton,SIGNAL(clicked()),SLOT(slotGetParams()));
  connect(ui->cwCheckBox,SIGNAL(toggled(bool)),SLOT(slotGetParams()));
  connect(ui->voxCheckBox,SIGNAL(toggled(bool)),SLOT(slotGetParams()));
  connect(ui->toCallLineEdit,SIGNAL(editingFinished ()),SLOT(slotGetParams()));
  connect(ui->operatorLineEdit,SIGNAL(editingFinished ()),SLOT(slotGetParams()));
  connect(ui->rsvLineEdit,SIGNAL(editingFinished ()),SLOT(slotGetParams()));
  connect(ui->xPlainTextEdit,SIGNAL(editingFinished()),SLOT(slotGetParams()));
  connect(ui->yPlainTextEdit,SIGNAL(editingFinished()),SLOT(slotGetParams()));
  connect(ui->zPlainTextEdit,SIGNAL(editingFinished()),SLOT(slotGetParams()));


  connect(ui->startToolButton, SIGNAL(clicked()), this, SLOT(slotStart()));
  connect(ui->stopToolButton, SIGNAL(clicked()), this, SLOT(slotStop()));
  connect(ui->drmProfileComboBox,SIGNAL(activated(int)),SLOT(slotProfileChanged(int )));

  connect(ui->generateToneToolButton, SIGNAL(clicked()), this, SLOT(slotGenerateSignal()));
  connect(ui->sweepToneToolButton, SIGNAL(clicked()), this, SLOT(slotSweepSignal()));
  connect(ui->repeaterToneToolButton, SIGNAL(clicked()), this, SLOT(slotGenerateRepeaterTone()));
  connect(ui->openToolButton, SIGNAL(clicked()), this, SLOT(slotFileOpen()));
  connect(ui->editToolButton, SIGNAL(clicked()), this, SLOT(slotEdit()));
  connect(ui->snapshotToolButton, SIGNAL(clicked()), this, SLOT(slotSnapshot()));
  connect(ui->binaryPushButton,SIGNAL(clicked()),this,SLOT(slotBinary()));
  connect(ui->sizeSlider,SIGNAL(valueChanged(int)),SLOT(slotSize(int)));
  connect(ui->settingsTableWidget,SIGNAL(currentChanged(int)),this, SLOT(slotTransmissionMode(int)));
  connect(imageViewerPtr,SIGNAL(imageChanged()),SLOT(slotImageChanged()));
  connect(ui->templateCheckBox,SIGNAL(toggled(bool)),SLOT(slotImageChanged()));
  connect(ui->templatesComboBox,SIGNAL(currentIndexChanged(int)),SLOT(slotImageChanged()));

  sizeRatio=-1;
}

txWidget::~txWidget()
{
  writeSettings();
  txFunctionsPtr->terminate();
  delete txFunctionsPtr;
  delete ui;
}

void txWidget::init()
{
  splashStr+=QString( "Setting up TX" ).rightJustified(25,' ')+"\n";
  splashPtr->showMessage ( splashStr ,Qt::AlignLeft,Qt::white);
  qApp->processEvents();

  readSettings();
  initView();
  setProfileNames();

  ed=NULL;
  repeaterIndex=0;

  repeaterTimer=new QTimer(this);
  connect(repeaterTimer,SIGNAL(timeout()),SLOT(slotRepeaterTimer()));
  repeaterTimer->start(60000*repeaterImageInterval);
  imageViewerPtr->setType(imageViewer::TXIMG);
  slotModeChanged(sstvModeIndexTx);
  ui->sizeSlider->setValue(sizeRatio);
  changeTransmissionMode(transmissionModeIndex);
  //  setSettingsTab();
  slotProfileChanged(0);
}

void txWidget::readSettings()
{
  QSettings qSettings;
  qSettings.beginGroup("TX");
  sstvModeIndexTx=((esstvMode)qSettings.value("sstvModeIndexTx",0).toInt());
  if((sstvModeIndexTx<M1) || (sstvModeIndexTx>=NOTVALID))
    {
      sstvModeIndexTx=M1;
    }

  templateIndex=qSettings.value("templateIndex",0).toInt();
  useTemplate=qSettings.value("useTemplate",false).toBool();

  useCW=qSettings.value("useCW",false).toBool();
  useVOX=qSettings.value("useVOX",false).toBool();
  useHybrid=qSettings.value("useHybrid",false).toBool();
  sizeRatio=qSettings.value("sizeRatio",25).toInt();
  drmParams.bandwith=qSettings.value("drmBandWith",0).toInt();
  drmParams.interleaver=qSettings.value("drmInterLeaver",0).toInt();
  drmParams.protection=qSettings.value("drmProtection",0).toInt();
  drmParams.qam=qSettings.value("drmQAM",0).toInt();
  drmParams.robMode=qSettings.value("drmRobMode",0).toInt();
  drmParams.reedSolomon=qSettings.value("drmReedSolomon",0).toInt();
  qSettings.endGroup();
  setParams();
}

void txWidget::writeSettings()
{
  QSettings qSettings;
  slotGetParams();
  qSettings.beginGroup("TX");
  qSettings.setValue( "sstvModeIndexTx", sstvModeIndexTx);
  qSettings.setValue( "templateIndex", templateIndex);
  qSettings.setValue( "useTemplate", useTemplate);
  qSettings.setValue( "useVOX", useVOX);
  qSettings.setValue( "useCW", useCW);
  qSettings.setValue( "useHybrid", useHybrid);
  qSettings.setValue("drmBandWith",drmParams.bandwith);
  qSettings.setValue("drmInterLeaver",drmParams.interleaver);
  qSettings.setValue("drmProtection",drmParams.protection);
  qSettings.setValue("drmQAM",drmParams.qam);
  qSettings.setValue("drmRobMode",drmParams.robMode);
  qSettings.setValue("drmReedSolomon",drmParams.reedSolomon);
  qSettings.setValue("sizeRatio",sizeRatio);
  qSettings.endGroup();
}

void txWidget::slotGetParams()
{
  int temp=sstvModeIndexTx;
  getIndex(temp,ui->sstvModeComboBox);
  sstvModeIndexTx=esstvMode(temp);
  getIndex(templateIndex,ui->templatesComboBox);
  getValue(useTemplate,ui->templateCheckBox);
  getValue(useVOX,ui->voxCheckBox);
  getValue(useCW,ui->cwCheckBox);
  getValue(useHybrid,ui->hybridCheckBox);
  getValue(imageViewerPtr->toCall,ui->toCallLineEdit);
  getValue(imageViewerPtr->toOperator,ui->operatorLineEdit);
  getValue(imageViewerPtr->rsv,ui->rsvLineEdit);
  getValue(imageViewerPtr->comment1,ui->xPlainTextEdit);
  getValue(imageViewerPtr->comment2,ui->yPlainTextEdit);
  getValue(imageViewerPtr->comment3,ui->zPlainTextEdit);
  getIndex(drmParams.bandwith,ui->drmTxBandwidthComboBox);
  getIndex(drmParams.interleaver,ui->drmTxInterleaveComboBox);
  getIndex(drmParams.protection,ui->drmTxProtectionComboBox);
  getIndex(drmParams.qam,ui->drmTxQAMComboBox);
  getIndex(drmParams.robMode,ui->drmTxModeComboBox);
  getIndex(drmParams.reedSolomon,ui->drmTxReedSolomonComboBox);
  getValue(sizeRatio,ui->sizeSlider);
  drmParams.callsign=myCallsign;
  if(txFunctionsPtr->getTXState()==txFunctions::TXIDLE)
    {
      applyTemplate();
    }
}

void txWidget::setParams()
{
  setIndex(((int)sstvModeIndexTx),ui->sstvModeComboBox);
  ui->templateCheckBox->blockSignals(true);
  ui->templatesComboBox->blockSignals(true);
  setIndex(templateIndex,ui->templatesComboBox);
  setValue(useTemplate,ui->templateCheckBox);
  ui->templateCheckBox->blockSignals(false);
  ui->templatesComboBox->blockSignals(false);
  setValue(useVOX,ui->voxCheckBox);
  setValue(useCW,ui->cwCheckBox);
  setValue(useHybrid,ui->hybridCheckBox);
  setIndex(drmParams.bandwith,ui->drmTxBandwidthComboBox);
  setIndex(drmParams.interleaver,ui->drmTxInterleaveComboBox);
  setIndex(drmParams.protection,ui->drmTxProtectionComboBox);
  setIndex(drmParams.qam,ui->drmTxQAMComboBox);
  setIndex(drmParams.robMode,ui->drmTxModeComboBox);
  setIndex(drmParams.reedSolomon,ui->drmTxReedSolomonComboBox);
  setValue(sizeRatio,ui->sizeSlider);
}

void txWidget::copyProfile(drmTxParams d)
{
  drmParams=d;
  setIndex(drmParams.bandwith,ui->drmTxBandwidthComboBox);
  setIndex(drmParams.interleaver,ui->drmTxInterleaveComboBox);
  setIndex(drmParams.protection,ui->drmTxProtectionComboBox);
  setIndex(drmParams.qam,ui->drmTxQAMComboBox);
  setIndex(drmParams.robMode,ui->drmTxModeComboBox);
  setIndex(drmParams.reedSolomon,ui->drmTxReedSolomonComboBox);
}

void txWidget::initView()
{
  imageViewerPtr->createImage(QSize(320,256),imageBackGroundColor,imageStretch);
  imageViewerPtr->setType(imageViewer::TXIMG);
  setupTemplatesComboBox();
  ui->txProgressBar->setRange(0,100);
}


void txWidget::setupTemplatesComboBox()
{
  QStringList sl;
  int i;
  ui->templatesComboBox->clear();
  sl=galleryWidgetPtr->getFilenames();
  for(i=0;i<sl.count();i++)
    {
      ui->templatesComboBox->insertItem(i,sl.at(i));
    }
  ui->templatesComboBox->setCurrentIndex(templateIndex);
}


QString txWidget::getPreviewFilename()
{
  return ui->previewWidget->getFilename();
}

void  txWidget::setPreviewWidget(QString fn)
{

  ui->previewWidget->openImage(fn,false,false,false);
}

void txWidget::startTX(bool st,bool check)
{
  if(st)
    {
      slotGetParams();
      if(check)
        {
          if(!imageViewerPtr->hasValidImage())
            {
              QMessageBox::warning(this,"TX Error","No image loaded");
              return;
            }
        }
    }
  else
    {
      writeSettings();
      txFunctionsPtr->stopAndWait();
    }
}

void txWidget::slotStart()
{
  QDateTime dt(QDateTime::currentDateTime().toUTC()); //this is compatible with QT 4.6
  dt.setTimeSpec(Qt::UTC);
  if(!imageViewerPtr->hasValidImage()) return;
  QFileInfo finf=imageViewerPtr->getFilename();
  QString fn;
  slotGetParams();
  switch(transmissionModeIndex)
    {
    case TRXSSTV:
      if(saveTXimages)
        {
          fn=QString("%1/%2_%3.%4").arg(txSSTVImagesPath).arg(finf.baseName()).arg(dt.toString("yyyyMMdd_HHmmss")).arg(defaultImageFormat);
          imageViewerPtr->save(fn,defaultImageFormat,true,false);
          galleryWidgetPtr->txImageChanged();
        }
      dispatcherPtr->startTX(txFunctions::TXSSTVIMAGE);
      break;
    case TRXDRM:
      if(saveTXimages)
        {
          fn=QString("%1/%2_%3.%4").arg(txDRMImagesPath).arg(finf.baseName()).arg(dt.toString("yyyyMMdd_HHmmss")).arg(defaultImageFormat);
          imageViewerPtr->save(fn,defaultImageFormat,true,false);
          galleryWidgetPtr->txImageChanged();
        }
      dispatcherPtr->startTX(txFunctions::TXSENDDRM);
      break;
      //    case FAX:
      //    break;
    case TRXNOMODE:
      break;
    }
}

void txWidget::sendBSR()
{
  QByteArray *p;
  bsrForm::eResult res;
  bsrForm bsrf;
  bsrf.init();
  res=(bsrForm::eResult)bsrf.exec();
  if(res==bsrForm::CANCEL) return;
  p=bsrf.getBA(res==bsrForm::COMPAT);
  txFunctionsPtr->sendBSR(p,bsrf.getDRMParams());
}


void txWidget::sendID()
{

  waterfallPtr->setText(myCallsign);
  dispatcherPtr->startTX(txFunctions::TXSENDID);
  addToLog("sendID",LOGTXMAIN);
}

void txWidget::sendWfText()
{
  waterfallForm wf;
  if((wf.exec()==QDialog::Accepted)&&(!wf.text().isEmpty()))
    {
      waterfallPtr->setText(wf.text());
      dispatcherPtr->startTX(txFunctions::TXSENDID);
      addToLog("sendID",LOGTXMAIN);
    }
}

void txWidget::slotStop()
{
  dispatcherPtr->startRX();
}


void txWidget::slotDisplayStatusMessage(QString s)
{
  statusBarPtr->showMessage(s);
}


//void txWidget::slotReplay()
//{
//  QString fn;
//  QImage *im=NULL;
//  fn=galleryWidgetPtr->getLastRxImage();
//  editorScene scene(0);
//  QFile f(fn);
//  if(!scene.load(f))
//    {
//      QMessageBox::warning(this,"Image Properties","Error while loading\n" + fn);
//      return;
//    }
//  im=scene.renderImage();
//  dispatcherPtr->setTXImage(im);
//  slotStartTX();
//}


void txWidget::slotFileOpen()
{
  QString fileName;
  imageViewerPtr->openImage(fileName,txStockImagesPath,true,true,true,false);
}

void txWidget::slotGenerateSignal()
{
  QDialog qd;
  Ui::freqForm *ff=new Ui::freqForm;

  ff->setupUi(&qd);
  int freq;
  int  duration;
  if(qd.exec())
    {
      getValue(freq,ff->frequencySpinBox);
      getValue(duration,ff->durationSpinBox);
      txFunctionsPtr->setToneParam((double)duration,(double)freq);
      dispatcherPtr->startTX(txFunctions::TXSENDTONE);
    }
}

void txWidget::slotSweepSignal()
{
  QDialog qd;
  Ui::sweepForm *ff=new Ui::sweepForm;
  ff->setupUi(&qd);
  int upperFreq;
  int lowerFreq;
  int  duration;
  if(qd.exec())
    {
      getValue(lowerFreq,ff->lowerFrequencySpinBox);
      getValue(upperFreq,ff->upperFrequencySpinBox);
      getValue(duration,ff->durationSpinBox);
      txFunctionsPtr->setToneParam((double)duration,(double)lowerFreq,(double)upperFreq);
      dispatcherPtr->startTX(txFunctions::TXSENDTONE);
    }
}



void txWidget::slotGenerateRepeaterTone()
{
  //  addToLog(QString("start of buffer %1").arg(soundIOPtr->txBuffer.count()),LOGTXMAIN);
  txFunctionsPtr->setToneParam(3.,1750);
  dispatcherPtr->startTX(txFunctions::TXSENDTONE);
  addToLog("sendTone",LOGTXMAIN);
}




void txWidget::slotEdit()
{
  if (ed!=NULL) delete ed;
  ed=new editor(this);
  if(txFunctionsPtr->isRunning())
    {
      QMessageBox::warning(this,"Editor","Transmission busy, editor not available");
      return;
    }
  connect(ed,SIGNAL(imageAvailable(QImage *)),SLOT(setImage(QImage *)));
  ed->setImage(imageViewerPtr->getImagePtr());
  ed->show();
}




/** \todo implement repeater */
void txWidget::applyTemplate()
{
  txFunctionsPtr->applyTemplate(imageViewerPtr,galleryWidgetPtr->getTemplateFileName(ui->templatesComboBox->currentIndex()));
}


void 	txWidget::setImage(QString fn)
{
  imageViewerPtr->openImage(fn,true,true,false);
}


void txWidget::setProgress(uint prg)
{
  ui->txProgressBar->setValue(prg);
}

void txWidget::slotModeChanged(int m)
{
  addToLog("slotModeChange",LOGTXMAIN);
  if(transmissionModeIndex==TRXSSTV)
    {
      sstvModeIndexTx=(esstvMode)m;
      applyTemplate();
    }
}


/** \todo implement repeater */

// void txWidget::repeat(QImage *im,esstvMode sm)
void txWidget::repeat(QImage *,esstvMode )
{
  /*	setValue((int)sm,ui->modeComboBox);
  txf->setModeIndex((sstvMode)sm);
//	slotModeChanged(sm);

  txf->setImage(im);
  dsp->setCaptureSource(true,NULL,false);
  txf->process();*/
}

void txWidget::slotRepeaterTimer()
{
  QString fn;
  QFile fi;
  if(txFunctionsPtr->isRunning()) repeaterTimer->start(60000*repeaterImageInterval);

  else if ((rxWidgetPtr->functionsPtr()->isRunning()) || (!repeaterEnable))
    {
      // wait 10 seconds and check if busy or repeaterEnable has changed
      repeaterTimer->start(10000);
    }
  else
    {
      switch(repeaterIndex)
        {
        case 0:
          fn=repeaterImage1;
          break;
        case 1:
          fn=repeaterImage2;
          break;
        case 2:
          fn=repeaterImage3;
          break;
        case 3:
          fn=repeaterImage4;
          break;
        default:
          fn=repeaterImage1;
          break;
        }
      fi.setFileName(fn);
      if (fi.exists())
        {
          slotModeChanged(repeaterTxMode);
          setImage(fn);
        }
      repeaterIndex++;
      if(repeaterIndex>3) repeaterIndex=0;
      repeaterTimer->start(60000*repeaterImageInterval);
      //QApplication::processEvents();
      startAutoRepeaterEvent* ce = new startAutoRepeaterEvent();
      QApplication::postEvent(dispatcherPtr, ce );
    }
}



void txWidget::slotSnapshot()
{
  QImage *im;
  cameraDialog camera;
  if(camera.exec()==QDialog::Accepted)
    {
      im=camera.getImage();
      if(im)
        {
          QTemporaryFile itmp(txStockImagesPath+"/snapshotXXXXXX.jpg");
          itmp.setAutoRemove(false);
          if(!itmp.open()) return;
          im->save(itmp.fileName());
          setImage(itmp.fileName());
          galleryWidgetPtr->txStockImageChanged();
        }
    }
}



void txWidget::setSettingsTab()
{
  {
    if(transmissionModeIndex==TRXDRM)
      {
        ui->hybridCheckBox->setEnabled(true);
        ui->binaryPushButton->setEnabled(true);
        ui->sizeLabel->setEnabled(true);
        ui->sizeSlider->setEnabled(true);
        ui->sizeKbLabel->setEnabled(true);
        mainWindowPtr->setSSTVDRMPushButton(true);
      }
    else
      {
        ui->hybridCheckBox->setEnabled(false);
        ui->binaryPushButton->setEnabled(false);
        ui->sizeLabel->setEnabled(false);
        ui->sizeSlider->setEnabled(false);
        ui->sizeKbLabel->setEnabled(false);
        mainWindowPtr->setSSTVDRMPushButton(false);
      }
  }

  applyTemplate();
}


void txWidget::slotSize(int v)
{
  QString t;
  if(sizeRatio!=v)
    {
      sizeRatioChanged=true;
      sizeRatio=v;
      slotSizeApply();
    }
}

void txWidget::slotSizeApply()
{
  float fileSize;
  QApplication::setOverrideCursor(Qt::WaitCursor);
  sizeRatioChanged=false;
  fileSize=imageViewerPtr->setSizeRatio(sizeRatio,transmissionModeIndex==TRXDRM);
  imageViewerPtr->displayImage();
  ui->sizeKbLabel->setText( QString::number(fileSize/1000)+ "kB");
  applyTemplate();
  QApplication::restoreOverrideCursor();
}

void txWidget::slotTransmissionMode(int rxtxMode)
{
  emit modeSwitch(rxtxMode);
}

void txWidget::changeTransmissionMode(int rxtxMode)
{
  int i;
  transmissionModeIndex=(etransmissionMode)rxtxMode;
  if((transmissionModeIndex>=0)&&(transmissionModeIndex<TRXNOMODE))
    {
      for(i=0;i<TRXNOMODE;i++)
        {
          if(i!=transmissionModeIndex) ui->settingsTableWidget->widget(i)->setEnabled(false);
        }
      ui->settingsTableWidget->widget(transmissionModeIndex)->setEnabled(true);
      ui->settingsTableWidget->blockSignals(true);
      ui->settingsTableWidget->setCurrentIndex(transmissionModeIndex);
      ui->settingsTableWidget->blockSignals(false);

    }
  setSettingsTab();
}

void txWidget::slotProfileChanged(int i)
{
  drmProfileConfigPtr->getDRMParams(i,drmParams);
  setParams();
}

void txWidget::setProfileNames()
{
  QString tmp;
  ui->drmProfileComboBox->clear();
  if(drmProfileConfigPtr->getName(0,tmp))
    {
      ui->drmProfileComboBox->addItem(tmp);
    }
  if(drmProfileConfigPtr->getName(1,tmp))
    {
      ui->drmProfileComboBox->addItem(tmp);
    }
  if(drmProfileConfigPtr->getName(2,tmp))
    {
      ui->drmProfileComboBox->addItem(tmp);
    }
}

void txWidget::slotImageChanged()
{
  int temp=sizeRatio;
  sizeRatio=-1;
  slotSize(temp);
}

void txWidget::slotBinary()
{
  slotGetParams();
  dispatcherPtr->startDRMTxBinary();
}


void txWidget::txTestPattern(etpSelect sel)
{
  txFunctionsPtr->txTestPattern(imageViewerPtr,sel);
}







