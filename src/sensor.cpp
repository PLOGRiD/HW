#include "sensor.h"
#include <iostream>
#include <fstream>
#include <wiringPi.h>

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
    data.A = as7265x.getA();
    data.B = as7265x.getB();
    data.C = as7265x.getC();
    data.D = as7265x.getD();
    data.E = as7265x.getE();
    data.F = as7265x.getF();
    data.G = as7265x.getG();
    data.H = as7265x.getH();
    data.I = as7265x.getI();
    data.J = as7265x.getJ();
    data.K = as7265x.getK();
    data.L = as7265x.getL();
    data.R = as7265x.getR();
    data.S = as7265x.getS();
    data.T = as7265x.getT();
    data.U = as7265x.getU();
    data.V = as7265x.getV();
    data.W = as7265x.getW();

    return data;
}

void SpectroscopySensor::collect_data_for_ai(){
    ofstream csvFile;
    csvFile.open(filename, ios::out | ios::app);

    csvFile.seekp(0, ios::end);
    if(csvFile.tellp() == 0){
        csvFile<<"Label,A,B,C,D,E,F,G,H,I,J,K,L,R,S,T,U,V,W\n";
    }

    int userInput =-1;
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

        cout<<"wait...\n";

        SpectroscopyData currentData = readAllChannels();

        csvFile << labels[userInput] << "," << currentData.A << ","<< currentData.B << ","<< currentData.C << ","<< currentData.D << ","<< currentData.E << ","
        << currentData.F << ","<< currentData.G << ","<< currentData.H << ","<< currentData.I << ","<< currentData.J << ","<< currentData.K << ","<< currentData.L << ","
        << currentData.R << ","<< currentData.S << ","<< currentData.T << ","<< currentData.U << ","<< currentData.V << ","<< currentData.W <<"\n";

        cout<<"finish\n";


    }

    csvFile.close();
}