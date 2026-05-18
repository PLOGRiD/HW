#include "sensor.h"
#include <iostream>
#include <fstream>
#include <wiringPi.h>
#include <string>

using namespace std;

SpectroscopySensor::SpectroscopySensor(){
    filename = "trash_spectroscopy_data.csv";
    labels = {
        {1, "empty"},
        {2, "PET"},
        {3, "glass"},
        {0, "END"}
    };
}

bool SpectroscopySensor::init(){
    if (!as7265x.begin()){
        std::cout<<"[AS7265X Sensor Error] Not found\n";
        return false;
    }
    return true;
}

SpectroscopyData SpectroscopySensor::readAllChannels(){
    SpectroscopyData data;

    as7265x.takeMeasurementsWithBulb();
    delay(100);
    data.A = as7265x.getCalibratedA();
    data.B = as7265x.getCalibratedB();
    data.C = as7265x.getCalibratedC();
    data.D = as7265x.getCalibratedD();
    data.E = as7265x.getCalibratedE();
    data.F = as7265x.getCalibratedF();
    data.G = as7265x.getCalibratedG();
    data.H = as7265x.getCalibratedH();
    data.I = as7265x.getCalibratedI();
    data.J = as7265x.getCalibratedJ();
    data.K = as7265x.getCalibratedK();
    data.L = as7265x.getCalibratedL();
    data.R = as7265x.getCalibratedR();
    data.S = as7265x.getCalibratedS();
    data.T = as7265x.getCalibratedT();
    data.U = as7265x.getCalibratedU();
    data.V = as7265x.getCalibratedV();
    data.W = as7265x.getCalibratedW();

    return data;
}

void SpectroscopySensor::collect_data_for_ai(){
    ofstream csvFile;
    csvFile.open(filename, ios::out | ios::app);

    csvFile.seekp(0, ios::end);
    if(csvFile.tellp() == 0){
        csvFile<<"Label,A,B,C,D,E,F,G,H,I,J,K,L,R,S,T,U,V,W,note\n";
    }

    int userInput =-1;
    string note;
    while(true){
        cout<<"---------------------------\n";
        for(const auto& pair:labels){
            cout<<"["<<pair.first<<"] "<< pair.second<<"\n";
        }
        cout<<"---------------------------\n";
        cout<<"write label num\n";
        
        cin>>userInput;

        if(labels.find(userInput) == labels.end()){
            cout<<"wrong num, retry\n";
            continue;
        }

        if (userInput == 0){
            cout<<"end\n";
            break;
        }

        note=" ";
        cout<<"memo : ";
        cin>>note;

        cout<<"wait...\n";

        SpectroscopyData currentData = readAllChannels();

        csvFile << labels[userInput] << "," << currentData.A << ","<< currentData.B << ","<< currentData.C << ","<< currentData.D << ","<< currentData.E << ","
        << currentData.F << ","<< currentData.G << ","<< currentData.H << ","<< currentData.I << ","<< currentData.J << ","<< currentData.K << ","<< currentData.L << ","
        << currentData.R << ","<< currentData.S << ","<< currentData.T << ","<< currentData.U << ","<< currentData.V << ","<< currentData.W<<","
        <<note<<"\n";

        cout<<"finish\n";


    }

    csvFile.close();
}