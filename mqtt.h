#include <stdint.h>

/* Max number of topics one can subscribe to in a single SUBSCRIBE message */
#define MSG_SUB_MAXNTOPICS 8 

/* Control Command Types */
enum
{
  CTRL_RESERVED,    /*  0 */
  CTRL_CONNECT,     /*  1 */
  CTRL_CONNACK,     /*  2 */
  CTRL_PUBLISH,     /*  3 */
  CTRL_PUBACK,      /*  4 */
  CTRL_PUBREC,      /*  5 */
  CTRL_PUBREL,      /*  6 */
  CTRL_PUBCOMP,     /*  7 */
  CTRL_SUBSCRIBE,   /*  8 */
  CTRL_SUBACK,      /*  9 */
  CTRL_UNSUBSCRIBE, /* 10 */
  CTRL_UNSUBACK,    /* 11 */
  CTRL_PINGREQ,     /* 12 */
  CTRL_PINGRESP,    /* 13 */
  CTRL_DISCONNECT,  /* 14 */
};

enum
{
  QOS_AT_MOST_ONCE,  /* 0 - "Fire and Forget" */
  QOS_AT_LEAST_ONCE, /* 1 - ARQ until ACK */
  QOS_EXACTLY_ONCE,  /* 2 - Transactional delivery */
};



/* Simple connect: No username/password, no QoS etc. */
int mqtt_encode_connect_msg(uint8_t* pu8dst, uint8_t* pu8clientid, uint16_t u16clientid_len); /* u8conn_flgs = 2, u16keepalive = 60 */
/* Advanced connect: More options available */
int mqtt_encode_connect_msg2(uint8_t* pu8dst, uint8_t u8conn_flgs, uint16_t u16keepalive, uint8_t* pu8clientid, uint16_t u16clientid_len);
int mqtt_encode_disconnect_msg(uint8_t* pu8dst);
int mqtt_encode_ping_msg(uint8_t* pu8dst);
int mqtt_encode_publish_msg(uint8_t* pu8dst, uint8_t* pu8topic, uint16_t u16topic_len, uint8_t u8qos, uint16_t u16msg_id, uint8_t* pu8payload, uint32_t u32data_len);
int mqtt_encode_subscribe_msg(uint8_t* pu8dst, uint8_t* pu8topic, uint16_t u16topic_len, uint8_t u8qos, uint16_t u16msg_id);
int mqtt_encode_subscribe_msg2(uint8_t* pu8dst, uint8_t** apu8topic, uint16_t* au16topic_len, uint8_t* au8qos, uint32_t u32nargs, uint16_t u16msg_id);
int mqtt_encode_unsubscribe_msg(uint8_t* pu8dst, uint8_t* pu8topic, uint16_t u16topic_len, uint8_t u8qos, uint16_t u16msg_id);
int mqtt_encode_unsubscribe_msg2(uint8_t* pu8dst, uint8_t** apu8topic, uint16_t* au16topic_len, uint8_t* au8qos, uint32_t u32nargs, uint16_t u16msg_id);

int mqtt_decode_connack_msg(uint8_t* pu8src, uint32_t u32nbytes);
int mqtt_decode_pingresp_msg(uint8_t* pu8src, uint32_t u32nbytes);
int mqtt_decode_puback_msg(uint8_t* pu8src, uint32_t u32nbytes, uint16_t* pu16msg_id);
int mqtt_decode_suback_msg(uint8_t* pu8src, uint32_t u32nbytes, uint16_t* pu16msg_id_out);

int mqtt_decode_msg(uint8_t* pu8src, uint8_t* pu8ctrl_type, uint8_t* pu8flgs, uint8_t* pu8data_out, uint32_t* pu32output_len);
int mqtt_decode_publish_msg(uint8_t* pu8src, uint32_t u32nbytes, uint8_t* pu8qos, uint16_t* pu16msg_id_out, uint16_t* pu16topic_len, uint8_t** ppu8topic, uint8_t** ppu8payload);


