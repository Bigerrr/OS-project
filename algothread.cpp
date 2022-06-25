#include "algothread.h"

QVector<int> inputArr;
QVector<int> adsArr;
int frameLen; // 驻留内存大小
int memTime = 100; // 内存访问时间
int pageMissTime = 105; // 缺页中断时间
int TLBTime = 5; // 查询快表时间
bool useTLB; // 是否使用快表

void AlgoThread::doFIFO()
{
    int errorCnt = 0, pos = 0, totalTime = 0, curTime, missPos = -1;
    QVector<int> pageFrame(frameLen, -1);
    QMap<int, int> map; // 模拟页表查询
    for(int i = 0; i < inputArr.length(); i++) {
        mutex.lock();
        curTime = 0;
        missPos = -1;
        if(!map.contains(inputArr[i])) { // 缺页中断
            ++errorCnt;
            if(useTLB) curTime += TLBTime;
            curTime += (3 * memTime);
            curTime += pageMissTime;
            if(map.count() == frameLen) { // 一般未命中
                map.remove(pageFrame[pos]);
                map.insert(inputArr[i], pos);
                pageFrame[pos] = inputArr[i];
            }
            else { // 一开始的冷不命中
                map.insert(inputArr[i], pos);
                pageFrame[pos] = inputArr[i];
            }
            missPos = pos;
            pos = (pos + 1) % frameLen; // 更新索引
        }
        else { // 命中
            if(useTLB) curTime += (TLBTime + memTime);
            else curTime += (2 * memTime);
        }
        totalTime += curTime;
        // 处理完毕信号
        emit processOver(pageFrame, missPos, i, errorCnt, FIFO, curTime, totalTime);
        mutex.unlock();
        msleep(curTime); // 按当前消耗时间睡眠
    }
}

void AlgoThread::doLRU()
{
    int errorCnt = 0, pos = 0, totalTime = 0, curTime, missPos = -1;
    QVector<int> pageFrame(frameLen, -1);
    QList<int> visited;
    QMap<int, int> map; // 模拟页表查询
    for(int i = 0; i < inputArr.length(); i++) {
        mutex.lock();
        curTime = 0;
        missPos = -1;
        if(!map.contains(inputArr[i])) { // 缺页中断
            ++errorCnt;
            if(useTLB) curTime += TLBTime;
            curTime += (3 * memTime);
            curTime += pageMissTime;
            if(map.count() == frameLen) { // 一般未命中
                pos = visited.at(0); // 头部为最长时间未访问
                visited.removeAt(0);
                map.remove(pageFrame[pos]);
                map.insert(inputArr[i], pos);
                pageFrame[pos] = inputArr[i];
            }
            else { // 一开始的冷不命中
                pos = map.count();
                map.insert(inputArr[i], pos);
                pageFrame[pos] = inputArr[i];
            }
            visited.push_back(pos);
            missPos = pos;
        }
        else { // 命中
            pos = map[inputArr[i]];
            int index = visited.indexOf(pos);
            visited.removeAt(index);
            visited.push_back(pos);
            if(useTLB) curTime += (TLBTime + memTime);
            else curTime += (2 * memTime);
        }
        totalTime += curTime;
        // 处理完毕信号
        emit processOver(pageFrame, missPos, i, errorCnt, LRU, curTime, totalTime);
        mutex.unlock();
        msleep(curTime); // 按当前消耗时间睡眠
    }
}

void AlgoThread::doLFU()
{
    int errorCnt = 0, pos = 0, totalTime = 0, curTime, missPos = -1;
    QVector<int> pageFrame(frameLen, -1), pageCount(frameLen, 0);
    QMap<int, int> map; // 模拟页表查询
    for(int i = 0; i < inputArr.length(); i++) {
        mutex.lock();
        curTime = 0;
        missPos = -1;
        if(!map.contains(inputArr[i])) { // 缺页中断
            ++errorCnt;
            if(useTLB) curTime += TLBTime;
            curTime += (3 * memTime);
            curTime += pageMissTime;
            if(map.count() == frameLen) { // 一般未命中
                int pageMin = 10000000;
                for(int j = 0; j < frameLen; j++) {
                    if(pageCount[j] < pageMin) {
                        pos = j;
                        pageMin = pageCount[j];
                    }
                    pageCount[j] = 0;
                }
                map.remove(pageFrame[pos]);
                map.insert(inputArr[i], pos);
                pageFrame[pos] = inputArr[i];
            }
            else { // 一开始的冷不命中
                pos = map.count();
                map.insert(inputArr[i], pos);
                pageFrame[pos] = inputArr[i];

            }
            missPos = pos;
        }
        else { // 命中
            pos = map[inputArr[i]];
            if(useTLB) curTime += (TLBTime + memTime);
            else curTime += (2 * memTime);
        }
        totalTime += curTime;
        pageCount[pos]++;
        // 处理完毕信号
        emit processOver(pageFrame, missPos, i, errorCnt, LFU, curTime, totalTime);
        mutex.unlock();
        msleep(curTime); // 按当前消耗时间睡眠
    }
}

void AlgoThread::doOPT()
{
    int errorCnt = 0, pos = 0, totalTime = 0, curTime, missPos = -1;
    QVector<int> pageFrame(frameLen, -1), pageCount(frameLen, 0);
    QMap<int, int> map; // 模拟页表查询
    for(int i = 0; i < inputArr.length(); i++) {
        mutex.lock();
        curTime = 0;
        missPos = -1;
        if(!map.contains(inputArr[i])) { // 缺页中断
            ++errorCnt;
            if(useTLB) curTime += TLBTime;
            curTime += (3 * memTime);
            curTime += pageMissTime;
            if(map.count() == frameLen) { // 一般未命中
                int posMin = 0;
                for(int j = 0; j < frameLen; j++) {
                    int listPos = -1;
                    for(int k = i + 1; k < inputArr.length(); k++) {
                        if(inputArr[k] == pageFrame[j]) {
                            listPos = k;
                            break;
                        }
                    }
                    if(listPos == -1) {
                        pos = j;
                        break;
                    }
                    else{
                        if(listPos > posMin) {
                            posMin = listPos;
                            pos = j;
                        }
                    }
                }
                map.remove(pageFrame[pos]);
                map.insert(inputArr[i], pos);
                pageFrame[pos] = inputArr[i];
            }
            else { // 一开始的冷不命中
                pos = map.count();
                map.insert(inputArr[i], pos);
                pageFrame[pos] = inputArr[i];
            }
            missPos = pos;
        }
        else { // 命中
            pos = map[inputArr[i]];
            if(useTLB) curTime += (TLBTime + memTime);
            else curTime += (2 * memTime);
        }
        totalTime += curTime;
        pageCount[pos]++;
        // 处理完毕信号
        emit processOver(pageFrame, missPos, i, errorCnt, OPT, curTime, totalTime);
        mutex.unlock();
        msleep(curTime); // 按当前消耗时间睡眠
    }
}

void AlgoThread::run()
{
    switch (algorithm) {
    case FIFO:
        doFIFO();
        break;
    case LRU:
        doLRU();
        break;
    case LFU:
        doLFU();
        break;
    case OPT:
        doOPT();
        break;
    }
    quit();
}
