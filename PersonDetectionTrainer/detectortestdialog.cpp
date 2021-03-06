#include "detectortestdialog.h"
#include "ui_detectortestdialog.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include "trainermainwindow.h"
#include "detectionwindow.h"
detectorTestDialog::detectorTestDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::detectorTestDialog)
{
    ui->setupUi(this);
}
void test_thread(detectorTestDialog *dialog, MyPersonDetector *detector,QStringList posData,QStringList negData,Mutex * mtx){
    for (QString filename:posData){
        int res=0;
        int cnt=0;

        Mat img=getCentral64x128Image(imread(filename.toStdString()));
        double pr=detector->predict(img);
        res+=isPositiveClass(pr);
        m_dbg<<"pos "<<pr;
        mtx->lock();
        dialog->updateProgress(res,++cnt);
        mtx->unlock();
        if (dialog->getTerminated())return;
    }
    for (QString filename:negData){

        int res=0;
        int cnt=0;
        Mat img=getCentral64x128Image( imread(filename.toStdString()));
        double pr=detector->predict(img);
        res+=isNegativeClass(pr);
        m_dbg<<"neg "<<pr;
        mtx->lock();
        dialog->updateProgress(res,++cnt);
        mtx->unlock();
        if (dialog->getTerminated())return;
    }
}

int getDataSize(QStringList posData,QStringList negData){
    return posData.size()+negData.size();
    int res=0;
    for (QString filename:negData){
        Mat img=imread(filename.toStdString());

        vector<DetectionWindow> slidingWindow=applySlidingWindow(img);
        for (DetectionWindow subimg:slidingWindow){
            res++;
        }
    }
    for (QString filename:posData){
        Mat img=imread(filename.toStdString());

        vector<DetectionWindow> slidingWindow=applySlidingWindow(img);
        for (DetectionWindow subimg:slidingWindow){
            res++;
        }
    }
    return res;
}
detectorTestDialog::detectorTestDialog(QWidget *parent,MyPersonDetector *detector,QStringList posData,QStringList negData) :
    detectorTestDialog(parent)
{
    terminated=false;
    total=0;
    totalCorrect=0;
    ui->progressBar->setRange(0,getDataSize(posData,negData));
    connect(this,SIGNAL(progressChanged(int)),ui->progressBar,SLOT(setValue(int)));
    numOfThreads=8;
    threads=new std::thread*[numOfThreads];
    int pst=0,psz=posData.size()/numOfThreads+1;
    int nst=0,nsz=negData.size()/numOfThreads+1;
    for (int t=0;t<numOfThreads;++t){
        QStringList pos;
        int en=pst;
        for (int i=pst;i<pst+psz&&i<posData.size();++i){
            pos+=posData[i];
            en=i+1;
        }
        pst=en;
        en=nst;
        QStringList neg;
        for (int i=nst;i<nst+nsz&&i<negData.size();++i){
            neg+=negData[i];
            en=i+1;
        }
        nst=en;
        m_dbg<<pos.size()<<neg.size();
        threads[t]=(new std::thread(test_thread,this,detector,pos,neg,&mtx));
    }

}
void detectorTestDialog::updateProgress(int correct,int all){
    totalCorrect+=correct;total+=all;
    ui->label->setText(QString("%1 correct out of %2 , Accuracy = %3\%").arg(totalCorrect).arg(total).arg(100.0*totalCorrect/total));
    progressChanged(total);

}

bool detectorTestDialog::getTerminated()
{
    return terminated;
}

void detectorTestDialog::cancel()
{
    m_dbg<<"canceling";
    terminated=true;

    for (int t=0;t<numOfThreads;++t ){
            threads[t]->join();
    }
    close();
}

detectorTestDialog::~detectorTestDialog()
{
    delete ui;
    for (int i=0;i<numOfThreads;++i){
        delete threads[i];
    }
    delete[] threads;
}
