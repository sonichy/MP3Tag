#ifndef FORM_H
#define FORM_H

#include <QWidget>
#include <QTextCodec>

namespace Ui {
class Form;
}

class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = 0);
    ~Form();
    Ui::Form *ui;
    QByteArray BA;

private:
    QTextCodec *TC;
    QList<QByteArray> listCodecs;

private slots:
    void changeCodec(QString codec);
    void filter(QString s);

};

#endif // FORM_H
