// printf
#include <stdio.h>
// uintxx_t
#include <stdint.h>

#include <signal.h>
#include <stdlib.h>

#include <SocketCAN.h>
#include <SLCAN.h>

#include "mosquitto.h"

#include <string>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <chrono>

#include "config.h"


struct mosquitto *mosq{nullptr};

bool isconnected = false;
bool iscancelled = false;
std::chrono::high_resolution_clock::time_point last_measurement = std::chrono::high_resolution_clock::now();

void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
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
      mosquitto_disconnect(mosq);
   }
   else
      isconnected = true;
}
void on_publish(struct mosquitto *mosq, void *obj, int mid)
{
   //      printf("Message with mid %d has been published.\n", mid);
}

void connect_mqtt(const Config &config)
{
   char clientid[24];
   snprintf(clientid, 23, "can2mq_%d", getpid());

   mosquitto_lib_init();
   mosq = mosquitto_new(nullptr, true, 0);
   mosquitto_connect_callback_set(mosq, on_connect);
   mosquitto_publish_callback_set(mosq, on_publish);

   int err = mosquitto_connect(mosq, config.mqttURI.c_str(), config.mqttPort, 60);
   if(err != MOSQ_ERR_SUCCESS)
   {
      fprintf(stderr, "Error wile connecting: %s\n", mosquitto_strerror(err));
      mosquitto_destroy(mosq);
      mosq = nullptr;
   }
   else
   {
      err = mosquitto_loop_start(mosq);
      if(err != MOSQ_ERR_SUCCESS)
      {
         fprintf(stderr, "Error in loop_start: %s\n", mosquitto_strerror(err));
      }

   }
}

void disconnect_mqtt()
{
   if(mosq != nullptr)
   {
      mosquitto_loop_stop(mosq, true);
      mosquitto_destroy(mosq);
      mosquitto_lib_cleanup();
   }
}

void rx_handler(can_frame_t* frame)
{
   if(!isconnected)
      return;

   auto convertByteToInt16 = [](uint8_t high, uint8_t low) -> int16_t
      {
         return static_cast<int16_t>(
            (static_cast<uint16_t>(high) << 8) |
            static_cast<uint16_t>(low));
      };

   char payload[1024];

   if(frame->can_id == 0x301)
   {
      std::chrono::high_resolution_clock::time_point new_measurement = std::chrono::high_resolution_clock::now();
      float intervalSeconds = std::chrono::duration<float>(new_measurement - last_measurement).count();

      // Calculate consumed/produced energy since last measurement
      // power is in W, energy in Wh
      float power = static_cast<float>(convertByteToInt16(frame->data[6], frame->data[7]));
      float energy = power * intervalSeconds / 3600.0;

      const char *topic = "can2mq/power/raw";
      snprintf(payload, 1024, "{\"total_power\": %d,\"phase1\": %d,\"phase2\": %d,\"phase3\":%d,\"energy\": %f}",
         static_cast<int>(convertByteToInt16(frame->data[6], frame->data[7])),
         static_cast<int>(convertByteToInt16(frame->data[0], frame->data[1])),
         static_cast<int>(convertByteToInt16(frame->data[2], frame->data[3])),
         static_cast<int>(convertByteToInt16(frame->data[4], frame->data[5])),
         energy);
      int err = mosquitto_publish(mosq, nullptr, topic, strlen(payload), payload, 0, false);
      if(err != MOSQ_ERR_SUCCESS)
         fprintf(stderr, "Error wile publishing: %s\n", mosquitto_strerror(err));

      last_measurement = new_measurement;
   }
   else if(frame->can_id == 0x302)
   {
      const char *topic_pot = "can2mq/pot/raw";
      snprintf(payload, 1024, "{\"freq\": %d,\"phase1_pot\": %d,\"phase2_pot\": %d,\"phase3_pot\":%d}",
         static_cast<int>(convertByteToInt16(frame->data[6], frame->data[7])),
         static_cast<int>(convertByteToInt16(frame->data[0], frame->data[1])),
         static_cast<int>(convertByteToInt16(frame->data[2], frame->data[3])),
         static_cast<int>(convertByteToInt16(frame->data[4], frame->data[5])));
      int err = mosquitto_publish(mosq, nullptr, topic_pot, strlen(payload), payload, 0, false);
      if(err != MOSQ_ERR_SUCCESS)
         fprintf(stderr, "Error wile publishing: %s\n", mosquitto_strerror(err));
   }
}


static void catch_function(int signo)
{
   printf("Received termination request...\n");
   iscancelled = true;
}

void run_socketcan_loop(const Config &config)
{
   SocketCAN* adapter = new SocketCAN();
   adapter->open(const_cast<char*>(config.canDevice.c_str()));

   adapter->reception_handler = &rx_handler;

   while(!iscancelled)
   {
      sleep(1);
   }

   adapter->close();

   delete adapter;
}

int main(int argc, char **argv)
{
   if (signal(SIGINT, catch_function) == SIG_ERR) {
      fputs("An error occurred while setting a signal handler.\n", stderr);
      return EXIT_FAILURE;
   }

   Config config;

   if(argc > 1)
      config.load(std::filesystem::path(argv[1]));

   connect_mqtt(config);

   run_socketcan_loop(config);

   disconnect_mqtt();

   return 0;
}
