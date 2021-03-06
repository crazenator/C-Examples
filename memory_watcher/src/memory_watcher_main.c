/**
 * @file    memory_watcher_main.c
 * @author  Kamran Rauf
 * @date    25 July 2017
 * @version 0.1
 * @brief  Simple system resource (memory) watcher
 * with interface to query for resource status from resource
 * watcher.
 */


// Library Includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/statvfs.h>

#include <linux/netlink.h>


//*************************************
// Module Macro Definitions
//*************************************
#define COM_NETLINK_LKM         31

#define COM_NETLINK_KERNEL_SIG  0x00
#define COM_NETLINK_MW_SIG      0x11001100

#define COM_NETLINK_SOURCE      getpid()    // Process ID as source
#define COM_NETLINK_DESTINATION 0           // Kernel as destination

#define COM_NETLINK_MAX_PAYLOAD sizeof(ComChan_Message_t)

#define MAX_EPOLL_EVENTS        2
#define EPOLL_EVENTS_TIMEOUT    10          // Seconds

#define RESOURCE_QUERY_TIMEOUT  5           // Seconds


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
// Module Utility Functions
//*************************************
static inline int64_t _GetCurrentTime();


static struct nlmsghdr* createNLMsgHdr(int maxPayloadSz);
static int createNLSocket(struct sockaddr_nl *pSrcAddr,
                          struct sockaddr_nl *pDstAddr);

static int destroyNLMsgHdr(struct nlmsghdr *pNLMsgHdr);
static int destroyNLSocket(int sock);

static int registerEvent(int epollFD, int eventFD);

static int handleResponseMsg(const int                 sock,
                             const struct nlmsghdr    *pNLMsgHdr);

static int sendMessage(const int                 sock,
                       const struct sockaddr_nl *pDstAddr,
                       const struct nlmsghdr    *pNLMsgHdr,
                       const ComChan_Message_t  *pMessage);


static inline int64_t _GetCurrentTime()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t) ts.tv_sec;
}


static struct nlmsghdr* createNLMsgHdr(int maxPayloadSz)
{
    struct nlmsghdr *pNLMsgHdr;

    /* Allocate netlink message header */
    pNLMsgHdr = (struct nlmsghdr *)malloc( NLMSG_SPACE(maxPayloadSz) );
    if (pNLMsgHdr == NULL)
    {
        printf("ERROR - %s:%d :: Failed to allocate netlink message header\n",
                __func__, __LINE__);
        return NULL;
    }

    /* Initialize netlink message header */
    memset(pNLMsgHdr, 0, NLMSG_SPACE(maxPayloadSz));
    pNLMsgHdr->nlmsg_len   = NLMSG_SPACE(maxPayloadSz);
    pNLMsgHdr->nlmsg_pid   = COM_NETLINK_SOURCE;
    pNLMsgHdr->nlmsg_flags = 0;

    return pNLMsgHdr;
}

static int createNLSocket(struct sockaddr_nl *pSrcAddr,
                          struct sockaddr_nl *pDstAddr)
{
    int sock;

    if ( (pSrcAddr == NULL) ||
         (pDstAddr == NULL) )
    {
        printf("ERROR - %s:%d :: Invalid input arguments (%p, %p)\n",
                __func__, __LINE__,
                pSrcAddr, pDstAddr);
        return -1;
    }

    /* Create netlink socket */
    sock = socket(PF_NETLINK, SOCK_RAW, COM_NETLINK_LKM);
    if (sock < 0) { return -1; }

    /* Initialize source and destination addresses */
    memset(pSrcAddr, 0x00, sizeof(struct sockaddr_nl));
    memset(pDstAddr, 0x00, sizeof(struct sockaddr_nl));

    /* Populate source address information */
    pSrcAddr->nl_family = AF_NETLINK;
    pSrcAddr->nl_pid    = COM_NETLINK_SOURCE;

    /* Populate destination address information */
    pDstAddr->nl_family = AF_NETLINK;
    pDstAddr->nl_pid    = COM_NETLINK_DESTINATION;

    /* Bind socket to source address */
    bind(sock, (struct sockaddr*)pSrcAddr, sizeof(struct sockaddr_nl));

    return sock;
}

static int destroyNLMsgHdr(struct nlmsghdr *pNLMsgHdr)
{
    if (pNLMsgHdr == NULL)
    {
        printf("ERROR - %s:%d :: Invalid input netlink message header %p\n",
                __func__, __LINE__,
                pNLMsgHdr);
        return -1;
    }

    /* Release netlink message header memory */
    free(pNLMsgHdr);

    return 0;
}

static int destroyNLSocket(int sock)
{
    if (sock <= 0)
    {
        printf("ERROR - %s:%d :: Invalid input netlink socket %d\n",
                __func__, __LINE__,
                sock);
        return -1;
    }

    close(sock);

    return 0;
}

static int handleResponseMsg(const int                 sock,
                             const struct nlmsghdr    *pNLMsgHdr)
{
    int retVal;

    struct iovec ioVector;
    struct msghdr msgHdr;

    ComChan_Message_t *pMessage;

    if (sock <= 0)
    {
        printf("ERROR - %s:%d :: Invalid input socket (%d) information\n",
                __func__, __LINE__,
                sock);
        return -1;
    }

    if (pNLMsgHdr == NULL)
    {
        printf("ERROR - %s:%d :: Invalid input pointer arguments (%p)\n",
                __func__, __LINE__,
                pNLMsgHdr);
        return -1;
    }

    /* Reset I/O vector and message header */
    memset(&ioVector, 0x00, sizeof(ioVector));
    memset(&msgHdr,   0x00, sizeof(msgHdr));

    /* Populate I/O vector information */
    ioVector.iov_base = (void *)pNLMsgHdr;
    ioVector.iov_len = pNLMsgHdr->nlmsg_len;

    /* Populate message header */
    msgHdr.msg_iov = &ioVector;
    msgHdr.msg_iovlen = 1;                          // Single I/O vector

    /* Receive message on netlink socket */
    retVal = recvmsg(sock, &msgHdr, 0);
    if (retVal < 0)
    {
        printf("ERROR - %s:%d :: Failed to read from socket (%d) [%m]\n",
                __func__, __LINE__,
                sock);
        return retVal;
    }

    /* Get message data */
    pMessage = (ComChan_Message_t *)NLMSG_DATA(pNLMsgHdr);
    if ( (pMessage != NULL) &&
         (pMessage->serviceSig == COM_NETLINK_KERNEL_SIG) )
    {
        switch (pMessage->resourceInfoID)
        {
            case MEMORY_RESOURCE_INFO:
            {
                printf("Memory Information (%lu, %lu)\n",
                        pMessage->res_info.memoryInfo.systemMemory,
                        pMessage->res_info.memoryInfo.freeMemory);
                break;
            }
        }
    }

    return retVal;
}

static int registerEvent(int epollFD, int eventFD)
{
    struct epoll_event epollEvent;

    if ( (epollFD < 0) || (eventFD < 0) )
    {
        printf("ERROR - %s:%d :: Invalid input arguments for event registration (%d, %d)\n",
                __func__, __LINE__,
                epollFD, eventFD);
        return -1;
    }

    /* Initialize event information */
    memset((void *)&epollEvent, 0x00, sizeof(struct epoll_event));

    /* Register reading event */
    epollEvent.events = EPOLLIN;
    epollEvent.data.fd = eventFD;

    if (epoll_ctl(epollFD, EPOLL_CTL_ADD, eventFD, &epollEvent) < 0)
    {
        printf("ERROR - %s:%d :: Failed to register event [%m]\n", __func__, __LINE__);
        return -1;
    }

    return 0;
}

static int sendMessage(const int                 sock,
                       const struct sockaddr_nl *pDstAddr,
                       const struct nlmsghdr    *pNLMsgHdr,
                       const ComChan_Message_t  *pMessage)
{
    int retVal;

    struct iovec ioVector;
    struct msghdr msgHdr;

    if (sock <= 0)
    {
        printf("ERROR - %s:%d :: Invalid input socket (%d) information\n",
                __func__, __LINE__,
                sock);
        return -1;
    }

    if ( (pDstAddr  == NULL) ||
         (pNLMsgHdr == NULL) ||
         (pMessage  == NULL) )
    {
        printf("ERROR - %s:%d :: Invalid input pointer arguments (%p, %p, %p)\n",
                __func__, __LINE__,
                pDstAddr, pNLMsgHdr,
                pMessage);
        return -1;
    }

    /* Copy message to netlink buffer */
    memcpy(NLMSG_DATA(pNLMsgHdr), pMessage, sizeof(ComChan_Message_t));

    /* Reset I/O vector and message header */
    memset(&ioVector, 0x00, sizeof(ioVector));
    memset(&msgHdr,   0x00, sizeof(msgHdr));

    /* Populate I/O vector information */
    ioVector.iov_base = (void *)pNLMsgHdr;
    ioVector.iov_len = pNLMsgHdr->nlmsg_len;

    /* Populate message header */
    msgHdr.msg_name = (void *)pDstAddr;
    msgHdr.msg_namelen = sizeof(struct sockaddr_nl);
    msgHdr.msg_iov = &ioVector;
    msgHdr.msg_iovlen = 1;                          // Single I/O vector

    retVal = sendmsg(sock, &msgHdr, 0);
    if (retVal < 0)
    {
        printf("ERROR - %s:%d :: Failed to transmit message on socket (%d) [%m]\n",
                __func__, __LINE__,
                sock);
    }

    return retVal;
}


//*************************************
// Module Main Function
//*************************************
int main(__attribute__((unused)) int argc, __attribute__((unused)) char **args)
{
    int64_t queryTimeout;
    unsigned char MW_SERVICE_RUNNING = 0x01;

    struct nlmsghdr *pNLMsgHdr;

    int epollFD, sock, nEvents;
    struct sockaddr_nl srcAddr, dstAddr;
    struct epoll_event epollEvents[MAX_EPOLL_EVENTS];

    ComChan_Message_t memWatcherMsg;

    /* Initialize netlink socket */
    sock = createNLSocket(&srcAddr, &dstAddr);
    if (sock <= 0) { return EXIT_FAILURE; }

    /* Create netlink message header */
    pNLMsgHdr = createNLMsgHdr(COM_NETLINK_MAX_PAYLOAD);
    if (pNLMsgHdr == NULL)
    {
        destroyNLSocket(sock);
        return EXIT_FAILURE;
    }

    /* Create event polling setup */
    epollFD = epoll_create1(0);
    if (epollFD < 0)
    {
        printf("ERROR - %s:%d :: Failed to create epoll [%m]\n", __func__, __LINE__);

        destroyNLMsgHdr(pNLMsgHdr);
        destroyNLSocket(sock);
        return EXIT_FAILURE;
    }

    /* Register netlink socket for events polling */
    if (registerEvent(epollFD, sock) < 0)
    {
        destroyNLMsgHdr(pNLMsgHdr);
        destroyNLSocket(sock);
        close(epollFD);
        return EXIT_FAILURE;
    }


    /* Populate message for service information */
    memWatcherMsg.serviceSig        = COM_NETLINK_MW_SIG;
    memWatcherMsg.resourceInfoID    = SERVICE_RESOURCE_INFO;
    memWatcherMsg.flags             = 0;

    memWatcherMsg.res_info.serviceInfo.servicePID = getpid();
    strncpy(memWatcherMsg.res_info.serviceInfo.serviceHostIP4, "127.0.0.1", strlen("127.0.0.1"));

    /* Send service information message */
    if (sendMessage(sock, &dstAddr, pNLMsgHdr, &memWatcherMsg) <= 0)
    {
        destroyNLMsgHdr(pNLMsgHdr);
        destroyNLSocket(sock);
        close(epollFD);
        return EXIT_FAILURE;
    }


    /* Initialize resource query timeout */
    queryTimeout = _GetCurrentTime();

    /* Resource watcher business logic */
    while (MW_SERVICE_RUNNING)
    {
        if (queryTimeout < _GetCurrentTime())
        {
            /* Populate message for service information */
            memset(&memWatcherMsg, 0x00, sizeof(ComChan_Message_t));
            memWatcherMsg.serviceSig        = COM_NETLINK_MW_SIG;
            memWatcherMsg.resourceInfoID    = MEMORY_RESOURCE_INFO;
            memWatcherMsg.flags             = 0;

            /* Send service information message */
            sendMessage(sock, &dstAddr, pNLMsgHdr, &memWatcherMsg);

            queryTimeout = _GetCurrentTime() + RESOURCE_QUERY_TIMEOUT;
        }

        /* Wait for events */
        nEvents = epoll_wait(epollFD, epollEvents, MAX_EPOLL_EVENTS, (EPOLL_EVENTS_TIMEOUT * 1000));

        /* Process events */
        while (nEvents > 0)
        {
            if (epollEvents[(nEvents - 1)].data.fd == sock)
            {
                if (epollEvents[(nEvents - 1)].events & EPOLLIN)
                {
                    /* Read message on netlink socket */
                    if (handleResponseMsg(sock, pNLMsgHdr) < 0)
                    {
                        MW_SERVICE_RUNNING = 0;
                        break;
                    }
                }
                else if (epollEvents[(nEvents - 1)].events & (EPOLLERR | EPOLLHUP))
                {
                    /* Error event detected on netlink socket */
                    MW_SERVICE_RUNNING = 0;
                    break;
                }
            }

            nEvents--;
        }
    }


    /* Destroy netlink message header */
    destroyNLMsgHdr(pNLMsgHdr);

    /* Destroy netlink socket */
    destroyNLSocket(sock);

    /* Close polling descriptor */
    if (epollFD > 0) { close(epollFD); }

    return EXIT_SUCCESS;
}

