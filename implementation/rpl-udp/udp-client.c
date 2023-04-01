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


#define LOG_MODULE "App"
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
uint16_t generate_key() {
  srand(time(NULL)); // Seed the random number generator with the current time
  uint16_t local_key = rand() % 65536; // Generate a random number between 0 and 65535 (inclusive)
  return local_key;
}

bool validate = false;
/*------------------------------------------------------------------------------------------------*/

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
         uint16_t testkey,
         const uint8_t *data,
         uint16_t datalen)
{
  LOG_INFO("%s: Received request '%.*s' from ", name, datalen, (char *) data);
 // LOG_INFO("%s: Received key '%s'", name, (char *) keyPub);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");
  rx_count++;
}
/*--------------------------------------------------------------------------------------------------
---------------------------------- Main process of the sync node -----------------------------------
--------------------------------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static char str[32];
  uip_ipaddr_t dest_ipaddr;
  static uint32_t tx_count;
  static uint32_t missed_tx_count;

  // Print the functionality of the process
  LOG_INFO("The mode of the node is set to: '%s'\n", name);

  PROCESS_BEGIN();

  /* Produce the PUF key */
  uint16_t local_key = generate_key();
  LOG_INFO_("test %u\n", local_key);

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, local_key, udp_rx_callback);

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
      LOG_INFO("Sending request %"PRIu32" to ", tx_count);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      snprintf(str, sizeof(str), "hello %" PRIu32 "", tx_count);
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
