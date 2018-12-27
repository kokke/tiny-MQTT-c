#include "client.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "mqtt.h"

int keepalive_sec = 4;
client_t c;
char buffer[BUFFER_SIZE_BYTES];
uint8_t buf[128];
int nbytes;
int is_subscriber = 1;

static void got_connection(client_t* psClnt)
{
  require(psClnt != 0);

  printf("CLNT%d: connected to server.\n", psClnt->sockfd);

  if (is_subscriber)
    nbytes = mqtt_encode_connect_msg2(buf, 0x02, keepalive_sec, (uint8_t*)"DIGI", 4);
  else
    nbytes = mqtt_encode_connect_msg2(buf, 0x02, keepalive_sec, (uint8_t*)"DOGO", 4);

  client_send(&c, (char*)buf, nbytes);
}

static void lost_connection(client_t* psClnt)
{
  require(psClnt != 0);

  printf("CLNT%d: disconnected from server.\n", psClnt->sockfd);
}

static void got_data(client_t* psClnt, char* data, int nbytes)
{
  require(psClnt != 0);

  printf("CLNT%d: got %d bytes, '", psClnt->sockfd, nbytes);
  int i;
  for (i = 0; i < nbytes; ++i)
  {
    if (data[i] == '\n')
    {
      printf("\\n");
    }
    else
    {
      printf("%c", data[i]);
    }
  }
  printf("'\n");
}

void inthandler(int dummy)
{
  (void)dummy;

  int nbytes = mqtt_encode_disconnect_msg(buf);
  if (nbytes != 0)
  {
    client_send(&c, (char*)buf, nbytes);
  }

  client_poll(&c, 10000000);

  exit(0);
}




int main(int argc, char* argv[])
{
  (void) argv;
  is_subscriber = (argc == 1);
  printf("client, %s\n", (is_subscriber ? "subscriber" : "publisher"));

  client_init(&c, "test.mosquitto.org", 1883, buffer, BUFFER_SIZE_BYTES);
  //client_init(&c, "mqtt.fluux.io", 1883, buffer, BUFFER_SIZE_BYTES);

  assert(client_set_callback(&c, CB_RECEIVED_DATA, got_data)        == 1);
  assert(client_set_callback(&c, CB_ON_CONNECTION, got_connection)  == 1);
  assert(client_set_callback(&c, CB_ON_DISCONNECT, lost_connection) == 1);

  signal(SIGINT, inthandler);

  const time_t ping_interval = (keepalive_sec - 1);
  int subscribed = 0;
  time_t next_pub = time(0) + 10;
  time_t next_ping = time(0) + ping_interval;

  char buf2[1024];
  int nbytes;
  while (1)
  {
    client_poll(&c, 0);

    if (is_subscriber && !subscribed && ((time(0) - c.last_active) >= 2))
    {
//g(uint8_t* pu8dst, uint8_t* pu8topic, uint16_t u16topic_len, uint8_t u8qos, uint16_t u16msg_i
      nbytes = mqtt_encode_subscribe_msg((uint8_t*)buf2, (uint8_t*)"a/b", 3, 1, 12345);
      client_send(&c, buf2, nbytes);
      subscribed = 1;
      client_poll(&c, 0);
    }

    if ((time(0) ) >= next_ping)
    {
      nbytes = mqtt_encode_ping_msg(buf);
      if (nbytes != 0)
      {
        printf("ping!\n");
        client_send(&c, (char*)buf, nbytes);
        client_poll(&c, 0);
        next_ping = time(0) + ping_interval;
      }
    }

    if ((time(0) > next_pub) && !is_subscriber)
    {
//int mqtt_encode_publish_msg(uint8_t* pu8dst, uint8_t* pu8topic, uint16_t u16topic_len, uint8_t u8qos, uint16_t u16msg_id, uint8_t* pu8payload, uint32_t u32data_len);
      nbytes = mqtt_encode_publish_msg((uint8_t*)buf2, (uint8_t*)"a/b", 3, 1, 10, (uint8_t*)"hi mom!", 7);
      client_send(&c, buf2, nbytes);
      next_pub = time(0) + 10;
      client_poll(&c, 1000000);
    }

    usleep(1000);
  }

  return 0;
}


