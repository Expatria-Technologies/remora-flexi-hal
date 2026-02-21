
#include "jsonConfigHandler.h"
#include "../remora.h"
#include "crc32.h"

volatile bool JsonConfigHandler::new_flash_json = false;

JsonConfigHandler::JsonConfigHandler(Remora* _remora) :
	remoraInstance(_remora)
{
	uint8_t status = loadConfiguration();
    remoraInstance->setStatus(status);
    if (status != 0x00) {
        return;
    }
    updateThreadFreq();
}

uint8_t JsonConfigHandler::loadConfiguration() {
	// Clear any existing configuration
    jsonContent.clear();
    doc.clear();

    // Read and parse the configuration file
    uint8_t status;

    #ifdef ETH_CTRL
        status = readConfigFromFlash();
    #else
        status = readConfigFromSD();
    #endif

    if (status != 0x00) {
        return status;
    }

    status = parseJson();
    if (status != 0x00) {
        return status;
    }

    return status;
}

void JsonConfigHandler::updateThreadFreq() {

    JsonArray Threads = doc["Threads"];

    if (!Threads.isNull()) 
    {
        // create objects from JSON data if set in config
        for (JsonArray::iterator it=Threads.begin(); it!=Threads.end(); ++it) 
        {
            thread = *it;
            const char* configor = thread["Thread"];
            uint32_t    freq = thread["Frequency"];
            if (!strcmp(configor,"Base")) {
                printf("Updating thread frequency - Setting BASE thread frequency to %lu\n", freq);
                remoraInstance->setBaseFreq(freq);
            }
            else if (!strcmp(configor,"Servo")) {
                printf("Updating thread frequency - Setting SERVO thread frequency to %lu\n", freq);
                remoraInstance->setServoFreq(freq);
            }
        }
    }
    else 
    {
        printf("BASE thread frequency set to: %lu\n", remoraInstance->getBaseFreq());
        printf("SERVO thread frequency set to: %lu\n", remoraInstance->getServoFreq());
    }
}

JsonArray JsonConfigHandler::getModules() {
	if (doc["Modules"].is<JsonVariant>())
        return doc["Modules"].as<JsonArray>();
    else
        return JsonArray();
}

uint8_t JsonConfigHandler::readConfigFromSD() {

	uint32_t bytesread; // bytes read count

    printf("\nReading JSON configuration file\n");

    // Try to mount the file system
    printf("Mounting the file system... \n");
    if(f_mount(&SDFatFS, (TCHAR const*)SDPath, 0) != FR_OK)
	{
    	printf("Failed to mount SD card\n\r");
    	return makeRemoraStatus(RemoraErrorSource::JSON_CONFIG, RemoraErrorCode::SD_MOUNT_FAILED, true);
	}

    //Open file for reading
    if(f_open(&SDFile, filename, FA_READ) != FR_OK)
    {
        printf("Failed to open JSON config file\n\n");
        return makeRemoraStatus(RemoraErrorSource::JSON_CONFIG, RemoraErrorCode::CONFIG_FILE_OPEN_FAILED, true);
    }

    int32_t length = f_size(&SDFile);
    printf("JSON config file length = %2ld\n", length);

    __attribute__((aligned(32))) char rtext[length];
    if(f_read(&SDFile, rtext, length, (UINT *)&bytesread) != FR_OK)
    {
        printf("JSON config file read FAILURE\n\n");
        f_close(&SDFile);
		return makeRemoraStatus(RemoraErrorSource::JSON_CONFIG, RemoraErrorCode::CONFIG_FILE_READ_FAILED, true);
    }

    printf("JSON config file read SUCCESS!\n\n");
    // put JSON char array into std::string
    jsonContent.reserve(length + 1);
    for (int i = 0; i < length; i++) {
        jsonContent = jsonContent + rtext[i];
    }

    // Remove comments from next line to print out the JSON config file
    //printf("\n%s\n", jsonContent.c_str());

    f_close(&SDFile);

	return makeRemoraStatus(RemoraErrorSource::NO_ERROR, RemoraErrorCode::NO_ERROR);
}

uint8_t JsonConfigHandler::readConfigFromFlash() {
    uint32_t jsonLength;

    printf("\nLoading JSON configuration file from Flash memory\n");

    // read byte 0 to determine length to read
    jsonLength = *(uint32_t*)(Platform_Config::JSON_storage_start_address);

    if (jsonLength == 0xFFFFFFFF)
    {
    	printf("Flash storage location is empty - no config file\n");
    	printf("Loading default configuration\n\n");

		for (uint32_t i = 0; i < sizeof(Config::defaultConfig); i++)
		{
			jsonContent.push_back(Config::defaultConfig[i]);
		}
        return makeRemoraStatus(RemoraErrorSource::NO_ERROR, RemoraErrorCode::CONFIG_LOADED_DEFAULT);
    }
    else
    {
        jsonContent.reserve(jsonLength);

		for (uint32_t i = 0; i < jsonLength; i++)
		{
            uint32_t json_config_addr = Platform_Config::JSON_storage_start_address + sizeof(json_metadata_t::jsonLength) + i;
            jsonContent[i] = *reinterpret_cast<uint8_t*>(json_config_addr);
		}
    }

	//printf("JSON Content: \n%s\n\n", jsonContent.c_str());
    return makeRemoraStatus(RemoraErrorSource::NO_ERROR, RemoraErrorCode::NO_ERROR);
}

uint8_t JsonConfigHandler::parseJson() {
	
	printf("\nParsing JSON configuration file\n");
	
    // Clear any existing parsed data
    doc.clear();

    // Parse JSON
    DeserializationError error = deserializeJson(doc, jsonContent.c_str());

    printf("Config deserialisation - ");

    switch (error.code())
    {
        case DeserializationError::Ok:
            printf("Deserialization succeeded\n\n");
            return makeRemoraStatus(RemoraErrorSource::NO_ERROR, RemoraErrorCode::NO_ERROR);

        case DeserializationError::InvalidInput:
            printf("Invalid input!\n\n");
            return makeRemoraStatus(RemoraErrorSource::JSON_CONFIG, RemoraErrorCode::CONFIG_INVALID_INPUT, true);

        case DeserializationError::NoMemory:
            printf("Not enough memory\n\n");
            return makeRemoraStatus(RemoraErrorSource::JSON_CONFIG, RemoraErrorCode::CONFIG_NO_MEMORY, true);

        default:
            printf("Deserialization failed: ");
            printf(error.c_str());
            printf("\n\n");
            return makeRemoraStatus(RemoraErrorSource::JSON_CONFIG, RemoraErrorCode::CONFIG_PARSE_FAILED, true);
    }

    return makeRemoraStatus(RemoraErrorSource::NO_ERROR, RemoraErrorCode::NO_ERROR);
}

int8_t JsonConfigHandler::json_check_length_and_CRC(void) 
{
	json_metadata_t* meta = (json_metadata_t*)Platform_Config::JSON_upload_start_address;

    uint32_t table[256];
    crc32::generate_table(table);
    int mod, padding;

	// Check length is reasonable
	if (meta->length > (Platform_Config::JSON_upload_end_address - Platform_Config::JSON_upload_start_address))
	{
		JsonConfigHandler::new_flash_json = false;
		printf("JSON Config length incorrect\n");
		return -1;
	}

    // for compatability with STM32 hardware CRC32, the config is padded to a 32 byte boundary
    mod = meta->jsonLength % 4;
    if (mod > 0)
    {
        padding = 4 - mod;
    }
    else
    {
        padding = 0;
    }
    printf("mod = %d, padding = %d\n", mod, padding);

	// Compute CRC
    char* json_upload_data_ptr = (char *)(Platform_Config::JSON_upload_start_address + metadata_len);
    uint32_t computed_crc32 = crc32::update(table, 0xFFFFFFFF, json_upload_data_ptr, meta->jsonLength + padding);

	printf("Length (words) = %d\n", (int)meta->length);
	printf("JSON length (bytes) = %d\n", (int)meta->jsonLength);
	printf("Expected crc32 = 0x%x\n", (unsigned int)meta->crc32);
	printf("Computed crc32 = 0x%x\n", (unsigned int)computed_crc32);

	// Check CRC
	if (computed_crc32 != meta->crc32)
	{
		JsonConfigHandler::new_flash_json = false;
		printf("JSON Config file CRC incorrect\n");
		return -1;
	}

	// JSON is OK, don't check it again
	JsonConfigHandler::new_flash_json = false;
	printf("JSON Config file received Ok\n");
	return 1;
}

#ifdef ETH_CTRL
void JsonConfigHandler::store_json_in_flash(void)
{
	uint32_t i = 0;
	json_metadata_t* meta = (json_metadata_t*)Platform_Config::JSON_upload_start_address;  // unsure if this going to work with how we have split the sector in two. 
    
	uint16_t jsonLength = meta->jsonLength;

    printf("JSON Length: %d\n", jsonLength);

    unlock_flash();

	// erase the old JSON config file
    printf("Erasing storage area of flash\n");
    mass_erase_config_storage();
	uint8_t status;

	// store the length of the file in the 0th byte
    printf("Write file length\n");
	status = write_to_flash_word(Platform_Config::JSON_storage_start_address, jsonLength);

    jsonLength = *(uint32_t*)(Platform_Config::JSON_storage_start_address);
    printf("Written JSON Length %d\n", jsonLength);

    jsonLength = meta->jsonLength;

    printf("Copying data\n");
    for (i = 0; i < jsonLength; i++)
    {
        if (status == 0)
        {
            status = write_to_flash_byte((Platform_Config::JSON_storage_start_address + 4 + i), *((uint8_t*)(Platform_Config::JSON_upload_start_address + metadata_len + i)));
        }
    }

    lock_flash();
    printf("Data Copied\n");

}
#endif