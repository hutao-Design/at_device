#include <at_device_w60x.h>
#include <netdev.h>


#include "sys.h"


#define LOG_TAG                        "at.sample.w60x"
#include <at_log.h>

#define W60X_SAMPLE_DEIVCE_NAME     "w60x0"


static struct at_device_w60x w60x0 =
{
	W60X_SAMPLE_DEIVCE_NAME,
	W60X_SAMPLE_CLIENT_NAME,
	W60X_RST_PIN,
	W60X_STA_PIN,
	W60X_SAMPLE_RECV_BUFF_LEN,
};
const char * cb_Type[12]={
			"NETDEV_CB_ADDR_IP",
			"NETDEV_CB_ADDR_NETMASK",
			"NETDEV_CB_ADDR_GATEWAY",
			"NETDEV_CB_ADDR_DNS_SERVER",
			"NETDEV_CB_STATUS_UP",
			"NETDEV_CB_STATUS_DOWN",
			"NETDEV_CB_STATUS_LINK_UP",
			"NETDEV_CB_STATUS_LINK_DOWN",
			"NETDEV_CB_STATUS_INTERNET_UP",
			"NETDEV_CB_STATUS_INTERNET_DOWN",
			"NETDEV_CB_STATUS_DHCP_ENABLE",
			"NETDEV_CB_STATUS_DHCP_DISABLE",
};
void connect_server(void){
extern int httpClient_getSecurity(void);
extern int tcpClient_Init(void);
	if(gSysParameter.serverReg.Get_Status!=RT_TRUE){
		httpClient_getSecurity();
	}else{
		tcpClient_Init();
	}
}
void disconnect_server(void){
extern int tcpClient_deinit(void);
extern int httpClient_deinit(void);	
	tcpClient_deinit();
	httpClient_deinit();
}



static void netdev_status_callback(struct netdev *netdev, enum netdev_cb_type type){
	LOG_I("netdev(%s) event=%s",netdev->name,cb_Type[type]);
	switch(type){
		case NETDEV_CB_STATUS_LINK_UP:
			//创建链接线程
			connect_server();
			break;
		case NETDEV_CB_STATUS_LINK_DOWN:
			disconnect_server();
		default:
			break;
	}
}

static int w60x_device_register(void){
	struct at_device_w60x *w60x = &w60x0;
	int ret=RT_EOK;
	ret = at_device_register(&(w60x->device),
														w60x->device_name,
														w60x->client_name,
														AT_DEVICE_CLASS_W60X,
														(void *) w60x);
	netdev_set_status_callback(w60x->device.netdev,netdev_status_callback);
	return ret;
}
INIT_APP_EXPORT(w60x_device_register);




