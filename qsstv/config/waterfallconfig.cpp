#include "waterfallconfig.h"
#include "ui_waterfallconfig.h"
#include <QFont>


QString startPicWF;
QString endPicWF;
QString fixWF;
QString bsrWF;
QString wfFont;
int wfFontSize;
bool wfBold;
QString sampleString;

waterfallConfig::waterfallConfig(QWidget *parent) : baseConfig(parent),  ui(new Ui::waterfallConfig)
{
  ui->setupUi(this);
  connect(ui->fontComboBox,SIGNAL(currentIndexChanged(QString)),SLOT(slotFontChanged()));
  connect(ui->sizeSpinBox,SIGNAL(valueChanged(int)),SLOT(slotFontChanged()));
  connect(ui->boldCheckBox,SIGNAL(clicked(bool)),SLOT(slotFontChanged()));
}

waterfallConfig::~waterfallConfig()
{
  delete ui;
}


void waterfallConfig::readSettings()
{
  QSettings qSettings;
  qSettings.beginGroup("WATERFALL");
  startPicWF=qSettings.value("startPicWF","START PIC").toString();
  endPicWF=qSettings.value("endPicWF","END PIC").toString();
  fixWF=qSettings.value("fixWF","FIX").toString();
  bsrWF=qSettings.value("bsrWF","BSR").toString();
  wfFont=qSettings.value("wfFont","Arial").toString();
  wfFontSize=qSettings.value("wfFontSize",12).toInt();
  wfBold=qSettings.value("wfBold",false).toBool();
  sampleString=qSettings.value("sampleString","Sample Text").toString();
  qSettings.endGroup();
  setParams();
}

void waterfallConfig::writeSettings()
{
  QSettings qSettings;
  getParams();
  qSettings.beginGroup("WATERFALL");
  qSettings.setValue("startPicWF",startPicWF);
  qSettings.setValue("endPicWF",endPicWF);
  qSettings.setValue("fixWF",fixWF);
  qSettings.setValue("bsrWF",bsrWF);
  qSettings.setValue("wfFont",wfFont);
  qSettings.setValue("wfFontSize",wfFontSize);
  qSettings.setValue("wfBold",wfBold);
  qSettings.setValue("sampleString",sampleString);

  qSettings.endGroup();
}

void waterfallConfig::getParams()
{
  getValue(startPicWF,ui->startPicTextEdit);
  getValue(endPicWF,ui->endPicTextEdit);
  getValue(fixWF,ui->fixTextEdit);
  getValue(bsrWF,ui->bsrTextEdit);
  getValue(wfFont,ui->fontComboBox);
  getValue(wfFontSize,ui->sizeSpinBox);
  getValue(wfBold,ui->boldCheckBox);
  getValue(sampleString,ui->sampleLineEdit);
}

void waterfallConfig::setParams()
{
  setValue(startPicWF,ui->startPicTextEdit);
  setValue(endPicWF,ui->endPicTextEdit);
  setValue(fixWF,ui->fixTextEdit);
  setValue(bsrWF,ui->bsrTextEdit);
  ui->fontComboBox->blockSignals(true);
  ui->sizeSpinBox->blockSignals(true);
  setValue(sampleString,ui->sampleLineEdit);
  setValue(wfFont,ui->fontComboBox);
  setValue(wfFontSize,ui->sizeSpinBox);
  ui->fontComboBox->blockSignals(false);
  ui->sizeSpinBox->blockSignals(false);
  setValue(wfBold,ui->boldCheckBox);
  slotFontChanged();


}

void waterfallConfig::slotFontChanged()
{
  getParams();
  QFont f(wfFont);
  f.setBold(wfBold);
  f.setPixelSize(wfFontSize);
  ui->sampleLineEdit->setFont(f);
}


