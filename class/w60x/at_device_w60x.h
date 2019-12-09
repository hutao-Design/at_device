#ifndef _AT_DEVICE_W60X_H_
#define _AT_DEVICE_W60X_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
	
#include <at_device.h>
	
/* The maximum number of sockets supported by the w60x device */
#define AT_DEVICE_W60X_SOCKETS_NUM  20

struct at_device_w60x
{
    char *device_name;
    char *client_name;
		int rst_pin;
		int status_pin;
    size_t recv_line_num;
    struct at_device device;

    void *user_data;
};

#ifdef AT_USING_SOCKET
	
/* w60x device socket initialize */
int w60x_socket_init(struct at_device *device);

/* w60x device class socket register */
int w60x_socket_class_register(struct at_device_class *class);

#endif /* AT_USING_SOCKET */	
	
#ifdef __cplusplus
}
#endif

#endif






