/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

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

// Set the name of the node and the mode of the node
const char name[]="UDP server";
const bool server = true;

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
-------------------------------------------- UDP Server --------------------------------------------
--------------------------------------------------------------------------------------------------*/

static struct simple_udp_connection udp_conn;

PROCESS(udp_server_process, name);
AUTOSTART_PROCESSES(&udp_server_process);

static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  LOG_INFO("%s: Received request '%.*s' from ", name, datalen, (char *) data);
//  LOG_INFO("%s: Received key '%s'", name, (char *));
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
#if WITH_SERVER_REPLY
  /* send back the same string to the client as an echo reply */
  LOG_INFO("Sending response from the %s.\n",name);
  simple_udp_sendto(&udp_conn, data, datalen, sender_addr);
#endif /* WITH_SERVER_REPLY */
}

/*--------------------------------------------------------------------------------------------------
---------------------------------- Main process of the root node -----------------------------------
--------------------------------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data){

  PROCESS_BEGIN();

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  // Print the functionality of the process
  LOG_INFO("The mode of the node is set to: '%s'\n", name);

  /* Produce the PUF key */
  uint16_t local_key = generate_key();
  LOG_INFO_("test %u\n", local_key);

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, local_key, udp_rx_callback);

  LOG_INFO("test print mode'%s'\n", name);



  PROCESS_END();
}
/*------------------------------------------------------------------------------------------------*/

