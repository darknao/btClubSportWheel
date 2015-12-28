// iWRAP external host controller library
// 2015-07-03 by Jeff Rowberg <jeff@rowberg.net>
//
// Changelog:
//  2015-07-03 - Fix signed/unsigned compiler warnings in Arduino 1.6.5
//  2015-04-27 - Fix MUX frame parser "length" value code
//  2014-12-06 - Add missing parser reset when MUX frame error occurs
//  2014-11-15 - Fix "RING" event numeric base for "channel" parameter
//  2014-07-03 - Fix "LIST" result response numeric base for "channel" parameter
//  2014-05-31 - Add convenience function/callbacks for sending MUX frames to data links
//  2014-05-25 - Initial release

/* ============================================
iWRAP host controller library code is placed under the MIT license
Copyright (c) 2015 Jeff Rowberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
===============================================
*/

#include <string.h>     // memcpy()
#include <stdlib.h>     // malloc()
#include <avr_functions.h>
#include "iWRAP.h"

uint8_t *iwrap_rx_packet;
uint16_t iwrap_rx_packet_length = 0;
uint16_t iwrap_rx_packet_size = 0;
uint8_t iwrap_rx_packet_channel = 0;
uint8_t iwrap_rx_packet_flags = 0;
uint16_t iwrap_rx_payload_length = 0;
uint8_t *iwrap_tptr;
uint8_t iwrap_in_packet = 0;

uint8_t iwrap_last_command_result = 0;
uint8_t iwrap_pending_boot = 0;
uint8_t iwrap_pending_commands = 0;
uint8_t iwrap_pending_info = 0;

#ifdef IWRAP_DEBUG
    int (*iwrap_debug)(const char *data);
    int iwrap_debug_char(char b);
    int iwrap_debug_hex(uint8_t b);
    int iwrap_debug_int(int32_t i);
#endif

/**
 * @brief Send iWRAP command, automatically wrapping in MUX frame if specified
 * @param cmd Command to send, in ASCII format (no line endings)
 * @param mode Sending mode (MUX or non-MUX)
 * @return Result code (non-zero indicates error)
 * @see IWRAP_MODE_COMMAND
 * @see IWRAP_MODE_MUX
 */
uint8_t iwrap_send_command(const char *cmd, uint8_t mode) {
    #ifdef IWRAP_INCLUDE_MUX
        uint16_t mux_length;
        uint8_t *mux_data, result;
    #endif
    
    // verify assigned output function
    if (!iwrap_output) return 0xFF;
    
    #ifdef IWRAP_INCLUDE_BUSY
        // trigger "busy" callback if previously idle
        if (iwrap_callback_busy && !iwrap_pending_commands) iwrap_callback_busy();
    #endif

    // check which command is being sent
    if (strncmp(cmd, "RESET", 5) == 0) {
        iwrap_pending_boot++;
    } else {
        iwrap_pending_commands++;
        if (strncmp(cmd, "INFO", 4) == 0) {
            iwrap_pending_info++;
        }
    }
    
    #ifdef IWRAP_INCLUDE_TXCOMMAND
        // trigger outgoing command callback
        if (iwrap_callback_txcommand) iwrap_callback_txcommand(strlen(cmd), (uint8_t *)cmd);
    #endif
    
    if (mode == IWRAP_MODE_MUX) {
        #ifdef IWRAP_INCLUDE_MUX
            // build and send mux packet
            if ((result = iwrap_pack_mux_frame(0xFF, strlen(cmd), (uint8_t *)cmd, &mux_length, &mux_data))) { return result; }
            iwrap_output(mux_length, mux_data);
            free(mux_data);
        #else
            return 0xFE; // MUX mode not supported
        #endif
    } else {
        // send normal packet
        iwrap_output(strlen(cmd), (uint8_t *)cmd);
        iwrap_output(2, (uint8_t *)"\r\n");
    }
    return 0;
}

/**
 * @brief Send data, automatically wrapping in MUX frame if specified
 * @param channel Link ID to which to send data
 * @param data_len Length of data to send in bytes
 * @param data Byte array of all data to send
 * @param mode Sending mode (MUX or non-MUX)
 * @return Result code (non-zero indicates error)
 */
uint8_t iwrap_send_data(uint8_t channel, uint16_t data_len, const uint8_t *data, uint8_t mode) {
    #ifdef IWRAP_INCLUDE_MUX
        uint16_t mux_length;
        uint8_t *mux_data, result;
    #endif
    
    // verify assigned output function
    if (!iwrap_output) return 0xFF;

    #ifdef IWRAP_INCLUDE_TXDATA
        // trigger outgoing data callback
        if (iwrap_callback_txdata) iwrap_callback_txdata(channel, data_len, data);
    #endif

    if (mode == IWRAP_MODE_MUX) {
        #ifdef IWRAP_INCLUDE_MUX
            // build and send mux packet
            if ((result = iwrap_pack_mux_frame(channel, data_len, (uint8_t *)data, &mux_length, &mux_data))) { return result; }
            iwrap_output(mux_length, mux_data);
            free(mux_data);
        #else
            return 0xFE; // MUX mode not supported
        #endif
    } else {
        // send normal packet
        iwrap_output(data_len, (unsigned char *)data);
    }
    return 0;
}

/**
 * @brief Parse incoming data from iWRAP module
 * @param b Incoming byte to parse
 * @param mode Receiving mode (MUX or non-MUX)
 * @return Result code (non-zero indicates error)
 */
uint8_t iwrap_parse(uint8_t b, uint8_t mode) {
    // make sure our packet container is big enough (always at least +1 byte)
    if (iwrap_rx_packet_length + 1 >= iwrap_rx_packet_size) {
        if (iwrap_rx_packet_size) {
            // increase by 16 bytes and verify allocation
            iwrap_tptr = (uint8_t *)realloc(iwrap_rx_packet, iwrap_rx_packet_size += 16);
            if (!iwrap_tptr) { return 1; }
            iwrap_rx_packet = iwrap_tptr;
        } else {
            // start with 64 bytes and verify allocation
            iwrap_rx_packet = (uint8_t *)malloc(iwrap_rx_packet_size = 64);
            if (!iwrap_rx_packet) { return 1; }
        }
    }

    // make sure data is valid
    if (mode != IWRAP_MODE_MUX || iwrap_in_packet || b == 0xBF) {
        // append this byte to packet
        iwrap_rx_packet[iwrap_rx_packet_length++] = b;
        iwrap_in_packet = 1;
        
        // check for a complete packet
        if ((mode == IWRAP_MODE_MUX && iwrap_rx_packet_length > 4 && iwrap_rx_packet_length == (uint16_t)(iwrap_rx_packet[3] + 5)) || (mode != IWRAP_MODE_MUX && b == '\n')) {
            // validate all correct packet
            if (mode == IWRAP_MODE_MUX) {
                #ifdef IWRAP_INCLUDE_MUX
                    // unpack MUX packet
                    if (iwrap_unpack_mux_frame(
                            iwrap_rx_packet_length,
                            iwrap_rx_packet,
                            &iwrap_rx_packet_channel,
                            &iwrap_rx_packet_flags,
                            &iwrap_rx_payload_length,
                            &iwrap_tptr,
                            0)) {
                           
                        // reset all packet metadata (MUX parsing error occurred)
                        iwrap_rx_packet_length = 0;
                        iwrap_rx_packet_channel = 0;
                        iwrap_rx_packet_flags = 0;
                        iwrap_in_packet = 0;
                        return 2;
                    }
                #else
                    return 0xFE; // MUX mode not supported
                #endif
            } else {
                iwrap_rx_payload_length = iwrap_rx_packet_length;
                iwrap_tptr = iwrap_rx_packet;
                iwrap_rx_packet_flags = 0;
                if (mode == IWRAP_MODE_COMMAND) {
                    // channel doesn't technically apply in non-MUX mode, but this allows
                    // the parser code below to work the same way regardless of whether
                    // you're in MUX mode with channel 0xFF or COMMAND mode
                    iwrap_rx_packet_channel = 0xFF;
                } else {
                    // 0xFE is not valid, but won't be 0xFF which is the important thing
                    iwrap_rx_packet_channel = 0xFE;
                }
            }
            
            // debug output
            #ifdef IWRAP_DEBUG
                if (iwrap_debug) {
                    uint16_t i;
                    iwrap_debug("<= RX ");
                    iwrap_debug_hex(iwrap_rx_packet_channel);
                    iwrap_debug(", ");
                    iwrap_debug_int(iwrap_rx_payload_length);
                    iwrap_debug(":\t");
                    for (i = 0; i < iwrap_rx_payload_length; i++) {
                        if (iwrap_tptr[i] > 31 && iwrap_tptr[i] < 127) {
                            iwrap_debug_char(iwrap_tptr[i]);
                        } else if (iwrap_tptr[i] == 9) {
                            iwrap_debug("\\t");
                        } else if (iwrap_tptr[i] == 10) {
                            iwrap_debug("\\n");
                        } else if (iwrap_tptr[i] == 13) {
                            iwrap_debug("\\r");
                        } else {
                            iwrap_debug("\\x");
                            iwrap_debug_hex(iwrap_tptr[i]);
                        }
                    }
                    iwrap_debug("\r\n");
                }
            #endif /* IWRAP_DEBUG */
            
            // process iWRAP command channel data
            if (iwrap_rx_packet_channel == 0xFF) {
                #ifdef IWRAP_INCLUDE_RXOUTPUT
                    // trigger general "RX output" callback
                    if (iwrap_callback_rxoutput) iwrap_callback_rxoutput(iwrap_rx_payload_length, iwrap_tptr);
                #endif
                    
                // check for known iWRAP responses/events
                if (strncmp((char *)iwrap_tptr, "OK.", 3) == 0) { // this one first since it happens most
                    if (iwrap_pending_commands) iwrap_pending_commands--;
                    if (iwrap_pending_info) iwrap_pending_info--;
                    #ifdef IWRAP_INCLUDE_IDLE
                        if (!iwrap_pending_commands && iwrap_callback_idle) iwrap_callback_idle(iwrap_last_command_result);
                    #endif
                    #ifdef IWRAP_INCLUDE_EVT_OK
                        if (iwrap_evt_ok) iwrap_evt_ok();
                    #endif
                    iwrap_last_command_result = 0;
              #if defined(IWRAP_INCLUDE_EVT_A2DP_STREAMING_START) || defined(IWRAP_INCLUDE_A2DP_STREAMING_STOP)
                } else if (strncmp((char *)iwrap_tptr, "A2DP STR", 8) == 0) {
                    if (iwrap_tptr[17] == 'A') {
                      #ifdef IWRAP_INCLUDE_EVT_A2DP_STREAMING_START
                        // A2DP STREAMING START {link_id}
                        if (iwrap_evt_a2dp_streaming_start) {
                            char *test = (char *)iwrap_tptr + 20;
                            uint8_t link_id = strtol(test, &test, 10);
                            iwrap_evt_a2dp_streaming_start(link_id);
                        }
                      #endif
                    } else {
                      #ifdef IWRAP_INCLUDE_EVT_A2DP_STREAMING_STOP
                        // A2DP STREAMING STOP {link_id}
                        if (iwrap_evt_a2dp_streaming_stop) {
                            char *test = (char *)iwrap_tptr + 19;
                            uint8_t link_id = strtol(test, &test, 10);
                            iwrap_evt_a2dp_streaming_stop(link_id);
                        }
                      #endif
                    }
              #endif
              #ifdef IWRAP_INCLUDE_RSP_CALL
                } else if (strncmp((char *)iwrap_tptr, "CALL ", 5) == 0) {
                    // CALL 
                    if (iwrap_rsp_call) {
                        char *test = (char *)iwrap_tptr + 5;
                        uint8_t link_id = strtol(test, &test, 10);
                        iwrap_rsp_call(link_id);
                    }
              #endif
              #ifdef IWRAP_INCLUDE_EVT_CONNECT
                } else if (strncmp((char *)iwrap_tptr, "CONN", 4) == 0) {
                    // CONNECT {link_id} {SCO | RFCOMM | A2DP | HID | HFP | HFP-AG {target} [address]
                    if (iwrap_evt_connect) {
                        char *test = (char *)iwrap_tptr + 8;
                        uint8_t link_id = strtol(test, &test, 10); test++;
                        char *profile = test;
                        test = strchr(test, ' ');
                        test[0] = 0; // null terminate target
                        test++;
                        uint16_t target = strtol(test, &test, 16); test++;
                        iwrap_address_t mac;
                        if ((uint16_t)((iwrap_tptr - (uint8_t *)test) + 17) < iwrap_rx_payload_length) {
                            // optional [address] parameter present
                            iwrap_hexstrtobin(test, &test, mac.address, 0); test++;
                            iwrap_evt_connect(link_id, profile, target, &mac);
                        } else {
                            iwrap_evt_connect(link_id, profile, target, 0);
                        }
                    }
              #endif
              #ifdef IWRAP_INCLUDE_RSP_HID_GET
                } else if (strncmp((char *)iwrap_tptr, "HID GET ", 8) == 0) {
                    // HID GET {length} {descriptor}
                    if (iwrap_rsp_hid_get) {
                        char *test = (char *)iwrap_tptr + 8;
                        uint8_t length = strtol(test, &test, 16); test++;
                        uint8_t descriptor[length];
                        iwrap_hexstrtobin(test, &test, descriptor, length * 2);
                        iwrap_rsp_hid_get(length, descriptor);
                    }
              #endif
              #if defined(IWRAP_INCLUDE_EVT_HID_OUTPUT) || defined(IWRAP_INCLUDE_EVT_HID_SUSPEND)
                } else if (strncmp((char *)iwrap_tptr, "HID ", 4) == 0 && iwrap_tptr[4] < 0x40) {
                    char *test = (char *)iwrap_tptr + 4;
                    uint8_t link_id = strtol(test, &test, 10); test++;
                    if (test[0] == 'O') {
                      #ifdef IWRAP_INCLUDE_EVT_HID_OUTPUT
                        // HID {link_id} OUTPUT {data_length} {data}
                        if (iwrap_evt_hid_output) {
                            test += 7;
                            uint8_t length = strtol(test, &test, 16); test++;
                            if (length == 0){
                              // ??
                              length = iwrap_rx_payload_length;
                            }
                            uint8_t data[length];
                            int i=0;
                            for(i=0;i<length;i++) {
                                if(test[0] == '\r' || test[0] == '\n') break;
                                if(test[0] != ' ') data[i] = strtol(test, &test, 16);
                                test++;
                            }
                            length = i;
                            //memcpy(data, test, length
                            iwrap_evt_hid_output(link_id, length, data);
                        }
                      #endif
                    } else {
                      #ifdef IWRAP_INCLUDE_EVT_HID_SUSPEND
                        // HID {link_id} SUSPEND
                        if (iwrap_evt_hid_suspend) {
                            iwrap_evt_hid_suspend(link_id);
                        }
                      #endif
                    }
              #endif
              #ifdef IWRAP_INCLUDE_EVT_HFP
                } else if (strncmp((char *)iwrap_tptr, "HFP ", 4) == 0) {
                    // HFP {link_id} ...content...
                    if (iwrap_evt_hfp) {
                        char *test = (char *)iwrap_tptr + 4;
                        uint8_t link_id = strtol(test, &test, 10); test++;
                        char *type = test;
                        test = strchr(test, ' ') + 1;
                        char *detail = test;
                        iwrap_tptr[iwrap_rx_payload_length - 2] = 0; // null terminate
                        iwrap_evt_hfp(link_id, type, detail);
                    }
              #endif
              #ifdef IWRAP_INCLUDE_EVT_HFP_AG
                } else if (strncmp((char *)iwrap_tptr, "HFP-AG ", 7) == 0) {
                    // HFP-AG {link_id} ...content...
                    if (iwrap_evt_hfp_ag) {
                        char *test = (char *)iwrap_tptr + 7;
                        uint8_t link_id = strtol(test, &test, 10); test++;
                        char *type = test;
                        test = strchr(test, ' ') + 1;
                        char *detail = test;
                        iwrap_tptr[iwrap_rx_payload_length - 2] = 0; // null terminate
                        iwrap_evt_hfp_ag(link_id, type, detail);
                    }
              #endif
              #ifdef IWRAP_INCLUDE_EVT_IDENT
                } else if (strncmp((char *)iwrap_tptr, "IDENT ", 6) == 0 && iwrap_tptr[6] != 'E') {
                    // IDENT {src}:{vendor_id} {product_id} {version} "[descr]"
                    if (iwrap_evt_ident) {
                        char *test = (char *)iwrap_tptr + 6;
                        char *src = test;
                        test = strchr(test, ':');
                        test[0] = 0; // null terminate "src" string
                        test++;
                        uint16_t vendor_id = strtol(test, &test, 16); test++;
                        uint16_t product_id = strtol(test, &test, 16); test++;
                        char *version = test;
                        test = strchr(test, ' ');
                        test[0] = 0; // null terminate "version" string
                        test += 2; // advance to first " character
                        char *descr = test;
                        test = strchr(test, '"');
                        test[0] = 0; // null terminate "descr" string
                        iwrap_evt_ident(src, vendor_id, product_id, version, descr);
                    }
              #endif
              #ifdef IWRAP_INCLUDE_EVT_IDENT_ERROR
                } else if (strncmp((char *)iwrap_tptr, "IDENT ER", 8) == 0) {
                    // IDENT ERROR {error_code} {address} [message]
                    if (iwrap_evt_ident_error) {
                        char *test = (char *)iwrap_tptr + 12;
                        uint16_t error_code = strtol(test, &test, 16); test++;
                        iwrap_address_t mac;
                        iwrap_hexstrtobin(test, &test, mac.address, 0); test++;
                        if ((uint16_t)((iwrap_tptr - (uint8_t *)test) + 3) < iwrap_rx_payload_length) {
                            // optional [message] parameter present
                            iwrap_tptr[iwrap_rx_payload_length - 2] = 0; // null terminate
                            iwrap_evt_ident_error(error_code, &mac, test);
                        } else {
                            iwrap_evt_ident_error(error_code, &mac, 0);
                        }
                    }
              #endif
              #if defined(IWRAP_INCLUDE_RSP_INQUIRY_COUNT) || defined(IWRAP_INCLUDE_RSP_INQUIRY_RESULT)
                } else if (strncmp((char *)iwrap_tptr, "INQUIRY ", 8) == 0) {
                    if (iwrap_rx_payload_length < 13) {
                      #ifdef IWRAP_INCLUDE_RSP_INQUIRY_COUNT
                        // INQUIRY {num_of_devices} 
                        if (iwrap_rsp_inquiry_count) {
                            char *test = (char *)iwrap_tptr + 5;
                            uint8_t num_of_devices = strtol(test, &test, 10);
                            iwrap_rsp_inquiry_count(num_of_devices);
                        }
                      #endif
                    } else {
                      #ifdef IWRAP_INCLUDE_RSP_INQUIRY_RESULT
                        // INQUIRY {addr} {class_of_device} [rssi]
                        if (iwrap_rsp_list_result) {
                            char *test = (char *)iwrap_tptr + 8;
                            iwrap_address_t mac;
                            iwrap_hexstrtobin(test, &test, mac.address, 0); test++;
                            uint32_t class_of_device = strtol(test, &test, 16);
                            int8_t rssi = 0;
                            if (test[0] == ' ') {
                                rssi = strtol(test + 1, &test, 10);
                            }
                            iwrap_rsp_inquiry_result(&mac, class_of_device, rssi);
                        }
                      #endif
                    }
              #endif
              #ifdef IWRAP_INCLUDE_EVT_INQUIRY_EXTENDED
                } else if (strncmp((char *)iwrap_tptr, "INQUIRY_E", 9) == 0) {
                    // INQUIRY_EXTENDED {addr} RAW {data}
                    if (iwrap_evt_inquiry_extended) {
                        char *test = (char *)iwrap_tptr + 16;
                        iwrap_address_t mac;
                        iwrap_hexstrtobin(test, &test, mac.address, 0); test += 5;
                        uint8_t data[(iwrap_rx_payload_length - 39) / 2];
                        uint8_t length = iwrap_hexstrtobin(test, 0, data, 0) / 2;
                        iwrap_evt_inquiry_extended(&mac, length, data);
                    }
              #endif
              #ifdef IWRAP_INCLUDE_EVT_INQUIRY_PARTIAL
                } else if (strncmp((char *)iwrap_tptr, "INQUIRY_P", 9) == 0) {
                    // INQUIRY_PARTIAL {address} {class_of_device} [{cached_name} {rssi}]
                    if (iwrap_evt_inquiry_partial) {
                        char *test = (char *)iwrap_tptr + 16;
                        iwrap_address_t mac;
                        iwrap_hexstrtobin(test, &test, mac.address, 0); test++;
                        uint32_t class_of_device = strtol(test, &test, 16); test++;
                        if (test[0] == '"') {
                            // optional [{cached_name} {rssi}] values present
                            char *name = ++test;
                            test = strchr(test, '"');
                            test[0] = 0; // null terminate name string
                            test++;
                            int8_t rssi = strtol(test, &test, 10);
                            iwrap_evt_inquiry_partial(&mac, class_of_device, name, rssi);
                        } else {
                            // name and RSSI not present
                            iwrap_evt_inquiry_partial(&mac, class_of_device, 0, 0);
                        }
                    }
              #endif
              #if defined(IWRAP_INCLUDE_RSP_LIST_COUNT) || defined(IWRAP_INCLUDE_RSP_LIST_RESULT)
                } else if (strncmp((char *)iwrap_tptr, "LIST ", 5) == 0) {
                    if (iwrap_rx_payload_length < 10) {
                      #ifdef IWRAP_INCLUDE_RSP_LIST_COUNT
                        // LIST {num_of_connections}
                        if (iwrap_rsp_list_count) {
                            char *test = (char *)iwrap_tptr + 5;
                            uint8_t num_of_connections = strtol(test, &test, 10);
                            iwrap_rsp_list_count(num_of_connections);
                        }
                      #endif
                    } else {
                      #ifdef IWRAP_INCLUDE_RSP_LIST_RESULT
                        // LIST {link_id} CONNECTED {mode} {blocksize} 0 0 {elapsed_time} {local_msc} {remote_msc} {addr} {channel} {direction} {powermode} {role} {crypt} {buffer} [ERETX]
                        if (iwrap_rsp_list_result) {
                            char *test = (char *)iwrap_tptr + 5;
                            uint8_t link_id = strtol(test, &test, 10); test += 11;
                            char *mode = test;
                            test = strchr(test, ' ');
                            test[0] = 0; // null terminate for in-place string access to "mode" w/o reallocation
                            test++;
                            uint16_t blocksize = strtol(test, &test, 10); test++;
                            strtol(test, &test, 10); test++; // ...two fixed "0" arguments...
                            strtol(test, &test, 10); test++; // ?
                            uint32_t elapsed_time = strtol(test, &test, 10); test++;
                            uint16_t local_msc = strtol(test, &test, 16); test++;
                            uint16_t remote_msc = strtol(test, &test, 16); test++;
                            iwrap_address_t addr;
                            iwrap_hexstrtobin(test, &test, addr.address, 0); test++;
                            uint16_t channel = strtol(test, &test, 16); test++;
                            uint8_t direction = 0;
                            if (test[0] == 'O') { direction = IWRAP_CONNECTION_DIRECTION_OUTGOING; test += 9; }
                            else if (test[0] == 'I') { direction = IWRAP_CONNECTION_DIRECTION_INCOMING; test += 9; }
                            uint8_t powermode = 0;
                            if (test[0] == 'A') { powermode = IWRAP_CONNECTION_POWERMODE_ACTIVE; test += 7; }
                            else if (test[0] == 'S') { powermode = IWRAP_CONNECTION_POWERMODE_SNIFF; test += 6; }
                            else if (test[0] == 'H') { powermode = IWRAP_CONNECTION_POWERMODE_HOLD; test += 5; }
                            else if (test[0] == 'P') { powermode = IWRAP_CONNECTION_POWERMODE_PARK; test += 5; }
                            uint8_t role = 0;
                            if (test[0] == 'M') { role = IWRAP_CONNECTION_ROLE_MASTER; test += 7; }
                            else if (test[0] == 'S') { role = IWRAP_CONNECTION_ROLE_SLAVE; test += 6; }
                            uint8_t crypt = 0;
                            if (test[0] == 'P') { crypt = IWRAP_CONNECTION_CRYPT_PLAIN; test += 6; }
                            else if (test[0] == 'E') { crypt = IWRAP_CONNECTION_CRYPT_ENCRYPTED; test += 10; }
                            uint16_t buffer = strtol(test, &test, 10); test++;
                            uint8_t eretx = 0;
                            if (test[0] == 'E') { eretx = 1; }
                            iwrap_rsp_list_result(link_id, mode, blocksize, elapsed_time, local_msc, remote_msc, &addr, channel, direction, powermode, role, crypt, buffer, eretx);
                        }
                      #endif
                    }
              #endif
              #ifdef IWRAP_INCLUDE_EVT_NAME
                } else if (strncmp((char *)iwrap_tptr, "NAME", 4) == 0 && iwrap_tptr[7] == ':') {
                    // NAME {bd_addr} "{name}"
                    if (iwrap_evt_name) {
                        char *test = (char *)iwrap_tptr + 5;
                        iwrap_address_t mac;
                        iwrap_hexstrtobin(test, &test, mac.address, 0); test += 2; // advance to first " character
                        char *friendly_name = test;
                        test = strchr(test, '"');
                        test[0] = 0; // null terminate name string
                        iwrap_evt_name(&mac, friendly_name);
                    }
              #endif
              #ifdef IWRAP_INCLUDE_EVT_NAME_ERROR
                } else if (strncmp((char *)iwrap_tptr, "NAME ER", 7) == 0) {
                    // NAME ERROR {error_code} {bd_addr} {reason}
                    if (iwrap_evt_name_error) {
                        char *test = (char *)iwrap_tptr + 11;
                        uint16_t error_code = strtol(test, &test, 16); test++;
                        iwrap_address_t mac;
                        iwrap_hexstrtobin(test, &test, mac.address, 0); test++;
                        if ((uint16_t)((iwrap_tptr - (uint8_t *)test) + 3) < iwrap_rx_payload_length) {
                            // optional [message] parameter present
                            iwrap_tptr[iwrap_rx_payload_length - 2] = 0; // null terminate
                            iwrap_evt_name_error(error_code, &mac, test);
                        } else {
                            iwrap_evt_name_error(error_code, &mac, 0);
                        }
                    }
              #endif
              #ifdef IWRAP_INCLUDE_EVT_NO_CARRIER
                } else if (strncmp((char *)iwrap_tptr, "NO CA", 5) == 0) {
                    if (iwrap_evt_no_carrier) {
                        // NO CARRIER {link_id} ERROR {error_code} [message]
                        char *test = (char *)iwrap_tptr + 11;
                        uint8_t link_id = strtol(test, &test, 10); test += 7;
                        uint16_t error_code = strtol(test, &test, 16); test++;
                        iwrap_tptr[iwrap_rx_payload_length - 2] = 0; // null terminate
                        iwrap_evt_no_carrier(link_id, error_code, test);
                    }
              #endif
              #ifdef IWRAP_INCLUDE_RSP_AT
                } else if (strncmp((char *)iwrap_tptr, "OK", 2) == 0) {
                    // OK
                    if (iwrap_rsp_at) iwrap_rsp_at();
              #endif
              #if defined(IWRAP_INCLUDE_RSP_PAIR) || defined(IWRAP_INCLUDE_EVT_PAIR)
                } else if (strncmp((char *)iwrap_tptr, "PAIR", 4) == 0) {
                    if (iwrap_rx_payload_length < 32) {
                      #ifdef IWRAP_INCLUDE_RSP_PAIR
                        // PAIR {bd_addr} {result}
                        if (iwrap_rsp_pair) {
                            char *test = (char *)iwrap_tptr + 5;
                            iwrap_address_t mac;
                            iwrap_hexstrtobin(test, &test, mac.address, 0); test++; // advance to first " character
                            iwrap_rsp_pair(&mac, test[0] == 'O' ? 0 : 1);
                        }
                      #endif
                    } else {
                      #ifdef IWRAP_INCLUDE_EVT_PAIR
                        // PAIR {address} {key_type} {link_key}
                        if (iwrap_evt_pair) {
                            char *test = (char *)iwrap_tptr + 5;
                            iwrap_address_t mac;
                            iwrap_hexstrtobin(test, &test, mac.address, 0); test++; // advance to first " character
                            uint8_t key_type = strtol(test, &test, 16); test++;
                            uint8_t link_key[16];
                            iwrap_hexstrtobin(test, &test, link_key, 32);
                            iwrap_evt_pair(&mac, key_type, link_key);
                        }
                      #endif
                    }
              #endif
              #ifdef IWRAP_INCLUDE_EVT_READY
                } else if (strncmp((char *)iwrap_tptr, "READY", 5) == 0) {
                    // READY.
                    if (iwrap_pending_boot) {
                        iwrap_pending_boot = 0;
                        iwrap_pending_commands = 0;
                    }
                    if (iwrap_evt_ready) iwrap_evt_ready();
              #endif
              #ifdef IWRAP_INCLUDE_EVT_RING
                } else if (strncmp((char *)iwrap_tptr, "RING", 4) == 0) {
                    // RING {link_id} {address} {SCO | {channel} {profile}}
                    if (iwrap_evt_ring) {
                        char *test = (char *)iwrap_tptr + 5;
                        uint8_t link_id = strtol(test, &test, 10); test++;
                        iwrap_address_t address;
                        iwrap_hexstrtobin(test, &test, address.address, 0); test++;
                        if (test[0] == 'S') {
                            // SCO (no "channel" parameter)
                            char *profile = test;
                            test = strchr(test, ' ');
                            test[0] = 0; // null terminate for in-place string access to "mode" w/o reallocation
                            iwrap_evt_ring(link_id, &address, 0, profile);
                        } else {
                            // not SCO
                            uint16_t channel = strtol(test, &test, 16); test++;
                            char *profile = test;
                            test = strchr(test, ' ');
                            test[0] = 0; // null terminate for in-place string access to "mode" w/o reallocation
                            iwrap_evt_ring(link_id, &address, channel, profile);
                        }
                    }
              #endif
              #ifdef IWRAP_INCLUDE_RSP_SET
                } else if (strncmp((char *)iwrap_tptr, "SET ", 4) == 0) {
                    // SET [{category} [{option} {value}]]
                    if (iwrap_rsp_set) {
                        uint8_t category = 0;
                        char *option, *value;
                        if (iwrap_tptr[4] == 'B') { // SET BT ...
                            category = IWRAP_SET_CATEGORY_BT;
                            option = (char *)(iwrap_tptr + 7);
                        } else if (iwrap_tptr[4] == 'C') {  // SET CONTROL ...
                            category = IWRAP_SET_CATEGORY_CONTROL;
                            option = (char *)(iwrap_tptr + 12);
                        } else if (iwrap_tptr[4] == 'P') {  // SET PROFILE ...
                            category = IWRAP_SET_CATEGORY_PROFILE;
                            option = (char *)(iwrap_tptr + 12);
                        }
                        
                        // ensure we have detected a valid category
                        if (category) {
                            iwrap_tptr[iwrap_rx_payload_length - 2] = 0;
                            value = strchr((char *)option, ' ');
                            value[0] = 0; value++;
                            iwrap_rsp_set(category, option, value);
                        }
                    }
                //} else if (strncmp((char *)iwrap_tptr, "SET", 3) == 0) {
                    // SET dump finished, should be logically handled by "OK." event following (if enabled)
              #endif
                } else if (strncmp((char *)iwrap_tptr, "SYN", 3) == 0) {
                    // SYNTAX ERROR
                    iwrap_last_command_result = 1;
                    #ifdef IWRAP_INCLUDE_RSP_SYNTAX_ERROR
                        if (iwrap_rsp_syntax_error) iwrap_rsp_syntax_error();
                    #endif
                } else {
                    // unmatched packet, check for pending INFO request
                  #ifdef IWRAP_INCLUDE_RSP_INFO
                    // (INFO command produces lines with various output formats
                    if (iwrap_pending_info && iwrap_rsp_info) {
                        iwrap_tptr[iwrap_rx_payload_length - 2] = 0;
                        iwrap_rsp_info(iwrap_rx_payload_length - 2, (char *)iwrap_tptr);
                    } else
                  #endif
                    {
                        // TODO: TEMP DEBUG OUTPUT FOR UNMATCHED RX PACKET
                        #ifdef IWRAP_DEBUG_TEMP
                            if (iwrap_debug) {
                                int i;
                                iwrap_debug("?? RX ");
                                iwrap_debug_hex(iwrap_rx_packet_channel);
                                iwrap_debug(", ");
                                iwrap_debug_int(iwrap_rx_payload_length);
                                iwrap_debug(":\t");
                                for (i = 0; i < iwrap_rx_payload_length; i++) {
                                    if (iwrap_tptr[i] > 31 && iwrap_tptr[i] < 127) {
                                        iwrap_debug_char(iwrap_tptr[i]);
                                    } else if (iwrap_tptr[i] == 9) {
                                        iwrap_debug("\\t");
                                    } else if (iwrap_tptr[i] == 10) {
                                        iwrap_debug("\\n");
                                    } else if (iwrap_tptr[i] == 13) {
                                        iwrap_debug("\\r");
                                    } else {
                                        iwrap_debug("\\x");
                                        iwrap_debug_hex(iwrap_tptr[i]);
                                    }
                                }
                                iwrap_debug("\n");
                            }
                        #endif /* IWRAP_DEBUG */
                    }
                }
          #ifdef IWRAP_INCLUDE_RXDATA
            } else {
                // data packet, so let the user app handle it
                if (iwrap_callback_rxdata) {
                    iwrap_tptr[iwrap_rx_payload_length] = 0; // null terminate
                    iwrap_callback_rxdata(iwrap_rx_packet_channel, iwrap_rx_payload_length, iwrap_tptr);
                }
          #endif
            }
            
            // reset all packet metadata
            iwrap_rx_packet_length = 0;
            iwrap_rx_packet_channel = 0;
            iwrap_rx_packet_flags = 0;
            iwrap_in_packet = 0;
            
            // free memory if necessary
            if (iwrap_rx_packet_size > 64) {
                // decrease to 64 bytes and verify allocation
                iwrap_tptr = (uint8_t *)realloc(iwrap_rx_packet, iwrap_rx_packet_size = 64);
                if (!iwrap_tptr) { return 1; }
                iwrap_rx_packet = iwrap_tptr;
            }
        }
    }
	return 0;
}

#ifdef IWRAP_INCLUDE_MUX
    
    /**
     * @brief Build MUX frame from given raw data and channel
     * @param channel Link ID or iWRAP command channel (0xFF)
     * @param in_len Full length of raw data byte array to pack into MUX frame
     * @param in Raw data byte array
     * @param out_len Full length of populated MUX frame byte array
     * @param out Byte array to populate with new data (must be pre-allocated)
     * @return Result code (non-zero indicates error)
     */
    uint8_t iwrap_pack_mux_frame(uint8_t channel, uint16_t in_len, uint8_t *in, uint16_t *out_len, uint8_t **out) {
        // allocate enough memory for the whole MUX frame
        *out = (uint8_t *)malloc(in_len + 5);
        
        // make sure allocation completed successfully
        if (out == 0) { return 1; }
        
        // build frame
        *out_len = in_len + 5;
        (*out)[0] = 0xBF;
        (*out)[1] = channel;
        (*out)[2] = 0x00 | ((in_len >> 8) & 0x03); // flags = 0 always in latest iWRAP (2014-05-05)
        (*out)[3] = in_len;
        memcpy((*out) + 4, in, in_len);
        (*out)[in_len + 4] = channel ^ 0xFF;
        return 0;
    }

    /**
     * @brief Disassemble MUX frame into components
     * @param in_len Full length of MUX frame
     * @param in MUX frame byte array
     * @param channel MUX frame channel
     * @param flags MUX frame flags byte
     * @param length Payload data length
     * @param out Payload data byte array
     * @param copy Flag to enable copying to new memory (non-zero), or just pointers to existing memory
     * @return Result code (non-zero indicates error)
     */
    uint8_t iwrap_unpack_mux_frame(uint16_t in_len, uint8_t *in, uint8_t *channel, uint8_t *flags, uint16_t *length, uint8_t **out, uint8_t copy) {
        if (in_len < 5 || in[0] != 0xBF) { return 2; }          // invalid MUX frame size/format
        if ((in[1] ^ 0xFF) != in[in_len - 1]) { return 3; }     // checksum failure
        
        *channel = in[1];
        *flags = in[2] >> 2;
        *length = in[3] | ((in[2] & 0x03) << 8);
        
        if (copy) {
            // allocate enough memory for the payload data
            *out = (uint8_t *)malloc(*length);
            
            // make sure allocation completed successfully
            if (out == 0) { return 1; }
            
            // copy payload into new allocated block
            memcpy(*out, in + 4, *length);
        } else {
            // just create a pointer
            *out = in + 4;
        }
        return 0;
    }
#endif /* IWRAP_INCLUDE_MUX */

/**
 * @brief Parse %02X... hexadecimal string into binary byte array
 * @param nptr Pointer to beginning of string to parse
 * @param endptr New pointer after parsing finishes
 * @param dest Container for parsed binary data
 * @param maxlen Maximum number of bytes to parse
 * @return Number of bytes actually parsed
 */
 uint8_t iwrap_hexstrtobin(const char *nptr, char **endptr, uint8_t *dest, uint8_t maxlen) {
    uint16_t i;
    char *newptr = (char *)nptr, b;
    if (nptr == 0 || dest == 0) return 0; // oops
    for (i = 0; (newptr - nptr) < maxlen || maxlen == 0; newptr++) {
        b = newptr[0];
        if (b > 0x60 && b < 0x7B) b &= (~0x20); // force uppercase hex notation
        if (b == ':') continue; // exception for ':' delineator between hex bytes
        if (!(b > 0x2F && b < 0x3A) && !(b > 0x40 && b < 0x5B)) break; // no more hexadecimal characters
        if (b > 0x39) b -= 7; // move A-F into correct range
        if (i & 1) dest[i / 2] |= (b - 0x30);
        else dest[i / 2] = (b - 0x30) << 4;
        i++;
    }
    if (endptr) *endptr = newptr;
    return newptr - nptr;
}

uint8_t iwrap_hexstrtobin2(const char *nptr, char **endptr, uint8_t *dest, uint8_t maxlen) {
    uint16_t i;
    char *newptr = (char *)nptr, b;
    if (nptr == 0 || dest == 0) return 0; // oops
    for (i = 0; (newptr - nptr) < maxlen || maxlen == 0; newptr++) {
        b = newptr[0];
        if (b > 0x60 && b < 0x7B) b &= (~0x20); // force uppercase hex notation
        if (b == ':' || b == ' ') continue; // exception for ':' delineator between hex bytes
        if (!(b > 0x2F && b < 0x3A) && !(b > 0x40 && b < 0x5B)) break; // no more hexadecimal characters
        if (b > 0x39) b -= 7; // move A-F into correct range
        if (i & 1) dest[i / 2] |= (b - 0x30);
        else dest[i / 2] = (b - 0x30) << 4;
        i++;
    }
    if (endptr) *endptr = newptr;
    return newptr - nptr;
}

/**
 * @brief Convert binary byte array into %02X... ASCII string
 * @param bin Input binary string
 * @param len Length of binary data to convert
 * @param dest Pointer to destination byte array (must be pre-allocated)
 * @param delin Delineator between bytes (e.g. ':' for "00:07:80:..."), 0 to disable
 * @param nullterm Flag to enable null-termination in destination
 * @return Number of bytes written to destination container
 */
uint8_t iwrap_bintohexstr(const uint8_t *bin, uint16_t len, char **dest, uint8_t delin, uint8_t nullterm) {
    uint8_t i, mult = 2;
    if (delin) mult = 3;
    if (*dest == 0) return 0; // oops
    for (i = 0; i < len; i++) {
        (*dest)[mult * i]     = (bin[i] / 0x10) + 48 + ((bin[i] / 0x10) / 10 * 7);
        (*dest)[mult * i + 1] = (bin[i] & 0x0f) + 48 + ((bin[i] & 0x0f) / 10 * 7);
        if (delin && i < len - 1) (*dest)[mult * i + 2] = delin;
    }
    if (nullterm) {
        if (delin) (*dest)[(mult * len) - 1] = 0;
        else (*dest)[mult * len] = 0;
    }
    return delin ? (mult * len) - 1 : (mult * len) - 1;
}

#ifdef IWRAP_DEBUG
    /**
     * @brief Output single character using debug transport function
     * @param b
     * @return Value from user-supplied debug output function
     */
    int iwrap_debug_char(char b) {
        char s[2];
        s[0] = b;
        s[1] = 0;
        return iwrap_debug(s);
    }
    
    /**
     * @brief Output %02X hex byte in ASCII format using debug transport function
     * @param b
     * @return Value from user-supplied debug output function
     */
    int iwrap_debug_hex(uint8_t b) {
        char s[3];
        s[0] = (b >> 4) + 48 + ((b >> 4) / 10 * 7);
        s[1] = (b & 0x0f) + 48 + ((b & 0x0f) / 10 * 7);
        s[2] = 0;
        return iwrap_debug(s);
    }

    /**
     * @brief Output integer in ASCII format using debug transport function
     * @param i Integer to display
     * @return Value from user-supplied debug output function
     */
    int iwrap_debug_int(int32_t i) {
        char s[12];
        itoa(i, s, 10); // smaller flash usage than sprintf(), but itoa() isn't ANSI C
        //sprintf(s, "%ld", i); // larger flash usage for some MCUs than itoa()
        return iwrap_debug(s);
    }
#endif /* IWRAP_DEBUG */

int (*iwrap_output)(int length, unsigned char *data);

#ifdef IWRAP_INCLUDE_TXCOMMAND
    void (*iwrap_callback_txcommand)(uint16_t length, const uint8_t *data);
#endif
#ifdef IWRAP_INCLUDE_TXDATA
    void (*iwrap_callback_txdata)(uint8_t channel, uint16_t length, const uint8_t *data);
#endif
#ifdef IWRAP_INCLUDE_RXOUTPUT
    void (*iwrap_callback_rxoutput)(uint16_t length, const uint8_t *data);
#endif
#ifdef IWRAP_INCLUDE_RXDATA
    void (*iwrap_callback_rxdata)(uint8_t channel, uint16_t length, const uint8_t *data);
#endif

#ifdef IWRAP_INCLUDE_BUSY
    void (*iwrap_callback_busy)();
#endif
#ifdef IWRAP_INCLUDE_IDLE
    void (*iwrap_callback_idle)(uint8_t result);
#endif

#ifdef IWRAP_INCLUDE_RSP_AIO
    void (*iwrap_rsp_aio)(uint8_t source, uint16_t value);
#endif
#ifdef IWRAP_INCLUDE_RSP_AT
    void (*iwrap_rsp_at)();
#endif
#ifdef IWRAP_INCLUDE_RSP_BER
    void (*iwrap_rsp_ber)(const iwrap_address_t *bd_addr, uint32_t ber);
#endif
#ifdef IWRAP_INCLUDE_RSP_CALL
    void (*iwrap_rsp_call)(uint8_t link_id);
#endif
#ifdef IWRAP_INCLUDE_RSP_HID_GET
    void (*iwrap_rsp_hid_get)(uint16_t length, const uint8_t *descriptor);
#endif
#ifdef IWRAP_INCLUDE_RSP_INFO
    void (*iwrap_rsp_info)(uint16_t length, const char *info);
#endif
#ifdef IWRAP_INCLUDE_RSP_INQUIRY_COUNT
    void (*iwrap_rsp_inquiry_count)(uint8_t num_of_devices);
#endif
#ifdef IWRAP_INCLUDE_RSP_INQUIRY_RESULT
    void (*iwrap_rsp_inquiry_result)(const iwrap_address_t *bd_addr, uint32_t class_of_device, int8_t rssi);
#endif
#ifdef IWRAP_INCLUDE_RSP_LIST_COUNT
    void (*iwrap_rsp_list_count)(uint8_t num_of_connections);
#endif
#ifdef IWRAP_INCLUDE_RSP_LIST_RESULT
    void (*iwrap_rsp_list_result)(uint8_t link_id, const char *mode, uint16_t blocksize, uint32_t elapsed_time, uint16_t local_msc, uint16_t remote_msc, const iwrap_address_t *bd_addr, uint16_t channel, uint8_t direction, uint8_t powermode, uint8_t role, uint8_t crypt, uint16_t buffer, uint8_t eretx);
#endif
#ifdef IWRAP_INCLUDE_RSP_OBEX
    void (*iwrap_rsp_obex)(uint16_t length, const uint8_t *data);
#endif
#ifdef IWRAP_INCLUDE_RSP_PAIR
    void (*iwrap_rsp_pair)(const iwrap_address_t *bd_addr, uint8_t result);
#endif
#ifdef IWRAP_INCLUDE_RSP_PIO_GET
    void (*iwrap_rsp_pio_get)(uint16_t state);
#endif
#ifdef IWRAP_INCLUDE_RSP_PIO_GETBIAS
    void (*iwrap_rsp_pio_getbias)(uint16_t state);
#endif
#ifdef IWRAP_INCLUDE_RSP_PIO_GETDIR
    void (*iwrap_rsp_pio_getdir)(uint16_t state);
#endif
#ifdef IWRAP_INCLUDE_RSP_PLAY
    void (*iwrap_rsp_play)(uint8_t result);
#endif
#ifdef IWRAP_INCLUDE_RSP_RFCOMM
    void (*iwrap_rsp_rfcomm)(uint8_t channel);
#endif
#ifdef IWRAP_INCLUDE_RSP_RSSI
    void (*iwrap_rsp_rssi)(const iwrap_address_t *bd_addr, int8_t rssi);
#endif
#ifdef IWRAP_INCLUDE_RSP_SDP
    void (*iwrap_rsp_sdp)(const iwrap_address_t *bd_addr, const char *record);
#endif
#ifdef IWRAP_INCLUDE_RSP_SDP_ADD
    void (*iwrap_rsp_sdp_add)(uint8_t channel);
#endif
#ifdef IWRAP_INCLUDE_RSP_SET
    void (*iwrap_rsp_set)(uint8_t category, const char *option, const char *value);
#endif
#ifdef IWRAP_INCLUDE_RSP_SSP_GETOOB
    void (*iwrap_rsp_ssp_getoob)(const uint8_t *key1, const uint8_t *key2);
#endif
#ifdef IWRAP_INCLUDE_RSP_SYNTAX_ERROR
    void (*iwrap_rsp_syntax_error)();
#endif
#ifdef IWRAP_INCLUDE_RSP_TEMP
    void (*iwrap_rsp_temp)(int8_t temp);
#endif
#ifdef IWRAP_INCLUDE_RSP_TEST
    void (*iwrap_rsp_test)(uint8_t result);
#endif
#ifdef IWRAP_INCLUDE_RSP_TESTMODE
    void (*iwrap_rsp_testmode)();
#endif
#ifdef IWRAP_INCLUDE_RSP_TXPOWER
    void (*iwrap_rsp_txpower)(const iwrap_address_t *bd_addr, int8_t txpower);
#endif

#ifdef IWRAP_INCLUDE_EVT_A2DP_CODEC
    void (*iwrap_evt_a2dp_codec)(const char *codec, uint8_t channel_mode, uint16_t rate, uint8_t bitpool_min, uint8_t bitpool_max);
#endif
#ifdef IWRAP_INCLUDE_EVT_A2DP_STREAMING_START
    void (*iwrap_evt_a2dp_streaming_start)(uint8_t link_id);
#endif
#ifdef IWRAP_INCLUDE_EVT_A2DP_STREAMING_STOP
    void (*iwrap_evt_a2dp_streaming_stop)(uint8_t link_id);
#endif
#ifdef IWRAP_INCLUDE_EVT_AUDIO_ROUTE
    void (*iwrap_evt_audio_route)(uint8_t link_id, uint8_t type, uint8_t channels);
#endif
#ifdef IWRAP_INCLUDE_EVT_AUTH
    void (*iwrap_evt_auth)(const iwrap_address_t *bd_addr);
#endif
#ifdef IWRAP_INCLUDE_EVT_ARVCP_RSP_PARSED
    void (*iwrap_evt_avrcp_rsp_parsed)(const char *pdu_name, uint16_t length, const char *data);
#endif
#ifdef IWRAP_INCLUDE_EVT_AVRCP_RSP_UNPARSED
    void (*iwrap_evt_avrcp_rsp_unparsed)(uint8_t pdu_id, uint16_t length, const uint8_t *data);
#endif
#ifdef IWRAP_INCLUDE_EVT_ARVCP_RSP_REJECTED
    void (*iwrap_evt_avrcp_rsp_rejected)(const char *pdu_name);
#endif
#ifdef IWRAP_INCLUDE_EVT_BATTERY
    void (*iwrap_evt_battery)(uint16_t mv);
#endif
#ifdef IWRAP_INCLUDE_EVT_BATTERY_FULL
    void (*iwrap_evt_battery_full)(uint16_t mv);
#endif
#ifdef IWRAP_INCLUDE_EVT_BATTERY_LOW
    void (*iwrap_evt_battery_low)(uint16_t mv);
#endif
#ifdef IWRAP_INCLUDE_EVT_BATTERY_SHOWDOWN
    void (*iwrap_evt_battery_shutdown)(uint16_t mv);
#endif
#ifdef IWRAP_INCLUDE_EVT_CLOCK
    void (*iwrap_evt_clock)(const iwrap_address_t *bd_addr, uint32_t clock);
#endif
#ifdef IWRAP_INCLUDE_EVT_CONNAUTH
    void (*iwrap_evt_connauth)(const iwrap_address_t *bd_addr, uint8_t protocol_id, uint16_t channel_id);
#endif
#ifdef IWRAP_INCLUDE_EVT_CONNECT
    void (*iwrap_evt_connect)(uint8_t link_id, const char *profile, uint16_t target, const iwrap_address_t *address);
#endif
#ifdef IWRAP_INCLUDE_EVT_HID_OUTPUT
    void (*iwrap_evt_hid_output)(uint8_t link_id, uint16_t data_length, const uint8_t *data);
#endif
#ifdef IWRAP_INCLUDE_EVT_HID_SUSPEND
    void (*iwrap_evt_hid_suspend)(uint8_t link_id);
#endif
#ifdef IWRAP_INCLUDE_EVT_HFP
    void (*iwrap_evt_hfp)(uint8_t link_id, const char *type, const char *detail);
#endif
#ifdef IWRAP_INCLUDE_EVT_HFP_AG
    void (*iwrap_evt_hfp_ag)(uint8_t link_id, const char *type, const char *detail);
#endif
#ifdef IWRAP_INCLUDE_EVT_IDENT
    void (*iwrap_evt_ident)(const char *src, uint16_t vendor_id, uint16_t product_id, const char *version, const char *descr);
#endif
#ifdef IWRAP_INCLUDE_EVT_IDENT_ERROR
    void (*iwrap_evt_ident_error)(uint16_t error_code, const iwrap_address_t *address, const char *message);
#endif
#ifdef IWRAP_INCLUDE_EVT_INQUIRY_EXTENDED
    void (*iwrap_evt_inquiry_extended)(const iwrap_address_t *address, uint8_t length, const uint8_t *data);
#endif
#ifdef IWRAP_INCLUDE_EVT_INQUIRY_PARTIAL
    void (*iwrap_evt_inquiry_partial)(const iwrap_address_t *address, uint32_t class_of_device, const char *cached_name, int8_t rssi);
#endif
#ifdef IWRAP_INCLUDE_EVT_NO_CARRIER
    void (*iwrap_evt_no_carrier)(uint8_t link_id, uint16_t error_code, const char *message);
#endif
#ifdef IWRAP_INCLUDE_EVT_NAME
    void (*iwrap_evt_name)(const iwrap_address_t *address, const char *friendly_name);
#endif
#ifdef IWRAP_INCLUDE_EVT_NAME_ERROR
    void (*iwrap_evt_name_error)(uint16_t error_code, const iwrap_address_t *address, const char *message);
#endif
#ifdef IWRAP_INCLUDE_EVT_OBEX_AUTH
    void (*iwrap_evt_obex_auth)(uint16_t user_id, uint8_t readonly, uint16_t realm);
#endif
#ifdef IWRAP_INCLUDE_EVT_OK
    void (*iwrap_evt_ok)();
#endif
#ifdef IWRAP_INCLUDE_EVT_PAIR
    void (*iwrap_evt_pair)(const iwrap_address_t *address, uint8_t key_type, const uint8_t *link_key);
#endif
#ifdef IWRAP_INCLUDE_EVT_PAIR_ERR_MAX_PAIRCOUNT
    void (*iwrap_evt_pair_err_max_paircount)();
#endif
#ifdef IWRAP_INCLUDE_EVT_READY
    void (*iwrap_evt_ready)();
#endif
#ifdef IWRAP_INCLUDE_EVT_RING
    void (*iwrap_evt_ring)(uint8_t link_id, const iwrap_address_t *address, uint16_t channel, const char *profile);
#endif
#ifdef IWRAP_INCLUDE_EVT_SSPAUTH
    void (*iwrap_evt_sspauth)(const iwrap_address_t *bd_addr);
#endif
#ifdef IWRAP_INCLUDE_EVT_SSP_COMPLETE
    void (*iwrap_evt_ssp_complete)(const iwrap_address_t *bd_addr, uint16_t error);
#endif
#ifdef IWRAP_INCLUDE_EVT_SSP_CONFIRM
    void (*iwrap_evt_ssp_confirm)(const iwrap_address_t *bd_addr, uint32_t passkey, uint8_t confirm_req);
#endif
#ifdef IWRAP_INCLUDE_EVT_SSP_PASSKEY
    void (*iwrap_evt_ssp_passkey)(const iwrap_address_t *bd_addr);
#endif
#ifdef IWRAP_INCLUDE_EVT_VOLUME
    void (*iwrap_evt_volume)(uint8_t volume);
#endif
