#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdint.h>
#include <inttypes.h>

#include "sys/log.h"

// import of boolean logic
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
//-- import randomness
#include <fcntl.h>
#include <unistd.h>
//--
#define LOG_MODULE "Client"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		  (60 * CLOCK_SECOND)

// Set the name of the node and the mode of the node
const char name[]="UDP Client";
const bool server = false;

/*--------------------------------------------------------------------------------------------------
--------------------------------------- Define the PUF Key -----------------------------------------
--------------------------------------------------------------------------------------------------*/
char local_client_key[20] = "initialkey";
bool initialSetupPUF=true;
bool validate = false;
/*------------------------------------------------------------------------------------------------*/
#define MAX_NODES 10

char remotekeys[MAX_NODES][20];
uint16_t sender_ports[MAX_NODES];
uip_ipaddr_t sender_addrs[MAX_NODES];
const uip_ipaddr_t uip_all_zeroes_addr;
/*--------------------------------------------------------------------------------------------------
-------------------------------------------- UDP Client --------------------------------------------
--------------------------------------------------------------------------------------------------*/

static struct simple_udp_connection udp_conn;
static uint32_t rx_count = 0;

PROCESS(udp_client_process, name);
AUTOSTART_PROCESSES(&udp_client_process);

static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{

    char data_copy[121];
    memcpy(data_copy, data, datalen);
    data_copy[121] = '\0';
    char *token = strtok(data_copy, " ");
    char remotekey[20];
    if (token != NULL) {
      strcpy(remotekey, token);
    }
    char *taken = strtok(NULL, " ");  // Get the next token
//    char second_field[20];
          LOG_INFO("Received message '%s'\n",taken);
    if (taken != NULL && strcmp(taken, "validate") == 0) {
      LOG_INFO("Received validation message\n");
      validate=true;
    }
   /*verify if the key of the node is the expected or not*/
  int i;
  for (i = 0; i < MAX_NODES; i++) {
    uip_ipaddr_t* ip_ptr = &sender_addrs[i];
    if (sender_ports[i] == sender_port && memcmp(ip_ptr, sender_addr, sizeof(uip_ip6addr_t)) == 0){
      if (strcmp(remotekeys[i], remotekey) == 0){
        LOG_INFO("The key '%s' of the node with Port:'%u' ",remotekey,sender_port);
        LOG_INFO_("IP: '");
        LOG_INFO_6ADDR(sender_addr);
        LOG_INFO_("' is verified.\n");
        break;
      } else{
        LOG_INFO_("The key '%s' of the node with Port:'%u' ",remotekey,sender_port);
        LOG_INFO_("IP: '");
        LOG_INFO_6ADDR(sender_addr);
        LOG_INFO_("' is not verified closing the communication with this node.\n");
        break;
        // THIS CAN REMOVE CONNECTIONS uip_udp_remove or uip_close
      }
  }else{
    // Find an empty cell in the arrays and store the values
    if (sender_ports[i] == 0 && uip_ipaddr_cmp(&sender_addrs[i], &uip_all_zeroes_addr)) {
      strcpy(remotekeys[i], token);
      sender_ports[i] = sender_port;
      uip_ipaddr_copy(&sender_addrs[i], sender_addr);
      LOG_INFO_("The mote with:key '%s' ,Port:'%u'  \n",remotekey,sender_port);
      LOG_INFO_(",IP: '");
      LOG_INFO_6ADDR(sender_addr);
      LOG_INFO_("' was added to the list of known mote.\n");
      break;
     }
    }
  }

  if(validate){
    // Keep the same key
     LOG_INFO("The key remains for the client '%s' the same\n",local_client_key);
     validate=false;
  }

  // Print the request received and the details of the sender
  LOG_INFO("%s: Received request '%.*s' from mote with: Port:'%u' key:'%s' ", name, datalen, (char*) data,sender_port, remotekey);
  LOG_INFO_("IP: '");
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("'\n");


#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");
  rx_count++;
}
/*--------------------------------------------------------------------------------------------------
---------------------------------- Main process of the client node -----------------------------------
--------------------------------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static char str[120];
  uip_ipaddr_t dest_ipaddr;
  static uint32_t tx_count;
  static uint32_t missed_tx_count;

  // Print the functionality of the process
  LOG_INFO("The mode of the node is set to: '%s'\n", name);

  PROCESS_BEGIN();

  /* Produce the PUF key */
  if(initialSetupPUF){
    int urandom_fd = open("/dev/urandom", O_RDONLY);
    unsigned int seed_value;
    read(urandom_fd, &seed_value, sizeof(seed_value));
    close(urandom_fd);
    srand(seed_value);
    for(int i = 0; i < 10; i++) {
      local_client_key[i] = rand() % 26 + 'a';
    }
    local_client_key[10] = '\0'; // terminate the string
    LOG_INFO("The PUF key of the client is: %s\n", local_client_key);
    initialSetupPUF=false;
  }

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if(NETSTACK_ROUTING.node_is_reachable() &&
        NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {

      /* Print statistics every 10th TX */
      if(tx_count % 10 == 0) {
        LOG_INFO("Tx/Rx/MissedTx: %" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
                 tx_count, rx_count, missed_tx_count);
      }

      /* Send to DAG root */
      LOG_INFO("Sending request %"PRIu32" with key: %s to ", tx_count, local_client_key);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      snprintf(str, sizeof(str), "%s hello %" PRIu32 "", local_client_key, tx_count);
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
      tx_count++;
    } else {
      LOG_INFO("Not reachable yet\n");
      if(tx_count > 0) {
        missed_tx_count++;
      }
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*------------------------------------------------------------------------------------------------*/
