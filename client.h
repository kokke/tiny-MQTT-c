#include <stdint.h>
#include <time.h>

#define NCONNECTIONS               1
#define BUFFER_SIZE_BYTES          1024

/* Assertion macro */
#define require(predicate)         assert((predicate))


/* Socket connection state */
typedef enum
{
  CREATED,
  CONNECTED,
  DISCONNECTED,
} conn_state_t;

/* Type definitions */
typedef enum
{
  CB_ON_CONNECTION,
  CB_ON_DISCONNECT,
  CB_RECEIVED_DATA,
} cb_type;

typedef struct
{
  int          sockfd;
  char*        rxbuf;
  uint32_t     rxbufsz;
  conn_state_t state;
  uint16_t     port;
  char         addr[32];
  time_t       last_active;

  /* Callbacks */
  void (*client_connected)   (void* psClnt);    /* new connection established */
  void (*client_disconnected)(void* psClnt);    /* a connection was closed */
  void (*client_new_data)    (void* psClnt, char* data, int nbytes); /* data has been received */
} client_t;



void client_init(client_t* psClnt, char* dst_addr, uint16_t dst_port, char* rxbuf, uint32_t rxbufsize);
int  client_set_callback(client_t* psClnt, cb_type eTyp, void* funcptr);
int  client_send(client_t* psClnt, char* data, uint32_t nbytes);
int  client_recv(client_t* psClnt, uint32_t timeout_us);
void client_poll(client_t* psClnt, uint32_t timeout_us);
void client_disconnect(client_t* psClnt);
int  client_connect(client_t* psClnt);
int  client_state(client_t* psClnt);


