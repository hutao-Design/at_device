#ifndef __AT_DEVICE_H__
#define __AT_DEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <at.h>
#include <at_socket.h>

#if !defined(RT_USING_NETDEV) || (!defined(AT_SW_VERSION_NUM) || AT_SW_VERSION_NUM < 0x10200)
#error "This RT-Thread version is older, please check and updata laster RT-Thread!"
#else

#include <arpa/inet.h>
#include <netdev.h>
#endif /* RT_USING_NETDEV */

#define AT_DEVICE_SW_VERSION           "1.0.0"
#define AT_DEVICE_SW_VERSION_NUM       0x10000

/* AT device class ID */
#define AT_DEVICE_CLASS_W60X           0x01U

/* Options and Commands for AT device control opreations */
#define MY_AT_DEVICE_CTRL_POWER_ON        0x01L
#define MY_AT_DEVICE_CTRL_POWER_OFF       0x02L
#define MY_AT_DEVICE_CTRL_RESET           0x03L
#define MY_AT_DEVICE_CTRL_LOW_POWER       0x04L
#define MY_AT_DEVICE_CTRL_SLEEP           0x05L
#define MY_AT_DEVICE_CTRL_WAKEUP          0x06L
#define MY_AT_DEVICE_CTRL_NET_CONN        0x07L
#define MY_AT_DEVICE_CTRL_NET_DISCONN     0x08L
#define MY_AT_DEVICE_CTRL_SET_WIFI_INFO   0x09L
#define MY_AT_DEVICE_CTRL_GET_SIGNAL      0x0AL
#define MY_AT_DEVICE_CTRL_GET_GPS         0x0BL
#define MY_AT_DEVICE_CTRL_GET_VER         0x0CL

/* Name type */
#define MY_AT_DEVICE_NAMETYPE_DEVICE      0x01
#define MY_AT_DEVICE_NAMETYPE_NETDEV      0x02
#define MY_AT_DEVICE_NAMETYPE_CLIENT      0x03

struct at_device;

/* AT device wifi ssid and password information */
struct at_device_ssid_pwd
{
    char *ssid;
    char *password;
};

/* AT device operations */
struct at_device_ops
{
    int (*init)(struct at_device *device);
    int (*deinit)(struct at_device *device);
    int (*control)(struct at_device *device, int cmd, void *arg);
};

struct at_device_class
{
    uint16_t class_id;                           /* AT device class ID */
    const struct at_device_ops *device_ops;      /* AT device operaiotns */
#ifdef AT_USING_SOCKET
    uint32_t socket_num;                         /* The maximum number of sockets support */
    const struct at_socket_ops *socket_ops;      /* AT device socket operations */
#endif
    rt_slist_t list;                             /* AT device class list */
};

struct at_device
{
    char name[RT_NAME_MAX];                      /* AT device name */
    rt_bool_t is_init;                           /* AT device initialization completed */
    struct at_device_class *class;               /* AT device class object */
    struct at_client *client;                    /* AT Client object for AT device */
    struct netdev *netdev;                       /* Network interface device for AT device */
#ifdef AT_USING_SOCKET
    rt_event_t socket_event;                     /* AT device socket event */
    struct at_socket *sockets;                   /* AT device sockets list */
#endif
    rt_slist_t list;                             /* AT device list */

    void *user_data;                             /* User-specific data */
};

/* Get AT device object */
struct at_device *at_device_get_first_initialized(void);
struct at_device *at_device_get_by_name(int type, const char *name);
#ifdef AT_USING_SOCKET
struct at_device *at_device_get_by_socket(int at_socket);
#endif

/* AT device control operaions */
int at_device_control(struct at_device *device, int cmd, void *arg);
/* Register AT device class object */
int at_device_class_register(struct at_device_class *class, uint16_t class_id);
/* Register AT device object */
int at_device_register(struct at_device *device, const char *device_name,
                        const char *at_client_name, uint16_t class_id, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* __AT_DEVICE_H__ */
