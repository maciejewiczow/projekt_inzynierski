#include <LiquidCrystal_I2C.h>
#include <BasicLinearAlgebra.h>
#include <print_util.h>
#include <KeepMeAlive.h>
#include "lcd_util.h"
#include "Mesh.h"
#include "Communication.h"
#include "Config.h"

using namespace lcdut;
using namespace prnt;

namespace meshconfig {
    constexpr size_t nElements = 5;
    constexpr size_t nNodes = nElements + 1;
}

namespace input {
    /*
        Stałe parametry:
            len - długość pieca [m] - zmienna kompilacji

        Dane wejściowe:
            v0 - prędkośc pierwszej szpuli [m/s] - wprowadzanie ręczne
            v1 - prędkość drugiej szpuli (>=v0) [m/s] - wprowadzanie ręczne
            r - promień wsadu [m] - wprowadzanie ręczne

            t_0 - temperatura początkowa wsadu [deg C] - wprowadzać ręcznie/z drugiego czujnika temp pokojowej??
            t_furnance - temperatura w piecu [deg C] - z termopary
    */
    float furnaceLength = 0.2; // [m]
    float v0 = 0.005;    // [m/s]
    float v1 = 0.005;    // [m/s]
    float r = 0.002;    // [m]

    float t0 = 20;      // [deg C]
}

Config config;

namespace simulation {
    float tauEnd;
    float dTau;
    int nIters;
}

LiquidCrystal_I2C lcd{0x3F, 16, 2};

Mesh<meshconfig::nNodes> mesh;

float getTemp() {
    // return 500.f;

    while (Serial.available() < sizeof(float));

    float res;
    Serial.readBytes((byte*) &res, sizeof(float));

    return res;
}

IterationDataPacket<meshconfig::nNodes> iterData(mesh.nodes);
BenchmarkDataPacket benchmark;

void initializeParams() {
    using namespace simulation;
    // założenia:
    //    - liniowe przyspieszenie między v0 i v1
    //    - piec jest po środku między szpulami
    //    - prędkość nie zmienia się znacząco na długości pieca
    float v = 1.5*input::v0 - 0.5*input::v1;

    tauEnd = input::furnaceLength/v;

    float a = config.K / (config.C*config.Ro);
    float elemSize = input::r/meshconfig::nElements;

    dTau = (elemSize*elemSize)/(0.5*a);
    nIters = (tauEnd/dTau) + 1;
    dTau = tauEnd / nIters;

    iterData.nIterations = nIters;

    mesh.generate(input::t0, elemSize);
}

void setup() {
    Serial.begin(57600);
    lcd.init();
    lcd.backlight();
    watchdogTimer.setDelay(8000);

    lcd << lcdut::pos(3, 0) << (char)0b10111100 << " Ready " << (char)0b11000101;
    // lcd << lcdut::pos(0, 1) << sizeof(unsigned long);

    while (!Serial);

    initializeParams();
    // Serial << "Params: " << endl
    //     << "tauEnd = " << simulation::tauEnd << endl
    //     << "dTau = " << simulation::dTau << endl
    //     << "nIters = " << simulation::nIters << endl << endl;

    iterData.iteration = 0;
    iterData.tau = 0.f;
    iterData.nodes = mesh.nodes;
}

void loop() {
    static float temp = getTemp();

    iterData.send();

    benchmark.start().send();
    mesh.integrateStep(simulation::dTau, input::r, temp, config);
    benchmark.end().send().clear();

    if (iterData.iteration < simulation::nIters) {
        iterData.iteration++;
        iterData.tau += simulation::dTau;
    } else {
        lcd << lcdut::clear << lcdut::pos(0, 0) << "Wew " << mesh.nodes[0].t << lcdut::symbols::deg << "C";
        lcd << lcdut::pos(0, 1) << "Zew " << mesh.nodes[meshconfig::nNodes-1].t << lcdut::symbols::deg << "C";

        iterData.iteration = 0;
        iterData.tau = 0.f;

        for (auto& node : mesh.nodes)
            node.t = input::t0;

        temp = getTemp();
    }
}
