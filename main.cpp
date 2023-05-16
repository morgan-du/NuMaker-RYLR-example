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
#include "RYLR998/RYLR998.h"

/***
 * Build the sample code to Tx or Rx
 * 1 to build TX
 * 0 to build Rx
 */
#define BUILD_TX 1

#define TX_MODULE_ADDRESS   121
#define RX_MODULE_ADDRESS   120
#define NETWORK_ID          18

#if defined(BUILD_TX) && BUILD_TX
#define MODULE_ADDRESS TX_MODULE_ADDRESS
#else
#define MODULE_ADDRESS RX_MODULE_ADDRESS
#endif

RYLR998 rylr(D1, D0, D2);

int main()
{
    printf("\nRYLR998 example uses ATCmdParser\n");
    printf("Mbed OS version %d\n", MBED_VERSION);

    /* Get module firmware version */
    struct RYLR998::fw_version rylr_v = rylr.get_fw_version();
    printf("RYLR998 version is %d.%d.%d\n", rylr_v.major, rylr_v.minor, rylr_v.patch);

    /* Get module unique ID */
    char *uid = rylr.get_uid();
    if (uid != NULL)
        printf("UID is %s\n", uid);
    else
        printf("Get UID failed.\n");

    /* Get RF frequency */
    printf("Band is %d\n", rylr.get_band());

    /* Get RF parameters */
    struct RYLR998::rf_param rf_param = rylr.get_rf_parameter();
    printf("RF Parameters are %d,%d,%d,%d\n", rf_param.sf, rf_param.bw, rf_param.cr, rf_param.pp);

    /* Get/Set Address */
    int mod_addr = rylr.get_address();
    if (mod_addr != MODULE_ADDRESS) {
        mod_addr = MODULE_ADDRESS;
        rylr.set_address(mod_addr);
        printf("Set Address to %d\n", mod_addr);
    }
    else {
        printf("Address is %d\n", mod_addr);
    }

    /* Get/Set Network ID */
    int net_id = rylr.get_network_id();
    if (net_id != NETWORK_ID) {
        net_id = NETWORK_ID;
        rylr.set_network_id(net_id);
        printf("Set Network ID to %d\n", net_id);
    }
    else {
        printf("Network ID is %d\n", net_id);
    }

    /* Get RF Output Power setting */
    printf("RF Output Power is %d\n", rylr.get_rf_output_power());

    /* Get Rx Boost setting */
    printf("RX Boost is %d\n", rylr.get_rx_boost());


#if defined(BUILD_TX) && BUILD_TX
    // Tx side
    char s[32];
    for(int i=0; i <= 100; i++) {
        sprintf(s, "HELLO %d", i);
        printf("Send \"%s\" ...", s);
        rylr.send(RX_MODULE_ADDRESS, s);
        printf("\n");
        ThisThread::sleep_for(2s);
    }

#else
    // Rx side
    char r[32];
    int i, rssi, snr, addr, len;
    while(1) {
        if ((len = rylr.get_size()) != 0)
        {
            i++;
            len = rylr.recv(addr, r, 31);
            snr = rylr.get_snr();
            rssi = rylr.get_rssi();
            printf("Recv #%d: Addr(%d) RSSI(%d) SNR(%d) Len(%d) \"%s\"\n", i, addr, rssi, snr, len, r);
        }
    }

#endif

    printf("\nDone\n");
}
