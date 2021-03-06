/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interface/vchi/vchi.h"

#include "interface/vmcs_host/vcgencmd.h"
#include "host_applications/framework/vmcs/host_msgfifo_wrapper.h"

#include "vcfw/drivers/chip/mphi.h"


extern const MPHI_DRIVER_T *mphi_get_func_table( );
extern void vcih_task_func(unsigned argc, void *argv);




main(int argc, char *argv[])
{
   int result, success, iterations;
   
   printf("Testing VCHI comms stack \n");
   printf("========================\n"); 
   
   if (argc > 1)
   {
      iterations = atoi(argv[1]);
   }
   else
      iterations = 1;
   
 
   // initialise the MPHI stack. This will need to move in to the 
   // main init code somewhere - shouldn't have to do it here.
   // On the VC side, drivers are automatically initialised, which is why
   // init sequence doesnt do it
   MPHI_DRIVER_T *mphi_driver = mphi_get_func_table();
   mphi_driver->init();
   
   os_logging_message("Press any key to start system\n");
   getchar();
   
   
   VCHI_INSTANCE_T initialise_instance;
   VCHI_CONNECTION_T *connection[1];

   // initialise the vchi layer
   
   success = vchi_initialise( &initialise_instance);
   os_assert( success == 0 );

   connection[0] = vchi_create_connection( single_get_func_table(),
                                           vchi_mphi_message_driver_func_table() );
   
   host_vchi_msgfifo_wrapper_init( initialise_instance, &connection[0], 1 );

   // now turn on the control service and initialise the connection
   success = vchi_connect( &connection[0], 1, initialise_instance );
   os_assert( success == 0 );


   os_logging_message("\n\n\ncalling vc_interface_init\n");      

   //Initialise interfaces
   if (vc_interface_init()!=0) // returns 0 for success, 1 for failure
         os_logging_message("vc_interface_init failed\n");      
      
   os_logging_message("\nStarting vcih_task thread...\n");      

   VCOS_THREAD_T ti;

   // Start up the interrupt monitor thread
   // This will eventually be done automatically in one of the other init calls.
   os_thread_start(&ti, vcih_task_func, NULL, 2048, NULL);

   os_logging_message("Calling gencmd init\n\n\n");      

   if (vc_gencmd_init() < 0)    // returns non-negative for success, -1 for failure
         os_logging_message("gencmd init failed\n");      

   os_logging_message("Calling vc_gencmd x %d\n", iterations);

   uint32_t start = os_get_time_in_usecs();
   int i, ok=0;
   char rx[1024];

   for (i=0;i<iterations;i++)
   {
      char *tx = "version";
   
      if (vc_gencmd(rx, 1024, tx) == 0)
      {
         os_logging_message("%d..ok\n", i);
         ok++; 
      }
      else
      {
         os_logging_message("%d..fail\n");
      }
   }      
   
   os_logging_message("completed %d in %ldms\n%d were successfull\n", i, (os_get_time_in_usecs() - start)/1000, ok);

   os_logging_message("===========result============\n%s\n=============================\n\n", rx);
   
   os_delay(500);
}



