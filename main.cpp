#include <iostream>
#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include "ftd2xx.h"
#include "WinTypes.h"
#include <unistd.h>

class Device {
private :
    FT_HANDLE dev {nullptr};


public :
    int openDev() {
        FT_STATUS st = FT_Open(0, &dev);
        FT_SetTimeouts(dev, 100, 5000);
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
    short int Ki;
    short int Kd;
    short int Kp;
    short int SetTemp;
    short int PDT;
    short int I;
    short int CycleTime;
    short int CurTemp;

public :
    PidRegulatorDev() {
        qDebug() << "device is open " << openDev();
    }
    void PidRegulatorDev::PidRegulatorDev (float ki, float kd, float kp, float settemp, float ct) {
        Ki = ki;
        Kd = kd;
        Kp = kp;
        SetTemp = settemp;
        PDT = 0;
        I = 0;
        CycleTime = ct;
    }
    float PWMCalculate() {
        getTemperature();
        short int DT = SetTemp - CurTemp;
        short int P = Kp * DT;
        I = I + Ki * DT * CycleTime;
        short int D = Kd * (DT - PDT) / CycleTime;
        PDT = DT;
        short int pid = P + I + D;
        if pid > 1024:
            pid = 1024;
        return pid;
    }
    
    void setPwm() {
        //установка скважности
        short int pid = PWMCalculate();
        unsigned char yb, olb, syncb, nameb, xorb;
        unsigned char cmd[5] = { 0x7E, 0x01, 0, 0, 0 };
        yb = pid; // мл байт
        olb = pid >> 8; // ст байт
        syncb = 0x7E;
        nameb = 0x01;
        xorb = yb ^ olb ^ syncb ^ nameb;
        cmd[2] = yb;
        cmd[3] = olb;
        cmd[4] = xorb;
        writeData((char*)cmd, 3);
    }

    float getTemperature() {
        //1. request temperature
        unsigned char cmd[3] = { 0x7E, 0x02, 0x11};
        writeData((char*)cmd, 3);
        //2. stupid wait...
        sleep(0,1);
        //3. read value of temperature
        char* data = new char[16];
        int size = 16;
        readData(data, size);
        CurTemp = .....;
        return 0;
    }

    void convertCmdToTemperature(const char* cmd, int size) {
       //перевод из 16 -- 2
        //смещение вправо на незнач биты
        //перевод в 10
        //*0,0625
     }

       

};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    PidRegulatorDev *pidDev = new PidRegulatorDev("ki", "kp", "kd", "settemp", "cycletime");
    pidDev->getTemperature();
    while CurTemp != SetTemp:
        pidDev->getTemperature();
        pidDev->setPwm;
        pidDev->getTemperature();
    pidDev->closeDev();
    delete pidDev;

    return a.exec();
}
