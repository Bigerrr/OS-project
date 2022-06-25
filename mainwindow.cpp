#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setCentralWidget(ui->tabWidget);
    ui->textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    ui->chartView->setRenderHint(QPainter::Antialiasing);

    // 算法进程初始化
    FIFOThread = new AlgoThread(FIFO);
    LRUThread = new AlgoThread(LRU);
    LFUThread = new AlgoThread(LFU);
    OPTThread = new AlgoThread(OPT);

    connect(FIFOThread, SIGNAL(processOver(const QVector<int>, int, int, int, Algorithm, int, int)),
            this, SLOT(update(const QVector<int>, int, int, int, Algorithm, int, int)));
    connect(LRUThread, SIGNAL(processOver(const QVector<int>, int, int, int, Algorithm, int, int)),
            this, SLOT(update(const QVector<int>, int, int, int, Algorithm, int, int)));
    connect(LFUThread, SIGNAL(processOver(const QVector<int>, int, int, int, Algorithm, int, int)),
            this, SLOT(update(const QVector<int>, int, int, int, Algorithm, int, int)));
    connect(OPTThread, SIGNAL(processOver(const QVector<int>, int, int, int, Algorithm, int, int)),
            this, SLOT(update(const QVector<int>, int, int, int, Algorithm, int, int)));

    connect(FIFOThread, SIGNAL(finished()), this, SLOT(threadOver()));
    connect(LRUThread, SIGNAL(finished()), this, SLOT(threadOver()));
    connect(LFUThread, SIGNAL(finished()), this, SLOT(threadOver()));
    connect(OPTThread, SIGNAL(finished()), this, SLOT(threadOver()));

    connect(ui->storeButton, SIGNAL(clicked()), this, SLOT(store()));


}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_checkBox_stateChanged(int arg1)
{
    ui->TLBTimeBox->setEnabled(arg1);
    useTLB = arg1;
}

// 响应生成序列按钮，获取并传参给randomGen
void MainWindow::on_pushButton_clicked()
{
    int adsRange = ui->pageNumBox->value() + 12; // 确定地址范围，从box中取出页号位数，页面4KB则加12位
    int arrayLen = ui->inputLenEdit->text().toInt(); // 获取输入序列长度
    ui->textEdit->clear(); // 清空
    ui->textEdit->setPlainText(randomGen(adsRange, arrayLen)); // 生成随机数序列写入到界面文本框
}

// 生成序列存入字符串中
QString MainWindow::randomGen(int adsRange, int arrayLen)
{
    QString s;
    int upperBound = (unsigned int)(-1) >> (32 - adsRange); // 获取adsRange下的地址上界
    for(int i = 0; i < arrayLen; i++) {
        int num = QRandomGenerator::global()->generate() % upperBound; // 根据上界产生随机数
        if(i != 0) s.append(",");
        s.append(QString::asprintf("%X", num));
    }
    return s;
}

//设定表头
void MainWindow::setHeader(QTableWidget *table)
{
    //QTableWidgetItem *headerItem; // 表格项类指针
    table->clear();
    QStringList headerText;
    headerText << "逻辑地址" << "页号";
    for(int i = 0; i < frameLen; i++)
        headerText << (QString::asprintf("物理块%d", i + 1));
    headerText << "是否缺页" << "访问时间";
    table->setRowCount(headerText.count());
    table->setColumnCount(inputArr.size());
    table->setVerticalHeaderLabels(headerText);
}

QString MainWindow::getLabelNum(QLabel *label)
{
    QString s = label->text().split(" ")[1];
    s.append(" ");
    return s;
}


void MainWindow::dataInit()
{
    inputArr.clear();
    for(int i = 0; i < 20; i++) {
        int num = QRandomGenerator::global()->bounded(0, 16);
        inputArr.push_back(num);
    }
}

int MainWindow::doFIFO(int frameSize)
{
    int errorCnt = 0, pos = 0;
    QVector<int> pageFrame(frameSize, -1);
    QMap<int, int> map; // 模拟页表查询
    for(int i = 0; i < inputArr.length(); i++) {
        if(!map.contains(inputArr[i])) { // 缺页中断
            ++errorCnt;
            if(map.count() == frameSize) { // 一般未命中
                map.remove(pageFrame[pos]);
                map.insert(inputArr[i], pos);
                pageFrame[pos] = inputArr[i];
            }
            else { // 一开始的冷不命中
                map.insert(inputArr[i], pos);
                pageFrame[pos] = inputArr[i];
            }
            pos = (pos + 1) % frameSize; // 更新索引
        }
    }
    return errorCnt;
}

int MainWindow::doLRU(int frameSize)
{
    int errorCnt = 0, pos = 0;
    QVector<int> pageFrame(frameSize, -1);
    QList<int> visited;
    QMap<int, int> map; // 模拟页表查询
    for(int i = 0; i < inputArr.length(); i++) {
        if(!map.contains(inputArr[i])) { // 缺页中断
            ++errorCnt;
            if(map.count() == frameSize) { // 一般未命中
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
        }
        else { // 命中
            pos = map[inputArr[i]];
            int index = visited.indexOf(pos);
            visited.removeAt(index);
            visited.push_back(pos);
        }
    }
    return errorCnt;
}

int MainWindow::doOPT(int frameSize)
{
    int errorCnt = 0, pos = 0;
    QVector<int> pageFrame(frameSize, -1), pageCount(frameSize, 0);
    QMap<int, int> map; // 模拟页表查询
    for(int i = 0; i < inputArr.length(); i++) {
        if(!map.contains(inputArr[i])) { // 缺页中断
            ++errorCnt;
            if(map.count() == frameSize) { // 一般未命中
                int posMin = 0;
                for(int j = 0; j < frameSize; j++) {
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
        }
        else { // 命中
            pos = map[inputArr[i]];
        }
        pageCount[pos]++;
    }

    return errorCnt;
}

QString MainWindow::toString(QVector<int> inputArr)
{
    QString s;
    for(int i = 0; i < inputArr.length(); i++) {
        if(i) s.append(",");
        s.append(QString::asprintf("%d", inputArr[i]));
    }
    return s;
}


// 响应初始化按钮
void MainWindow::on_initButton_clicked()
{
    // 基本信息读入
    QString s = ui->textEdit->toPlainText(); // 从界面文本框获取输入字符串
    QStringList list = s.split(",", Qt::SkipEmptyParts); //以','为界分割并跳过空白字符串
    bool ok;
    adsArr.resize(0);
    inputArr.resize(0);
    for(int i = 0; i < list.length(); i++) { // 初始化输入序列
        adsArr.push_back(list[i].toUInt(&ok, 16));
        inputArr.push_back((unsigned int) adsArr[i] >> 12);
    }
    if(inputArr.empty()) {
        QMessageBox::warning(this, "警告", "输入序列不可为空！");
        return;
    };

    frameLen = ui->frameLenEdit->text().toInt();
    memTime = ui->memTimeBox->value();
    pageMissTime = ui->misTimeBox->value();
    TLBTime = ui->TLBTimeBox->value();
    useTLB = ui->checkBox->isChecked();

    // 初始化表格
    setHeader(ui->FIFOTable);
    setHeader(ui->LRUTable);
    setHeader(ui->LFUTable);
    setHeader(ui->OPTTable);

    // 进度条重置
    ui->FIFOProgressBar->setValue(0);
    ui->LRUProgressBar->setValue(0);
    ui->LFUProgressBar->setValue(0);
    ui->OPTProgressBar->setValue(0);

    // 按键可用
    ui->FIFOButton->setEnabled(true);
    ui->FIFOButton->setText("FIFO开始");
    ui->LRUButton->setEnabled(true);
    ui->LRUButton->setText("LRU开始");
    ui->LFUButton->setEnabled(true);
    ui->LFUButton->setText("LFU开始");
    ui->OPTButton->setEnabled(true);
    ui->OPTButton->setText("OPT开始");

    ui->allStartButton->setEnabled(true);
}


void MainWindow::on_FIFOButton_clicked()
{
    if(ui->FIFOButton->text() == "暂停") {
        ui->FIFOButton->setText("继续");
        // 进程暂停操作
        FIFOThread->threadPause();
    }
    else if(ui->FIFOButton->text() == "继续") {
        ui->FIFOButton->setText("暂停");
        // 进程重启操作
        FIFOThread->threadStart();
    }
    else {
        FIFOThread->start();
        ui->FIFOButton->setText("暂停");
    }
}


void MainWindow::on_LRUButton_clicked()
{
    if(ui->LRUButton->text() == "暂停") {
        ui->LRUButton->setText("继续");
        // 进程暂停操作
        LRUThread->threadPause();
    }
    else if(ui->LRUButton->text() == "继续") {
        ui->LRUButton->setText("暂停");
        // 进程重启操作
        LRUThread->threadStart();
    }
    else {
        LRUThread->start();
        ui->LRUButton->setText("暂停");
    }
}


void MainWindow::on_LFUButton_clicked()
{
    if(ui->LFUButton->text() == "暂停") {
        ui->LFUButton->setText("继续");
        // 进程暂停操作
        LFUThread->threadPause();
    }
    else if(ui->LFUButton->text() == "继续") {
        ui->LFUButton->setText("暂停");
        // 进程重启操作
        LFUThread->threadStart();
    }
    else {
        LFUThread->start();
        ui->LFUButton->setText("暂停");
    }
}


void MainWindow::on_OPTButton_clicked()
{
    if(ui->OPTButton->text() == "暂停") {
        ui->OPTButton->setText("继续");
        // 进程暂停操作
        OPTThread->threadPause();
    }
    else if(ui->OPTButton->text() == "继续") {
        ui->OPTButton->setText("暂停");
        // 进程重启操作
        OPTThread->threadStart();
    }
    else {
        OPTThread->start();
        ui->OPTButton->setText("暂停");
    }
}


void MainWindow::on_allStartButton_clicked()
{
    // 分别启动进程
    FIFOThread->start();
    LRUThread->start();
    LFUThread->start();
    OPTThread->start();

    // 按键状态修改
    ui->allStartButton->setDisabled(true);
    ui->FIFOButton->setText("暂停");
    ui->FIFOButton->setEnabled(true);
    ui->LRUButton->setText("暂停");
    ui->LRUButton->setEnabled(true);
    ui->LFUButton->setText("暂停");
    ui->LFUButton->setEnabled(true);
    ui->OPTButton->setText("暂停");
    ui->OPTButton->setEnabled(true);
}

// 根据信号传参进行处理
void MainWindow::update(const QVector<int> frames, int misPos, int curPos, int errorCnt, Algorithm algorithm, int curTime, int totalTime)
{
    QTableWidget *table;
    QTableWidgetItem *item;
    QProgressBar *bar;
    QLabel *label;
    QString s, tmp;
    switch (algorithm) { // 根据算法不同分别赋值
    case FIFO:
        table = ui->FIFOTable;
        bar = ui->FIFOProgressBar;
        label = ui->FIFOLabel;
        tmp = "FIFO: ";
        break;
    case LRU:
        table = ui->LRUTable;
        bar = ui->LRUProgressBar;
        label = ui->LRULabel;
        tmp = "LRU: ";
        break;
    case LFU:
        table = ui->LFUTable;
        bar = ui->LFUProgressBar;
        label = ui->LFULabel;
        tmp = "LFU: ";
        break;
    case OPT:
        table = ui->OPTTable;
        bar = ui->OPTProgressBar;
        label = ui->OPTLabel;
        tmp = "OPT: ";
    }
    // 逻辑地址填表
    item = new QTableWidgetItem(QString::asprintf("%X", adsArr[curPos]));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    table->setItem(0, curPos, item);
    // 页号填表
    item = new QTableWidgetItem(QString::asprintf("%u", (unsigned) adsArr[curPos] >> 12));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    table->setItem(1, curPos, item);
    // 物理块填表
    for(int i = 0; i < frames.length(); i++) {
        if(frames[i] == -1) continue;
        item = new QTableWidgetItem(QString::asprintf("%u", frames[i]));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        if(i == misPos)
            item->setForeground(Qt::red);
        table->setItem(i + 2, curPos, item);
    }
    // 是否缺页填表
    if(misPos >= 0)
        s = "√";
    item = new QTableWidgetItem(s);
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    table->setItem(frames.length() + 2, curPos, item);
    // 访问时间填表
    item = new QTableWidgetItem(QString::asprintf("%dns", curTime));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    table->setItem(frames.length() + 3, curPos, item);

    bar->setValue((double) errorCnt / inputArr.length() * 100); // 缺页率进度条更新
    tmp.append(QString::asprintf("%dns", totalTime)); // 总时间更新
    label->setText(tmp);
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    // 历史记录界面触发
    if(index == 1) {
        // 读文件
        QString curPath = QDir::currentPath() + "/history.txt";
        QString dlgTitle = "打开历史记录文件";
        QString filter = "文本文件(*.txt)";
        QString filename = QFileDialog::getOpenFileName(this, dlgTitle, curPath, filter);

        // 画表头
        QTableWidget *table = ui->tableWidget;
        QStringList headerText;
        headerText << "驻留内存页个数" << "访问页面数" << "页号范围" << "快表访问时间"
                   << "内存访问时间" << "缺页中断时间" << "输入序列"
                   << "FIFO访问时间" << "LFU访问时间" << "LRU访问时间" << "OPT访问时间"
                   << "FIFO缺页率" << "LFU缺页率" << "LRU缺页率" << "OPT缺页率";
        table->setColumnCount(headerText.count());
        table->setRowCount(100);
        table->setHorizontalHeaderLabels(headerText);
        // 画表格
        QFile file(filename);
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream stream(&file);
        QTableWidgetItem *item;
        QStringList list = stream.readAll().split("\n", Qt::SkipEmptyParts);
        table->setRowCount(list.length());
        for(int i = 0; i < list.length(); i++) {
            QStringList line = list[i].split(" ", Qt::SkipEmptyParts);
            for(int j = 0; j < line.length(); j++) {
                item = new QTableWidgetItem(line[j]);
                item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
                table->setItem(i, j, item);
            }
        }
        file.close();
    }
    else if(index == 2) {
        chartGen();
    }

}

void MainWindow::store()
{
    // 打开文件
    QString s;
    QString fileName = QDir::currentPath() + "/history.txt";
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
        return;
    QTextStream stream(&file);

    // 文件结尾附加数据
    s.append(QString::asprintf("%d %d %d ", frameLen,
                               inputArr.length(), ui->pageNumBox->value()));
    s.append(QString::asprintf("%d %d %d ", TLBTime, memTime, pageMissTime));
    for(int i = 0; i < adsArr.length(); i++) {
        s.append(QString::asprintf("%X;", adsArr[i]));
    }
    s.append(" ");
    s.append(getLabelNum(ui->FIFOLabel));
    s.append(getLabelNum(ui->LRULabel));
    s.append(getLabelNum(ui->LFULabel));
    s.append(getLabelNum(ui->OPTLabel));
    s.append(QString::asprintf("%d%% ", ui->FIFOProgressBar->value()));
    s.append(QString::asprintf("%d%% ", ui->LRUProgressBar->value()));
    s.append(QString::asprintf("%d%% ", ui->LFUProgressBar->value()));
    s.append(QString::asprintf("%d%% ", ui->OPTProgressBar->value()));

    stream << s << "\n";
    file.close();

    QMessageBox::information(this, "消息", "保存成功！");
    ui->storeButton->setDisabled(true);
}

void MainWindow::threadOver()
{
    if(FIFOThread->isFinished() && LRUThread->isFinished() && LFUThread->isFinished() && OPTThread->isFinished())
        ui->storeButton->setEnabled(true);
}


void MainWindow::chartGen()
{
    QChart *chart = new QChart();
    chart->setTitle("页面置换缺页数曲线");
    QFont font("黑体", 32, 16);
    chart->setTitleFont(font);
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->legend()->setAlignment(Qt::AlignRight);
    font = *(new QFont("黑体", 16, 10));
    chart->legend()->setFont(font);

    ui->chartView->setChart(chart);

    QLineSeries *FIFO = new QLineSeries();
    QLineSeries *LRU = new QLineSeries();
    QLineSeries *OPT = new QLineSeries();

    FIFO->setName("FIFO");
    LRU->setName("LRU");
    OPT->setName("OPT");

    for(int i = 1; i <= 12; i++) {
        FIFO->append(i, doFIFO(i));
        LRU->append(i, doLRU(i));
        OPT->append(i, doOPT(i));
    }

    QString s = toString(inputArr);
    ui->inputlabel->setText(s);

    chart->addSeries(FIFO);
    chart->addSeries(LRU);
    chart->addSeries(OPT);
    chart->createDefaultAxes();

    chart->axes(Qt::Horizontal).first()->setRange(0, 12);
    chart->axes(Qt::Vertical).first()->setRange(6, 20);

    QValueAxis *axisX = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first());
    QValueAxis *axisY = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).first());

    font = *(new QFont("宋体", 12, 20));
    axisX->setLabelFormat("%d");
    axisX->setTickCount(13);
    axisX->setTitleText("内存驻留集个数");
    axisX->setTitleFont(font);
    axisX->setLabelsFont(QFont("微软雅黑", 12, 8));

    axisY->setLabelFormat("%d");
    axisY->setTickCount(15);
    axisY->setTitleText("缺页数");
    axisY->setTitleFont(font);
    axisY->setLabelsFont(QFont("微软雅黑", 12, 8));
}


void MainWindow::on_refreshButton_clicked()
{
    QLineSeries *FIFO = (QLineSeries *)ui->chartView->chart()->series().at(0);
    QLineSeries *LRU = (QLineSeries *)ui->chartView->chart()->series().at(1);
    QLineSeries *OPT = (QLineSeries *)ui->chartView->chart()->series().at(2);

    FIFO->clear();
    LRU->clear();
    OPT->clear();

    if(QRandomGenerator::global()->bounded(0, 5) == 3)
        inputArr = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5, 7, 4, 3, 2, 5, 6, 3, 1};
    else
        dataInit();
    QString s = toString(inputArr);
    ui->inputlabel->setText(s);
    for(int i = 1; i <= 12; i++) {
        FIFO->append(i, doFIFO(i));
        LRU->append(i, doLRU(i));
        OPT->append(i, doOPT(i));
    }
}



