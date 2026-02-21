#ifndef JSON_CONFIG_HANDLER_H
#define JSON_CONFIG_HANDLER_H

#include <string>
#include <ArduinoJson.h>
#include "fatfs.h"

#ifdef ETH_CTRL
#include "remora-hal/hal_utils.h"
#endif

class Remora; //forward declaration

constexpr uint16_t metadata_len = 512; // bytes
constexpr uint16_t metadata_padding_len = metadata_len - (sizeof(uint32_t) * 3);

typedef struct __attribute__((packed))
{
  uint32_t crc32;   		// crc32 of JSON
  uint32_t length;			// length in words for CRC calculation
  uint32_t jsonLength;  	// length in of JSON config in bytes
  uint8_t padding[metadata_padding_len];		// ensure struct size = metadata_len in bytes, as this is how TFTP is loading in packets. 
} json_metadata_t;

class JsonConfigHandler {
private:

	Remora* remoraInstance;
	std::string jsonContent = "";
	const char* filename = "config.txt";
	JsonDocument doc;
	JsonObject thread;
	//bool configError;
	uint8_t loadConfiguration();
	uint8_t readConfigFromSD();
	uint8_t readConfigFromFlash();	
	uint8_t parseJson();

public:
	static volatile bool new_flash_json;

	JsonConfigHandler(Remora* _remora);
	void updateThreadFreq();
	JsonArray getModules();
	JsonObject getModuleConfig(const char* threadName, const char* moduleType);	

	int8_t json_check_length_and_CRC(void);
	#ifdef ETH_CTRL
	void store_json_in_flash(void);
	#endif
};
#endif
