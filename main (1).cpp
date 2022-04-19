#include <iostream>
#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include "ftd2xx.h"
#include "WinTypes.h"
#include <unistd.h>
#include <fstream>

class Device {
private :
    FT_HANDLE dev {nullptr};

public :
    ~Device() {
        closeDev();
    }
    int openDev() {
        FT_STATUS st = FT_Open(0, &dev);
        FT_SetTimeouts(dev, 100, 5000);
        return st;
    }

    void closeDev() {
        FT_Close(dev);
        qDebug() << "dev was closed";
    }

    int writeData(const unsigned char* data, int size) {
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
    float Ki;
    int Kd;
    int Kp;
    int SetTemp;
    int PDT;
    int I;
    int CycleTime;
    float CurTemp;

public :
    PidRegulatorDev() {
        qDebug() << "device is open " << openDev();
    }
    PidRegulatorDev (float ki, float kd, float kp, float settemp, float ct) : PidRegulatorDev() {
        Ki = ki;
        Kd = kd;
        Kp = kp;
        SetTemp = settemp;
        PDT = 0;
        I = 0;
        CycleTime = ct;
    }

    ~PidRegulatorDev() {
        setPwm(0);
    }

    int PWMCalculate(float curTemp) {
        int DT = SetTemp - curTemp;
        int P = Kp * DT;
        I = I + Ki * DT * CycleTime;
        int D = Kd * (DT - PDT) / CycleTime;
        PDT = DT;
        int pid = P + I + D;
        if (pid > 1024)
            pid = 1024;
        return pid;
    }
    void work() {
        ofstream fout("project12.txt");
        if (!fout.is_open()) // если файл не открыт
            cout << "Файл не может быть открыт!\n"; // сообщить об этом
        else
        {
            int i = 0;
            while (1) {
                CurTemp = getTemperature();
                int pid = PWMCalculate(CurTemp);
                fout << i << ' ' << CurTemp << ' ' << pid << endl;
                i++;
                setPwm(pid);
                sleep(1);
            }
        }
    }
    void setPwm(int pwmVal) {
        unsigned char yb, olb, syncb, nameb, xorb;
        unsigned char cmd[8] = { 0x7E, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
        yb = pwmVal;
        olb = pwmVal >> 8;
        syncb = 0x7E;
        nameb = 0x01;
        xorb = yb ^ olb ^ syncb ^ nameb;
        cmd[3] = yb;
        cmd[4] = olb;
        cmd[7] = xorb;
        writeData(cmd, 8);
    }

    float getTemperature() {
        //1. request temperature
        unsigned char cmd[8] = {0x7E, 0x07, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
        writeData(cmd, 8);
        //2. stupid wait...
        sleep(1);
        char* data = new char[16];
        int size = 16;
        int readBytes = readData(data, size);
        assert(readBytes == 0);
        //3. read value of temperature
        float temp = data[4] ??? data[3] / 100;
        //osvobodit pamyat
        return temp;
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    PidRegulatorDev *pidDev = new PidRegulatorDev(10, 0, 0, 50, 1);

    pidDev->work();

    delete pidDev;

    return a.exec();
}
