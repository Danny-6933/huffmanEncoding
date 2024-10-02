#include "mainwindow.h"

#include <QtWidgets>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), frequencies(256,0), charEncodingStrings(0) {

    QWidget *center = new QWidget();
    setCentralWidget(center);

    QVBoxLayout *mainLayout = new QVBoxLayout(center);
    QHBoxLayout *topLayout = new QHBoxLayout();

    loadButton = new QPushButton("load");
    connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadButtonClicked);
    loadButton->setToolTip(tr("Load a file"));

    encodeButton = new QPushButton("encode");
    connect(encodeButton, &QPushButton::clicked, this, &MainWindow::encodeButtonClicked);
    encodeButton->setEnabled(false);
    encodeButton->setToolTip(tr("Encode a file"));

    decodeButton = new QPushButton("decode");
    connect(decodeButton, &QPushButton::clicked, this, &MainWindow::decodeButtonClicked);

    topLayout -> addWidget(loadButton, 0, Qt::AlignCenter);
    topLayout -> addWidget(encodeButton, 0, Qt::AlignCenter);
    topLayout -> addWidget(decodeButton, 0, Qt::AlignCenter);

    mainLayout -> addLayout(topLayout);

    table = new QTableWidget(256,4);
    table->setHorizontalHeaderLabels(QStringList() << "code" << "char" << "occurances" << "encoding");
    table->verticalHeader()->setVisible(false);
    table->setColumnHidden(0,true);
    table->setColumnHidden(1,true);
    table->setColumnHidden(2,true);
    table->setColumnHidden(3,true);
    for (int i = 0; i<256; ++i) {
        QTableWidgetItem* codeItem = new QTableWidgetItem();
        codeItem -> setData(Qt::DisplayRole, i);
        table->setItem(i, 0, codeItem);

        QTableWidgetItem* charItem = new QTableWidgetItem(QChar(i));
        table->setItem(i, 1, charItem);

        QTableWidgetItem* countItem = new QTableWidgetItem();
        countItem -> setData(Qt::DisplayRole, 0);
        table->setItem(i, 2, countItem);

        table->setRowHidden(i, true);
    }


    mainLayout -> addWidget(table);

}

MainWindow::~MainWindow() {}

void MainWindow::loadButtonClicked() {
    table->setSortingEnabled(false);
    encodeButton->setEnabled(true);
    QString fName = QFileDialog::getOpenFileName(this, "Please select file to open", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    if (fName.isEmpty()) return;

    QFile inFile(fName);

    if (!inFile.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, "Error", QString("Cannot open file \"%1\"").arg(fName));
    }

    QFileInfo fileInfo(fName);
    extention = fileInfo.suffix();


    data = inFile.readAll();

    inFile.close();

    ogFileSize = inFile.size();

    if (data.isEmpty()) {
        QMessageBox::information(this, "Empty File", "Your file is empty, nothing to encode:(");
    }
    populateFrequencies();
    table->setColumnHidden(3, true);
}

void MainWindow::encodeButtonClicked() {
    table->setColumnHidden(3,false);
    qDebug() << "encoding in process";

    populateFrequencies();


    codeHuffman();
    // qDebug() << data[0];
    encodeFile();


}

void MainWindow::decodeButtonClicked() {
    qDebug() << "decoding in process";

    decodeFile();

}

void MainWindow::codeHuffman() {
    // QMessageBox::information(this, "Progress", QString("Huffman encoding in process"));

    QMultiMap<int, QByteArray> toDo;
    QMap<QByteArray, QPair<QByteArray, QByteArray> > children;

    for (int i = 0; i < data.size(); i++) {
        uniqueChars.insert(static_cast<unsigned char>(data[i]));
        if (uniqueChars.size() > 1) {
            break;
        }
    }

    if (uniqueChars.size() < 2) {
        return;
    }
    qDebug() << "file as more than 1 unique character";

    for (int code = 0; code < 256; ++code) { // maps a frequency to the QByteArray it corresponds to
        if (frequencies[code] > 0) {
            toDo.insert(frequencies[code], QByteArray(1,code));
        }
    }
    // qDebug() << toDo;
    while (toDo.size() > 1) { // make huffman tree by sorting through toDo list
        int weight0, weight1;
        QByteArray array0,array1;

        weight0 = toDo.begin().key();
        array0 = toDo.begin().value();
        toDo.erase(toDo.begin());

        weight1 = toDo.begin().key();
        array1 = toDo.begin().value();
        toDo.erase(toDo.begin());

        int parentWeights = weight0 + weight1;
        QByteArray parentChars = array0 + array1;

        children[parentChars] = qMakePair(array0, array1);

        toDo.insert(parentWeights, parentChars);
    }
    for (int i = 0; i < table->rowCount(); ++i) {
        table->setRowHidden(i, false);
    }
    // qDebug() << toDo;

    QByteArray root = toDo.begin().value();

    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {

            QByteArray target;
            target.append(static_cast<unsigned char>(i));

            QByteArray current = root;
            QString code;


            while (current != target) {
                if (children[current].first.contains(target)) {
                    code.append("0");
                    current = children[current].first;
                } else if (children[current].second.contains(target)) {
                    code.append("1");
                    current = children[current].second;
                } else {
                    qDebug() << QString("Error: %1 was not found in the Huffman Tree").arg(target);
                    break;
                }
            }
            // qDebug() << "i: "<< i << "code: " << code << "target" << target;
            charEncodingStrings.append(code);

            QTableWidgetItem* encodedChar = new QTableWidgetItem(code);
            table->setItem(i,3, encodedChar);


            // qDebug() << "Character:" << target << "-> Huffman Code:" << code;
        } else {
            charEncodingStrings.append("");
        }
        // qDebug() << charEncodingStrings;
    }

    for (int i = 0; i < 256; ++i) {
        table->setRowHidden(i, !(frequencies[i]>0));
    }
}

void MainWindow::encodeFile() {

    QString encodedFile;
    QByteArray bytesOfFile;
    int fileSize = data.size();
    int fileGarbage;
    bool singleChar = uniqueChars.size() == 1;

    if (!singleChar) {

        for (int i = 0; i < fileSize; i++) {

            unsigned char character = static_cast<unsigned char>(data[i]);
            // qDebug() << charEncodingStrings[character];

            QString encodedStr = charEncodingStrings[character];
            // qDebug() << charEncodingStrings[character] << "char: " << character;
            encodedFile.append(encodedStr);
        }


        fileGarbage = 8 - (encodedFile.size() % 8);

        if (fileGarbage != 8) {
            encodedFile.append(QString(fileGarbage, '0'));
        }

        for (int i = 0; i < encodedFile.size(); i += 8) {
            QString byteString = encodedFile.mid(i, 8);
            bool ok;
            char byte = static_cast<char>(byteString.toUInt(&ok, 2));
            if (ok) {
                bytesOfFile.append(byte);
            } else {
                qDebug() << "Error converting binary string to byte.";
                return;
            }
        }
    }

    // qDebug() << encodedFile;
    // qDebug() << bytesOfFile;


    QString fOutName = QFileDialog::getSaveFileName(this, "Save file to:", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), tr("Binary file (*.bin)"));
    // *.huf or *.bin

    if (fOutName.isEmpty()) return;

    QFile outFile(fOutName);

    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::information(this, "Error", QString("Cannot write to file \"%1\"").arg(fOutName));
        return;
    }

    QDataStream out(&outFile);

    // qDebug() << "262: " << fileSize;


    if (singleChar) {
        qDebug() << "265: " << singleChar << static_cast<quint8>(data[0]) << fileSize << extention;
        out << singleChar << static_cast<quint8>(data[0]) << fileSize << extention;
    } else {

        out << singleChar << charEncodingStrings << fileGarbage << extention;
        out.writeRawData(bytesOfFile.data(), bytesOfFile.size());

    }

    outFile.close();

    // message to user

    codedFileSize = outFile.size();

    QString fileInfo = QString("The original file size is %1 bytes\nThe new file size is %2 bytes").arg(ogFileSize / 8).arg(codedFileSize / 8);

    QMessageBox::information(this, "File Update", fileInfo);

}

void MainWindow::decodeFile() {

    QString fName = QFileDialog::getOpenFileName(this, "Please select file to decode", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    if (fName.isEmpty()) return;

    QFile inFile(fName);

    if (!inFile.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, "Error", QString("Cannot open file \"%1\"").arg(fName));
    }
    QVector<QString> encodingKey;
    int offset;
    QString fileExt;
    bool singleChar;
    QDataStream in (&inFile);
    data.clear();

    in >> singleChar;

    if (singleChar) {
        int charOccurances;
        char charInFile;
        in >> charInFile >> charOccurances >> fileExt;
        qDebug() << "308:" << charInFile << charOccurances << fileExt;

        for (int i = charOccurances; i > 0; --i) {
            data.append(charInFile);
        }


    } else {
        in >> encodingKey >> offset >> fileExt;

        QByteArray encodedData = inFile.readAll();

        QString binaryString;
        for (int i = 0; i < encodedData.size(); ++i) {
            QString byteString = QString::number((unsigned char)encodedData[i], 2).rightJustified(8, '0');
            binaryString.append(byteString);
        }


        if (offset > 0) {
            binaryString.chop(offset);
        }

        QString currentBits;
        for (int i = 0; i < binaryString.size(); ++i) {
            currentBits.append(binaryString[i]);
            int charIndex = encodingKey.indexOf(currentBits);
            if (charIndex != -1) {
                data.append(static_cast<char>(charIndex));
                currentBits.clear();
            }
        }

        for (int i = 0; i < 256; ++i) {
            QTableWidgetItem* encodedChar = new QTableWidgetItem(encodingKey[i]);
            table->setItem(i,3, encodedChar);

        }
    }
    inFile.close();

    populateFrequencies();

    table->setColumnHidden(3,false);


    QString saveFileName = QFileDialog::getSaveFileName(this, "Save decoded file as:", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));


    if (saveFileName.isEmpty()) return;

    saveFileName.append("." + fileExt);

    QFile outFile(saveFileName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::information(this, "Error", QString("Cannot save file \"%1\"").arg(saveFileName));
        return;
    }

    outFile.write(data);
    outFile.close();



    QMessageBox::information(this, "Success", "File decoded and saved successfully!");

}

void MainWindow::populateFrequencies() {
    // table -> setSortingEnabled(false);

    // ensure frequencies is reset
    for (int i = 0; i < 256; ++i) {
        frequencies[i] = 0;
    }

    int fLen = data.length();

    for (int iPos = 0; iPos < fLen; ++iPos) {
        ++frequencies[(unsigned char)data[iPos]];
    }

    for (int i = 0; i < 256; ++i) {
        QTableWidgetItem* count = new QTableWidgetItem();
        count -> setData(Qt::DisplayRole, frequencies[i]);
        table->setItem(i, 2, count);
        table->setRowHidden(i, !(frequencies[i]>0));
    }
    // qDebug() << frequencies;

    table->setColumnHidden(0,false);
    table->setColumnHidden(1,false);
    table->setColumnHidden(2,false);

    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // encodeButton->setEnabled(true);
}



// TESTS:
// empty file GOOD
// file "aaa"
// text GOOD
// binary data









