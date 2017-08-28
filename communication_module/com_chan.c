/**
 * @file    com_chan.c
 * @author  Kamran Rauf
 * @date    25 July 2017
 * @version 0.1
 * @brief  Basic loadable kernel module (LKM) that can relay
 * data between services (local/LAN hosted).
 */


// Library Includes
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pid.h>

#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#include <asm-generic/errno.h>


//*************************************
// Module Specifications
//*************************************
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kamran Rauf");
MODULE_DESCRIPTION("Simple data relay kernel module.");
MODULE_VERSION("0.1");


//*************************************
// Module Macro Definitions
//*************************************
#define COM_NETLINK_LKM 31

#define COM_NETLINK_KERNEL_SIG  0x00
#define COM_NETLINK_RW_SIG      0xA5A5A5A5
#define COM_NETLINK_DW_SIG      0x10101010
#define COM_NETLINK_MW_SIG      0x11001100

#define COM_NETLINK_MAX_PAYLOAD sizeof(ComChan_Message_t)


#define POPULATE_COM_CHAN_QUERY(MSG, R_ID)              \
{                                                       \
    memset(&(MSG), 0x00, sizeof(ComChan_Message_t));    \
    (MSG).serviceSig     = COM_NETLINK_KERNEL_SIG;      \
    (MSG).resourceInfoID = (R_ID);                      \
}


//*************************************
// Module Data Structures
//*************************************
enum
{
    INVALID_RESOURCE_INFO_ID,

    DISK_RESOURCE_INFO,
    MEMORY_RESOURCE_INFO,
    SERVICE_RESOURCE_INFO,
};


typedef struct RW_DiskInfo_s
{
    uint64_t                systemMemory;       ///< System total disk memory
    uint64_t                freeMemory;         ///< System free disk memory
} RW_DiskInfo_t;

typedef struct RW_MemoryInfo_s
{
    uint64_t                systemMemory;       ///< System total memory
    uint64_t                freeMemory;         ///< System free memory
} RW_MemoryInfo_t;

typedef struct ServiceInfo_s
{
    uint32_t                servicePID;         ///< Service process ID
    char                    serviceHostIP4[16]; ///< Service host IPV4 address
} ServiceInfo_t;

typedef struct ComChan_Message_s
{
    uint32_t                serviceSig;         ///< Service signature (ID)
    uint32_t                resourceInfoID;     ///< Resource information identifier

    uint32_t                flags;              ///< Message flags

    union
    {
        RW_DiskInfo_t       diskInfo;           ///< Disk information
        RW_MemoryInfo_t     memoryInfo;         ///< Memory information

        ServiceInfo_t       serviceInfo;        ///< Service information
    } res_info;
} ComChan_Message_t;


//*************************************
// Module Local Varialbes
//*************************************
static struct sock *pNLSock = NULL;

static int DW_PID = 0;
static int MW_PID = 0;
static int RW_PID = 0;


//*************************************
// Module Utility Functions
//*************************************
static void com_chan_recv(struct sk_buff *pSKB);

static void handleDWMessage(ComChan_Message_t *pMessage);
static void handleMWMessage(ComChan_Message_t *pMessage);
static void handleRWMessage(ComChan_Message_t *pMessage);

static void sendMessage(int srvPID, ComChan_Message_t *pMessage);


static void com_chan_recv(struct sk_buff *pSKB)
{
    struct nlmsghdr   *pNLHdr;
    ComChan_Message_t *pMessage;

    if (pSKB == NULL) { return; }

    printk(KERN_INFO "%s\n", __func__);

    /* Fetch netlink header */
    pNLHdr = (struct nlmsghdr*)pSKB->data;

    /* Get message pointer */
    pMessage = (ComChan_Message_t *)nlmsg_data(pNLHdr);

    printk(KERN_INFO "##############################\n");
    printk(KERN_INFO "Signature 0x%X | Resource-ID %u\n", pMessage->serviceSig, pMessage->resourceInfoID);
    switch (pMessage->serviceSig)
    {
        case COM_NETLINK_DW_SIG:
        {
            handleDWMessage(pMessage);
            break;
        }

        case COM_NETLINK_MW_SIG:
        {
            handleMWMessage(pMessage);
            break;
        }

        case COM_NETLINK_RW_SIG:
        {
            handleRWMessage(pMessage);
            break;
        }
    }
    printk(KERN_INFO "##############################\n");
}


static void handleDWMessage(ComChan_Message_t *pMessage)
{
    if (pMessage == NULL) { return; }

    switch (pMessage->resourceInfoID)
    {
        case DISK_RESOURCE_INFO:
        {
            printk(KERN_INFO "Disk information query received\n");

            if ( (RW_PID > 0) &&
                 (find_get_pid(RW_PID) != NULL) )
            {
                ComChan_Message_t resQuery;
                /* TODO:: Add request to queue */

                /* Populate resource information query */
                POPULATE_COM_CHAN_QUERY(resQuery, DISK_RESOURCE_INFO);

                /* Send resource query to resource watcher service */
                sendMessage(RW_PID, &resQuery);
            }
            else { RW_PID = 0; }
            break;
        }

        case SERVICE_RESOURCE_INFO:
        {
            if ( (DW_PID == 0) ||
                 (find_get_pid(DW_PID) != NULL) )
            {
                DW_PID = pMessage->res_info.serviceInfo.servicePID;
                printk(KERN_INFO "PID %u | Host %s\n", pMessage->res_info.serviceInfo.servicePID, pMessage->res_info.serviceInfo.serviceHostIP4);
            }
            break;
        }
    }
}

static void handleMWMessage(ComChan_Message_t *pMessage)
{
    if (pMessage == NULL) { return; }

    switch (pMessage->resourceInfoID)
    {
        case MEMORY_RESOURCE_INFO:
        {
            printk(KERN_INFO "Memory information query received\n");

            if ( (RW_PID > 0) &&
                 (find_get_pid(RW_PID) != NULL) )
            {
                ComChan_Message_t resQuery;
                /* TODO:: Add request to queue */

                /* Populate resource information query */
                POPULATE_COM_CHAN_QUERY(resQuery, MEMORY_RESOURCE_INFO);

                /* Send resource query to resource watcher service */
                sendMessage(RW_PID, &resQuery);
            }
            else { RW_PID = 0; }
            break;
        }

        case SERVICE_RESOURCE_INFO:
        {
            if ( (MW_PID == 0) ||
                 (find_get_pid(MW_PID) != NULL) )
            {
                MW_PID = pMessage->res_info.serviceInfo.servicePID;
                printk(KERN_INFO "PID %u | Host %s\n", pMessage->res_info.serviceInfo.servicePID, pMessage->res_info.serviceInfo.serviceHostIP4);
            }
            break;
        }
    }
}

static void handleRWMessage(ComChan_Message_t *pMessage)
{
    if (pMessage == NULL) { return; }

    switch (pMessage->resourceInfoID)
    {
        case DISK_RESOURCE_INFO:
        {
            /* TODO:: Check queue for query */
            printk(KERN_INFO "Total %llu | Free %llu\n", pMessage->res_info.diskInfo.systemMemory, pMessage->res_info.diskInfo.freeMemory);

            if ( (DW_PID > 0) &&
                 (find_get_pid(DW_PID) != NULL) )
            {
                ComChan_Message_t resInfo;

                /* Populate resource information */
                resInfo.serviceSig      = COM_NETLINK_KERNEL_SIG;
                resInfo.resourceInfoID  = DISK_RESOURCE_INFO;
                resInfo.flags           = 0;

                resInfo.res_info.diskInfo.systemMemory = pMessage->res_info.diskInfo.systemMemory;
                resInfo.res_info.diskInfo.freeMemory   = pMessage->res_info.diskInfo.freeMemory;

                /* Send resource query to resource watcher service */
                sendMessage(DW_PID, &resInfo);
            }
            else { DW_PID = 0; }
            break;
        }

        case MEMORY_RESOURCE_INFO:
        {
            /* TODO:: Check queue for query */
            printk(KERN_INFO "Total %llu | Free %llu\n", pMessage->res_info.memoryInfo.systemMemory, pMessage->res_info.memoryInfo.freeMemory);

            if ( (MW_PID > 0) &&
                 (find_get_pid(MW_PID) != NULL) )
            {
                ComChan_Message_t resInfo;

                /* Populate resource information */
                resInfo.serviceSig      = COM_NETLINK_KERNEL_SIG;
                resInfo.resourceInfoID  = MEMORY_RESOURCE_INFO;
                resInfo.flags           = 0;

                resInfo.res_info.memoryInfo.systemMemory = pMessage->res_info.memoryInfo.systemMemory;
                resInfo.res_info.memoryInfo.freeMemory   = pMessage->res_info.memoryInfo.freeMemory;

                /* Send resource query to resource watcher service */
                sendMessage(MW_PID, &resInfo);
            }
            else { MW_PID = 0; }
            break;
        }

        case SERVICE_RESOURCE_INFO:
        {
            /* TODO:: Add resource monitor to nodes queue for future queries */

            if ( (RW_PID == 0) ||
                 (find_get_pid(RW_PID) == NULL) )
            {
                RW_PID = pMessage->res_info.serviceInfo.servicePID;
                printk(KERN_INFO "RW_PID:: PID %u | Host %s\n", pMessage->res_info.serviceInfo.servicePID, pMessage->res_info.serviceInfo.serviceHostIP4);
            }
            break;
        }
    }
}

static void sendMessage(int srvPID, ComChan_Message_t *pMessage)
{
    struct sk_buff  *pSKB;
    struct nlmsghdr *pNLMsgHdr;

    if (srvPID <= 0) { return; }
    if (pMessage == NULL) { return; }

    /* Allocate memory for netlink SK-Buffer */
    pSKB = nlmsg_new(COM_NETLINK_MAX_PAYLOAD, 0);
    if (pSKB == NULL)
    {
        printk(KERN_ALERT "Netlink message creation failed\n");
        return;
    }

    /* Add netlink message header to SK-Buffer */
    pNLMsgHdr = nlmsg_put(pSKB, 0, 0, NLMSG_DONE, COM_NETLINK_MAX_PAYLOAD, 0);
    if (pNLMsgHdr == NULL)
    {
        printk(KERN_ALERT "Netlink message header addition to SK-Buffer failed\n");

        /* Release netlink SK-Buffer memory */
        nlmsg_free(pSKB);
        return;
    }

    /* Clear multi-cast group, flow is unicast */
    NETLINK_CB(pSKB).dst_group = 0;

    /* Copy message to netlink message header */
    memcpy(nlmsg_data(pNLMsgHdr), pMessage, COM_NETLINK_MAX_PAYLOAD);

    /* Send netlink message to service srvPID */
    if (nlmsg_unicast(pNLSock, pSKB, srvPID) < 0)
    {
        printk(KERN_ALERT "Netlink message sending failed\n");

        /* Release netlink SK-Buffer memory */
        nlmsg_free(pSKB);
        return;
    }
}


//*************************************
// Module Interface Functions
//*************************************
/** @brief The module initialization function
 *  It is the first function that will be called
 *  when module is installed (inmod). The function
 *  contains module initialization and required
 *  resource allocation; module exit function
 *  de-initialize module and release allocated
 *  resources.
 *  @return returns 0 if successful
 */
static int __init com_chan_init(void)
{
    struct netlink_kernel_cfg cfg = { .input = com_chan_recv, };

    printk(KERN_INFO "%s\n", __func__);

    /* Allocate netlink socket */
    pNLSock = netlink_kernel_create(&init_net, COM_NETLINK_LKM, &cfg);
    if (pNLSock == NULL)
    {
        printk(KERN_ALERT "Netlink socket creation failed\n");
        return -ECHILD;
    }

    return 0;
}

/** @brief The module cleanup function
 *  It is the last function that will be called
 *  when module is removed (rmmod). The function
 *  contains module cleanup procedure.
 */
static void __exit com_chan_exit(void)
{
    printk(KERN_INFO "%s\n", __func__);

    if (pNLSock != NULL)
    {
        netlink_kernel_release(pNLSock);
    }
}


//*************************************
// Module Callback Bindings
//*************************************
module_init(com_chan_init);
module_exit(com_chan_exit);
