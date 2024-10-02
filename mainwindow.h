#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QPushButton *loadButton;
    QPushButton *encodeButton;
    QPushButton *decodeButton;

private:
    QTableWidget *table;
    QByteArray data;
    QVector<int> frequencies;
    QMap<QByteArray, QString> encodingMap;
    QVector<QString> charEncodingStrings;
    QString extention;

    int ogFileSize;
    int codedFileSize;
    QSet<unsigned char> uniqueChars;


public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


public slots:
    void loadButtonClicked();
    void encodeButtonClicked();
    void decodeButtonClicked();
    void populateFrequencies();
    void codeHuffman();
    void encodeFile();
    void decodeFile();
};


#endif // MAINWINDOW_H
