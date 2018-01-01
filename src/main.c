#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event_loop.h"

#include "nvs_flash.h"

static const char *TAG = "chip_info";

SemaphoreHandle_t mutex;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
  switch(event->event_id)
  {

  case SYSTEM_EVENT_STA_START:
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
    ESP_ERROR_CHECK(esp_wifi_connect());
    break;

  case SYSTEM_EVENT_STA_GOT_IP:
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
    ESP_LOGI(TAG, "got ip:%s\n",
             ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
    break;

  case SYSTEM_EVENT_STA_DISCONNECTED:
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
    ESP_ERROR_CHECK(esp_wifi_connect());
    break;

  default:
    break;
  }

  return ESP_OK;
}

static void wifi_start(void)
{
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_config_t wifi_config =
  {
    .sta =
    {
      .ssid = MY_WIFI_SSID,
      .password = MY_WIFI_PASSWORD,
    },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "*** WIFI_START finished ***");
}


void secondTask( void * parameter )
{
    xSemaphoreTake( mutex, portMAX_DELAY );
    printf("Running core           : %d\n", xPortGetCoreID() );
    xSemaphoreGive( mutex );
    vTaskDelete(NULL);
}


void app_main()
{
  esp_err_t ret = nvs_flash_init();
  mutex = xSemaphoreCreateMutex();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }

  ESP_ERROR_CHECK( ret );

  vTaskDelay(3000/portTICK_PERIOD_MS); // wait for serial monitor

  puts( "\n\nESP32 Chip Info - IDF - Version 1.0.5 by Dr. Thorsten Ludewig");
  puts( "Build date: " __DATE__ " " __TIME__ "\n");

  printf( "esp idf version : %s\n", esp_get_idf_version());

  esp_chip_info_t chip_info;
  esp_chip_info( &chip_info );

  printf( "\nchip # of cores        : %d\n", chip_info.cores );
  printf( "chip revision          : %d\n", chip_info.revision );
  printf( "chip feature wifi bgn  : %s\n", ( chip_info.features & CHIP_FEATURE_WIFI_BGN ) ? "true" : "false" );
  printf( "chip feature ble       : %s\n", ( chip_info.features & CHIP_FEATURE_BLE ) ? "true" : "false" );
  printf( "chip feature bt        : %s\n", ( chip_info.features & CHIP_FEATURE_BT ) ? "true" : "false" );

  printf( "\nflash size             : %dMB (%s)\n", spi_flash_get_chip_size() / (1024 * 1024),
              (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
              
  printf( "free heap size         : %d\n", esp_get_free_heap_size() );

  printf( "Running core           : %d\n", xPortGetCoreID() );

  xTaskCreatePinnedToCore( &secondTask, "secondTask", 2048, NULL, 1, NULL, 1 );
  vTaskDelay(50/portTICK_PERIOD_MS);
  xSemaphoreTake( mutex, portMAX_DELAY );

  uint8_t macBuffer;
  uint8_t *mac = &macBuffer;
  esp_err_t error_code = esp_efuse_mac_get_default( mac );

  if ( error_code == ESP_OK )
  {
    printf("\nEFUSE default mac address : %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
  }
  else
  {
    puts( "\nERROR: Reading EFUSE default mac address.");
  }

  puts("");
  xSemaphoreGive( mutex );

  vTaskDelay(3000/portTICK_PERIOD_MS);

  wifi_start();

  ESP_LOGI(TAG, "*** APP_MAIN finished ***");
  fflush(stdout);
}
