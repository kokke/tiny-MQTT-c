#include "client.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>


/* Helper functions: */
static void _dummy_connect  (client_t* s, int f, const char* b)  { (void) s; (void) f; (void) b; }
static void _dummy_recv_data(client_t* s, int f, char* d, int l) { (void) s; (void) f; (void) d; (void) l; }

static void _change_state(client_t* psClnt, conn_state_t new_state)
{
  require(psClnt != 0);

  psClnt->state = new_state;
  psClnt->last_active = time(0);
}



/*
   Implementation of exported interface begins here
*/

void client_init(client_t* psClnt, char* dst_addr, uint16_t dst_port, char* rxbuf, uint32_t rxbufsize)
{
  require(psClnt != 0);
  require(dst_addr != 0);
  require(rxbuf != 0);

  strcpy(psClnt->addr, dst_addr);
  psClnt->port = dst_port;
  psClnt->sockfd = 0;
  psClnt->rxbuf   = rxbuf;
  psClnt->rxbufsz = rxbufsize;
  psClnt->client_connected    = (void*)_dummy_connect;
  psClnt->client_disconnected = (void*)_dummy_connect;
  psClnt->client_new_data     = (void*)_dummy_recv_data;

  /*
     Set the socket I/O mode: In this case FIONBIO  
     enables or disables the blocking mode for the   
     socket based on the numerical value of iMode.  
     If iMode = 0, blocking is enabled;   
     If iMode != 0, non-blocking mode is enabled.
  */
  int iMode = 0;
  ioctl(psClnt->sockfd, FIONBIO, &iMode);

  /*
  // WINDOWS
  DWORD timeout = SOCKET_READ_TIMEOUT_SEC * 1000;
  setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
  */

  _change_state(psClnt, CREATED);

  /* Ignore SIGPIPE - "broken pipe" / disconnected socket */
  signal(SIGPIPE, SIG_IGN);
}

int client_set_callback(client_t* psClnt, cb_type eTyp, void* funcptr)
{
  require(psClnt != 0);
  require(funcptr != 0);

  int success = 1;

  switch(eTyp)
  {
    case CB_ON_CONNECTION:    psClnt->client_connected    = funcptr;    break;
    case CB_ON_DISCONNECT:    psClnt->client_disconnected = funcptr;    break;
    case CB_RECEIVED_DATA:    psClnt->client_new_data     = funcptr;    break;
    default:                  success = 0; /* unknown callback-type */  break;
  }

  return success;
}

int client_send(client_t* psClnt, char* data, uint32_t nbytes)
{
  require(psClnt != 0);
  require(data != 0);

  psClnt->last_active = time(0);
  printf("CLNT%u: sending %u bytes.\n", psClnt->sockfd, nbytes);

  int success = send(psClnt->sockfd, data, nbytes, 0);

  if (success < 0)
  {
    perror("send");
    client_disconnect(psClnt);
  }

  return success;
}

int client_recv(client_t* psClnt, uint32_t timeout_us)
{
  require(psClnt != 0);

  struct timeval tv;
  tv.tv_sec = 0;
  while (timeout_us >= 1000000)
  {
    timeout_us -= 1000000;
    tv.tv_sec += 1;
  }
  tv.tv_usec = timeout_us; /* Not init'ing this can cause strange errors */
  setsockopt(psClnt->sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

  int nbytes = recv(psClnt->sockfd, psClnt->rxbuf, psClnt->rxbufsz, 0);
  if (nbytes <= 0)
  {
    /* got error or connection closed by server? */
    if (    (nbytes == -1)           /* nothing for us  */
         || (nbytes == EAGAIN)       /* try again later */
         || (nbytes == EWOULDBLOCK)) /* same as above   */
    {
      /* do nothing */
    }
    else
    {
      /* connection lost / reset by peer */
      perror("recv");
      client_disconnect(psClnt);
    }
  }
  else
  {
    psClnt->client_new_data(psClnt, psClnt->rxbuf, nbytes);
    psClnt->last_active = time(0);
  }
  return nbytes;
}

int client_state(client_t* psClnt)
{
  return ((psClnt != 0) ? psClnt->state : 0);
}

void client_poll(client_t* psClnt, uint32_t timeout_us)
{
  /* 10usec timeout is a reasonable minimum I think */
  timeout_us = ((timeout_us >= 10) ? timeout_us : 10);

  require(psClnt != 0);

  switch (psClnt->state)
  {
    case CREATED:
    {
      _change_state(psClnt, DISCONNECTED);
    } break;

    case CONNECTED:
    {
      client_recv(psClnt, timeout_us);
/*
      static time_t timeLastMsg = 0;
      if ((time(0) - timeLastMsg) > 0)
      {
        timeLastMsg = time(0);
        client_send(psClnt, "hej mor!", 8);
      }
*/
    } break;

    case DISCONNECTED:
    {
      /* ~1 second cooldown */
      if (time(0) > psClnt->last_active)
      {
        client_connect(psClnt);
      }
    } break;
  }

}


void client_disconnect(client_t* psClnt)
{
  require(psClnt != 0);

  shutdown(psClnt->sockfd, 2);
  close(psClnt->sockfd);

  _change_state(psClnt, DISCONNECTED);
  psClnt->client_disconnected(psClnt);
}

int client_connect(client_t* psClnt)
{
  require(psClnt != 0);

  int success = 0;

  struct sockaddr_in serv_addr;
  struct hostent* server;

  psClnt->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (psClnt->sockfd < 0)
  {
    perror("socket");
  }
  else
  {
    server = gethostbyname(psClnt->addr);
    if (server == NULL)
    {
      fprintf(stderr,"ERROR, no such host\n");
    }
    else
    {
      memset(&serv_addr, 0, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
      serv_addr.sin_port = htons(psClnt->port);

      if (connect(psClnt->sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
      {
        perror("connect");
        /* Clean up by calling close() on socket before allocating a new socket. */
        client_disconnect(psClnt);
      }
      else
      {
        _change_state(psClnt, CONNECTED);
        psClnt->client_connected(psClnt);
        success = 1;
      }
    }
  }

  return success;
}

