/*
W5500_Networking.h

Enables use of W5500 networking and ethernet control to your Remora board. This directory provides slightly modified versions of LWIP and Wiznet W5500 libraries. Credit for these libs are found in the folders as is. 

To include these in your PlatformIO.ini project: 
1) Ensure that your lib file is discoverable in the platformio.ini file
    lib_deps = 
        W5500_Networking=file://Src/remora-core/drivers/W5500_Networking/W5500_Networking.zip ; notes that lib_extra_dirs was deprecated in PIO 6+

2) Add to your Platformio.ini file a reference to the linker script. You may need to build an LS for your target. You can also set a new size for flash memory now that this is been allocated, recalculate this and enter the value for your specific target build
    board_build.ldscript = LinkerScripts/STM32F446XX_ETHCOMMS_BL.ld ; linker script with JSON memory allocated for F4xx
    board_upload.maximum_size = 458752   ; For example this is 512K - (16K * 4). 2x 16KB sectors for bootloader, and 2x JSON config sectors - one for upload and the other for storage.  

3) add the extra build flag to denote that you want ethernet control for your build:
    build_flags = 
        -D ETH_CTRL=1
        -D _WIZCHIP_=5500
        -D WIZ_RST="\"PB_5"\" ; replace these with the GPIO and SPI peripheral pins of your board
        -D SPI_CS="\"PB_6"\"
        -D SPI_CLK="\"PA_5"\"
        -D SPI_MISO="\"PA_6"\"
        -D SPI_MOSI="\"PA_7"\"

To use this with LinuxCNC, you will need the Ethernet component:
Compile the component using halcompile
```
sudo halcompile --install remora-eth-3.0.c
```

Configs are loaded via tftpy
```
pip3 install tftpy # If not using virtualenv you may get an error about breaking system packages, use the --break-system-packages flag if needed
python3 upload_config.py NucleoF411RE-Config.txt
```

Without these build flags, this header and the cpp file have been commented out to avoid creating errors when the compiler tries looking for the library files.
*/


#ifndef W5500_NETWORKING_H
#define W5500_NETWORKING_H

#include "remora-core/configuration.h"  

#ifdef ETH_CTRL

#include <stdio.h>
#include <string.h>
#include <memory>

#include "remora-core/comms/commsInterface.h"
#include "../../json/jsonConfigHandler.h"

#include "remora-hal/pin/pin.h"
#include "remora-hal/hal_utils.h"

#include "arch/cc.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "lwip/pbuf.h" 
#include "lwip/mem.h"
#include "lwip/udp.h"
#include "lwip/apps/lwiperf.h"
#include "lwip/etharp.h"
#include "socket.h"

//tftp defines
#define TFTP_OPCODE_LEN         2
#define TFTP_BLKNUM_LEN         2
#define TFTP_DATA_LEN_MAX       512
#define TFTP_DATA_PKT_HDR_LEN   (TFTP_OPCODE_LEN + TFTP_BLKNUM_LEN)
#define TFTP_ERR_PKT_HDR_LEN    (TFTP_OPCODE_LEN + TFTP_ERRCODE_LEN)
#define TFTP_ACK_PKT_LEN        (TFTP_OPCODE_LEN + TFTP_BLKNUM_LEN)
#define TFTP_DATA_PKT_LEN_MAX   (TFTP_DATA_PKT_HDR_LEN + TFTP_DATA_LEN_MAX)
#define TFTP_MAX_RETRIES        3
#define TFTP_TIMEOUT_INTERVAL   5

// Networking defines
#define ETHERNET_MTU 1500
#define SOCKET_MACRAW 0
#define PORT_LWIPERF 5001

#define RING_BUFFER_SIZE 8   // We used a ring buffer for lost packet detection. Can be 2, 4 or 8 or any multiple above.

// typedef struct {
//     uint8_t data[sizeof(rxData_t)];
//     uint16_t len;
// } packet_t;

namespace network 
{
    extern CommsInterface *ptr_eth_comms;
    
    extern Pin *ptr_csPin;
    extern Pin *ptr_rstPin;

    //extern volatile packet_t ring_buffer[RING_BUFFER_SIZE];
    extern volatile uint8_t head;
    extern volatile uint8_t tail;
    extern volatile uint8_t dropped_packets;   

    void EthernetInit(CommsInterface*, Pin*, Pin*);

    void udpServerInit();
    void EthernetTasks();
    void udp_data_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
    void network_initialize(wiz_NetInfo net_info);

    /*! \brief Print network information
    *  \ingroup w5x00_spi
    *
    *  Print network information about MAC address, IP address, Subnet mask, Gateway, DHCP and DNS address.
    *
    *  \param net_info network information.
    */
    void print_network_information(wiz_NetInfo net_info);
}

namespace lwip 
{
    extern struct netif g_netif;

    extern uint8_t mac[6];
    extern int8_t retval;
    extern uint8_t *pack;
    extern uint16_t pack_len;
    extern struct pbuf *p;    
    
    /**
     * ----------------------------------------------------------------------------------------------------
     * Functions
     * ----------------------------------------------------------------------------------------------------
     */
    /*! \brief send an ethernet packet
    *  \ingroup w5x00_lwip
    *
    *  It is used to send outgoing data to the socket.
    *
    *  \param sn socket number
    *  \param buf a pointer to the data to send
    *  \param len the length of data in packet
    *  \return he sent data size
    */
    int32_t send_lwip(uint8_t sn, uint8_t *buf, uint16_t len);

    /*! \brief read an ethernet packet
    *  \ingroup w5x00_lwip
    *
    *  It is used to read incoming data from the socket.
    *
    *  \param sn socket number
    *  \param buf a pointer buffer to read incoming data
    *  \param len the length of the data in the packet
    *  \return the real received data size
    */
    int32_t recv_lwip(uint8_t sn, uint8_t *buf, uint16_t len);

    /*! \brief callback function
    *  \ingroup w5x00_lwip
    *
    *  This function is called by ethernet_output() when it wants
    *  to send a packet on the interface. This function outputs
    *  the pbuf as-is on the link medium.
    *
    *  \param netif a pre-allocated netif structure
    *  \param p main packet buffer struct
    *  \return ERR_OK if data was sent.
    */
    err_t netif_output(struct netif *netif, struct pbuf *p);

    /*! \brief callback function
    *  \ingroup w5x00_lwip
    *
    *  Callback function for link.
    *
    *  \param netif a pre-allocated netif structure
    */
    void netif_link_callback(struct netif *netif);

    /*! \brief callback function
    *  \ingroup w5x00_lwip
    *
    *   Callback function for status.
    *
    *   \param netif a pre-allocated netif structure
    */
    void netif_status_callback(struct netif *netif);

    /*! \brief callback function
    *  \ingroup w5x00_lwip
    *
    *  Callback function that initializes the interface.
    *
    *  \param netif a pre-allocated netif structure
    *  \return ERR_OK if Network interface initialized
    */
    err_t netif_initialize(struct netif *netif);

    /*! \brief ethernet frame cyclic redundancy check
    *  \ingroup w5x00_lwip
    *
    *  Perform cyclic redundancy check on ethernet frame
    *
    *  \param data a pointer to the ethernet frame
    *  \param length the total length of ethernet frame
    *  \return an ethernet frame cyclic redundancy check result value
    */
    uint32_t ethernet_frame_crc(const uint8_t *data, int length);    
}

namespace wiznet 
{
    /**
     * ----------------------------------------------------------------------------------------------------
     * Functions
     * ----------------------------------------------------------------------------------------------------
     */
    /* W5x00 */
    /*! \brief Set CS pin
    *  \ingroup w5x00_spi
    *
    *  Set chip select pin of spi0 to low(Active low).
    *
    *  \param none
    */
    //void wizchip_select(void);

    /*! \brief Set CS pin
    *  \ingroup w5x00_spi
    *
    *  Set chip select pin of spi0 to high(Inactive high).
    *
    *  \param none
    */
    //void wizchip_deselect(void);

    /*! \brief Read from an SPI device, blocking
    *  \ingroup w5x00_spi
    *
    *  Set spi_read_blocking function.
    *  Read byte from SPI to rx_data buffer.
    *  Blocks until all data is transferred. No timeout, as SPI hardware always transfers at a known data rate.
    *
    *  \param none
    */
    //static uint8_t wizchip_read(void);

    /*! \brief Write to an SPI device, blocking
    *  \ingroup w5x00_spi
    *
    *  Set spi_write_blocking function.
    *  Write byte from tx_data buffer to SPI device.
    *  Blocks until all data is transferred. No timeout, as SPI hardware always transfers at a known data rate.
    *
    *  \param tx_data Buffer of data to write
    */
    //static void wizchip_write(uint8_t tx_data);

    /*! \brief Configure all DMA parameters and optionally start transfer
    *  \ingroup w5x00_spi
    *
    *  Configure all DMA parameters and read from DMA
    *
    *  \param pBuf Buffer of data to read
    *  \param len element count (each element is of size transfer_data_size)
    */
    // static void wizchip_read_burst(uint8_t *pBuf, uint16_t len);

    // /*! \brief Configure all DMA parameters and optionally start transfer
    // *  \ingroup w5x00_spi
    // *
    // *  Configure all DMA parameters and write to DMA
    // *
    // *  \param pBuf Buffer of data to write
    // *  \param len element count (each element is of size transfer_data_size)
    // */
    // static void wizchip_write_burst(uint8_t *pBuf, uint16_t len);

    // /*! \brief Enter a critical section
    // *  \ingroup w5x00_spi
    // *
    // *  Set ciritical section enter blocking function.
    // *  If the spin lock associated with this critical section is in use, then this
    // *  method will block until it is released.
    // *
    // *  \param none
    // */
    //void wizchip_critical_section_lock(void);

    /*! \brief Release a critical section
    *  \ingroup w5x00_spi
    *
    *  Set ciritical section exit function.
    *  Release a critical section.
    *
    *  \param none
    */
    //void wizchip_critical_section_unlock(void);

    /*! \brief Initialize SPI instances and Set DMA channel
    *  \ingroup w5x00_spi
    *
    *  Set GPIO to spi0.
    *  Puts the SPI into a known state, and enable it.
    *  Set DMA channel completion channel.
    *
    *  \param none
    */
    void wizchip_cris_initialize(void);

    /*! \brief W5x00 chip reset
    *  \ingroup w5x00_spi
    *
    *  Set a reset pin and reset.
    *
    *  \param none
    */
    void wizchip_reset(void);

    /*! \brief Initialize WIZchip
    *  \ingroup w5x00_spi
    *
    *  Set callback function to read/write byte using SPI.
    *  Set callback function for WIZchip select/deselect.
    *  Set memory size of W5x00 chip and monitor PHY link status.
    *
    *  \param none
    */
    void wizchip_initialize(void);

    /*! \brief Check chip version
    *  \ingroup w5x00_spi
    *
    *  Get version information.
    *
    *  \param none
    */
    void wizchip_check(void);

    /* Network */
    /*! \brief Initialize network
    *  \ingroup w5x00_spi
    *
    *  Set network information.
    *
    *  \param net_info network information.
    */
    //uint8_t SPI_read_byte(void);
    //uint8_t SPI_write_byte(uint8_t);       
    //void SPI_DMA_read(uint8_t*, uint16_t);    
    //void SPI_DMA_write(uint8_t*, uint16_t);
}

namespace tftp 
{
    typedef struct
    {
    int op;    /*WRQ */
    /* last block read */
    char data[TFTP_DATA_PKT_LEN_MAX];
    int  data_len;
    /* destination ip:port */
    ip_addr_t to_ip;
    int to_port;
    /* next block number */
    int block;
    /* total number of bytes transferred */
    int tot_bytes;
    /* timer interrupt count when last packet was sent */
    /* this should be used to resend packets on timeout */
    unsigned long long last_time;
    
    } tftp_connection_args;

    /* TFTP opcodes as specified in RFC1350   */
    typedef enum {
    TFTP_RRQ = 1,
    TFTP_WRQ = 2,
    TFTP_DATA = 3,
    TFTP_ACK = 4,
    TFTP_ERROR = 5
    } tftp_opcode;


    /* TFTP error codes as specified in RFC1350  */
    typedef enum {
    TFTP_ERR_NOTDEFINED,
    TFTP_ERR_FILE_NOT_FOUND,
    TFTP_ERR_ACCESS_VIOLATION,
    TFTP_ERR_DISKFULL,
    TFTP_ERR_ILLEGALOP,
    TFTP_ERR_UKNOWN_TRANSFER_ID,
    TFTP_ERR_FILE_ALREADY_EXISTS,
    TFTP_ERR_NO_SUCH_USER,
    } tftp_errorcode;

    void IAP_tftpd_init(void);
}

#endif

#endif
