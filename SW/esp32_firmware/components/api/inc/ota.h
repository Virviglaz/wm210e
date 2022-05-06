#ifndef __OTA_H__
#define __OTA_H__

#include <stdint.h>

/* FreeRTOS */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

/**
 * @brief OTA initialization settings struct.
 * 
 */
typedef struct {
	/** Application name */
	const char *app_name;

	/** Wifi access point name */
	const char *wifi_uuid;

	/** Wifi access point password */
	const char *wifi_pass;

	/** OTA server IP address */
	const char *server_ip;

	/** OTA server port */
	uint32_t port;
} ota_h;


/**
 * @brief Over-the-air self firmware update class.
 * 
 */
class ota
{
public:
	/**
	 * @brief Create the ota object.
	 * 
	 * @param settings Pointer to the OTA setting struct.
	 */
	ota(ota_h *settings);

	/**
	 * @brief Destroy the ota object
	 * 
	 */
	~ota();
private:
};

#endif /* __OTA_H__ */
