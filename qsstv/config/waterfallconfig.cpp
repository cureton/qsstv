#include "waterfallconfig.h"
#include "ui_waterfallconfig.h"
#include <QFont>


QString startPicWF;
QString endPicWF;
QString fixWF;
QString bsrWF;
QString wfFont;
int wfFontSize;

waterfallConfig::waterfallConfig(QWidget *parent) : baseConfig(parent),  ui(new Ui::waterfallConfig)
{
  ui->setupUi(this);
  connect(ui->fontComboBox,SIGNAL(currentIndexChanged(QString)),SLOT(slotFontChanged()));
  connect(ui->sizeSpinBox,SIGNAL(valueChanged(int)),SLOT(slotFontChanged()));
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
}

void waterfallConfig::setParams()
{
  setValue(startPicWF,ui->startPicTextEdit);
  setValue(endPicWF,ui->endPicTextEdit);
  setValue(fixWF,ui->fixTextEdit);
  setValue(bsrWF,ui->bsrTextEdit);
  setValue(wfFont,ui->fontComboBox);
  setValue(wfFontSize,ui->sizeSpinBox);
}

void waterfallConfig::slotFontChanged()
{
  getParams();
  QFont f(wfFont);
  f.setPixelSize(wfFontSize);
  ui->sampleLineEdit->setFont(f);
}


