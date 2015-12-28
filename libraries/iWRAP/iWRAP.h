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

#ifndef _IWRAP_H_
#define _IWRAP_H_

#include <stdint.h>

#ifndef IWRAP_CONFIGURED
    #define IWRAP_DEBUG
    //#define IWRAP_DEBUG_TEMP

    /******************************************************************************/
    /* ENABLE SUPPORT FOR THE FUNCTIONALTIY YOU NEED, DISABLE TO REDUCE FLASH USE */
    /******************************************************************************/

    #define IWRAP_INCLUDE_MUX                           // READY

    #define IWRAP_INCLUDE_TXCOMMAND                     // READY
    #define IWRAP_INCLUDE_TXDATA                        // READY
    #define IWRAP_INCLUDE_RXOUTPUT                      // READY
    #define IWRAP_INCLUDE_RXDATA                        // READY
    #define IWRAP_INCLUDE_BUSY                          // READY
    #define IWRAP_INCLUDE_IDLE                          // READY

    #define IWRAP_INCLUDE_RSP_AIO                       // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_AT                        // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_BER                       // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_CALL                      // READY
    #define IWRAP_INCLUDE_RSP_HID_GET                   // READY
    #define IWRAP_INCLUDE_RSP_INFO                      // READY
    #define IWRAP_INCLUDE_RSP_INQUIRY_COUNT             // READY
    #define IWRAP_INCLUDE_RSP_INQUIRY_RESULT            // READY
    #define IWRAP_INCLUDE_RSP_LIST_COUNT                // READY
    #define IWRAP_INCLUDE_RSP_LIST_RESULT               // READY
    #define IWRAP_INCLUDE_RSP_OBEX                      // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_PAIR                      // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_PIO_GET                   // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_PIO_GETBIAS               // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_PIO_GETDIR                // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_PLAY                      // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_RFCOMM                    // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_RSSI                      // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_SDP                       // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_SDP_ADD                   // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_SET                       // READY
    #define IWRAP_INCLUDE_RSP_SSP_GETOOB                // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_SYNTAX_ERROR              // READY
    #define IWRAP_INCLUDE_RSP_TEMP                      // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_TEST                      // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_TESTMODE                  // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_RSP_TXPOWER                   // NOT IMPLEMENTED

    #define IWRAP_INCLUDE_EVT_A2DP_CODEC                // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_A2DP_STREAMING_START      // READY
    #define IWRAP_INCLUDE_EVT_A2DP_STREAMING_STOP       // READY
    #define IWRAP_INCLUDE_EVT_AUDIO_ROUTE               // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_AUTH                      // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_AVRCP_RSP_PARSED          // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_AVRCP_RSP_UNPARSED        // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_AVRCP_RSP_REJECTED        // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_BATTERY                   // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_BATTERY_FULL              // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_BATTERY_LOW               // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_BATTERY_SHUTDOWN          // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_CLOCK                     // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_CONNAUTH                  // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_CONNECT                   // READY
    #define IWRAP_INCLUDE_EVT_HID_OUTPUT                // READY
    #define IWRAP_INCLUDE_EVT_HID_SUSPEND               // READY
    #define IWRAP_INCLUDE_EVT_HFP                       // READY
    #define IWRAP_INCLUDE_EVT_HFP_AG                    // READY
    #define IWRAP_INCLUDE_EVT_IDENT                     // READY
    #define IWRAP_INCLUDE_EVT_IDENT_ERROR               // READY
    #define IWRAP_INCLUDE_EVT_INQUIRY_EXTENDED          // READY
    #define IWRAP_INCLUDE_EVT_INQUIRY_PARTIAL           // READY
    #define IWRAP_INCLUDE_EVT_NO_CARRIER                // READY
    #define IWRAP_INCLUDE_EVT_NAME                      // READY
    #define IWRAP_INCLUDE_EVT_NAME_ERROR                // READY
    #define IWRAP_INCLUDE_EVT_OBEX_AUTH                 // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_OK                        // READY
    #define IWRAP_INCLUDE_EVT_PAIR                      // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_PAIR_ERR_MAX_PAIRCOUNT    // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_READY                     // READY
    #define IWRAP_INCLUDE_EVT_RING                      // READY
    #define IWRAP_INCLUDE_EVT_SSPAUTH                   // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_SSP_COMPLETE              // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_SSP_CONFIRM               // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_SSP_PASSKEY               // NOT IMPLEMENTED
    #define IWRAP_INCLUDE_EVT_VOLUME                    // NOT IMPLEMENTED
#endif

/******************************************************************************/

#define IWRAP_VERSION       502

#define IWRAP_MODE_COMMAND  1
#define IWRAP_MODE_DATA     2
#define IWRAP_MODE_MUX      3

#define IWRAP_SET_CATEGORY_BT       1
#define IWRAP_SET_CATEGORY_CONTROL  2
#define IWRAP_SET_CATEGORY_PROFILE  3

#define IWRAP_CONNECTION_DIRECTION_OUTGOING     1
#define IWRAP_CONNECTION_DIRECTION_INCOMING     2
#define IWRAP_CONNECTION_POWERMODE_ACTIVE       1
#define IWRAP_CONNECTION_POWERMODE_HOLD         2
#define IWRAP_CONNECTION_POWERMODE_SNIFF        3
#define IWRAP_CONNECTION_POWERMODE_PARK         4
#define IWRAP_CONNECTION_ROLE_MASTER            1
#define IWRAP_CONNECTION_ROLE_SLAVE             2
#define IWRAP_CONNECTION_CRYPT_PLAIN            1
#define IWRAP_CONNECTION_CRYPT_ENCRYPTED        2

typedef struct {
    uint8_t address[6];
} iwrap_address_t;

uint8_t iwrap_send_command(const char *cmd, uint8_t mode);
uint8_t iwrap_send_data(uint8_t channel, uint16_t data_len, const uint8_t *data, uint8_t mode);
uint8_t iwrap_parse(uint8_t b, uint8_t mode);
#ifdef IWRAP_INCLUDE_MUX
    uint8_t iwrap_pack_mux_frame(uint8_t channel, uint16_t in_len, uint8_t *in, uint16_t *out_len, uint8_t **out);
    uint8_t iwrap_unpack_mux_frame(uint16_t in_len, uint8_t *in, uint8_t *channel, uint8_t *flags, uint16_t *length, uint8_t **out, uint8_t copy);
#endif

uint8_t iwrap_hexstrtobin(const char *nptr, char **endptr, uint8_t *dest, uint8_t maxlen);
uint8_t iwrap_bintohexstr(const uint8_t *bin, uint16_t len, char **dest, uint8_t delin, uint8_t nullterm);

extern uint8_t iwrap_pending_boot;
extern uint8_t iwrap_pending_commands;
extern uint8_t iwrap_last_command_result;

extern int (*iwrap_output)(int length, unsigned char *data);

#ifdef IWRAP_DEBUG
    extern int (*iwrap_debug)(const char *data);
#endif

#ifdef IWRAP_INCLUDE_TXCOMMAND
    extern void (*iwrap_callback_txcommand)(uint16_t length, const uint8_t *data);
#endif
#ifdef IWRAP_INCLUDE_TXDATA
    extern void (*iwrap_callback_txdata)(uint8_t channel, uint16_t length, const uint8_t *data);
#endif
#ifdef IWRAP_INCLUDE_RXOUTPUT
    extern void (*iwrap_callback_rxoutput)(uint16_t length, const uint8_t *data);
#endif
#ifdef IWRAP_INCLUDE_RXDATA
    extern void (*iwrap_callback_rxdata)(uint8_t channel, uint16_t length, const uint8_t *data);
#endif

#ifdef IWRAP_INCLUDE_BUSY
    extern void (*iwrap_callback_busy)();
#endif
#ifdef IWRAP_INCLUDE_IDLE
    extern void (*iwrap_callback_idle)(uint8_t result);
#endif

#ifdef IWRAP_INCLUDE_RSP_AIO
    extern void (*iwrap_rsp_aio)(uint8_t source, uint16_t value);
#endif
#ifdef IWRAP_INCLUDE_RSP_AT
    extern void (*iwrap_rsp_at)();
#endif
#ifdef IWRAP_INCLUDE_RSP_BER
    extern void (*iwrap_rsp_ber)(const iwrap_address_t *bd_addr, uint32_t ber);
#endif
#ifdef IWRAP_INCLUDE_RSP_CALL
    extern void (*iwrap_rsp_call)(uint8_t link_id);
#endif
#ifdef IWRAP_INCLUDE_RSP_HID_GET
    extern void (*iwrap_rsp_hid_get)(uint16_t length, const uint8_t *descriptor);
#endif
#ifdef IWRAP_INCLUDE_RSP_INFO
    extern void (*iwrap_rsp_info)(uint16_t length, const char *info);
#endif
#ifdef IWRAP_INCLUDE_RSP_INQUIRY_COUNT
    extern void (*iwrap_rsp_inquiry_count)(uint8_t num_of_devices);
#endif
#ifdef IWRAP_INCLUDE_RSP_INQUIRY_RESULT
    extern void (*iwrap_rsp_inquiry_result)(const iwrap_address_t *bd_addr, uint32_t class_of_device, int8_t rssi);
#endif
#ifdef IWRAP_INCLUDE_RSP_LIST_COUNT
    extern void (*iwrap_rsp_list_count)(uint8_t num_of_connections);
#endif
#ifdef IWRAP_INCLUDE_RSP_LIST_RESULT
    extern void (*iwrap_rsp_list_result)(uint8_t link_id, const char *mode, uint16_t blocksize, uint32_t elapsed_time, uint16_t local_msc, uint16_t remote_msc, const iwrap_address_t *bd_addr, uint16_t channel, uint8_t direction, uint8_t powermode, uint8_t role, uint8_t crypt, uint16_t buffer, uint8_t eretx);
#endif
#ifdef IWRAP_INCLUDE_RSP_OBEX
    extern void (*iwrap_rsp_obex)(uint16_t length, const uint8_t *data);
#endif
#ifdef IWRAP_INCLUDE_RSP_PAIR
    extern void (*iwrap_rsp_pair)(const iwrap_address_t *bd_addr, uint8_t result);
#endif
#ifdef IWRAP_INCLUDE_RSP_PIO_GET
    extern void (*iwrap_rsp_pio_get)(uint16_t state);
#endif
#ifdef IWRAP_INCLUDE_RSP_PIO_GETBIAS
    extern void (*iwrap_rsp_pio_getbias)(uint16_t state);
#endif
#ifdef IWRAP_INCLUDE_RSP_PIO_GETDIR
    extern void (*iwrap_rsp_pio_getdir)(uint16_t state);
#endif
#ifdef IWRAP_INCLUDE_RSP_PLAY
    extern void (*iwrap_rsp_play)(uint8_t result);
#endif
#ifdef IWRAP_INCLUDE_RSP_RFCOMM
    extern void (*iwrap_rsp_rfcomm)(uint8_t channel);
#endif
#ifdef IWRAP_INCLUDE_RSP_RSSI
    extern void (*iwrap_rsp_rssi)(const iwrap_address_t *bd_addr, int8_t rssi);
#endif
#ifdef IWRAP_INCLUDE_RSP_SDP
    extern void (*iwrap_rsp_sdp)(const iwrap_address_t *bd_addr, const char *record);
#endif
#ifdef IWRAP_INCLUDE_RSP_SDP_ADD
    extern void (*iwrap_rsp_sdp_add)(uint8_t channel);
#endif
#ifdef IWRAP_INCLUDE_RSP_SET
    extern void (*iwrap_rsp_set)(uint8_t category, const char *option, const char *value);
#endif
#ifdef IWRAP_INCLUDE_RSP_SSP_GETOOB
    extern void (*iwrap_rsp_ssp_getoob)(const uint8_t *key1, const uint8_t *key2);
#endif
#ifdef IWRAP_INCLUDE_RSP_SYNTAX_ERROR
    extern void (*iwrap_rsp_syntax_error)();
#endif
#ifdef IWRAP_INCLUDE_RSP_TEMP
    extern void (*iwrap_rsp_temp)(int8_t temp);
#endif
#ifdef IWRAP_INCLUDE_RSP_TEST
    extern void (*iwrap_rsp_test)(uint8_t result);
#endif
#ifdef IWRAP_INCLUDE_RSP_TESTMODE
    extern void (*iwrap_rsp_testmode)();
#endif
#ifdef IWRAP_INCLUDE_RSP_TXPOWER
    extern void (*iwrap_rsp_txpower)(const iwrap_address_t *bd_addr, int8_t txpower);
#endif

#ifdef IWRAP_INCLUDE_EVT_A2DP_CODEC
    extern void (*iwrap_evt_a2dp_codec)(const char *codec, uint8_t channel_mode, uint16_t rate, uint8_t bitpool_min, uint8_t bitpool_max);
#endif
#ifdef IWRAP_INCLUDE_EVT_A2DP_STREAMING_START
    extern void (*iwrap_evt_a2dp_streaming_start)(uint8_t link_id);
#endif
#ifdef IWRAP_INCLUDE_EVT_A2DP_STREAMING_STOP
    extern void (*iwrap_evt_a2dp_streaming_stop)(uint8_t link_id);
#endif
#ifdef IWRAP_INCLUDE_EVT_AUDIO_ROUTE
    extern void (*iwrap_evt_audio_route)(uint8_t link_id, uint8_t type, uint8_t channels);
#endif
#ifdef IWRAP_INCLUDE_EVT_AUTH
    extern void (*iwrap_evt_auth)(const iwrap_address_t *bd_addr);
#endif
#ifdef IWRAP_INCLUDE_EVT_AVRCP_RSP_PARSED
    extern void (*iwrap_evt_avrcp_rsp_parsed)(const char *pdu_name, uint16_t length, const char *data);
#endif
#ifdef IWRAP_INCLUDE_EVT_AVRCP_RSP_UNPARSED
    extern void (*iwrap_evt_avrcp_rsp_unparsed)(uint8_t pdu_id, uint16_t length, const uint8_t *data);
#endif
#ifdef IWRAP_INCLUDE_EVT_AVRCP_RSP_REJECTED
    extern void (*iwrap_evt_avrcp_rsp_rejected)(const char *pdu_name);
#endif
#ifdef IWRAP_INCLUDE_EVT_BATTERY
    extern void (*iwrap_evt_battery)(uint16_t mv);
#endif
#ifdef IWRAP_INCLUDE_EVT_BATTERY_FULL
    extern void (*iwrap_evt_battery_full)(uint16_t mv);
#endif
#ifdef IWRAP_INCLUDE_EVT_BATTERY_LOW
    extern void (*iwrap_evt_battery_low)(uint16_t mv);
#endif
#ifdef IWRAP_INCLUDE_EVT_BATTERY_SHUTDOWN
    extern void (*iwrap_evt_battery_shutdown)(uint16_t mv);
#endif
#ifdef IWRAP_INCLUDE_EVT_CLOCK
    extern void (*iwrap_evt_clock)(const iwrap_address_t *bd_addr, uint32_t clock);
#endif
#ifdef IWRAP_INCLUDE_EVT_CONNAUTH
    extern void (*iwrap_evt_connauth)(const iwrap_address_t *bd_addr, uint8_t protocol_id, uint16_t channel_id);
#endif
#ifdef IWRAP_INCLUDE_EVT_CONNECT
    extern void (*iwrap_evt_connect)(uint8_t link_id, const char *profile, uint16_t target, const iwrap_address_t *address);
#endif
#ifdef IWRAP_INCLUDE_EVT_HID_OUTPUT
    extern void (*iwrap_evt_hid_output)(uint8_t link_id, uint16_t data_length, const uint8_t *data);
#endif
#ifdef IWRAP_INCLUDE_EVT_HID_SUSPEND
    extern void (*iwrap_evt_hid_suspend)(uint8_t link_id);
#endif
#ifdef IWRAP_INCLUDE_EVT_HFP
    extern void (*iwrap_evt_hfp)(uint8_t link_id, const char *type, const char *detail);
#endif
#ifdef IWRAP_INCLUDE_EVT_HFP_AG
    extern void (*iwrap_evt_hfp_ag)(uint8_t link_id, const char *type, const char *detail);
#endif
#ifdef IWRAP_INCLUDE_EVT_IDENT
    extern void (*iwrap_evt_ident)(const char *src, uint16_t vendor_id, uint16_t product_id, const char *version, const char *descr);
#endif
#ifdef IWRAP_INCLUDE_EVT_IDENT_ERROR
    extern void (*iwrap_evt_ident_error)(uint16_t error_code, const iwrap_address_t *address, const char *message);
#endif
#ifdef IWRAP_INCLUDE_EVT_INQUIRY_EXTENDED
    extern void (*iwrap_evt_inquiry_extended)(const iwrap_address_t *address, uint8_t length, const uint8_t *data);
#endif
#ifdef IWRAP_INCLUDE_EVT_INQUIRY_PARTIAL
    extern void (*iwrap_evt_inquiry_partial)(const iwrap_address_t *address, uint32_t class_of_device, const char *cached_name, int8_t rssi);
#endif
#ifdef IWRAP_INCLUDE_EVT_NO_CARRIER
    extern void (*iwrap_evt_no_carrier)(uint8_t link_id, uint16_t error_code, const char *message);
#endif
#ifdef IWRAP_INCLUDE_EVT_NAME
    extern void (*iwrap_evt_name)(const iwrap_address_t *address, const char *friendly_name);
#endif
#ifdef IWRAP_INCLUDE_EVT_NAME_ERROR
    extern void (*iwrap_evt_name_error)(uint16_t error_code, const iwrap_address_t *address, const char *message);
#endif
#ifdef IWRAP_INCLUDE_EVT_OBEX_AUTH
    extern void (*iwrap_evt_obex_auth)(uint16_t user_id, uint8_t readonly, uint16_t realm);
#endif
#ifdef IWRAP_INCLUDE_EVT_OK
    extern void (*iwrap_evt_ok)();
#endif
#ifdef IWRAP_INCLUDE_EVT_PAIR
    extern void (*iwrap_evt_pair)(const iwrap_address_t *address, uint8_t key_type, const uint8_t *link_key);
#endif
#ifdef IWRAP_INCLUDE_EVT_PAIR_ERR_MAX_PAIRCOUNT
    extern void (*iwrap_evt_pair_err_max_paircount)();
#endif
#ifdef IWRAP_INCLUDE_EVT_READY
    extern void (*iwrap_evt_ready)();
#endif
#ifdef IWRAP_INCLUDE_EVT_RING
    extern void (*iwrap_evt_ring)(uint8_t link_id, const iwrap_address_t *address, uint16_t channel, const char *profile);
#endif
#ifdef IWRAP_INCLUDE_EVT_SSPAUTH
    extern void (*iwrap_evt_sspauth)(const iwrap_address_t *bd_addr);
#endif
#ifdef IWRAP_INCLUDE_EVT_SSP_COMPLETE
    extern void (*iwrap_evt_ssp_complete)(const iwrap_address_t *bd_addr, uint16_t error);
#endif
#ifdef IWRAP_INCLUDE_EVT_SSP_CONFIRM
    extern void (*iwrap_evt_ssp_confirm)(const iwrap_address_t *bd_addr, uint32_t passkey, uint8_t confirm_req);
#endif
#ifdef IWRAP_INCLUDE_EVT_SSP_PASSKEY
    extern void (*iwrap_evt_ssp_passkey)(const iwrap_address_t *bd_addr);
#endif
#ifdef IWRAP_INCLUDE_EVT_VOLUME
    extern void (*iwrap_evt_volume)(uint8_t volume);
#endif

#endif /* _IWRAP_H_ */