#include "mqtt.h"
#include <assert.h>
#include <stdint.h>




static int mqtt_encode_length(unsigned int nbytes, uint8_t* au8data)
{
    int nbytes_encoded = 0;
    if (nbytes < 0x10000000) /* max MQTT packet size */
    {
        unsigned int x = nbytes;
        do
        {
            uint8_t u8digit = x % 128;
            x /= 128; /* >> 7 */
            if (x != 0)
                u8digit |= 0x80;
            au8data[nbytes_encoded++] = u8digit;
        }
        while (x > 0);
    }
    return nbytes_encoded;
}



static int mqtt_decode_length(uint8_t* au8data)
{
    int nbytes = 0;
    int multiplier = 1;
    unsigned int array_idx = 0;
    uint8_t u8digit;
    do
    {
        u8digit = au8data[array_idx++];
        nbytes += (u8digit & 127) * multiplier;
        multiplier *= 128;
    }
    while (    (++array_idx < 4)
            && ((u8digit & 128) != 0));
    return nbytes;
}


static int mqtt_encode_msg(uint8_t* pu8dst, uint8_t u8ctrl_type, uint8_t u8flgs, uint8_t** apu8data_in, uint32_t* au32input_len, uint32_t u32nargs, uint32_t u32input_len)
{
  int nbytes_encoded = 0;
  if (    (pu8dst != 0)
       && (u8ctrl_type > 0)    /* cmd type = [1:14] .. 0 + 15 are reserved */
       && (u8ctrl_type < 15)
       && (u8flgs < 16)        /* flag are 4 bits wide, [0:15] */
       && (    (u32nargs == 0) /* Allow for msg's without payload */
            || (    (apu8data_in != 0) /* ... otherwise, check the args */
                 && (au32input_len != 0))))
  {
    pu8dst[0] = (u8ctrl_type << 4) | (u8flgs);

    int idx = mqtt_encode_length(u32input_len, &pu8dst[1]);
    uint32_t i, j;
    for (i = 0; i < u32nargs; ++i)
    {
      for (j = 0; j < au32input_len[i]; ++j, ++nbytes_encoded)
      {
        pu8dst[idx + nbytes_encoded + 1] = apu8data_in[i][j];
      }
    }
    nbytes_encoded += (1 + idx);
  }
  return nbytes_encoded;
}


int mqtt_decode_msg(uint8_t* pu8src, uint8_t* pu8ctrl_type, uint8_t* pu8flgs, uint8_t* pu8data_out, uint32_t* pu32output_len)
{
  int nbytes_decoded = 0;
  if (    (pu8src != 0)
       && (pu8ctrl_type != 0)
       && (pu8flgs != 0)
       && (pu8data_out != 0)
       && (pu32output_len != 0))
  {
    *pu8ctrl_type = ((*pu8src) >> 4);
    *pu8flgs = ((*pu8src) & 0x0F);
    int idx = 1;
    int nbytes = mqtt_decode_length(&pu8src[idx]);
    //printf("nbytes = %d\n", nbytes);
    if (nbytes < 0x80)          /*     0 <= nbytes < 128     */
    {
      idx += 1;
    }
    else if (nbytes < 0x4000)   /*   128 <= nbytes < 16384   */
    {
      idx += 2;
    }
    else if (nbytes < 0x200000) /* 16384 <= nbytes < 2097152 */
    {
      idx += 3;
    }
    else
    {
      idx += 4;
      assert(nbytes < 0x10000000);
    }
    /* Copy decoded data to output-buffer */
    int i;
    for (i = 0; i < nbytes; ++i)
    {
      pu8data_out[i] = pu8src[idx + i];
    }
    //memcpy(pu8data_out, &pu8src[idx], nbytes);
    *pu32output_len = nbytes;
    nbytes_decoded = nbytes + idx;
  }
  return nbytes_decoded;
}



/* Advanced connect: More options available */
int mqtt_encode_connect_msg2(uint8_t* pu8dst, uint8_t u8conn_flgs, uint16_t u16keepalive, uint8_t* pu8clientid, uint16_t u16clientid_len)
{
  int nbytes_encoded = 0;
  if (    (pu8dst != 0)
       && (pu8clientid != 0))
  {
    const uint32_t u32hdr_len  = 12;
    uint8_t u8keepalive_msb    = (u16keepalive & 0xFF00) >> 8;    /* Bug if on Big-Endian machine */
    uint8_t u8keepalive_lsb    = (u16keepalive & 0x00FF);
    uint8_t u8clientid_len_msb = (u16clientid_len & 0xFF00) >> 8; /* Bug if on Big-Endian machine */
    uint8_t u8clientid_len_lsb = (u16clientid_len & 0x00FF);
    uint8_t au8conn_buf[u32hdr_len];
    /* Variable part of header for CONNECT msg */
    int idx = 0;
    au8conn_buf[idx++] = 0x00; /* protocol version length U16 */
    au8conn_buf[idx++] = 0x04; /* 4 bytes long (MQTT) */
    au8conn_buf[idx++] = 'M';
    au8conn_buf[idx++] = 'Q';
    au8conn_buf[idx++] = 'T';
    au8conn_buf[idx++] = 'T';
    au8conn_buf[idx++] = 0x04; /* 4 == MQTT version 3.1.1 */
    au8conn_buf[idx++] = u8conn_flgs;
    au8conn_buf[idx++] = u8keepalive_msb;
    au8conn_buf[idx++] = u8keepalive_lsb;
    au8conn_buf[idx++] = u8clientid_len_msb;
    au8conn_buf[idx++] = u8clientid_len_lsb;

    uint8_t* buffers[] = { au8conn_buf, pu8clientid };
    uint32_t sizes[] = { u32hdr_len, u16clientid_len };

    nbytes_encoded = mqtt_encode_msg(pu8dst, CTRL_CONNECT, 0, buffers, sizes, 2, u32hdr_len + u16clientid_len);
  }
  return nbytes_encoded;
}


/* Simple connect: No username/password, no QoS etc. */
int mqtt_encode_connect_msg(uint8_t* pu8dst, uint8_t* pu8clientid, uint16_t u16clientid_len) /* u8conn_flgs = 2, u16keepalive = 60 */
{
  return mqtt_encode_connect_msg2(pu8dst, 0x02, 60, pu8clientid, u16clientid_len);
}

int mqtt_encode_disconnect_msg(uint8_t* pu8dst)
{
  return mqtt_encode_msg(pu8dst, CTRL_DISCONNECT, 0x02, 0, 0, 0, 0);
}

int mqtt_encode_ping_msg(uint8_t* pu8dst)
{
  return mqtt_encode_msg(pu8dst, CTRL_PINGREQ, 0x02, 0, 0, 0, 0);
}


int mqtt_encode_publish_msg(uint8_t* pu8dst, uint8_t* pu8topic, uint16_t u16topic_len, uint8_t u8qos, uint16_t u16msg_id, uint8_t* pu8payload, uint32_t u32data_len)
{
  int nbytes_encoded = 0;
  if (pu8topic != 0)
  {
    uint32_t u32msg_len = sizeof(uint16_t) + u16topic_len + sizeof(uint16_t) + u32data_len;
    uint8_t u8topic_len_msb = (u16topic_len & 0xFF00) >> 8; /* Bug if on Big-Endian machine */
    uint8_t u8topic_len_lsb = (u16topic_len & 0x00FF);
    uint8_t u8msg_id_msb    = (u16msg_id & 0xFF00) >> 8;    /* Bug if on Big-Endian machine */
    uint8_t u8msg_id_lsb    = (u16msg_id & 0x00FF);
    uint8_t au8topic_len_buf[sizeof(uint16_t)] = { u8topic_len_msb, u8topic_len_lsb };
    uint8_t au8msg_id_buf[sizeof(uint16_t)] = { u8msg_id_msb, u8msg_id_lsb };
    uint8_t* buffers[] = { au8topic_len_buf, pu8topic, au8msg_id_buf, pu8payload };
    uint32_t sizes[] = { sizeof(uint16_t), u16topic_len, sizeof(uint16_t), u32data_len };
    nbytes_encoded = mqtt_encode_msg(pu8dst, CTRL_PUBLISH, u8qos << 1, buffers, sizes, 4, u32msg_len);
  }
  return nbytes_encoded;
}




static int encode_pubsub_msg2(uint8_t* pu8dst, uint8_t u8ctrl, uint8_t** apu8topic, uint16_t* au16topic_len, uint8_t* au8qos, uint32_t u32nargs, uint16_t u16msg_id)
{
  int nbytes_encoded = 0;
  if (    (apu8topic != 0)
       && (u32nargs < MSG_SUB_MAXNTOPICS))
  {
    uint8_t topicsizes[sizeof(uint16_t)][MSG_SUB_MAXNTOPICS];
    uint32_t sizes[1 + (3 * MSG_SUB_MAXNTOPICS)];   /* for each topic a topic-len, the topic itself, and the qos -- hence 3 * max(subs_pr_msg) */
    uint8_t* buffers[1 + (3 * MSG_SUB_MAXNTOPICS)]; /* msgid, [topic-len + topic + qos]+ */
    uint8_t u8msg_id_msb    = (u16msg_id & 0xFF00) >> 8;    /* Bug if on Big-Endian machine */
    uint8_t u8msg_id_lsb    = (u16msg_id & 0x00FF);
    uint8_t au8msg_id_buf[sizeof(uint16_t)] = { u8msg_id_msb, u8msg_id_lsb };
    int bufidx = 0;
    buffers[bufidx] = au8msg_id_buf;
    sizes[bufidx++] = sizeof(uint16_t);
    uint32_t u32msg_len = sizeof(uint16_t); /* msgid */
    uint8_t u8highest_qos = 0;
    uint32_t i;
    for (i = 0; i < u32nargs; ++i)
    {
      u32msg_len += sizeof(uint16_t) + au16topic_len[i] + sizeof(uint8_t); /* topic-len + topic + qos */
      uint8_t u8topic_len_msb = (au16topic_len[i] & 0xFF00) >> 8; /* Bug if on Big-Endian machine */
      uint8_t u8topic_len_lsb = (au16topic_len[i] & 0x00FF);
      topicsizes[i][0] = u8topic_len_msb;
      topicsizes[i][1] = u8topic_len_lsb;
      buffers[bufidx] = topicsizes[i];
      sizes[bufidx++] = sizeof(uint16_t);
      buffers[bufidx] = apu8topic[i];
      sizes[bufidx++] = au16topic_len[i];
      buffers[bufidx] = &au8qos[i];
      sizes[bufidx++] = sizeof(uint8_t);
      /* Send MQTT SUBSCRIBE msg with highest QoS of all subscribtions */
      if (au8qos[i] > u8highest_qos)
      {
        u8highest_qos = au8qos[i];
      }
    }
    nbytes_encoded = mqtt_encode_msg(pu8dst, u8ctrl, u8highest_qos << 1, buffers, sizes, bufidx, u32msg_len);
  }
  return nbytes_encoded;
}



int mqtt_encode_subscribe_msg(uint8_t* pu8dst, uint8_t* pu8topic, uint16_t u16topic_len, uint8_t u8qos, uint16_t u16msg_id)
{
  return encode_pubsub_msg2(pu8dst, CTRL_SUBSCRIBE, &pu8topic, &u16topic_len, &u8qos, 1, u16msg_id);
}

int mqtt_encode_unsubscribe_msg(uint8_t* pu8dst, uint8_t* pu8topic, uint16_t u16topic_len, uint8_t u8qos, uint16_t u16msg_id)
{
  return encode_pubsub_msg2(pu8dst, CTRL_UNSUBSCRIBE, &pu8topic, &u16topic_len, &u8qos, 1, u16msg_id);
}
int mqtt_encode_subscribe_msg2(uint8_t* pu8dst, uint8_t** apu8topic, uint16_t* au16topic_len, uint8_t* au8qos, uint32_t u32nargs, uint16_t u16msg_id)
{
  return encode_pubsub_msg2(pu8dst, CTRL_SUBSCRIBE, apu8topic, au16topic_len, au8qos, u32nargs, u16msg_id);
}


int mqtt_encode_unsubscribe_msg2(uint8_t* pu8dst, uint8_t** apu8topic, uint16_t* au16topic_len, uint8_t* au8qos, uint32_t u32nargs, uint16_t u16msg_id)
{
  return encode_pubsub_msg2(pu8dst, CTRL_UNSUBSCRIBE, apu8topic, au16topic_len, au8qos, u32nargs, u16msg_id);
}




int mqtt_decode_connack_msg(uint8_t* pu8src, uint32_t u32nbytes)
{
  return (    (pu8src != 0)
           && (u32nbytes >= 4)
           && (pu8src[0] == 0x20)   /* 0x20 : CTRL_CONNACK << 4        */
           && (pu8src[1] == 0x02)   /* 0x02 : bytes after fixed header */
        // && (pu8src[3] == 0x00)   /* 0x00 : 3rd byte is reserved     */
           && (pu8src[3] == 0x00)); /* 0x00 : Connection Accepted      */
}


int mqtt_decode_pingresp_msg(uint8_t* pu8src, uint32_t u32nbytes)
{
  return (    (pu8src != 0)
           && (u32nbytes >= 2)
           && (pu8src[0] == 0xd0)   /* 0xD0 : CTRL_PINGRESP << 4        */
           && (pu8src[1] == 0x00)); /* 0x00 : bytes after fixed header  */
}

int mqtt_decode_puback_msg(uint8_t* pu8src, uint32_t u32nbytes, uint16_t* pu16msg_id_out)
{
  int success = 0;
  if (    (pu8src != 0)
       && (u32nbytes >= 4)
       && (pu8src[0] == 0x40)      /* 0x40 : CTRL_PUBACK << 4         */
       && (pu8src[1] == 0x02)      /* 0x02 : bytes after fixed header */
       && (pu16msg_id_out != 0))
  {
    *pu16msg_id_out = (pu8src[2] << 8) | pu8src[3];
    success = 1;
  }
  return success;
}


int mqtt_decode_suback_msg(uint8_t* pu8src, uint32_t u32nbytes, uint16_t* pu16msg_id_out)
{
  int success = 0;
  if (    (pu8src != 0)
       && (u32nbytes >= 4)
       && (pu8src[0] == 0x90)      /* 0x90 : CTRL_SUBACK << 4         */
       && (pu8src[1] == 0x02)      /* 0x02 : bytes after fixed header */
       && (pu16msg_id_out != 0))
  {
    *pu16msg_id_out = (pu8src[2] << 8) | pu8src[3];
    success = 1;
  }
  return success;
}

int mqtt_decode_publish_msg(uint8_t* pu8src, uint32_t u32nbytes, uint8_t* pu8qos, uint16_t* pu16msg_id_out, uint16_t* pu16topic_len, uint8_t** ppu8topic, uint8_t** ppu8payload)
{
  int success = 0;
  if (    (pu8src != 0)
       && (u32nbytes >= 6)
       && (pu8src[0] >> 4 == CTRL_PUBLISH)
       && (pu16msg_id_out != 0)
       && (pu16topic_len != 0)
       && (ppu8topic != 0)
       && (ppu8payload != 0)    )
  {
    *pu8qos = (pu8src[0] >> 1) & 3;
    uint16_t u16topic_len = (pu8src[2] << 8) | pu8src[3];
    *pu16topic_len = u16topic_len;
    *ppu8topic = &pu8src[4];
    *pu16msg_id_out = (pu8src[4 + u16topic_len] << 8) | pu8src[5 + u16topic_len];
    *ppu8payload = &pu8src[6 + u16topic_len];
    success = 1;
  }
  return success;
}



#if defined(TEST) && (TEST == 1)

int main(void)
{
  // int mqtt_encode_connect_msg2(uint8_t* pu8dst, uint8_t u8conn_flgs, uint16_t u16keepalive, uint8_t* pu8clientid, uint16_t u16clientid_len)
  uint8_t buf[128];
  int nbytes;
  int i;

  nbytes = mqtt_encode_connect_msg2(buf, 0x02, 60, (uint8_t*)"DIGI", 4);
  printf("con: ");
  for (i = 0; i < nbytes; ++i)
    printf("0x%.02x ", buf[i]);
  printf("\n");

  nbytes = mqtt_encode_connect_msg(buf, (uint8_t*)"DIGI", 4);
  printf("con: ");
  for (i = 0; i < nbytes; ++i)
    printf("0x%.02x ", buf[i]);
  printf("\n");

  nbytes = mqtt_encode_disconnect_msg(buf);
  for (i = 0; i < nbytes; ++i)
    printf("0x%.02x ", buf[i]);
  printf("\n");

  nbytes = mqtt_encode_ping_msg(buf);
  for (i = 0; i < nbytes; ++i)
    printf("0x%.02x ", buf[i]);
  printf("\n");

  nbytes = mqtt_encode_publish_msg(buf, (uint8_t*)"a/b", 3, 1, 32767, (uint8_t*)"payload", 8);
//nbytes = mqtt_encode_publish_msg(buf, (uint8_t*)"a/b", 3, 1, 10, 0, 0);
  printf("pub: ");
  for (i = 0; i < nbytes; ++i)
  {
    if (buf[i] >= 'A' && buf[i] <= 'z')
      printf("%c ", buf[i]);
    else
      printf("0x%.02x ", buf[i]);
  }
  printf("\n");

  {
    uint16_t msg_id, topic_len;
    uint8_t qos;
    uint8_t* topic;
    uint8_t* payload;
    int rc = mqtt_decode_publish_msg(buf, nbytes, &qos, &msg_id, &topic_len, &topic, &payload);
    printf("nbytes = %u\n", nbytes);
    printf("decode pub msg = %d:\n", rc);
    printf("  qos       = %u\n", qos);
    printf("  msg id    = %u\n", msg_id);
    printf("  topic-len = %u\n", topic_len);
    printf("  topic     = '%s'\n", topic);
    printf("  payload   = '%s'\n", payload);
  }

  nbytes = mqtt_encode_subscribe_msg(buf, (uint8_t*)"a/b", 3, 1, 32767);
  printf("sub: ");
  for (i = 0; i < nbytes; ++i)
    printf("0x%.02x ", buf[i]);
  printf("\n");

  uint8_t* tpcs[] = { (uint8_t*)"a/b" };
  uint16_t tpclens[] = { 3 };
  uint8_t  tpcqos[] = { 1 };
  nbytes = mqtt_encode_subscribe_msg2(buf, tpcs, tpclens, tpcqos, 1, 32767);
  printf("sub: ");
  for (i = 0; i < nbytes; ++i)
    printf("0x%.02x ", buf[i]);
  printf("\n");

  nbytes = mqtt_encode_unsubscribe_msg(buf, (uint8_t*)"a/b", 3, 1, 32767);
  printf("unsub: ");
  for (i = 0; i < nbytes; ++i)
    printf("0x%.02x ", buf[i]);
  printf("\n");

  nbytes = mqtt_encode_unsubscribe_msg2(buf, tpcs, tpclens, tpcqos, 1, 32767);
  printf("unsub: ");
  for (i = 0; i < nbytes; ++i)
    printf("0x%.02x ", buf[i]);
  printf("\n");



  uint8_t au8connack_msg[] = { 0x20, 0x02, 0x00, 0x00 };
  printf("connack(msg) = %d \n", mqtt_decode_connack_msg(au8connack_msg, sizeof(au8connack_msg)));

  uint8_t au8pingresp_msg[] = { 0xd0, 0x00 };
  printf("pingresp(msg) = %d \n", mqtt_decode_pingresp_msg(au8pingresp_msg, sizeof(au8pingresp_msg)));

  uint16_t u16msg_id;
  uint8_t au8puback_msg[] = { 0x40, 0x02, 0x7f, 0xff };
  printf("puback(msg) = %d \n", mqtt_decode_puback_msg(au8puback_msg, sizeof(au8puback_msg), &u16msg_id));



  return 0;
}

#endif

