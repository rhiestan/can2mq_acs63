
#include "MQManager.h"
#include "config.h"

#include "mosquitto.h"

#include <cstdio>
#include <string>
#include <cstddef>
#include <cstring>

#if defined(_WIN32)
// To compile on Windows
int getpid()
{
   return 0;
}
#else
#include <unistd.h>
#endif


// Singleton instance
MQManager MQManager::instance_;

MQManager::MQManager()
{
}

MQManager::~MQManager()
{
   disconnect();
}

void MQManager::connect(const Config &config)
{
   char clientid[24];
   snprintf(clientid, 23, "can2mq_%d", getpid());

   mosquitto_lib_init();
   mosq_ = mosquitto_new(nullptr, true, 0);
   mosquitto_connect_callback_set(mosq_, on_connect);
   //mosquitto_publish_callback_set(mosq_, on_publish);

   int err = mosquitto_connect(mosq_, config.mqttURI.c_str(), config.mqttPort, 60);
   if(err != MOSQ_ERR_SUCCESS)
   {
      fprintf(stderr, "Error wile connecting: %s\n", mosquitto_strerror(err));
      mosquitto_destroy(mosq_);
      mosq_ = nullptr;
   }
   else
   {
      err = mosquitto_loop_start(mosq_);
      if(err != MOSQ_ERR_SUCCESS)
      {
         fprintf(stderr, "Error in loop_start: %s\n", mosquitto_strerror(err));
      }

   }
}

void MQManager::disconnect()
{
   if(mosq_ != nullptr)
   {
      mosquitto_loop_stop(mosq_, true);
      mosquitto_destroy(mosq_);
      mosq_ = nullptr;
      mosquitto_lib_cleanup();
   }
}

void MQManager::publish(const char *topic, const char *payload)
{
   if(mosq_ != nullptr && isConnected_)
   {
      int err = mosquitto_publish(mosq_, nullptr, topic, strlen(payload), payload, 0, false);
      if(err != MOSQ_ERR_SUCCESS)
         fprintf(stderr, "Error wile publishing: %s\n", mosquitto_strerror(err));
   }
}

void MQManager::on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
   /* Print out the connection result. mosquitto_connack_string() produces an
   * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
   * clients is mosquitto_reason_string().
   */
   printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
   if(reason_code != 0){
      /* If the connection fails for any reason, we don't want to keep on
      * retrying in this example, so disconnect. Without this, the client
      * will attempt to reconnect. */
      mosquitto_disconnect(instance_.mosq_);
      instance_.mosq_ = nullptr;
   }
   else
      instance_.isConnected_ = true;
}
