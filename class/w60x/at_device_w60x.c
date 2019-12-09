#include <stdio.h>
#include <string.h>
#include <at_device_w60x.h>

#define LOG_TAG                        "at.dev.w60x"

#include <at_log.h>

#ifdef MY_AT_DEVICE_USING_W60X

/* =============================  w60x network interface operations ============================= */
static int w60x_netdev_set_up(struct netdev *netdev)
{
	int result=RT_EOK;
	struct at_device * device =RT_NULL;
	device =at_device_get_by_name(AT_DEVICE_NAMETYPE_NETDEV,netdev->name);
	if(device==RT_NULL){
		LOG_E("get device(%s) failed.", netdev->name);
    return -RT_ERROR;
	}
	if(device->is_init==RT_FALSE){
		result=device->class->device_ops->control(device,AT_DEVICE_CTRL_RESET,RT_NULL);	
		if(result==RT_EOK){
			netdev_low_level_set_status(netdev,RT_TRUE);		//挂载网卡
			LOG_D("network interface device(%s) set up status", netdev->name);
		}
	}
	return 	result;
}

static int w60x_netdev_set_down(struct netdev *netdev)
{
	struct at_device *device = RT_NULL;

	device = at_device_get_by_name(AT_DEVICE_NAMETYPE_NETDEV, netdev->name);
	if (device == RT_NULL)
	{
			LOG_E("get device by netdev(%s) failed.", netdev->name);
			return -RT_ERROR;
	}
	if (device->is_init == RT_TRUE)
	{
			device->is_init = RT_FALSE;
			netdev_low_level_set_status(netdev, RT_FALSE);
			LOG_D("network interface device(%s) set down status", netdev->name);
	}
	return RT_EOK;
}

static int w60x_netdev_set_addr_info(struct netdev *netdev, ip_addr_t *ip_addr, ip_addr_t *netmask, ip_addr_t *gw)
{
	return 0;
}

static int w60x_netdev_set_dns_server(struct netdev *netdev, uint8_t dns_num, ip_addr_t *dns_server)
{
	return 0;
}

static int w60x_netdev_set_dhcp(struct netdev *netdev, rt_bool_t is_enabled)
{
	return 0;
}

#ifdef NETDEV_USING_PING
static int w60x_netdev_ping(struct netdev *netdev, const char *host,
                size_t data_len, uint32_t timeout, struct netdev_ping_resp *ping_resp)
{
	return 0;
}
#endif

#ifdef NETDEV_USING_NETSTAT
void w60x_netdev_netstat(struct netdev *netdev)
{
	
}
#endif

static const struct netdev_ops w60x_netdev_ops =
{
    w60x_netdev_set_up,
    w60x_netdev_set_down,

    w60x_netdev_set_addr_info,
    w60x_netdev_set_dns_server,
    w60x_netdev_set_dhcp,

#ifdef NETDEV_USING_PING
    w60x_netdev_ping,
#endif
#ifdef NETDEV_USING_NETSTAT
    w60x_netdev_netstat,
#endif
};

static struct netdev *w60x_netdev_add(const char *netdev_name)
{
#define ETHERNET_MTU        1500
#define HWADDR_LEN          6
    struct netdev *netdev = RT_NULL;

    RT_ASSERT(netdev_name);

    netdev = (struct netdev *) rt_calloc(1, sizeof(struct netdev));
    if (netdev == RT_NULL)
    {
        LOG_E("no memory for netdev create.");
        return RT_NULL;
    }

    netdev->mtu = ETHERNET_MTU;
    netdev->ops = &w60x_netdev_ops;
    netdev->hwaddr_len = HWADDR_LEN;

#ifdef SAL_USING_AT
    extern int sal_at_netdev_set_pf_info(struct netdev *netdev);
    /* set the network interface socket/netdb operations */
    sal_at_netdev_set_pf_info(netdev);
#endif

    netdev_register(netdev, netdev_name, RT_NULL);

    return netdev;
}

/* =============================  w60x device operations ============================= */

static void urc_func(struct at_client *client, const char *data, rt_size_t size)
{
    struct at_device *device = RT_NULL;
    char *client_name = client->device->parent.name;

    RT_ASSERT(client && data && size);

    device = at_device_get_by_name(AT_DEVICE_NAMETYPE_CLIENT, client_name);
    if (device == RT_NULL)
    {
        LOG_E("get device(%s) failed.", client_name);
        return;
    }	
		if (rt_strstr(data, "WIFI CONNECTED"))
    {
			LOG_I("%s device wifi is connected.", device->name);
		}
		else if (rt_strstr(data, "WIFI DISCONNECT"))
    {
			LOG_I("%s device wifi is disconnect.", device->name);
		}
}
static const struct at_urc urc_table[] =
{
    {"WIFI CONNECTED",   "\r\n",           urc_func},
    {"WIFI DISCONNECT",  "\r\n",           urc_func},
};
static int w60x_init(struct at_device *device)
{
	struct at_device_w60x *w60x = (struct at_device_w60x *) device->user_data;
	
	/* initialize AT client */
  at_client_init(w60x->client_name, w60x->recv_line_num);
	
	device->client = at_client_get(w60x->client_name);
	if (device->client == RT_NULL)
	{
			LOG_E("get AT client(%s) failed.", w60x->client_name);
			return -RT_ERROR;
	}
	/* register URC data execution function  */
  at_obj_set_urc_table(device->client, urc_table, sizeof(urc_table) / sizeof(urc_table[0]));
	if(w60x->rst_pin!=-1){
		rt_pin_mode(w60x->rst_pin,PIN_MODE_OUTPUT);
		rt_pin_write(w60x->rst_pin,0);
	}
	if(w60x->status_pin!=-1){
		rt_pin_mode(w60x->status_pin,PIN_MODE_INPUT);
	}
#ifdef AT_USING_SOCKET
  w60x_socket_init(device);
#endif
	/* add w60x device to the netdev list */
	device->netdev = w60x_netdev_add(w60x->device_name);
	if (device->netdev == RT_NULL)
	{
			LOG_E("add netdev(%s) failed.", w60x->device_name);
			return -RT_ERROR;
	}

	/* initialize w60x device network */
	do{
		w60x_netdev_set_up(device->netdev);
	}while(device->is_init==RT_TRUE);
	return RT_EOK;
}

static int w60x_deinit(struct at_device *device)
{
	return 0;
}

/* reset w60x device and initialize device network again */
static int w60x_reset(struct at_device *device)
{ 
#define W60X_WAIT_CONNECT_TIME  5000
	int result = RT_EOK;
	struct at_device_w60x * w60x=device->user_data;
	struct at_client *client = device->client;
	if(w60x->rst_pin!=-1){
		LOG_D("hardware reset");
		rt_pin_write(w60x->rst_pin,1);
		rt_thread_mdelay(100);
		rt_pin_write(w60x->rst_pin,0);
	}else{
		LOG_D("software reset");
		/* send "AT+RST" commonds to esp8266 device */
    result = at_obj_exec_cmd(client, RT_NULL, "AT+RST");
		
	}
	if(result==RT_EOK){				//复位成功
		rt_thread_mdelay(1000);
		if (at_client_obj_wait_connect(client, W60X_WAIT_CONNECT_TIME))			//测试链接成功
		{
				return -RT_ETIMEOUT;
		}
		device->is_init = RT_TRUE;																					//w60x 网卡初始化成功
	}else{
		LOG_E("%s device rst fail",device->name);
	}
	return result;
}

static int w60x_control(struct at_device *device, int cmd, void *arg)
{
    int result = -RT_ERROR;

    RT_ASSERT(device);

    switch (cmd)
    {
    case AT_DEVICE_CTRL_POWER_ON:
    case AT_DEVICE_CTRL_POWER_OFF:
    case AT_DEVICE_CTRL_LOW_POWER:
    case AT_DEVICE_CTRL_SLEEP:
    case AT_DEVICE_CTRL_WAKEUP:
    case AT_DEVICE_CTRL_NET_CONN:
    case AT_DEVICE_CTRL_NET_DISCONN:
    case AT_DEVICE_CTRL_GET_SIGNAL:
    case AT_DEVICE_CTRL_GET_GPS:
    case AT_DEVICE_CTRL_GET_VER:
		case AT_DEVICE_CTRL_SET_WIFI_INFO:
        LOG_W("not support the control cmd(%d).", cmd);
        break;
    case AT_DEVICE_CTRL_RESET:
        result = w60x_reset(device);
        break;
    default:
        LOG_E("input error control cmd(%d).", cmd);
        break;
    }

    return result;
}

static const struct at_device_ops w60x_device_ops =
{
    w60x_init,
    w60x_deinit,
    w60x_control,
};

static int w60x_device_class_register(void)
{
 struct at_device_class *class = RT_NULL;

    class = (struct at_device_class *) rt_calloc(1, sizeof(struct at_device_class));
    if (class == RT_NULL)
    {
        LOG_E("no memory for class create.");
        return -RT_ENOMEM;
    }

    /* fill ESP8266 device class object */
#ifdef AT_USING_SOCKET
    w60x_socket_class_register(class);
#endif
    class->device_ops = &w60x_device_ops;

    return at_device_class_register(class, AT_DEVICE_CLASS_W60X);
}
INIT_DEVICE_EXPORT(w60x_device_class_register);
#endif /*MY_AT_DEVICE_USING_W60X*/


