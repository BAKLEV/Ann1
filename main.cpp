#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include "ftd2xx.h"
#include "WinTypes.h"

class Device {
private :
    FT_HANDLE dev {nullptr};


public :
    int openDev() {
        FT_STATUS st = FT_Open(0, &dev);
//        FT_SetTimeouts(dev, 100, 5000);
        return st;
    }

    void closeDev() {
        FT_Close(dev);
        qDebug() << "dev was closed";
    }

    int writeData(const char* data, int size) {
        if (dev == nullptr) {
            return -1;
        }

        DWORD writtenBytes = 0;
        FT_STATUS result = FT_Write(dev, (void*)&data[0], size, &writtenBytes);
        qDebug() << "data to dev was send... status - " << (result == FT_OK ? "OK" : "BAD");
        return writtenBytes - size;
    }

    int readData(char* data, int& size) {
        if (dev == nullptr) {
            return -1;
        }

        DWORD readBytes = 0;
        FT_STATUS result = FT_Read(dev, (void*)data, size, &readBytes);

        size = readBytes;

        QString temp;
        for (int i = 0; i < size; i++) {
            temp.append(QString("0x%1 ").arg((unsigned char)data[i], 2, 16, QLatin1Char(0x30)));
        }
        qDebug() << temp;

        qDebug() << "data to dev was read... status - " << (result == FT_OK ? "OK" : "BAD");
        return readBytes - size;
    }
};

class PidRegulatorDev : public Device {
private :
    int Ki;
    int Kd;
    int Kp;


public :
    PidRegulatorDev() {
        qDebug() << "device is open " << openDev();
    }
    void initPidDev(int Ki, int Kd, int Kp) {}

    void setPwm(int valPwm) {
        unsigned char cmd[3] = {0xF0, 0x01, 0x7E};
        writeData((char*)cmd, 3);
    }

    float getTemperature() {
        //1. request temperature
        //2. stupid wait...
        //3. read value of temperature

        char* data = new char [16];
        int size = 16;
        readData(data, size);

//        convertCmdToTemperature(const char* cmd, int size)

        return 0.1;

    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    PidRegulatorDev *pidDev = new PidRegulatorDev();
    pidDev->setPwm(100);

    pidDev->getTemperature();

    pidDev->closeDev();
    delete pidDev;

    return a.exec();
}
