#ifndef WATERFALLCONFIG_H
#define WATERFALLCONFIG_H

#include "baseconfig.h"

extern QString startPicWF;
extern QString endPicWF;
extern QString fixWF;
extern QString bsrWF;
extern QString wfFont;
extern int wfFontSize;


namespace Ui {
class waterfallConfig;
}

class waterfallConfig : public baseConfig
{
  Q_OBJECT
  
public:
  explicit waterfallConfig(QWidget *parent = 0);
  ~waterfallConfig();
  void readSettings();
  void writeSettings();
  void getParams();
  void setParams();

public slots:
  void slotFontChanged();
  
private:
  Ui::waterfallConfig *ui;
};

#endif // WATERFALLCONFIG_H
