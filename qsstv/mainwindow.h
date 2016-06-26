#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class configDialog;
class spectrumWidget;

namespace Ui {
  class MainWindow;
  }

class mainWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit mainWindow(QWidget *parent = 0);
  ~mainWindow();
  void init();
  void startRunning();
  void setNewFont();
  void setPTT(bool p);
  void setSSTVDRMPushButton(bool inDRM);
  spectrumWidget *spectrumFramePtr;

private slots:
  void slotConfigure();
  void slotExit();
  void slotResetLog();
  void slotLogSettings();
  void slotAboutQt();
  void slotAboutQSSTV();
  void slotDocumentation();
  void slotCalibrate();
  void slotModeChange(int);
  void slotSendID();
  void slotSendBSR();
  void slotSendWfText();
  void slotSetFrequency(int freqIndex);


#ifndef QT_NO_DEBUG
  void slotShowDataScope();
  void slotShowSyncScopeNarrow();
  void slotShowSyncScopeWide();
  void slotScopeOffset();
  void slotDumpSamplesPerLine();

#endif

private:
  Ui::MainWindow *ui;
  void closeEvent ( QCloseEvent *e );
  void readSettings();
  void writeSettings();
  void restartSound(bool inStartUp);
  void cleanUpCache(QString dirPath);
  QComboBox *transmissionModeComboBox;
  QPushButton *wfTextPushButton;
  QPushButton *fixPushButton;
  QPushButton *bsrPushButton;
  QPushButton *idPushButton;
  QComboBox *freqComboBox;
  QLabel pttText;
  QLabel *pttIcon;
  QLabel *freqDisplay;
  void timerEvent(QTimerEvent *);
  QStringList modModeList;
};

#endif // MAINWINDOW_H
