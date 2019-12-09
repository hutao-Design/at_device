#include <at_device_w60x.h>

#define LOG_TAG                        "at.sample.esp"
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

static int w60x_device_register(void){
	struct at_device_w60x *w60x = &w60x0;

	return at_device_register(&(w60x->device),
														w60x->device_name,
														w60x->client_name,
														AT_DEVICE_CLASS_W60X,
														(void *) w60x);
}
INIT_APP_EXPORT(w60x_device_register);


