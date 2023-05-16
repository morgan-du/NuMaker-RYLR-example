/*
 * Copyright (c) 2023, Nuvoton Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#ifndef __RYLR998_H__
#define __RYLR998_H__

#include <stdint.h>
#include <inttypes.h>

#include "drivers/BufferedSerial.h"
#include "drivers/DigitalOut.h"
#include "PinNames.h"
#include "platform/ATCmdParser.h"
#include "platform/mbed_chrono.h"
#include "platform/mbed_error.h"
#include "platform/mbed_mem_trace.h"
#include "platform/Callback.h"
#include "rtos/Mutex.h"
#include "rtos/ThisThread.h"

#ifdef MBED_CONF_RYLR998_SERIAL_BAUDRATE
#define RYLR998_DEFAULT_BAUD_RATE   MBED_CONF_RYLR998_SERIAL_BAUDRATE 
#endif

#ifndef RYLR998_DEFAULT_BAUD_RATE
#define RYLR998_DEFAULT_BAUD_RATE   115200
#endif

#ifndef RYLR998_CMD_TIMEOUT
#define RYLR998_CMD_TIMEOUT     std::chrono::milliseconds(500)
#endif

#ifndef RYLR998_RECV_TIMEOUT
#define RYLR998_RECV_TIMEOUT    std::chrono::milliseconds(800)
#endif

/** _Packet_Node class.
    This is a node class for receved packets
 */
class _Packet_Node {
public:
    _Packet_Node *next;
    char *data;
    int addr;
    int size;
    int rssi;
    int snr;

    _Packet_Node(int addr, char *data, int size, int rssi, int snr) : addr(addr), size(size), rssi(rssi), snr(snr), next(nullptr) {
        this->data = new char[size];
        std::memcpy(this->data, data, size * sizeof(char));
    }

    ~_Packet_Node() {
        if (data != nullptr)
            delete[] data;
    }
};

/** _Packet_LinedList class.
    This is a link list class to manage nodes
 */
class _Packet_LinkedList {
private:
    _Packet_Node *head;
    _Packet_Node *tail;
    int count;

public:
    _Packet_LinkedList() {
        head = nullptr;
        tail = nullptr;
        count = 0;
    }    

    ~_Packet_LinkedList() {
        _Packet_Node *current = head;
        while(current != nullptr) {
            _Packet_Node *next = current->next;
            delete current;
            current = next;    
        }
    }

    int peek_size(void) {
        return (head == nullptr) ? 0 : head->size;
    }

    void push(int addr, char *data, int size, int rssi, int snr) {        
        _Packet_Node *node = new _Packet_Node(addr, data, size, rssi, snr);
        if (head == nullptr) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        count++;
    }

    int pull(int &addr, char *data, int size, int &rssi, int &snr) {

        if (head == nullptr) return 0;

        if (size > head->size)
            size = head->size;

        std::memcpy(data, head->data, size * sizeof(char));
        
        addr = head->addr;
        rssi = head->rssi;
        snr  = head->snr;

        _Packet_Node *node = head;
        head = head->next;
        delete node;
        
        count--;
        return size;
    }

    int size() {
        return count;
    }
};

/** RYLR998 class.
    This is a class for a RYLR998 module.
 */
class RYLR998 {
public:
    RYLR998(PinName tx, PinName rx, PinName reset = NC, bool debug = false);
    ~RYLR998();

    /**
    * RYLR998 firmware version
    *
    * @param major Major version number
    * @param minor Minor version number
    * @param patch Patch version number
    */
    struct fw_version {
        int major;
        int minor;
        int patch;
        fw_version(int major, int minor, int patch) : major(major), minor(minor), patch(patch) {}
    };

    struct rf_param {
        int sf; // Spreading Facror
        int bw; // Bandwidth
        int cr; // Coding Rate
        int pp; // Programmed Preamble
        rf_param(int sf, int bw, int cr, int pp) : sf(sf), bw(bw), cr(cr), pp(pp) {}
    };


    /**
    * Hardware reset RYLR998 module
    *
    */
    void hw_reset(void);

    /**
    * Reset RYLR998 module
    *
    * @return true only if RYLR998 resets successfully
    */
    bool reset(void);

    /**
    * Check AT command interface of RYLR998
    *
    * @return true if ready to respond on AT commands
    */    
    bool at_available(void);

    /**
    * Check firmware version
    *
    * @return fw_version which tells major, minor and patch version
    */
    struct fw_version get_fw_version();

    /**
    * Get module ID
    *
    * @return A string that has 24-digit ID
    */
    char *get_uid();

    /**
    * Set the RF parameters
    *
    * @param sf the Spreading Factor
    * @param bw the Bandwidth
    * @param cr the Coding Rate
    * @param pp the Programmed Preamble
    */
    void set_rf_parameter(int sf, int bw, int cr, int pp);

    /**
    * Return RF parameters
    *
    * @return rf_param which tells Spreading Factor, Bandwidth, Coding Rate, and
    *         Programmed Preamble
    */
    struct rf_param get_rf_parameter();

    /**
    * Set the wireless work mode
    *
    * @param mode work mode. 0 to transmit and receive mode, 1 to sleep mode
    */
    void set_mode(int mode);

    /**
    * Set the UART baud rate
    *
    * @param rate the UART baud rate. Default is 115200.
    */
    void set_baudrate(int rate);

    /**
    * Return the UART baud rate
    *
    * @return The UART baud rate
    */
    int get_baudrate(void);

    /**
    * Set RF frequency
    *
    * @param freq the RF frequency, unit is Hz. Default is 915000000 for 915MHz.
    */
    void set_band(int freq);

    /**
    * Return RF frequency
    *
    * @return the RF frequency
    */
    int get_band(void);

    /**
    * Set the address of module
    *
    * @param addr the address of module. Default is 0, up to 65535.
    */
    void set_address(int addr);

    /**
    * Return the address of module
    *
    * @return the module address
    */
    int get_address();

    /**
    * Set the network ID
    *
    * @param id the network ID. Default is 1, up to 255.
    */
    void set_network_id(int id);

    /**
    * Return the network ID
    *
    * @return the network ID
    */
    int get_network_id();

    /**
    * Set the RF output power
    *
    * @param power the power in dBm. Default is 22 (22dBm), down to 00 (0dBm).
    */
    void set_rf_output_power(int power);

    /**
    * Return the RF output power
    *
    * @return the RF output power
    */
    int get_rf_output_power();

    /**
    * On/Off receive boost mode
    *
    * @param mode 0 to off boost, 1 to enable boost.
    */
    void set_rx_boost(bool mode);

    /**
    * Return the receive boost mode
    *
    * @return the mode of receive boost
    */
    bool get_rx_boost();

    /**
    * Send data to appointed address
    *
    * @param addr address that from 0 to 65535. 0 will send to all address.
    * @param data point to a data string
    */
    void send(int addr, char *data);

    /**
    * Get the received data
    * 
    * @param addr the transmitter address
    * @param data buffer that store the receive data
    * @param size the data buffer size
    * @return the real data size stored in buffer
    */
    int recv(int& addr, char *data, int size);

    /**
    * Peeking the size of the next received data
    * 
    * @return the data size of next received data
    */
    int get_size(void);

    /**
    * Return the RSSI value of the latest recevied packet
    * 
    * @return the RSSI value
    */
    int get_rssi(void) {
        return _r_rssi;
    };

    /**
    * Return the SNR value of the latest recevied packet
    * 
    * @return the SNR value
    */
    int get_snr(void) {
        return _r_snr;
    }

    /**
    * Allows timeout to be changed between commands
    *
    * @param timeout timeout of the command
    */
    void set_timeout(mbed::chrono::milliseconds_u32 timeout = RYLR998_CMD_TIMEOUT);

    /**
     * Flush the serial port input buffers.
     *
     * If you do HW reset for module, you should
     * flush the input buffers from existing responses
     * from the device.
     */
    void flush();

private:
    struct fw_version _fw_ver;
    struct rf_param _rf_param;
    char _uid[25];  // 24 bytes with a zero terminator
    int _band;
    int _addr;
    int _network_id;
    int _rf_output_power;
    bool _rx_boost;
    int _r_rssi;
    int _r_snr;
    int _last_error;

    mbed::BufferedSerial _serial;
    mbed::DigitalOut _reset;
    rtos::Mutex _smutex;

    mbed::ATCmdParser _parser;
    _Packet_LinkedList _packet_buffer;

    // OOB processing
    void _process_oob(std::chrono::duration<uint32_t, std::milli> timeout, bool all);

    // OOB message handlers
    void _oob_packet_hdlr();
    void _oob_error_hdlr();
};

#endif // __RYLR998_H__
