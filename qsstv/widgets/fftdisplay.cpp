#include "fftdisplay.h"
#include "appglobal.h"
#include "configparams.h"
#include "math.h"
#include "arraydumper.h"
#include <QPainter>


fftDisplay::fftDisplay(QWidget *parent) : QLabel(parent)
{
  //  blockIndex=0;
  arMagSAvg=NULL;
  //  hammingBuffer=NULL;
  fftArray=NULL;
  //  out=NULL;
  //  dataBuffer=NULL;
  showWaterfall=false;
  fftMax=FFTMAX;
  range=RANGE;
  avgVal=0.05;
  //  plan=0;
  setScaledContents(true);
  imagePtr=NULL;
  imWidth=-1;
  imHeight=-1;
  arMagWAvg=NULL;
  displayCounter=0;
}

fftDisplay::~fftDisplay()
{
  delete imagePtr;
  if(fftArray) delete fftArray;
  if(arMagSAvg) delete []arMagSAvg;
  if(arMagWAvg) delete []arMagWAvg;
}


void fftDisplay::init(int length,int nblocks,int isamplingrate)
{
  int i;
  windowSize=length;
  fftLength=windowSize*nblocks;
  samplingrate=isamplingrate;
  if(fftArray) delete fftArray;
  if(arMagSAvg) delete [] arMagSAvg;
  step=(double)samplingrate/(double)fftLength;  //freq step per bin
  binBegin=(int) rint(FFTLOW/step);
  binEnd  =(int) rint(FFTHIGH/step);
  binDiff=binEnd-binBegin;

  fftArray=new QPolygon(binDiff);
  arMagSAvg=new double[binDiff];

  for(i=0;i<binDiff;i++)
    {
      arMagSAvg[i]=-30.;
    }

  // create the fftw plan
  //  plan = fftw_plan_r2r_1d(fftLength, dataBuffer, out, FFTW_R2HC, FFTW_ESTIMATE);
  update();
  QLabel::update();
}




void fftDisplay::setMarkers(int mrk1, int mrk2, int mrk3)
{
  marker1=mrk1;
  marker2=mrk2;
  marker3=mrk3;
  update();
}

void fftDisplay::showFFT(double *fftData)
{
  int i,j;
  QColor c;
  double re,imag,tmp;
  if((!showWaterfall) && (slowCPU))
    {
      if(displayCounter++<1) return;
      else displayCounter=0;
    }
  if((imWidth!=width()) || (imHeight!=height()))
    {
      if(imWidth!=width())
        {
          if(arMagWAvg!=NULL) delete [] arMagWAvg;
          arMagWAvg=new double[width()];
          for(i=0;i<width();i++)
            {
              arMagWAvg[i]=0;
            }
        }
      imWidth=width();
      imHeight=height();
      if(showWaterfall)
        {
          if(imagePtr==NULL)
            {
              imagePtr=new QImage( width(),height(),QImage::Format_RGB32);
              imagePtr->fill(Qt::black);
            }
          else
            {

              *imagePtr=imagePtr->scaled(QSize(imWidth,imHeight));
            }
        }
    }
  if(!showWaterfall)
    {
      for (i=binBegin,j=0;i<binEnd;i++,j++)
        {
          re=fftData[i]/fftLength;
          imag=fftData[fftLength-i]/fftLength;
          tmp=10*log10((re*re+imag*imag))-77.27;  // 0.5 Vtt is 0db
          if(arMagSAvg[j]<-100)
            {
              arMagSAvg[j]=-100;
            }
          if(arMagSAvg[j]<tmp)  arMagSAvg[j]=arMagSAvg[j]*(1-0.4)+0.4*tmp;
          else arMagSAvg[j]=arMagSAvg[j]*(1-avgVal)+avgVal*tmp;
          tmp=(fftMax-arMagSAvg[j])/range;
          if(tmp<0) tmp=0;
          if (tmp>1)tmp=1;
          int pos=(int)rint((double)(j*(imWidth-1))/(double)binDiff);
          fftArray->setPoint(j,pos,(imHeight-1)*tmp); // range 0 -> -1
        }
    }
  else
    {

      memmove(imagePtr->scanLine(1),imagePtr->scanLine(0),(imWidth*(imHeight-2))*sizeof(uint));
      uint *ptr=(uint *)imagePtr->scanLine(0);
      for(i=0;i<imWidth;i++)
        {
          int idx=rint((double)(FFTLOW+(i*FFTSPAN)/(double)imWidth)/step);
          re=fftData[idx]/fftLength;
          imag=fftData[fftLength-idx]/fftLength;
          tmp=10*log10((re*re+imag*imag))-77.27;  // 0.5 Vtt is 0db
          arMagWAvg[i]=arMagWAvg[i]*(1-avgVal)+avgVal*tmp;
          if( arMagWAvg[i]<-100)
            {
               arMagWAvg[i]=-100;
            }
          tmp=1-(fftMax-arMagWAvg[i])/range;
          if(tmp<0) tmp=0;
          if (tmp>1)tmp=1;
          c.setHsv(240-tmp*60,255,tmp*255);
          ptr[i]=c.rgb();
        }
    }
  update();
}




void fftDisplay::paintEvent(QPaintEvent *p)
{
  QPen pn;
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  if(!showWaterfall)
    {
      pn.setColor(Qt::red);
      pn.setWidth(1);
      painter.setPen(pn);
      painter.drawLine((((marker1-FFTLOW)*imWidth)/FFTSPAN),0,(((marker1-FFTLOW)*imWidth)/FFTSPAN),imHeight);
      painter.drawLine((((marker2-FFTLOW)*imWidth)/FFTSPAN),0,(((marker2-FFTLOW)*imWidth)/FFTSPAN),imHeight);
      painter.drawLine((((marker3-FFTLOW)*imWidth)/FFTSPAN),0,(((marker3-FFTLOW)*imWidth)/FFTSPAN),imHeight);
      pn.setColor(Qt::green);
      painter.setPen(pn);
      painter.drawPolyline(*fftArray);
    }
  else
    {
      if(imagePtr)
        {
          scaledImage=imagePtr->scaled(QSize(width(),height()));
          painter.drawImage(0,0,scaledImage);
        }
    }
  QLabel::paintEvent(p);
}

