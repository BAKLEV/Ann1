#include <iostream>
#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include "ftd2xx.h"
#include "WinTypes.h"
#include <unistd.h>
#include <fstream>
#include <QThread>
#include <QTime>

using namespace std;

class Device {
private :
    FT_HANDLE dev {nullptr};

public :
    ~Device() {
        closeDev();
    }
    int openDev() {
        FT_STATUS st = FT_Open(0, &dev);
        FT_SetTimeouts(dev, 1000, 5000);

        FT_SetBaudRate(dev, 115200);
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
        FT_STATUS result = FT_Read(dev, (void*)&data[0], size, &readBytes);

        std::cout << std::dec << "r : ";
        for (int i = 0; i < (int)readBytes; i++) {
            std::cout << std::hex << (unsigned int)(data[i] & 0x00FF) << " ";
        }
        std::cout << std::endl;

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
    float I;
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
    }

    ~PidRegulatorDev() {
        setPwm(0);
    }

    int PWMCalculate(float curTemp) {
        int DT = SetTemp - curTemp;
        int P = Kp * DT;
        I = I + Ki * DT;
        std::cout << "I = " << I << std::endl;
        int D = Kd * (DT - PDT);
        PDT = DT;
        int pid = P + I + D;
        if (pid > 8096)
            pid = 8096;
        return pid;
    }
    void work() {
        ofstream fout("project12.txt");
        if (!fout.is_open()) {
            cout << "File is not opened!\n";
        }
        else
        {
            int i = 0;
            QTime time;
            while (1) {
                CurTemp = getTemperature();
                int pid = PWMCalculate(CurTemp);
                time = time.currentTime();
                fout << i << "\t" << time.hour() << ":" << time.minute() << ":" << time.second() << '\t' << CurTemp << '\t' << pid << endl;
                std::cout << CurTemp << " " << pid << std::endl;
                i++;
                setPwm(pid);
                sleep(1);
            }
            fout.close();
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
        unsigned char cmd[9] = {0x7E, 0x07, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        writeData(cmd, 9);
        //2. stupid wait...
        QThread::msleep(100);
        int size = 16;
        char data[16];// = new char[16];
        int readBytes = readData(&data[0], size);
//        assert(readBytes == 0);
        //3. read value of temperature
        unsigned short tempTemp = (unsigned char)data[4];
        tempTemp <<= 8;
        tempTemp |= (unsigned char)data[3];
        float temp = (float)tempTemp / 100.0;
        return temp;
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    PidRegulatorDev *pidDev = new PidRegulatorDev(Ki, Kd, Kp, SetTemp);

    pidDev->work();

    delete pidDev;

    return a.exec();
}
