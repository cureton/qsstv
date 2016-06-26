#ifndef RIGCONTROLFORM_H
#define RIGCONTROLFORM_H

#include <QWidget>
#include "rigcontrol.h"


namespace Ui {
class rigControlForm;
}

class rigControlForm : public QWidget
{
  Q_OBJECT
  
public:
  explicit rigControlForm(QWidget *parent = 0);
  ~rigControlForm();
  void attachRigController(rigControl *rigCtrl);
  void readSettings();
  void writeSettings();

  bool needsRestart() { return changed;}


public slots:
  void slotEnableCAT();
  void slotEnablePTT();
  void slotEnableXMLRPC();
  void slotRestart();
  void slotCheckPTT0();
  void slotCheckPTT1();
  void slotCheckPTT2();
  void slotCheckPTT3();
  
private:

  Ui::rigControlForm *ui;
  bool changed;
  void getParams();
  void setParams();
  scatParams *cp;
  rigControl *rigController;
  void checkPTT(int p,bool b);
};

#endif // RIGCONTROLFORM_H
