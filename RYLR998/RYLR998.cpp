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
 
#include "mbed.h"
#include "RYLR998.h"

RYLR998::RYLR998(PinName tx, PinName rx, PinName reset, bool debug)
    : _fw_ver(-1, -1, -1),
      _rf_param(-1, -1, -1, -1),
      _reset(reset),
      _serial(tx, rx, RYLR998_DEFAULT_BAUD_RATE),
      _parser(&_serial)
{
    _parser.debug_on(debug);
    _parser.set_delimiter("\r\n");
    _parser.oob("+RCV", callback(this, &RYLR998::_oob_packet_hdlr));
    _parser.oob("+ERR", callback(this, &RYLR998::_oob_error_hdlr));
    set_timeout(RYLR998_CMD_TIMEOUT);

    if (_reset.is_connected())
    {
        _reset = 1;
        ThisThread::sleep_for(200ms);
        flush();
    }

    _uid[0] = '\0';
    _band = 0;
    _addr = 0;
    _network_id = 0;
    _rf_output_power = 0;
    _rx_boost = false;
    _r_rssi = 0;
    _r_snr = 0;
}

void RYLR998::hw_reset(void)
{
    if (_reset.is_connected())
    {
        _reset = 0;
        ThisThread::sleep_for(100ms);
        _reset = 1;
    }
}

bool RYLR998::at_available(void)
{
    bool done;

    for(int i=0; i<5; i++)
    {
        _smutex.lock();
        done = _parser.send("AT")
               && _parser.recv("+OK");
        _smutex.unlock();

        if (done) break;
    }

    return done;
}


bool RYLR998::reset(void)
{
    _smutex.lock();
    bool done = _parser.send("AT+RESET")
                && _parser.recv("+READY");
    _smutex.unlock();

    return done;
}

struct RYLR998::fw_version RYLR998::get_fw_version()
{
    bool done;
    int major, minor, patch;

    _smutex.lock();
    done = _parser.send("AT+VER?")
           && _parser.recv("+VER=RYLR998_REYAX_V%d.%d.%d\n", &major, &minor, &patch);
    _smutex.unlock();

    if (done)
    {
        _fw_ver.major = major;
        _fw_ver.minor = minor;
        _fw_ver.patch = patch;
    }

    return _fw_ver;
}

char * RYLR998::get_uid(void)
{
    bool done;

    _smutex.lock();
    done = _parser.send("AT+UID?")
           && _parser.recv("+UID=%24s\n", _uid);
    _smutex.unlock();

    return (done)? _uid : NULL;
}

struct RYLR998::rf_param RYLR998::get_rf_parameter(void)
{
    bool done;
    int sf, bw, cr, pp;

    _smutex.lock();
    done = _parser.send("AT+PARAMETER?")
           && _parser.recv("+PARAMETER=%d,%d,%d,%d\n", &sf, &bw, &cr, &pp);
    _smutex.unlock();

    if (done)
    {
        _rf_param.sf = sf;
        _rf_param.bw = bw;
        _rf_param.cr = cr;
        _rf_param.pp = pp;
    }

    return _rf_param;
}

void RYLR998::set_rf_parameter(int sf, int bw, int cr, int pp)
{
    if (sf < 7 || sf > 11 ||
        bw < 0 || bw > 9 ||
        cr < 1 || cr > 4 ||
        pp < 4 || pp > 24)
        return;

    _smutex.lock();
    bool done = _parser.send("AT+PARAMETER=%d,%d,%d,%d", sf, bw, cr, pp)
           && _parser.recv("+OK");
    _smutex.unlock();

    if (done)
    {
        _rf_param.sf = sf;
        _rf_param.bw = bw;
        _rf_param.cr = cr;
        _rf_param.pp = pp;
    }    
}

void RYLR998::set_mode(int mode)
{
    if (mode < 0 || mode > 2)
        return;

    _smutex.lock();
    bool done = _parser.send("AT+MODE=%d", mode)
                && _parser.recv("+OK");
    _smutex.unlock();
}

void RYLR998::set_baudrate(int rate)
{
    int r;
    _smutex.lock();
    bool done = _parser.send("AT+IPR=%d", rate)
                && _parser.recv("+IPR=%d\n", &r);
    _smutex.unlock();
}

int RYLR998::get_baudrate(void)
{
    int r;

    _smutex.lock();
    bool done = _parser.send("AT+IPR?")
                && _parser.recv("+IPR=%d\n", &r);
    _smutex.unlock();

    return r;
}

void RYLR998::set_band(int freq)
{
    _smutex.lock();
    bool done = _parser.send("AT+BAND=%d", freq)
                && _parser.recv("+OK");
    _smutex.unlock();
}

int RYLR998::get_band(void)
{
    int band;

    _smutex.lock();
    bool done = _parser.send("AT+BAND?")
                && _parser.recv("+BAND=%d\n", &band);
    _smutex.unlock();

    if (done)
        _band = band;

    return _band;
}

void RYLR998::set_address(int addr)
{
    if (addr < 0 || addr > 65535)
        return;

    _smutex.lock();
    bool done = _parser.send("AT+ADDRESS=%d", addr)
                && _parser.recv("+OK");
    _smutex.unlock();

    if (done)
        _addr = addr;
}

int RYLR998::get_address(void)
{
    int addr;

    _smutex.lock();
    bool done = _parser.send("AT+ADDRESS?")
                && _parser.recv("+ADDRESS=%d\n", &addr);
    _smutex.unlock();

    if (done)
        _addr = addr;

    return _addr;
}

void RYLR998::set_network_id(int id)
{
    if (id < 1 || id > 255)
        return;

    _smutex.lock();
    bool done = _parser.send("AT+NETWORKID=%d", id)
                && _parser.recv("+OK");
    _smutex.unlock();

    if (done)
        _network_id = id;
}

int RYLR998::get_network_id(void)
{
    int id;

    _smutex.lock();
    bool done = _parser.send("AT+NETWORKID?")
                && _parser.recv("+NETWORKID=%d\n", &id);
    _smutex.unlock();

    if (done)
        _network_id = id;

    return _network_id;
}

void RYLR998::set_rf_output_power(int power)
{
    if (power < 0 || power > 22)
        return;

    _smutex.lock();
    bool done = _parser.send("AT+CRFOP=%d", power)
                && _parser.recv("+OK");
    _smutex.unlock();

    if (done)
        _rf_output_power = power;
}

int RYLR998::get_rf_output_power(void)
{
    int power;

    _smutex.lock();
    bool done = _parser.send("AT+CRFOP?")
                && _parser.recv("+CRFOP=%d\n", &power);
    _smutex.unlock();

    if (done)
        _rf_output_power = power;

    return _rf_output_power;
}

void RYLR998::set_rx_boost(bool mode)
{
    _smutex.lock();
    bool done = _parser.send("AT+RXBOOST=%d", (mode) ? 1 : 0)
                && _parser.recv("+OK");
    _smutex.unlock();

    if (done)
        _rx_boost = ((mode) != 0);
}

bool RYLR998::get_rx_boost(void)
{
    int mode;

    _smutex.lock();
    bool done = _parser.send("AT+RXBOOST?")
                && _parser.recv("+RXBOOST=%d\n", &mode);
    _smutex.unlock();

    if (done)
        _rx_boost = ((mode) != 0);

    return _rx_boost;
}

void RYLR998::send(int addr, char *data)
{
    if (addr < 0 || addr > 65535 || data == NULL)
        return;

    int len = strlen(data);
    if (len > 240)
        return;

    _smutex.lock();
    bool done = _parser.send("AT+SEND=%d,%d,%s", addr, len, data)
                && _parser.recv("+OK");
    _smutex.unlock();
}

int RYLR998::get_size(void)
{
    _smutex.lock();
    _process_oob(RYLR998_RECV_TIMEOUT, true);
    _smutex.unlock();

    return (_packet_buffer.peek_size());
}

int RYLR998::recv(int& addr, char *buf, int size)
{
    int len = 0;

    _smutex.lock();
    _process_oob(RYLR998_RECV_TIMEOUT, true);
    _smutex.unlock();

    if (_packet_buffer.size()) {
        len = _packet_buffer.pull(addr, buf, size, _r_rssi, _r_snr);
    }
    buf[len] = '\0';

    return len;
}

void RYLR998::flush()
{
    _smutex.lock();
    _parser.flush();
    _smutex.unlock();
}
void RYLR998::_oob_packet_hdlr(void)
{
    int addr, len, rssi, snr;
    char buf[241];

    _parser.scanf("=%d,%d,", &addr, &len);
    _parser.read(buf, len);
    buf[len] = '\0';
    _parser.scanf(",%d,%d\n", &rssi, &snr);

    _packet_buffer.push(addr, buf, len, rssi, snr);
}

void RYLR998::_oob_error_hdlr(void)
{
    _parser.scanf("=%d\n", &_last_error);
}

void RYLR998::set_timeout(std::chrono::duration<uint32_t, milli> timeout)
{
    _parser.set_timeout(timeout.count());
}

void RYLR998::_process_oob(std::chrono::duration<uint32_t, milli> timeout, bool all)
{
    set_timeout(timeout);
    // Poll for inbound packets
    while (_parser.process_oob() && all) {
    }
    set_timeout();
}

RYLR998::~RYLR998()
{
    flush();
    // release all oob data
}