/*  This file includes the functions used in for the PUF actions */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#undef LOG_MODULE
#define LOG_MODULE "PUF key actions"

// libraries for DH
#include <math.h>

/*--------------------------------------------------------------------------------------------------
------------------------------------- Puf key extraction process -----------------------------------
--------------------------------------------------------------------------------------------------*/
static int PUFkeyGeneration(){

  // The PUFkey function generates a pseudo PUF key for the node

  /* The PUF key is based on a random generation, rather than generating an true PUF key.
     The reason behind this decision was taken because the need of the implementation is to embeded
     the puf in the RPL protocol, instead of focusing on the PUF key generation
  */

  // Seed the random number generator
  srand(time(NULL));

  // Generate the PUF key
  for (int i = 0; i < KEY_LENGTH; i++) {
      puf_key[i] = rand() % 256;
  }

  // Save the PUF key to a single variable named **pufkey**
  for (int i = 0; i < KEY_LENGTH; i++) {
      sprintf(&pufkey[i * 2], "%02x", puf_key[i]);
  }

  // the variable **pufkey** has the PUF key of the node.
  pufkey[KEY_LENGTH * 2] = '\0';

  // Print the PUF key
  LOG_INFO("The PUF key for: %s is: %s\n", name, pufkey);

  return 0;
}

/*--------------------------------------------------------------------------------------------------
------------------------------------- PUF key server validation process ----------------------------
--------------------------------------------------------------------------------------------------*/
static int ServerValidation(){

// key

   if ( server == true ){
      LOG_INFO("The mode is server, running\n");
      return 0;
   }
   else{
      LOG_INFO("mode for others\n");
      return 0;
   }

}

/*--------------------------------------------------------------------------------------------------
------------------------------------- PUF key client validation process ----------------------------
--------------------------------------------------------------------------------------------------*/
static int ClientValidation(){

// key
   if ( server == true ){
      LOG_INFO("The mode is server, skip execution\n");
      return 0;
   }
   else{
      LOG_INFO("The PUF key validation client\n");
      return 0;
   }


}











// Restore the LOG_MODULE back to the original value
#undef LOG_MODULE
#define LOG_MODULE "App"