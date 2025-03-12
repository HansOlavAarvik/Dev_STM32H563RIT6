/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_netxduo.c
  * @author  MCD Application Team
  * @brief   NetXDuo applicative file
  ******************************************************************************
    * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_netxduo.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include <stdio.h>
#include <stdbool.h> 
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TX_THREAD      NxAppThread;
NX_PACKET_POOL NxAppPool;
NX_IP          NetXDuoEthIpInstance;
/* USER CODE BEGIN PV */
NX_UDP_SOCKET UDPSocket;
ULONG IpAddress;
ULONG NetMask;
//TX_THREAD AudioDataThread;
TX_THREAD AppLinkThread;

extern ETH_HandleTypeDef heth;


// // For audio
// TX_EVENT_FLAGS_GROUP audio_events;
// static uint16_t DMA_size = AUDIO_BUFFER_SIZE;
// extern I2S_HandleTypeDef hi2s2;
// volatile int16_t data_i2s[AUDIO_BUFFER_SIZE];
// extern volatile uint8_t half;
// int16_t processed_audio[AUDIO_BUFFER_SIZE];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static VOID nx_UDP_thread_entry (ULONG thread_input);
/* USER CODE BEGIN PFP */
void blink_blue(void);
static VOID App_Link_Thread_Entry(ULONG thread_input);
UINT MX_NetXDuo_Init(VOID *memory_ptr);
UINT UDP_Send(void* data_ptr, UINT data_size, ULONG destination_ip, UINT destination_port);

// // for audio
// static VOID audio_thread_entry(ULONG thread_input);
// int16_t process_i2s_data(volatile int16_t* source, int16_t* dest, uint16_t size);

/* USER CODE END PFP */

/**
  * @brief  Application NetXDuo Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_NetXDuo_Init(VOID *memory_ptr)
{
  UINT ret = NX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

   /* USER CODE BEGIN App_NetXDuo_MEM_POOL */
  (void)byte_pool;
  /* USER CODE END App_NetXDuo_MEM_POOL */
  /* USER CODE BEGIN 0 */
  /* USER CODE END 0 */

  /* Initialize the NetXDuo system. */
  CHAR *pointer;
  nx_system_initialize();

    /* Allocate the memory for packet_pool.  */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_PACKET_POOL_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the Packet pool to be used for packet allocation,
   * If extra NX_PACKET are to be used the NX_APP_PACKET_POOL_SIZE should be increased
   */
  ret = nx_packet_pool_create(&NxAppPool, "NetXDuo App Pool", DEFAULT_PAYLOAD_SIZE, pointer, NX_APP_PACKET_POOL_SIZE);

  if (ret != NX_SUCCESS)
  {
    return NX_POOL_ERROR;
  }

    /* Allocate the memory for Ip_Instance */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, Nx_IP_INSTANCE_THREAD_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

   /* Create the main NX_IP instance */
  ret = nx_ip_create(&NetXDuoEthIpInstance, "NetX Ip instance", NX_APP_DEFAULT_IP_ADDRESS, NX_APP_DEFAULT_NET_MASK, &NxAppPool, nx_stm32_eth_driver,
                     pointer, Nx_IP_INSTANCE_THREAD_SIZE, NX_APP_INSTANCE_PRIORITY);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

    /* Allocate the memory for ARP */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Enable the ARP protocol and provide the ARP cache size for the IP instance */

  /* USER CODE BEGIN ARP_Protocol_Initialization */
  /* USER CODE END ARP_Protocol_Initialization */

  ret = nx_arp_enable(&NetXDuoEthIpInstance, (VOID *)pointer, DEFAULT_ARP_CACHE_SIZE);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable the ICMP */

  /* USER CODE BEGIN ICMP_Protocol_Initialization */

  /* USER CODE END ICMP_Protocol_Initialization */

  ret = nx_icmp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable TCP Protocol */

  /* USER CODE BEGIN TCP_Protocol_Initialization */

  /* USER CODE END TCP_Protocol_Initialization */

  ret = nx_tcp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable the UDP protocol required for  DHCP communication */

  /* USER CODE BEGIN UDP_Protocol_Initialization */

  /* USER CODE END UDP_Protocol_Initialization */

  ret = nx_udp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

   /* Allocate the memory for main thread   */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the main thread */
  ret = tx_thread_create(&NxAppThread, "NetXDuo App thread", nx_UDP_thread_entry , 0, pointer, NX_APP_THREAD_STACK_SIZE,
                         NX_APP_THREAD_PRIORITY, NX_APP_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

  if (ret != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

  /* USER CODE BEGIN MX_NetXDuo_Init */

  //// for audio
  // if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  // {
  //   return TX_POOL_ERROR;
  // }

  // /* Create the main thread */
  // ret = tx_thread_create(&AudioDataThread, "Audio thread", audio_thread_entry , 0, pointer, NX_APP_THREAD_STACK_SIZE,
  //                        NX_APP_THREAD_PRIORITY, NX_APP_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_DONT_START);

  // if (ret != TX_SUCCESS)
  // {
  //   return TX_THREAD_ERROR;
  // }
//   ret = tx_thread_create(&AudioDataThread, "Audio thread", audio_thread_entry , 0, pointer, NX_APP_THREAD_STACK_SIZE,
//     NX_APP_THREAD_PRIORITY, NX_APP_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_DONT_START);

// if (ret != TX_SUCCESS)
// {
// return TX_THREAD_ERROR;
// }
// // event flag
// ret = tx_event_flags_create(&audio_events, "Audio Events");
// if (ret != TX_SUCCESS)
// {
// return TX_GROUP_ERROR;
// }
// if (tx_byte_allocate(byte_pool, (VOID **) &pointer,2 *  DEFAULT_MEMORY_SIZE, TX_NO_WAIT) != TX_SUCCESS)
// {
// return TX_POOL_ERROR;
// }

  /* USER CODE END MX_NetXDuo_Init */

  return ret;
}

/**
* @brief  Main thread entry.
* @param thread_input: ULONG user argument used by the thread entry
* @retval none
*/
static VOID nx_UDP_thread_entry (ULONG thread_input)
{
  /* USER CODE BEGIN Nx_App_Thread_Entry 0 */
  (void)thread_input;
  UINT ret;
  tx_thread_sleep(300);
  // printf("Starting UDP server after initialization delay\r\n");
  // printf("Current MAC: %02X-%02X-%02X-%02X-%02X-%02X\r\n", 
  //   heth.Init.MACAddr[0], heth.Init.MACAddr[1], heth.Init.MACAddr[2],
  //   heth.Init.MACAddr[3], heth.Init.MACAddr[4], heth.Init.MACAddr[5]);
 
  // ret = nx_ip_address_get(&NetXDuoEthIpInstance, &IpAddress, &NetMask);
  // if (ret != TX_SUCCESS)
  // {
  //     //printf("Failed to get IP address, error: %d\r\n", ret);
  //     Error_Handler();
  // }
  // else
  // {
  //     //printf("IP Configuration:\r\n");
  //     PRINT_IP_ADDRESS(IpAddress);
  //     printf("Subnet mask: %lu.%lu.%lu.%lu\r\n", 
  //            (NetMask >> 24) & 0xFF, (NetMask >> 16) & 0xFF,
  //            (NetMask >> 8) & 0xFF, NetMask & 0xFF);
  // }
  ret = nx_udp_socket_create(&NetXDuoEthIpInstance, &UDPSocket,
                             "UDP Audio Socket", NX_IP_NORMAL,
                              NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, QUEUE_MAX_SIZE);
  if (ret != NX_SUCCESS)
  {
      Error_Handler();
      //printf("Socket not created.\r\n");
  }
  ret = nx_udp_socket_bind(&UDPSocket, AUDIO_PORT, TX_WAIT_FOREVER);
  if (ret != NX_SUCCESS)
  {
    //printf("Socket bind failed with error: %d\r\n", ret);
    Error_Handler();
  }
  // else
  // {
  //     printf("Data will be sent on PORT %d\r\n", AUDIO_PORT);
  // }
  //tx_thread_resume(&AudioDataThread);


  /* USER CODE END Nx_App_Thread_Entry 0 */

}
/* USER CODE BEGIN 1 */
static VOID App_Link_Thread_Entry(ULONG thread_input)
{ 
  (void)thread_input;   
  ULONG actual_status;
  UINT linkdown = 0, status;

  while(1)
  {
    /* Get Physical Link status. */
    status = nx_ip_interface_status_check(&NetXDuoEthIpInstance, 0, NX_IP_LINK_ENABLED,
                                      &actual_status, 10);

    if(status == NX_SUCCESS)
    {
      if(linkdown == 1)
      {
        linkdown = 0;
        status = nx_ip_interface_status_check(&NetXDuoEthIpInstance, 0, NX_IP_ADDRESS_RESOLVED,
                                      &actual_status, 10);
        if(status == NX_SUCCESS)
        {
          
          //printf("The network cable is connected again.\n");
       }
        else
        {
          //printf("The network cable is connected.\n");
        }
        nx_ip_driver_direct_command(&NetXDuoEthIpInstance, NX_LINK_ENABLE,
                                    &actual_status);
        
        tx_thread_resume(&NxAppThread);
        blink_blue();
      }
    }
    else
    {
      if(0 == linkdown)
      {
        linkdown = 1;
        /* The network cable is not connected. */
        //printf("The network cable is not connected.\n");
        //tx_thread_suspend(&AudioDataThread);
        tx_thread_suspend(&NxAppThread);
      }
    }
    blink_blue();
    tx_thread_sleep(100);
  }
}
UINT UDP_Send(void* data_ptr, UINT data_size, ULONG destination_ip, UINT destination_port)
{
  UINT ret;
  NX_PACKET *nx_packet_ptr;
  if (data_ptr == NULL || data_size == 0)
  {
      return NX_INVALID_PARAMETERS;
  } 
  ret = nx_packet_allocate(&NxAppPool, &nx_packet_ptr, NX_UDP_PACKET, TX_WAIT_FOREVER);
  if (ret != NX_SUCCESS)
  {
      //printf("Packet allocation failed: %d\r\n", ret);
      return ret;
  }  
  ret = nx_packet_data_append(nx_packet_ptr, data_ptr, data_size, 
                             &NxAppPool, TX_WAIT_FOREVER);
  if (ret != NX_SUCCESS)
  {
      //printf("Data append failed: %d\r\n", ret);
      nx_packet_release(nx_packet_ptr);
      return ret;
  }  
  ret = nx_udp_socket_send(&UDPSocket, nx_packet_ptr, destination_ip, destination_port);
  if (ret != NX_SUCCESS)
  {
      //printf("UDP send failed: %d\r\n", ret);
      nx_packet_release(nx_packet_ptr);
      return ret;
  }
  
  return NX_SUCCESS;
}

void blink_blue(void){
  HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin);
  tx_thread_sleep(20);
  HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port,LED_BLUE_Pin);
  tx_thread_sleep(20);
  return;
}
/* USER CODE END 1 */
