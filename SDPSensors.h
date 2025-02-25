/*
    SDP3x.h - Library for the SDP31 and SDP32 digital pressure sensors produced by Sensirion.
    Created by Bryan T. Meyers, February 14, 2017.
    Modified by UT2UH on May 9th, 2021 to add SDP8xx sensors.

    Copyright (c) 2018 Bryan T. Meyers

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software
    and associated documentation files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all copies or
    substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
    BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
    DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef SDPSENSORS_H
#define SDPSENSORS_H

#include "Arduino.h"
#include <Wire.h>

/* Model allows us to specify which digital SDP3x sensor is detected */
typedef enum {
    SDP31_500,
    SDP32_125,
    SDP800_500,
    SDP810_500,
    SDP801_500,
    SDP811_500,
    SDP800_125,
    SDP810_125
} Model;

/*  The valid addresses for SDP3x sensors. */
const uint8_t Address1 = 0x21;  //SDP31, SDP32
const uint8_t Address2 = 0x22;  //SDP31, SDP32
const uint8_t Address3 = 0x23;  //SDP31, SDP32  
const uint8_t Address5 = 0x25;  //SDP8x0
const uint8_t Address6 = 0x26;  //SDP8x1

/* I2C Command Definitions */
const uint8_t StartContMassFlowAvg[2]     = { 0x36, 0x03 };
const uint8_t StartContMassFlow[2]        = { 0x36, 0x08 };
const uint8_t StartContDiffPressureAvg[2] = { 0x36, 0x15 };
const uint8_t StartContDiffPressure[2]    = { 0x36, 0x1E };
const uint8_t StopCont[2]                 = { 0x3F, 0xF9 };
const uint8_t TrigMassFlow[2]             = { 0x36, 0x24 };
const uint8_t TrigMassFlowStretch[2]      = { 0x37, 0x26 };
const uint8_t TrigDiffPressure[2]         = { 0x36, 0x2F };
const uint8_t TrigDiffPressureStretch[2]  = { 0x37, 0x2D };
const uint8_t ReadInfo1[2]                = { 0x36, 0x7C };
const uint8_t ReadInfo2[2]                = { 0xE1, 0x02 };
const uint8_t SoftReset[2]                = { 0x00, 0x06 };

/*  TempCompensation is used to set the temperature compensation mode for the sensor

    MassFlow     - Use this mode for Mass Flow applications
    DiffPressure - Use this mode for Differential Pressure applications (absolute pressure
    matters)
*/
typedef enum { MassFlow, DiffPressure } TempCompensation;

/*  Pressure Range of the sensor instance

    SDP_250 - would be great to have it :)
*/
typedef enum { SDP_NA, SDP_125, SDP_250, SDP_500 } PressureRange;

#define SPD31_500_PID  0x03010100
#define SDP32_125_PID  0x03010200
#define SDP800_500_PID 0x03020100
#define SDP810_500_PID 0x03020A00
#define SDP801_500_PID 0x03020400
#define SDP811_500_PID 0x03020D00
#define SDP800_125_PID 0x03020200
#define SDP810_125_PID 0x03020B00

/*  Model-Specific Parameters

    Pressure Units    - 1/Pa
    Temperature Units - 1/C
*/
const uint8_t DiffScale_500Pa = 60;
const uint8_t DiffScale_125Pa = 240;
const uint8_t SDP3X_TempScale = 200;

/* The SDP3x class can be used to interface any SDP sensors */
class SDPSensor {
    private:
        /* This sensor instance model number */
        Model number;
        /* This sensor instance port */
        TwoWire * port = NULL;
        /* This sensor instance I2C address */
        uint8_t addr;
        /* The Temperature Compensation mode to use */
        TempCompensation comp;
        /* This sensor instance temperature scale */
        uint8_t scale;
        /* Internal buffer to reuse for reads */
        uint8_t buffer[13];

        /*  Send a write command

            @param cmd - the two byte command to send
            @return true iff all ACKs received
        */
        bool writeCommand(const uint8_t cmd[2]);

        /*  Read data back from the device

            @param words - the number of words to read
            @returns true iff all words read and CRC passed
        */
        bool readData(uint8_t words);

    public:
        /*  Constructor

            @param addr - the Address value for I2C
            @param comp - the Temperature Compensation Mode (Mass Flow or Differential Pressure)
            @returns a new SDP3X as configured
        */
        SDPSensor(const uint8_t addr, const TempCompensation comp = DiffPressure, TwoWire &wirePort = Wire);

        /*  Finish Initializing the sensor object

            @returns sensor pressure range, iff everything went correctly or false iff not found
        */
        PressureRange begin();

        /*  Begin taking continuous readings

            @param averaging - average samples until read occurs, otherwise read last value only
            @returns true, iff everything went correctly
        */
        bool startContinuous(bool averaging);

        /*  Disable continuous measurements

            This may be useful to conserve power when sampling all the time is no longer necessary.
            @returns true, iff everything went correctly
        */
        bool stopContinuous();

        /*
         * Get the sensor model.
         *
         * @returns the Model enum
         */
        Model getModel();

        /*  Start a one-shot reading.

            Clock-stretching is used to delay the a Read response to the Master when a new reading
            is not yet available (ie. blocking read). If left disabled, any reading taken less than
            45ms after a trigger will fail.

            @param stretching - enable clock stretching
            @returns true, iff everything went correctly
        */
        bool triggerMeasurement(bool stretching);

        /*  A handy function to read the differential pressure only.
            Same as readMeasurement(pressure, NULL, NULL).
        */
        bool readPressure(int16_t *pressure);

        /*  Get a pending reading.

            This may be used periodically or in a call-back when monitoring interrupts.

            Both "temp" and "scale" should be left NULL if not used. This will reduce read times.
            @param pressure - a pointer to store the raw pressur value
            @param temp  - a pointer to store the raw temperature value
            @param scale - a pointer to store the pressure scaling factor
            @returns true, iff everything went correctly
        */
        bool readMeasurement(int16_t *pressure, int16_t *temp, int16_t *scale);

        /*  Read back the sensor's internal information

            If a serial number is not needed, "serial" should be set to NULL. This will reduce read
           times.
            @param pid     - a pointer to store the 32-bit product ID
            @param serial  - if not null, a pointer to store the 64-bit manufacturer serial number
            @returns true, iff everything went correctly
        */
        bool readProductID(uint32_t *pid, uint64_t *serial);

        /*  Reset the device to default settings

            WARNING: This will reset all other I2C devices that support it.
            @returns true, iff everything went correctly
        */
        bool reset();

        /*  Get the Pressure Scale for this sensor

            @returns scale in units of 1/Pa
        */
        uint8_t getPressureScale();

        /*  Get the Temperature Scale for this sensor

            @returns scale in units of 1/C
        */
        uint8_t getTemperatureScale();

        /*  CRC-8 Lookup Table

            Settings:
            INIT           - 0xFF
            POLY           - 0x31
            Reflect Input  - No
            Reflect Output - No
            Final XOR      - 0x00

            Source: http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
        */
        const uint8_t CRC_LUT[256] =
            { 0x00, 0x31, 0x62, 0x53, 0xC4, 0xF5, 0xA6, 0x97, 0xB9, 0x88, 0xDB, 0xEA, 0x7D, 0x4C, 0x1F,
            0x2E, 0x43, 0x72, 0x21, 0x10, 0x87, 0xB6, 0xE5, 0xD4, 0xFA, 0xCB, 0x98, 0xA9, 0x3E, 0x0F,
            0x5C, 0x6D, 0x86, 0xB7, 0xE4, 0xD5, 0x42, 0x73, 0x20, 0x11, 0x3F, 0x0E, 0x5D, 0x6C, 0xFB,
            0xCA, 0x99, 0xA8, 0xC5, 0xF4, 0xA7, 0x96, 0x01, 0x30, 0x63, 0x52, 0x7C, 0x4D, 0x1E, 0x2F,
            0xB8, 0x89, 0xDA, 0xEB, 0x3D, 0x0C, 0x5F, 0x6E, 0xF9, 0xC8, 0x9B, 0xAA, 0x84, 0xB5, 0xE6,
            0xD7, 0x40, 0x71, 0x22, 0x13, 0x7E, 0x4F, 0x1C, 0x2D, 0xBA, 0x8B, 0xD8, 0xE9, 0xC7, 0xF6,
            0xA5, 0x94, 0x03, 0x32, 0x61, 0x50, 0xBB, 0x8A, 0xD9, 0xE8, 0x7F, 0x4E, 0x1D, 0x2C, 0x02,
            0x33, 0x60, 0x51, 0xC6, 0xF7, 0xA4, 0x95, 0xF8, 0xC9, 0x9A, 0xAB, 0x3C, 0x0D, 0x5E, 0x6F,
            0x41, 0x70, 0x23, 0x12, 0x85, 0xB4, 0xE7, 0xD6, 0x7A, 0x4B, 0x18, 0x29, 0xBE, 0x8F, 0xDC,
            0xED, 0xC3, 0xF2, 0xA1, 0x90, 0x07, 0x36, 0x65, 0x54, 0x39, 0x08, 0x5B, 0x6A, 0xFD, 0xCC,
            0x9F, 0xAE, 0x80, 0xB1, 0xE2, 0xD3, 0x44, 0x75, 0x26, 0x17, 0xFC, 0xCD, 0x9E, 0xAF, 0x38,
            0x09, 0x5A, 0x6B, 0x45, 0x74, 0x27, 0x16, 0x81, 0xB0, 0xE3, 0xD2, 0xBF, 0x8E, 0xDD, 0xEC,
            0x7B, 0x4A, 0x19, 0x28, 0x06, 0x37, 0x64, 0x55, 0xC2, 0xF3, 0xA0, 0x91, 0x47, 0x76, 0x25,
            0x14, 0x83, 0xB2, 0xE1, 0xD0, 0xFE, 0xCF, 0x9C, 0xAD, 0x3A, 0x0B, 0x58, 0x69, 0x04, 0x35,
            0x66, 0x57, 0xC0, 0xF1, 0xA2, 0x93, 0xBD, 0x8C, 0xDF, 0xEE, 0x79, 0x48, 0x1B, 0x2A, 0xC1,
            0xF0, 0xA3, 0x92, 0x05, 0x34, 0x67, 0x56, 0x78, 0x49, 0x1A, 0x2B, 0xBC, 0x8D, 0xDE, 0xEF,
            0x82, 0xB3, 0xE0, 0xD1, 0x46, 0x77, 0x24, 0x15, 0x3B, 0x0A, 0x59, 0x68, 0xFF, 0xCE, 0x9D,
            0xAC };
};


class SDP3X : public SDPSensor {
public:
    SDP3X(const uint8_t addr, TempCompensation comp, TwoWire &port) : SDPSensor(addr, comp, port) {}
    SDP3X(const uint8_t addr, TempCompensation comp) : SDPSensor(addr, comp, Wire) {}
    SDP3X(const uint8_t addr) : SDPSensor(addr, DiffPressure, Wire) {}
    SDP3X() : SDPSensor(Address1, DiffPressure, Wire) {}
};

class SDP8XX : public SDPSensor {
public:
    SDP8XX(const uint8_t addr, TempCompensation comp, TwoWire &port) : SDPSensor(addr, comp, port) {}
    SDP8XX(const uint8_t addr, TempCompensation comp) : SDPSensor(addr, comp, Wire) {}
    SDP8XX(const uint8_t addr) : SDPSensor(addr, DiffPressure, Wire) {}
};

#endif
