#ifndef MAINWINDOW_H

#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QRandomGenerator>
#include "algothread.h"
#include <QMessageBox>
#include <QTableWidget>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QPainter>
#include <QMovie>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

extern QVector<int> inputArr;
extern QVector<int> adsArr;
extern int frameLen;
extern int memTime;
extern int pageMissTime;
extern int TLBTime;
extern bool useTLB;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_checkBox_stateChanged(int arg1);

    void on_pushButton_clicked();

    void on_initButton_clicked();

    void on_FIFOButton_clicked();

    void on_LRUButton_clicked();

    void on_LFUButton_clicked();

    void on_OPTButton_clicked();

    void on_allStartButton_clicked();

    void update(const QVector<int> frames, int missPos,
                int curPos, int errorCnt, Algorithm algorithm, int curTime, int totalTime);

    void on_tabWidget_currentChanged(int index);

    void store();

    void threadOver();


    void on_refreshButton_clicked();

private:
    Ui::MainWindow *ui;
    AlgoThread *FIFOThread;
    AlgoThread *LRUThread;
    AlgoThread *LFUThread;
    AlgoThread *OPTThread;
    int pageTableLen = 10;

    QString randomGen(int adsRange, int arrayLen);
    void setHeader(QTableWidget *table);
    QString getLabelNum(QLabel* label);
    void dataInit();
    void chartGen();

    int doFIFO(int frameSize);
    int doLRU(int frameSize);
    int doOPT(int frameSize);

    QString toString(QVector<int> inputArr);

};
#endif // MAINWINDOW_H
