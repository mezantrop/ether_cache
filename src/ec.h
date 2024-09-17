/* -------------------------------------------------------------------------- */
/*
  "THE BEER-WARE LICENSE" (Revision 42):
  zmey20000@yahoo.com wrote this file. As long as you retain this notice you
  can do whatever you want with this stuff. If we meet some day, and you think
  this stuff is worth it, you can buy me a beer in return Mikhail Zakharov
*/

/* -------------------------------------------------------------------------- */

#include <netinet/ether.h>

/* -------------------------------------------------------------------------- */
typedef struct __attribute__((packed)) eth_cache {
        struct {
                struct ether_header eh;
                struct {
                        uint16_t pp;            /* Prefix within payload */
                        uint8_t cmd;            /* <0x00 send; >0x80 retrive */
                        uint8_t pl[1493];       /* Payload */
                } pl;
        } fm;
        uint32_t crc;
} eth_cache;

