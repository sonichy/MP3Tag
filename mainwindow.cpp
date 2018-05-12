#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "form.h"
#include "ui_form.h"
#include <QDesktopWidget>
#include <QFileDialog>
#include <QDebug>
#include <QTextCodec>
#include <QMessageBox>
#include <QMimeData>

#include <QLineEdit>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    move((QApplication::desktop()->width()-width())/2, (QApplication::desktop()->height()-height())/2);
    connect(ui->action_quit, SIGNAL(triggered()), qApp, SLOT(quit()));

    comboBox = new QComboBox;
    comboBox->setFixedWidth(100);
    comboBox->setFocusPolicy(Qt::NoFocus);
    connect(comboBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(changeCodec(QString)));
    ui->toolBar->addWidget(comboBox);

    lineEdit_filter = new QLineEdit;
    lineEdit_filter->setFixedWidth(70);
    lineEdit_filter->setPlaceholderText("过滤");
    connect(lineEdit_filter,SIGNAL(textChanged(QString)),this,SLOT(filter(QString)));
    ui->toolBar->addWidget(lineEdit_filter);

    listCodecs = QTextCodec::availableCodecs();
    filter("");
    TC = QTextCodec::codecForLocale();
    comboBox->setCurrentIndex(comboBox->findText(TC->name()));
    ui->statusBar->showMessage("共 "+ QString::number(listCodecs.size()) + " 种编码，系统编码 " + TC->name());

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
    //qDebug() << "open" << filename;
    ui->label->setPixmap(QPixmap());
    path = filename;
    setWindowTitle("MP3Tag - " + QFileInfo(filename).fileName());
    ui->textBrowser->setText("");

    QLayoutItem *layoutItem;
    while ((layoutItem = ui->verticalLayout_2->takeAt(0)) != 0 ) {
        delete layoutItem->widget();
        delete layoutItem;
    }

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
            } else if (FTag == "TYER") {
                qDebug() << FTag << FSize << BA.toHex().toUpper();
                //if(BA.contains("\xFF\xFE")){
                    qDebug() << FTag << "Unicode" << BA.mid(3,FSize-3).toHex().toUpper();
                    //ui->textBrowser->append(FTag + ": " + QString::fromUtf16(reinterpret_cast<const ushort*>(BA.mid(3,FSize-3).data())));
                    ui->textBrowser->append(FTag + ": " + TC->toUnicode(BA));
                //}//else{
//                    qDebug() << FTag << FSize << BA.mid(1,FSize-2);
//                    ui->textBrowser->append(FTag + ": " + BA.mid(1,FSize-2));
//                }
            } else if (FTag == "COMM") {
                QString language = BA.mid(1,3);
                qDebug() << FTag << FSize << language << BA.mid(10,FSize-12).toHex().toUpper();
                //ui->textBrowser->append(FTag + ": " + language + " "+ QString::fromUtf16(reinterpret_cast<const ushort*>(BA.mid(10,FSize-12).data())));
                ui->textBrowser->append(FTag + ": " + TC->toUnicode(BA));
            } else {
                if(FTag != ""){
                    QByteArray UFlag = BA.left(1);
                    qDebug() << "UFlag" << UFlag.toHex().toUpper();
                    //if(UFlag == "\x00"){
                        qDebug() << FTag << BA.right(FSize-1).toHex().toUpper();
                        ui->textBrowser->append(FTag + ": " + TC->toUnicode(BA.right(1)));
                    //}else{
                        // QByteArray转UTF16 https://stackoverflow.com/questions/11279371/converting-utf-16-qbytearray-to-qstring
                        //qDebug() << FTag << BA.right(FSize-3).toHex().toUpper();
                        //ui->textBrowser->append(FTag + ": " + QString::fromUtf16(reinterpret_cast<const ushort*>(BA.right(FSize-3).data())));
                    //}
                }
            }
        }
    } else {
        ui->textBrowser->append("没有ID3V2");
    }
    ui->textBrowser->append("----------------------------------------");
    pos = file.size()-128;
    file.seek(pos);
    QString TAG,Title,Artist,Album,Year,Comment,Reserved,Track,Genre;
    TAG = QString(file.read(3));
    if (TAG == "TAG") {
        qDebug() << "----------------------------------------ID3V1--------------------------------------------";

        QByteArray BA;
        ui->textBrowser->append("ID3V1");
        BA = file.read(30);
        qDebug() << "Title" << BA.toHex().toUpper();
        Title = TC->toUnicode(BA);
        ui->textBrowser->append("标题：" + Title);
        Form *form = new Form;
        form->BA = BA;
        form->ui->label_tag->setText("标题：");
        form->ui->lineEdit_content->setText(Title);
        form->ui->comboBox->findText(comboBox->currentText());
        form->ui->lineEdit_filter->setText(lineEdit_filter->text());
        ui->verticalLayout_2->addWidget(form);

        BA = file.read(30);
        qDebug() << "Artist" << BA.toHex().toUpper();
        Artist = TC->toUnicode(BA);
        ui->textBrowser->append("歌手：" + Artist);
        form = new Form;
        form->BA = BA;
        form->ui->label_tag->setText("歌手：");
        form->ui->lineEdit_content->setText(Artist);
        form->ui->comboBox->findText(comboBox->currentText());
        form->ui->lineEdit_filter->setText(lineEdit_filter->text());
        ui->verticalLayout_2->addWidget(form);

        BA = file.read(30);
        qDebug() << "Album" << BA.toHex().toUpper();
        Album = TC->toUnicode(BA);
        ui->textBrowser->append("专辑：" + Album);
        form = new Form;
        form->BA = BA;
        form->ui->label_tag->setText("专辑：");
        form->ui->lineEdit_content->setText(Album);
        form->ui->comboBox->findText(comboBox->currentText());
        form->ui->lineEdit_filter->setText(lineEdit_filter->text());
        ui->verticalLayout_2->addWidget(form);

        BA = file.read(4);
        qDebug() << "Year" << BA.toHex().toUpper();
        Year = QString(BA);
        ui->textBrowser->append("年份：" + Year);
        form = new Form;
        form->BA = BA;
        form->ui->label_tag->setText("年份：");
        form->ui->lineEdit_content->setText(Year);
        form->ui->comboBox->findText(comboBox->currentText());
        form->ui->lineEdit_filter->setText(lineEdit_filter->text());
        ui->verticalLayout_2->addWidget(form);

        BA = file.read(28);
        qDebug() << "Comment" << BA.toHex().toUpper();
        Comment = TC->toUnicode(BA);
        ui->textBrowser->append("备注：" + Comment);
        form = new Form;
        form->BA = BA;
        form->ui->label_tag->setText("备注：");
        form->ui->lineEdit_content->setText(Comment);
        form->ui->comboBox->findText(comboBox->currentText());
        form->ui->lineEdit_filter->setText(lineEdit_filter->text());
        ui->verticalLayout_2->addWidget(form);

        BA = file.read(1);
        qDebug() << "Reserved" << BA.toHex().toUpper();
        Reserved = QString(BA);
        ui->textBrowser->append("保留：" + Reserved);
        form = new Form;
        form->BA = BA;
        form->ui->label_tag->setText("保留：");
        form->ui->lineEdit_content->setText(Reserved);
        form->ui->comboBox->findText(comboBox->currentText());
        form->ui->lineEdit_filter->setText(lineEdit_filter->text());
        ui->verticalLayout_2->addWidget(form);

        BA = file.read(1);
        qDebug() << "Track" << BA.toHex().toUpper();
        Track = QString(BA);
        ui->textBrowser->append("音轨：" + Track);
        form = new Form;
        form->BA = BA;
        form->ui->label_tag->setText("音轨：");
        form->ui->lineEdit_content->setText(Track);
        form->ui->comboBox->findText(comboBox->currentText());
        form->ui->lineEdit_filter->setText(lineEdit_filter->text());
        ui->verticalLayout_2->addWidget(form);

        BA = file.read(1);
        qDebug() << "Genre" << BA.toHex().toUpper();
        Genre = QString::number(BA.toInt());
        ui->textBrowser->append("种类：" + Genre);
        form = new Form;
        form->BA = BA;
        form->ui->label_tag->setText("种类：");
        form->ui->lineEdit_content->setText(Genre);
        form->ui->comboBox->findText(comboBox->currentText());
        form->ui->lineEdit_filter->setText(lineEdit_filter->text());
        ui->verticalLayout_2->addWidget(form);

        ui->verticalLayout_2->addStretch();
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

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    qDebug() << "dragEnter:" << e->mimeData()->formats();
    if(e->mimeData()->hasFormat("text/uri-list"))
        e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *e)
{
    QList<QUrl> urls = e->mimeData()->urls();
    if(urls.isEmpty())
        return ;

    QString fileName = urls.first().toLocalFile();

    foreach (QUrl u, urls) {
        qDebug() << u.toString();
    }
    qDebug() << urls.size();

    if(fileName.isEmpty())
        return;

    qDebug() << "drop:" << fileName;
    open(fileName);
}

void MainWindow::changeCodec(QString codec)
{
    if(path!=""){
        TC = QTextCodec::codecForName(codec.toLatin1());
        if(TC){
            open(path);
        }else{
            ui->statusBar->showMessage(codec + " 编码没找到");
        }
    }
}

void MainWindow::filter(QString s)
{
    comboBox->clear();
    for(int i=0; i<listCodecs.size(); i++){
        if(listCodecs.at(i).contains(s.toLatin1()))
            comboBox->addItem(listCodecs.at(i));
    }
    ui->statusBar->showMessage("过滤出 " + QString::number(comboBox->count()) + " 种编码");
}
