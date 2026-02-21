#include "W5500_Networking.h"

#ifdef DEBUG
#include <assert.h>
#else
#define assert(expr) ((void)0)  
#endif

/**
 * Copyright (c) 2022 WIZnet Co.,Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifdef ETH_CTRL

namespace network 
{
    CommsInterface* ptr_eth_comms = nullptr;

    Pin *ptr_csPin = nullptr;
    Pin *ptr_rstPin = nullptr;

    //volatile packet_t ring_buffer[RING_BUFFER_SIZE];
    volatile uint8_t head = 0;
    volatile uint8_t tail = 0;
    volatile uint8_t dropped_packets = 0;    

    static ip_addr_t g_ip;
    static ip_addr_t g_mask;
    static ip_addr_t g_gateway;

    void EthernetInit(CommsInterface* _ptr_eth_comms, Pin *_ptr_csPin, Pin *_ptr_rstPin)
    {
        ptr_eth_comms = _ptr_eth_comms; 
        ptr_csPin = _ptr_csPin;
        ptr_rstPin = _ptr_rstPin;

        // initial network setting
        IP4_ADDR(&g_ip, Config::ip_address[0], Config::ip_address[1], Config::ip_address[2], Config::ip_address[3]);       
        IP4_ADDR(&g_mask, Config::subnet_mask[0], Config::subnet_mask[1], Config::subnet_mask[2], Config::subnet_mask[3]);
        IP4_ADDR(&g_gateway, Config::gateway[0], Config::gateway[1], Config::gateway[2], Config::gateway[3]);            

        wiznet::wizchip_cris_initialize();

        wiznet::wizchip_reset();
        wiznet::wizchip_initialize();
        wiznet::wizchip_check();

        // Set ethernet chip MAC address
        setSHAR(lwip::mac);
        ctlwizchip(CW_RESET_PHY, 0);

        // Initialize LWIP in NO_SYS mode
        lwip_init();

        netif_add(
                &lwip::g_netif, 
                &network::g_ip, 
                &network::g_mask, 
                &network::g_gateway, 
                NULL, 
                lwip::netif_initialize, 
                netif_input);

        lwip::g_netif.name[0] = 'e';
        lwip::g_netif.name[1] = '0';

        // Assign callbacks for link and status
        netif_set_link_callback(&lwip::g_netif, lwip::netif_link_callback);
        netif_set_status_callback(&lwip::g_netif, lwip::netif_status_callback);

        // MACRAW socket open
        lwip::retval = socket(SOCKET_MACRAW, Sn_MR_MACRAW, PORT_LWIPERF, 0x00);

        if (lwip::retval < 0)
        {
            printf(" MACRAW socket open failed\n");
        }

        // Set the default interface and bring it up
        netif_set_link_up(&lwip::g_netif);
        netif_set_up(&lwip::g_netif);

        // initialise UDP and TFTP
        udpServerInit();
        tftp::IAP_tftpd_init();            
    }

    void EthernetTasks()
    {
        getsockopt(SOCKET_MACRAW, SO_RECVBUF, &lwip::pack_len);

        if (lwip::pack_len > 0)
        {
            lwip::pack_len = lwip::recv_lwip(SOCKET_MACRAW, (uint8_t *)lwip::pack, lwip::pack_len);

            if (lwip::pack_len)
            {
                lwip::p = pbuf_alloc(PBUF_RAW, lwip::pack_len, PBUF_POOL);
                pbuf_take(lwip::p, lwip::pack, lwip::pack_len);
                free(lwip::pack);

                lwip::pack = static_cast<uint8_t *>(malloc(ETHERNET_MTU));
            }
            else
            {
                printf(" No packet received\n");
            }

            if (lwip::pack_len && lwip::p != NULL)
            {
                LINK_STATS_INC(link.recv);

                if (lwip::g_netif.input(lwip::p, &lwip::g_netif) != ERR_OK)
                {
                    pbuf_free(lwip::p);
                }
            }
            sys_check_timeouts();
        }
    }

    void udpServerInit(void)
    {
        struct udp_pcb *upcb;
        err_t err;

        // UDP control block for data
        upcb = udp_new();
        err = udp_bind(upcb, &g_ip, 27181);  // 27181 is the server UDP port

        /* 3. Set a receive callback for the upcb */
        if(err == ERR_OK)
        {
            udp_recv(upcb, udp_data_callback, NULL);
        }
        else
        {
            udp_remove(upcb);
        }
    }

    void udp_data_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
    {
        uint16_t txlen = 0;
        struct pbuf *txBuf;

        /*
        The pre Remora-core port used a ping pong buffer, but this was used throughout the core, which ours doesnt have.
        instead we'll implement parts of a ring buffer. This doesn't mean the PRU will handle it, just detect it 
        because the memcpy into the RX Buffer still works on the latest packet. 
        */ 

        uint8_t next_head = (head + 1) & (RING_BUFFER_SIZE - 1);

        if (next_head == tail) 
        {
            dropped_packets++;
            printf("Warning, PRU is dropping packets. Dropped Packets: %d\n", dropped_packets);
        } 
        else 
        {
            // disabling the actual storage to the ring buffer for now
            // ring_buffer[head].len = p->len;     // Copy payload into ring slot
            //memcpy((void *)ring_buffer[head].data, p->payload, std::min((size_t)p->len, sizeof(rxData_t))); // ensures proper alignment to size of rxData_t which is alinged to 32 bytes
            head = next_head;
        }
        
        // ours will just process the latest entry. Again this doesn't handle the lost packet, parts of the ring buffer is just there to detect it. The PRU will always read the latest packet
        assert(((uintptr_t)network::ptr_eth_comms->ptrRxData % alignof(rxData_t)) == 0);                   // rxBuffer is aligned to 32bits. Could easily end up in a situation where other platform builds don't respect this
        memcpy((void *)network::ptr_eth_comms->ptrRxData, p->payload, p->len); 

        //received a PRU request
        if (network::ptr_eth_comms->ptrRxData->header == Config::pruRead)
        {       
            network::ptr_eth_comms->ptrTxData->header = Config::pruData;
            txlen = Config::dataBuffSize;
            network::ptr_eth_comms->flag_new_data();      
        }
        else if (network::ptr_eth_comms->ptrRxData->header == Config::pruWrite)
        {
            network::ptr_eth_comms->ptrTxData->header = Config::pruAcknowledge;
            txlen = Config::dataBuffSize;   
            network::ptr_eth_comms->flag_new_data();      
        }	

        // allocate pbuf from RAM
        txBuf = pbuf_alloc(PBUF_TRANSPORT, txlen, PBUF_RAM);

        // copy the data into the buffer
        pbuf_take(txBuf, (char*)&network::ptr_eth_comms->ptrTxData->txBuffer, txlen);

        // Connect to the remote client
        udp_connect(upcb, addr, port);

        // Send a Reply to the Client
        udp_send(upcb, txBuf);

        // free the UDP connection, so we can accept new clients
        udp_disconnect(upcb);

        // Free the p_tx buffer
        pbuf_free(txBuf);

        // Free the p buffer
        pbuf_free(p);

        // since these memory operations are blocking, only update the tail if we reach the end of this function. The idea is that the dropped packets algo above will only cause use issues if the interrupt is refired before this function completes
        // TODO - see if we can fire an update callback instead to update the tail once Remora has finished processing the data. 
        tail = (tail + 1) & (RING_BUFFER_SIZE - 1);        
    }

    void network_initialize(wiz_NetInfo net_info)
    {
        ctlnetwork(CN_SET_NETINFO, (void *)&net_info);
    }

    void print_network_information(wiz_NetInfo net_info)
    {
        uint8_t tmp_str[8] = {
            0,
        };

        ctlnetwork(CN_GET_NETINFO, (void *)&net_info);
        ctlwizchip(CW_GET_ID, (void *)tmp_str);

        if (net_info.dhcp == NETINFO_DHCP)
        {
            printf("====================================================================================================\n");
            printf(" %s network configuration : DHCP\n\n", (char *)tmp_str);
        }
        else
        {
            printf("====================================================================================================\n");
            printf(" %s network configuration : static\n\n", (char *)tmp_str);
        }

        printf(" MAC         : %02X:%02X:%02X:%02X:%02X:%02X\n", net_info.mac[0], net_info.mac[1], net_info.mac[2], net_info.mac[3], net_info.mac[4], net_info.mac[5]);
        printf(" IP          : %d.%d.%d.%d\n", net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3]);
        printf(" Subnet Mask : %d.%d.%d.%d\n", net_info.sn[0], net_info.sn[1], net_info.sn[2], net_info.sn[3]);
        printf(" Gateway     : %d.%d.%d.%d\n", net_info.gw[0], net_info.gw[1], net_info.gw[2], net_info.gw[3]);
        printf(" DNS         : %d.%d.%d.%d\n", net_info.dns[0], net_info.dns[1], net_info.dns[2], net_info.dns[3]);
        printf("====================================================================================================\n\n");
    }
}

namespace lwip 
{
    struct netif g_netif;

    uint8_t mac[6] = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56};
    int8_t retval = 0;
    uint8_t *pack = static_cast<uint8_t *>(malloc(ETHERNET_MTU));
    uint16_t pack_len = 0;
    struct pbuf *p = NULL;    

    static uint8_t tx_frame[1542];
    static const uint32_t ethernet_polynomial_le = 0xedb88320U;

    int32_t send_lwip(uint8_t sn, uint8_t *buf, uint16_t len)
    {
        uint16_t freesize = 0;
        //uint8_t tmp = getSn_SR(sn);
        getSn_SR(sn);

        freesize = getSn_TxMAX(sn);
        if (len > freesize)
            len = freesize; // check size not to exceed MAX size.

        wiz_send_data(sn, buf, len);
        setSn_CR(sn, Sn_CR_SEND);
        while (getSn_CR(sn))
            ;

        while (1)
        {
            uint8_t IRtemp = getSn_IR(sn);
            if (IRtemp & Sn_IR_SENDOK)
            {
                setSn_IR(sn, Sn_IR_SENDOK);
                // printf("Packet sent ok\n");
                break;
            }
            else if (IRtemp & Sn_IR_TIMEOUT)
            {
                setSn_IR(sn, Sn_IR_TIMEOUT);
                // printf("Socket is closed\n");
                //  There was a timeout
                return -1;
            }
        }

        return (int32_t)len;
    }

    int32_t recv_lwip(uint8_t sn, uint8_t *buf, uint16_t len)
    {
        uint8_t head[2];
        uint16_t pack_len = 0;

        pack_len = getSn_RX_RSR(sn);

        if (pack_len > 0)
        {
            wiz_recv_data(sn, head, 2);
            setSn_CR(sn, Sn_CR_RECV);

            // byte size of data packet (2byte)
            pack_len = head[0];
            pack_len = (pack_len << 8) + head[1];
            pack_len -= 2;

            if (pack_len > len)
            {
                // Packet is bigger than buffer - drop the packet
                wiz_recv_ignore(sn, pack_len);
                setSn_CR(sn, Sn_CR_RECV);
                return 0;
            }

            wiz_recv_data(sn, buf, pack_len); // data copy
            setSn_CR(sn, Sn_CR_RECV);
        }

        return (int32_t)pack_len;
    }

    err_t netif_output(struct netif *netif, struct pbuf *p)
    {
        uint32_t tot_len = 0;

        memset(tx_frame, 0x00, sizeof(tx_frame));

        for (struct pbuf *q = p; q != NULL; q = q->next)
        {
            memcpy(tx_frame + tot_len, q->payload, q->len);

            tot_len += q->len;

            if (q->len == q->tot_len)
            {
                break;
            }
        }

        if (tot_len < 60)
        {
            // pad
            tot_len = 60;
        }

        ethernet_frame_crc(tx_frame, tot_len); // originally stored: uint32_t crc = ethernet_frame_crc(tx_frame, tot_len);

        send_lwip(0, tx_frame, tot_len);

        return ERR_OK;
    }

    void netif_link_callback(struct netif *netif)
    {
        printf("netif link status changed %s\n", netif_is_link_up(netif) ? "up" : "down");
    }

    void netif_status_callback(struct netif *netif)
    {
        printf("netif status changed %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
    }

    err_t netif_initialize(struct netif *netif)
    {
        netif->linkoutput = netif_output;
        netif->output = etharp_output;
        netif->mtu = ETHERNET_MTU;
        netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
        SMEMCPY(netif->hwaddr, lwip::mac, sizeof(netif->hwaddr));
        netif->hwaddr_len = sizeof(netif->hwaddr);
        return ERR_OK;
    }

    uint32_t ethernet_frame_crc(const uint8_t *data, int length)
    {
        uint32_t crc = 0xffffffff; /* Initial value. */

        while (--length >= 0)
        {
            uint8_t current_octet = *data++;

            for (int bit = 8; --bit >= 0; current_octet >>= 1)
            {
                if ((crc ^ current_octet) & 1)
                {
                    crc >>= 1;
                    crc ^= lwip::ethernet_polynomial_le;
                }
                else
                    crc >>= 1;
            }
        }

        return ~crc;
    }
}

namespace wiznet 
{
    static uint8_t SPI_read_byte(void);
    static uint8_t SPI_write_byte(uint8_t);       
    static void SPI_DMA_read(uint8_t*, uint16_t);    
    static void SPI_DMA_write(uint8_t*, uint16_t);

    static volatile bool spin_lock = false;

    static void wizchip_select(void)
    {
        network::ptr_csPin->set(false); 
    }

    static void wizchip_deselect(void)
    {
        network::ptr_csPin->set(true);
    }

    void wizchip_reset()
    {
        network::ptr_rstPin->set(false);
        delay_ms(100);
        network::ptr_rstPin->set(true);
        delay_ms(100);
    }

    static void wizchip_critical_section_lock(void)
    {
        while(wiznet::spin_lock);

        wiznet::spin_lock = true;
    }

    static void wizchip_critical_section_unlock(void)
    {
        wiznet::spin_lock = false;
    }

    void wizchip_cris_initialize(void)
    {
        wiznet::spin_lock = false;
        reg_wizchip_cris_cbfunc(wizchip_critical_section_lock, wizchip_critical_section_unlock);
    }

    void wizchip_initialize(void)
    {
        // Set both CS and RST pins high by default
        wizchip_reset();
        wizchip_deselect();

        // Call back function register for wizchip CS pin 
        reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);

        // Set up our callback functions for the Wiznet to trigger SPI read and write from the HAL class
        reg_wizchip_spi_cbfunc(SPI_read_byte, SPI_write_byte); // for individual SPI bytes only
        reg_wizchip_spiburst_cbfunc(SPI_DMA_read, SPI_DMA_write); // burst function will be better suited to DMA transfers. 

        /* W5x00 initialize */
        uint8_t temp;
        #if (_WIZCHIP_ == W5100S)
            uint8_t memsize[2][4] = {{8, 0, 0, 0}, {8, 0, 0, 0}};
        #elif (_WIZCHIP_ == W5500)
            uint8_t memsize[2][8] = {{8, 0, 0, 0, 0, 0, 0, 0}, {8, 0, 0, 0, 0, 0, 0, 0}};
        #endif

        printf("Attempting to initialize W5500 PHY...:");

        if (ctlwizchip(CW_INIT_WIZCHIP, (void *)memsize) == -1)
        {
            printf(" W5x00 initialization failed\n");

            return;
        }

        /* Check PHY link status */
        do
        {
            if (ctlwizchip(CW_GET_PHYLINK, (void *)&temp) == -1)
            {
                printf(" Unknown PHY link status\n");

                return;
            }
        } while (temp == PHY_LINK_OFF);

        printf(" Success!\n"); 
    }

    void wizchip_check(void)
    {
    #if (_WIZCHIP_ == W5100S)
        /* Read version register */
        if (getVER() != 0x51)
        {
            printf(" ACCESS ERR : VERSION != 0x51, read value = 0x%02x\n", getVER());

            while (1)
                ;
        }
    #elif (_WIZCHIP_ == W5500)
        /* Read version register */
        if (getVERSIONR() != 0x04)
        {
            printf(" ACCESS ERR : VERSION != 0x04, read value = 0x%02x\n", getVERSIONR());

            while (1)
                ;
        }
    #endif
    }

    static uint8_t SPI_read_byte(void)
    {
        if (network::ptr_eth_comms) {
            return network::ptr_eth_comms->read_byte();
        }
        return 0;
    }

    static uint8_t SPI_write_byte(uint8_t byte)
    {
        if (network::ptr_eth_comms) {
            return network::ptr_eth_comms->write_byte(byte);      
        }
        return 0;
    }

    static void SPI_DMA_read(uint8_t *data, uint16_t len)
    {
        if (network::ptr_eth_comms) {
            network::ptr_eth_comms->DMA_read(data, len);
        }
    }

    static void SPI_DMA_write(uint8_t *data, uint16_t len) 
    {
        if (network::ptr_eth_comms) {
            network::ptr_eth_comms->DMA_write(data, len);         
        }
    }    
}

namespace tftp 
{
    static uint32_t Flash_Write_Address; // start with some offset, write it after first 32 bytes to make room for the metadata
    static struct udp_pcb *UDPpcb;
    static uint32_t total_count = 0;

    static void IAP_wrq_recv_callback(void *_args, struct udp_pcb *upcb, struct pbuf *pkt_buf,
                            const ip_addr_t *addr, u16_t port);

    static int IAP_tftp_process_write(struct udp_pcb *upcb, const ip_addr_t *to, int to_port);

    static void IAP_tftp_recv_callback(void *arg, struct udp_pcb *Upcb, struct pbuf *pkt_buf,
                            const ip_addr_t *addr, u16_t port);

    static void IAP_tftp_cleanup_wr(struct udp_pcb *upcb, tftp_connection_args *args);
    static tftp_opcode IAP_tftp_decode_op(char *buf);
    static u16_t IAP_tftp_extract_block(char *buf);
    static void IAP_tftp_set_opcode(char *buffer, tftp_opcode opcode);
    static void IAP_tftp_set_block(char* packet, u16_t block);
    static err_t IAP_tftp_send_ack_packet(struct udp_pcb *upcb, const ip_addr_t *to, int to_port, int block);

    /**
     * @brief Returns the TFTP opcode
     * @param buf: pointer on the TFTP packet
     * @retval None
     */
    static tftp_opcode IAP_tftp_decode_op(char *buf)
    {
        return (tftp_opcode)(buf[1]);
    }

    /**
     * @brief  Extracts the block number
     * @param  buf: pointer on the TFTP packet
     * @retval block number
     */
    static u16_t IAP_tftp_extract_block(char *buf)
    {
        u16_t *b = (u16_t*)buf;
        return ntohs(b[1]);
    }

    /**
     * @brief Sets the TFTP opcode
     * @param  buffer: pointer on the TFTP packet
     * @param  opcode: TFTP opcode
     * @retval None
     */
    static void IAP_tftp_set_opcode(char *buffer, tftp_opcode opcode)
    {
        buffer[0] = 0;
        buffer[1] = (u8_t)opcode;
    }

    /**
     * @brief Sets the TFTP block number
     * @param packet: pointer on the TFTP packet
     * @param  block: block number
     * @retval None
     */
    static void IAP_tftp_set_block(char* packet, u16_t block)
    {
        u16_t *p = (u16_t *)packet;
        p[1] = htons(block);
    }

    /**
     * @brief Sends TFTP ACK packet
     * @param upcb: pointer on udp_pcb structure
     * @param to: pointer on the receive IP address structure
     * @param to_port: receive port number
     * @param block: block number
     * @retval: err_t: error code
     */
    static err_t IAP_tftp_send_ack_packet(struct udp_pcb *upcb, const ip_addr_t *to, int to_port, int block)
    {
        err_t err;
        struct pbuf *pkt_buf; /* Chain of pbuf's to be sent */

        /* create the maximum possible size packet that a TFTP ACK packet can be */
        char packet[TFTP_ACK_PKT_LEN];

            memset(packet, 0, TFTP_ACK_PKT_LEN *sizeof(char));

        /* define the first two bytes of the packet */
        IAP_tftp_set_opcode(packet, TFTP_ACK);

        /* Specify the block number being ACK'd.
        * If we are ACK'ing a DATA pkt then the block number echoes that of the DATA pkt being ACK'd (duh)
        * If we are ACK'ing a WRQ pkt then the block number is always 0
        * RRQ packets are never sent ACK pkts by the server, instead the server sends DATA pkts to the
        * host which are, obviously, used as the "acknowledgement".  This saves from having to sEndTransferboth
        * an ACK packet and a DATA packet for RRQs - see RFC1350 for more info.  */
        IAP_tftp_set_block(packet, block);

        /* PBUF_TRANSPORT - specifies the transport layer */
        pkt_buf = pbuf_alloc(PBUF_TRANSPORT, TFTP_ACK_PKT_LEN, PBUF_POOL);

        if (!pkt_buf)      /*if the packet pbuf == NULL exit and EndTransfertransmission */
        {
            return ERR_MEM;
        }

        /* Copy the original data buffer over to the packet buffer's payload */
        memcpy(pkt_buf->payload, packet, TFTP_ACK_PKT_LEN);

        /* Sending packet by UDP protocol */
        err = udp_sendto(upcb, pkt_buf, to, to_port);

        /* free the buffer pbuf */
        pbuf_free(pkt_buf);

        return err;
    }

    /**
     * @brief  Processes data transfers after a TFTP write request
     * @param  _args: used as pointer on TFTP connection args
     * @param  upcb: pointer on udp_pcb structure
     * @param pkt_buf: pointer on a pbuf stucture
     * @param ip_addr: pointer on the receive IP_address structure
     * @param port: receive port address
     * @retval None
     */
    static void IAP_wrq_recv_callback(void *_args, struct udp_pcb *upcb, struct pbuf *pkt_buf, const ip_addr_t *addr, u16_t port)
    {
        tftp_connection_args *args = (tftp_connection_args *)_args;
        uint8_t data_buffer[512] = {0}; // the size of a TFTP block, needs to be initalised with "0" for flash write to work on a partial block
        //uint16_t count=0;

        if (pkt_buf->len != pkt_buf->tot_len)
        {
            return;
        }

        /* Does this packet have any valid data to write? */
        if ((pkt_buf->len > TFTP_DATA_PKT_HDR_LEN) &&
            (IAP_tftp_extract_block((char*)pkt_buf->payload) == (args->block + 1)))
        {
            /* copy packet payload to data_buffer */
            pbuf_copy_partial(pkt_buf, data_buffer, pkt_buf->len - TFTP_DATA_PKT_HDR_LEN,
                            TFTP_DATA_PKT_HDR_LEN);

            total_count += pkt_buf->len - TFTP_DATA_PKT_HDR_LEN;

            // Write received data in flash
            uint8_t status;
            uint16_t *data = (uint16_t *)data_buffer;
            uint32_t address = Flash_Write_Address; 
            uint32_t remaining = 512;
            status = unlock_flash();
            while(remaining && status == 0) {
                status = write_to_flash_halfword(address, *data++);
                status = write_to_flash_halfword(address + 2, *data++);
                address += 4;
                remaining -= 4;
            }
            lock_flash();

            // Increment the write address
            Flash_Write_Address = Flash_Write_Address + 512;   

            /* update our block number to match the block number just received */
            args->block++;
            /* update total bytes  */
            (args->tot_bytes) += (pkt_buf->len - TFTP_DATA_PKT_HDR_LEN);

            /* This is a valid pkt but it has no data.  This would occur if the file being
            written is an exact multiple of sizeof(json_metadata_t).  In this case, the args->block
            value must still be updated, but we can skip everything else.    */
        }
        else if (IAP_tftp_extract_block((char*)pkt_buf->payload) == (args->block + 1))
        {
            /* update our block number to match the block number just received  */
            args->block++;
        }

        /* Send the appropriate ACK pkt*/
        IAP_tftp_send_ack_packet(upcb, addr, port, args->block);

        /* If the last write returned less than the maximum TFTP data pkt length,
        * then we've received the whole file and so we can quit (this is how TFTP
        * signals the EndTransferof a transfer!)
        */
        if (pkt_buf->len < TFTP_DATA_PKT_LEN_MAX)
        {
            IAP_tftp_cleanup_wr(upcb, args);
            pbuf_free(pkt_buf);
            JsonConfigHandler::new_flash_json = true;
            printf("New JSON file detected, uploading\n");          
        }
        else
        {
            pbuf_free(pkt_buf);
            return;
        }
    }

/**
  * @brief  Processes TFTP write request
  * @param  to: pointer on the receive IP address
  * @param  to_port: receive port number
  * @retval None
  */
    static int IAP_tftp_process_write(struct udp_pcb *upcb, const ip_addr_t *to, int to_port)
    {
        tftp_connection_args *args = NULL;
        /* This function is called from a callback,
        * therefore interrupts are disabled,
        * therefore we can use regular malloc   */
        args = (tftp_connection_args*)mem_malloc(sizeof *args);
        if (!args)
        {
            IAP_tftp_cleanup_wr(upcb, args);
            return 0;
        }

        args->op = TFTP_WRQ;
        args->to_ip.addr = to->addr;
        args->to_port = to_port;
        /* the block # used as a positive response to a WRQ is _always_ 0!!! (see RFC1350)  */
        args->block = 0;
        args->tot_bytes = 0;

        /* set callback for receives on this UDP PCB (Protocol Control Block) */
        udp_recv(upcb, IAP_wrq_recv_callback, args);

        total_count = 0;
        if((unlock_flash()) == 0) {
            mass_erase_upload_storage();
        }
        lock_flash();        

        Flash_Write_Address = Platform_Config::JSON_upload_start_address;

        /* initiate the write transaction by sending the first ack */
        IAP_tftp_send_ack_packet(upcb, to, to_port, args->block);

        return 0;
    }


    /**
     * @brief  Processes traffic received on UDP port 69
     * @param  args: pointer on tftp_connection arguments
     * @param  upcb: pointer on udp_pcb structure
     * @param  pbuf: pointer on packet buffer
     * @param  addr: pointer on the receive IP address
     * @param  port: receive port number
     * @retval None
     */
    static void IAP_tftp_recv_callback(void *arg, struct udp_pcb *upcb, struct pbuf *pkt_buf,
                            const ip_addr_t *addr, u16_t port)
    {
        tftp_opcode op;
        struct udp_pcb *upcb_tftp_data;
        err_t err;

        /* create new UDP PCB structure */
        upcb_tftp_data = udp_new();
        if (!upcb_tftp_data)
        {
            /* Error creating PCB. Out of Memory  */
            return;
        }

        /* bind to port 0 to receive next available free port */
        /* NOTE:  This is how TFTP works.  There is a UDP PCB for the standard port
        * 69 which al transactions begin communication on, however, _all_ subsequent
        * transactions for a given "stream" occur on another port  */
        err = udp_bind(upcb_tftp_data, IP_ADDR_ANY, 0);
        if (err != ERR_OK)
        {
            /* Unable to bind to port */
            return;
        }

        op = IAP_tftp_decode_op((char*)pkt_buf->payload);
        if (op != TFTP_WRQ)
        {
            /* remove PCB */
            udp_remove(upcb_tftp_data);
        }
        else
        {
            /* Start the TFTP write mode*/
            IAP_tftp_process_write(upcb_tftp_data, addr, port);
        }
        pbuf_free(pkt_buf);
    }


    /**
     * @brief  disconnect and close the connection
     * @param  upcb: pointer on udp_pcb structure
     * @param  args: pointer on tftp_connection arguments
     * @retval None
     */
    static void IAP_tftp_cleanup_wr(struct udp_pcb *upcb, tftp_connection_args *args)
    {
        /* Free the tftp_connection_args structure */
        mem_free(args);

        /* Disconnect the udp_pcb */
        udp_disconnect(upcb);

        /* close the connection */
        udp_remove(upcb);

        /* reset the callback function */
        udp_recv(UDPpcb, IAP_tftp_recv_callback, NULL);

    }

    /**
     * @brief  Creates and initializes a UDP PCB for TFTP receive operation
     * @param  None
     * @retval None
     */
    void IAP_tftpd_init(void)
    {
        err_t err;
        unsigned port = 69; /* 69 is the port used for TFTP protocol initial transaction */

        /* create a new UDP PCB structure  */
        UDPpcb = udp_new();
        if (!UDPpcb)
        {
            /* Error creating PCB. Out of Memory  */
            return;
        }

        /* Bind this PCB to port 69  */
        err = udp_bind(UDPpcb, IP_ADDR_ANY, port);
        if (err == ERR_OK)
        {
            /* Initialize receive callback function  */
            udp_recv(UDPpcb, IAP_tftp_recv_callback, NULL);
        }
    }
}

#endif