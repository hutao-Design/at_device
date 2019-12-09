#include <stdio.h>
#include <string.h>

#include <at_device_w60x.h>

#define LOG_TAG                       "at.skt.w60x"
#include <at_log.h>

#if defined(MY_AT_DEVICE_USING_W60X) && defined(AT_USING_SOCKET)

#define W60X_MODULE_SEND_MAX_SIZE   2048
/* set real event by current socket and current state */
#define SET_EVENT(socket, event)       (((socket + 1) << 16) | (event))

/* AT socket event type */
#define W60X_EVENT_CONN_OK          (1L << 0)
#define W60X_EVENT_SEND_OK          (1L << 1)
#define W60X_EVENT_RECV_OK          (1L << 2)
#define W60X_EVNET_CLOSE_OK         (1L << 3)
#define W60X_EVENT_CONN_FAIL        (1L << 4)
#define W60X_EVENT_SEND_FAIL        (1L << 5)

static at_evt_cb_t at_evt_cb_set[] = {
        [AT_SOCKET_EVT_RECV] = NULL,
        [AT_SOCKET_EVT_CLOSED] = NULL,
};

static int w60x_socket_event_send(struct at_device *device, uint32_t event)
{
    return (int) rt_event_send(device->socket_event, event);
}

static int w60x_socket_event_recv(struct at_device *device, uint32_t event, uint32_t timeout, rt_uint8_t option)
{
    int result = 0;
    rt_uint32_t recved;

    result = rt_event_recv(device->socket_event, event, option | RT_EVENT_FLAG_CLEAR, timeout, &recved);
    if (result != RT_EOK)
    {
        return -RT_ETIMEOUT;
    }

    return recved;
}

/**
 * close socket by AT commands.
 *
 * @param current socket
 *
 * @return  0: close socket success
 *         -1: send AT commands error
 *         -2: wait socket event timeout
 *         -5: no memory
 */
static int w60x_socket_close(struct at_socket *socket)
{
	return 0;
}	

/**
 * create TCP/UDP client or server connect by AT commands.
 *
 * @param socket current socket
 * @param ip server or client IP address
 * @param port server or client port
 * @param type connect socket type(tcp, udp)
 * @param is_client connection is client
 *
 * @return   0: connect success
 *          -1: connect failed, send commands error or type error
 *          -2: wait socket event timeout
 *          -5: no memory
 */
static int w60x_socket_connect(struct at_socket *socket, char *ip, int32_t port, enum at_socket_type type, rt_bool_t is_client)
{
	return 0;
}

/**
 * send data to server or client by AT commands.
 *
 * @param socket current socket
 * @param buff send buffer
 * @param bfsz send buffer size
 * @param type connect socket type(tcp, udp)
 *
 * @return >=0: the size of send success
 *          -1: send AT commands error or send data error
 *          -2: waited socket event timeout
 *          -5: no memory
 */
static int w60x_socket_send(struct at_socket *socket, const char *buff, size_t bfsz, enum at_socket_type type)
{
	return 0;
}

/**
 * domain resolve by AT commands.
 *
 * @param name domain name
 * @param ip parsed IP address, it's length must be 16
 *
 * @return  0: domain resolve success
 *         -2: wait socket event timeout
 *         -5: no memory
 */
static int w60x_domain_resolve(const char *name, char ip[16])
{
	return 0;
}

/**
 * set AT socket event notice callback
 *
 * @param event notice event
 * @param cb notice callback
 */
static void w60x_socket_set_event_cb(at_socket_evt_t event, at_evt_cb_t cb)
{
    if (event < sizeof(at_evt_cb_set) / sizeof(at_evt_cb_set[1]))
    {
        at_evt_cb_set[event] = cb;
    }
}

static const struct at_socket_ops w60x_socket_ops =
{
    w60x_socket_connect,
    w60x_socket_close,
    w60x_socket_send,
    w60x_domain_resolve,
    w60x_socket_set_event_cb,
};

static void urc_send_func(struct at_client *client, const char *data, rt_size_t size)
{
    int device_socket = 0;
    struct at_device *device = RT_NULL;
    struct at_device_w60x *w60x = RT_NULL;
    char *client_name = client->device->parent.name;

    RT_ASSERT(data && size);

    device = at_device_get_by_name(AT_DEVICE_NAMETYPE_CLIENT, client_name);
    if (device == RT_NULL)
    {
        LOG_E("get device(%s) failed.", client_name);
        return;
    }
    w60x = (struct at_device_w60x *) device->user_data;
    device_socket = (int) w60x->user_data;

    if (rt_strstr(data, "SEND OK"))
    {
        w60x_socket_event_send(device, SET_EVENT(device_socket, W60X_EVENT_SEND_OK));
    }
    else if (rt_strstr(data, "SEND FAIL"))
    {
        w60x_socket_event_send(device, SET_EVENT(device_socket, W60X_EVENT_SEND_FAIL));
    }
}

static void urc_send_bfsz_func(struct at_client *client, const char *data, rt_size_t size)
{
    static int cur_send_bfsz = 0;

    RT_ASSERT(data && size);

    sscanf(data, "Recv %d bytes", &cur_send_bfsz);
}

static void urc_close_func(struct at_client *client, const char *data, rt_size_t size)
{
    int index = 0;
    struct at_socket *socket = RT_NULL;
    struct at_device *device = RT_NULL;
    char *client_name = client->device->parent.name;

    RT_ASSERT(data && size);

    device = at_device_get_by_name(AT_DEVICE_NAMETYPE_CLIENT, client_name);
    if (device == RT_NULL)
    {
        LOG_E("get device(%s) failed.", client_name);
        return;
    }

    sscanf(data, "%d,CLOSED", &index);
    socket = &(device->sockets[index]);

    /* notice the socket is disconnect by remote */
    if (at_evt_cb_set[AT_SOCKET_EVT_CLOSED])
    {
        at_evt_cb_set[AT_SOCKET_EVT_CLOSED](socket, AT_SOCKET_EVT_CLOSED, RT_NULL, 0);
    }
}

static void urc_recv_func(struct at_client *client, const char *data, rt_size_t size)
{
    int device_socket = 0;
    rt_int32_t timeout = 0;
    rt_size_t bfsz = 0, temp_size = 0;
    char *recv_buf = RT_NULL, temp[8] = {0};
    struct at_socket *socket = RT_NULL;
    struct at_device *device = RT_NULL;
    char *client_name = client->device->parent.name;

    RT_ASSERT(data && size);

    device = at_device_get_by_name(AT_DEVICE_NAMETYPE_CLIENT, client_name);
    if (device == RT_NULL)
    {
        LOG_E("get device(%s) failed.", client_name);
        return;
    }

    /* get the at deveice socket and receive buffer size by receive data */
    sscanf(data, "+IPD,%d,%d:", &device_socket, (int *) &bfsz);

    /* set receive timeout by receive buffer length, not less than 10ms */
    timeout = bfsz > 10 ? bfsz : 10;

    if (device_socket < 0 || bfsz == 0)
        return;

    recv_buf = (char *) rt_calloc(1, bfsz);
    if (recv_buf == RT_NULL)
    {
        LOG_E("no memory receive buffer(%d).", bfsz);
        /* read and clean the coming data */
        while (temp_size < bfsz)
        {
            if (bfsz - temp_size > sizeof(temp))
            {
                at_client_obj_recv(client, temp, sizeof(temp), timeout);
            }
            else
            {
                at_client_obj_recv(client, temp, bfsz - temp_size, timeout);
            }
            temp_size += sizeof(temp);
        }
        return;
    }

    /* sync receive data */
    if (at_client_obj_recv(client, recv_buf, bfsz, timeout) != bfsz)
    {
        LOG_E("%s device receive size(%d) data failed.", device->name, bfsz);
        rt_free(recv_buf);
        return;
    }

    /* get at socket object by device socket descriptor */
    socket = &(device->sockets[device_socket]);

    /* notice the receive buffer and buffer size */
    if (at_evt_cb_set[AT_SOCKET_EVT_RECV])
    {
        at_evt_cb_set[AT_SOCKET_EVT_RECV](socket, AT_SOCKET_EVT_RECV, recv_buf, bfsz);
    }
}

static const struct at_urc urc_table[] =
{
    {"SEND OK",          "\r\n",           urc_send_func},
    {"SEND FAIL",        "\r\n",           urc_send_func},
    {"Recv",             "bytes\r\n",      urc_send_bfsz_func},
    {"",                 ",CLOSED\r\n",    urc_close_func},
    {"+IPD",             ":",              urc_recv_func},
};

int w60x_socket_init(struct at_device *device)
{
    RT_ASSERT(device);

    /* register URC data execution function  */
    at_obj_set_urc_table(device->client, urc_table, sizeof(urc_table) / sizeof(urc_table[0]));

    return RT_EOK;
}

int w60x_socket_class_register(struct at_device_class *class)
{
    RT_ASSERT(class);

    class->socket_num = AT_DEVICE_W60X_SOCKETS_NUM;
    class->socket_ops = &w60x_socket_ops;

    return RT_EOK;
}

#endif


