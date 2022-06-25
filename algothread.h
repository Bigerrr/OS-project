#ifndef ALGOTHREAD_H
#define ALGOTHREAD_H
#include <QThread>
#include <QVector>
#include <QMap>
#include <QMutex>
#include <QWaitCondition>

enum Algorithm {
    FIFO,
    LRU,
    LFU,
    OPT,
};

class AlgoThread : public QThread
{
    Q_OBJECT
private:
    Algorithm algorithm;
    bool pause;
    QMutex mutex;
    QWaitCondition algoContinue;

    void doFIFO();
    void doLRU();
    void doLFU();
    void doOPT();

protected:
    void run() Q_DECL_OVERRIDE;

public:
    AlgoThread(Algorithm algo) : algorithm(algo){};
    void threadPause() { mutex.lock(); }
    void threadStart() { mutex.unlock(); }

signals:
    void processOver(const QVector<int> frames, int missPos, int curPos,
                     int errorCnt, Algorithm algorithm, int curTime, int totalTime);

};



#endif // ALGOTHREAD_H
