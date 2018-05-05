#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDesktopWidget>
#include <QFileDialog>
#include <QDebug>
#include <QTextCodec>
#include <QBuffer>
#include <QImageReader>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    move((QApplication::desktop()->width()-width())/2, (QApplication::desktop()->height()-height())/2);
    connect(ui->action_quit, SIGNAL(triggered()), qApp, SLOT(quit()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_action_open_triggered()
{
    if (path == "") {
        path = QFileDialog::getOpenFileName(this, "打开文本", ".", "*.mp3");
    } else {
        path = QFileDialog::getOpenFileName(this, "打开文本", path, "*.mp3");
    }
    if (!path.isEmpty()) {
        open(path);
    }
}

void MainWindow::open(QString filename)
{
    qDebug() << "open" << filename;
    path = filename;
    setWindowTitle(QFileInfo(filename).fileName() + "[*]");
    ui->textBrowser->setText("");
    QTextCodec *TC = QTextCodec::codecForName("GBK");
    //Qt读写文件 https://blog.csdn.net/zhuyunfei/article/details/51249378
    //Mp3Tag读取 https://blog.csdn.net/junglesong/article/details/1747887
    //Mp3文件结构 https://blog.csdn.net/fulinwsuafcie/article/details/8972346
    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    QString ID3,Ver,Revision,Flag;
    bool ok;
    qint64 pos,size;
    ID3 = QString(file.read(3));
    if(ID3 == "ID3"){
        ui->textBrowser->append(ID3);
        Ver = QString::number(file.read(1).toHex().toInt(&ok,16));
        ui->textBrowser->append("版本号：" + Ver);
        Revision = QString::number(file.read(1).toHex().toInt(&ok,16));
        ui->textBrowser->append("副版本号：" + Revision);
        Flag = QString::number(file.read(1).toHex().toInt(&ok,16));
        ui->textBrowser->append("Flag：" + Flag);
        //size = file.read(4).toHex().toLongLong(&ok,16);
//        QByteArray a = file.read(1);
//        QByteArray b = file.read(1);
//        QByteArray c = file.read(1);
//        QByteArray d = file.read(1);
//        qDebug() << a << b << c << d;
        size = (file.read(1).toHex().toInt(&ok,16) & 0xEF) << 21 | (file.read(1).toHex().toInt(&ok,16) & 0xEF) << 14 | (file.read(1).toHex().toInt(&ok,16) & 0xEF) << 7 | file.read(1).toHex().toInt(&ok,16) & 0xEF;
        //size = 20000;
        qDebug() << "ID3Size" << size;
        ui->textBrowser->append("大小：" + QString::number(size));
        while (file.pos() < size) {
            QString FTag(file.read(4));
            qint64 FSize = file.read(4).toHex().toLongLong(&ok,16);
            //qint64 FSize = file.read(1).toHex().toInt(&ok,16) << 24 | file.read(1).toHex().toInt(&ok,16) << 16 | file.read(1).toHex().toInt(&ok,16) << 8 | file.read(1).toHex().toInt(&ok,16);
            qDebug() << FTag << FSize;
            Flag = QString::number(file.read(2).toHex().toInt(&ok,16));
            QByteArray BA = file.read(FSize);
            if (FTag == "APIC") {
                QPixmap pixmap;
                ok = pixmap.loadFromData(BA);
                qDebug() << "QPixmap.loadFromData(QByteArray)" << ok;
                pixmap.save("cover.jpg");
                ui->label->setPixmap(pixmap);
                break;
            } else {
                ui->textBrowser->append(FTag + ": " + QString(TC->toUnicode(BA)));
            }
        }
    }else{
        ui->textBrowser->append("没有ID3V2");
    }
    ui->textBrowser->append("----------------------------------------");
    pos = file.size()-128;
    file.seek(pos);
    QString TAG,Title,Artist,Album,Year,Comment,Reserved,Track,Genre;
    TAG = QString(file.read(3));
    if(TAG == "TAG"){
        ui->textBrowser->append(TAG);
        Title = QString(TC->toUnicode(file.read(30)));
        ui->textBrowser->append("标题：" + Title);
        Artist = QString(TC->toUnicode(file.read(30)));
        ui->textBrowser->append("歌手：" + Artist);
        Album = QString(TC->toUnicode(file.read(30)));
        ui->textBrowser->append("专辑：" + Album);
        Year = QString(file.read(4));
        ui->textBrowser->append("年份："+Year);
        Comment = QString(TC->toUnicode(file.read(28)));
        ui->textBrowser->append("备注：" + Comment);
        Reserved = QString(file.read(1));
        ui->textBrowser->append("保留：" + Reserved);
        Track = QString(file.read(1));
        ui->textBrowser->append("音轨：" + Track);
        Genre = QString::number(file.read(1).toInt());
        ui->textBrowser->append("种类：" + Genre);
    }else{
        ui->textBrowser->append("没有ID3V1");
    }
    file.close();
    ui->textBrowser->moveCursor(QTextCursor::Start);
}
