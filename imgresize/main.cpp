/*
EquaRes - a tool for numerical analysis of equations.
Copyright (C) 2014 Stepan Orlov

This file is part of EquaRes.

Full text of copyright notice can be found in file copyright_notice.txt in the EquaRes root directory.
License agreement can be found in file LICENSE.md in the EquaRes root directory.
*/

#include <QCoreApplication>
#include <QImage>
#include <QBuffer>
#include <QFile>
#include <QPainter>
#include <cstdio>
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QStringList args = a.arguments();
    QFile input;
    bool base64input = false;
    if (args.length() == 4) {
        if(args.last() == "64") {
            base64input = true;
            input.open(stdin, QIODevice::ReadOnly);
            Q_ASSERT(input.isOpen());
        }
        else {
            input.setFileName(args.last());
            if (!input.open(QIODevice::ReadOnly)) {
                cerr << "Failed to open input file " << args.last().toStdString() << endl;
                return -1;
            }
        }
    }
    else if (args.length() != 3) {
        cerr << "Image max width and height arguments were required" << endl;
        return -1;
    }
    else {
        input.open(stdin, QIODevice::ReadOnly);
        Q_ASSERT(input.isOpen());
    }
    int width = args[1].toInt(), height = args[2].toInt();

    QByteArray data = input.readAll();
    if (base64input)
        data = QByteArray::fromBase64(data);

    QImage srcimg;
    if (!srcimg.loadFromData(data)) {
        cerr << "Failed to load image" << endl;
        return -1;
    }
    int sw = srcimg.width(), sh = srcimg.height();
    if (sw < 1 || sh < 1) {
        cerr << "Invalid image size" << endl;
        return -1;
    }
    QImage dstimg(width, height, srcimg.format());
    dstimg.fill(0u);
    {
        QPainter painter(&dstimg);
        QRect rc;
        if (sw <= width && sh <= height)
            rc = QRect((width-sw)/2, (height-sh)/2, sw, sh);
        else {
            double rs = static_cast<double>(sw) / sh,
                   r  = static_cast<double>(width) / height;
            if (rs >= r) {
                double s = static_cast<double>(width) / sw;
                int x = static_cast<int>(0.5*(height - sh*s + 1));
                rc = QRect(0, x, width, static_cast<int>(sh*s + 0.5));
            } else {
                double s = static_cast<double>(height) / sh;
                int x = static_cast<int>(0.5*(width - sw*s + 1));
                rc = QRect(x, 0, static_cast<int>(sw*s + 0.5), height);
            }
        }
        painter.drawImage(rc, srcimg, srcimg.rect());
    }

    QBuffer f;
    f.open(QIODevice::WriteOnly);
    dstimg.save(&f, "PNG");
    cout << f.data().toBase64().constData();

    return 0;
}
