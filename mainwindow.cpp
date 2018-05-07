#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDesktopWidget>
#include <QFileDialog>
#include <QDebug>
#include <QTextCodec>
#include <QMessageBox>

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
    ui->label->setPixmap(QPixmap());
    path = filename;
    setWindowTitle("MP3Tag - " + QFileInfo(filename).fileName());
    ui->textBrowser->setText("");

    //Qt读写文件 https://blog.csdn.net/zhuyunfei/article/details/51249378
    //Mp3Tag读取 https://blog.csdn.net/junglesong/article/details/1747887
    //Mp3文件结构 https://blog.csdn.net/fulinwsuafcie/article/details/8972346
    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    QString ID3,Ver,Revision,Flag;
    bool ok;
    qint64 pos,size;
    ID3 = QString(file.read(3));
    if (ID3 == "ID3") {
        QTextCodec *TC = QTextCodec::codecForName("GBK");
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
        qDebug() << "ID3V2 Size:" << size;
        ui->textBrowser->append("大小：" + QString::number(size));
        while (file.pos() < size) {
            QString FTag(file.read(4));
            qint64 FSize = file.read(4).toHex().toLongLong(&ok,16);
            //qint64 FSize = file.read(1).toHex().toInt(&ok,16) << 24 | file.read(1).toHex().toInt(&ok,16) << 16 | file.read(1).toHex().toInt(&ok,16) << 8 | file.read(1).toHex().toInt(&ok,16);
            Flag = QString::number(file.read(2).toHex().toInt(&ok,16));
            QByteArray BA = file.read(FSize);
            if (FTag == "APIC") {
                BA = BA.right(FSize-14);
                QPixmap pixmap;
                ok = pixmap.loadFromData(BA);
                //qDebug() << "QPixmap.loadFromData(QByteArray)" << ok;
                ui->label->setPixmap(pixmap);
                break;
            } else {
                if(FTag == "TYER"){
                    qDebug() << FTag << FSize << BA;
                    ui->textBrowser->append(FTag + ": " + BA.mid(1,FSize-2));
                }else if(FTag == "COMM"){
                    QString language = BA.mid(1,3);
                    qDebug() << FTag << FSize << language << BA.mid(10,FSize-12).toHex().toUpper();
                    ui->textBrowser->append(FTag + ": " + language + " "+ QString::fromUtf16(reinterpret_cast<const ushort*>(BA.mid(10,FSize-12).data())));
                }else{
                    // QByteArray转UTF16 https://stackoverflow.com/questions/11279371/converting-utf-16-qbytearray-to-qstring
                    qDebug() << FTag << FSize << BA.mid(3,FSize-5).toHex().toUpper();
                    ui->textBrowser->append(FTag + ": " + QString::fromUtf16(reinterpret_cast<const ushort*>(BA.mid(3,FSize-5).data())));
                }
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
    if (TAG == "TAG") {
        QTextCodec *TC = QTextCodec::codecForName("GBK");
        ui->textBrowser->append("ID3V1");
        Title = TC->toUnicode(file.read(30));
        ui->textBrowser->append("标题：" + Title);
        Artist = TC->toUnicode(file.read(30));
        ui->textBrowser->append("歌手：" + Artist);
        Album = TC->toUnicode(file.read(30));
        ui->textBrowser->append("专辑：" + Album);
        Year = QString(file.read(4));
        ui->textBrowser->append("年份：" + Year);
        Comment = TC->toUnicode(file.read(28));
        ui->textBrowser->append("备注：" + Comment);
        Reserved = QString(file.read(1));
        ui->textBrowser->append("保留：" + Reserved);
        Track = QString(file.read(1));
        ui->textBrowser->append("音轨：" + Track);
        Genre = QString::number(file.read(1).toInt());
        ui->textBrowser->append("种类：" + Genre);
    } else {
        ui->textBrowser->append("没有ID3V1");
    }
    file.close();
    ui->textBrowser->moveCursor(QTextCursor::Start);
}

void MainWindow::on_action_about_triggered()
{
    QMessageBox aboutMB(QMessageBox::NoIcon, "关于", "海天鹰 MP3 ID3 信息查看器 1.0\n\n一款基于 Qt 的 MP3 ID3 信息查看程序。\n作者：黄颖\nE-mail: sonichy@163.com\n主页：https://github.com/sonichy");
    aboutMB.setIconPixmap(QPixmap(":/icon.svg"));
    aboutMB.setWindowIcon(QIcon(":/icon.svg"));
    aboutMB.exec();
}
