// printf
#include <stdio.h>
// uintxx_t
#include <stdint.h>

#include <signal.h>
#include <stdlib.h>

#include <SocketCAN.h>
#include <SLCAN.h>

#include <string>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <chrono>
#include <vector>

#include "config.h"
#include "PublishToMQ.h"
#include "MQManager.h"


bool iscancelled = false;
std::chrono::high_resolution_clock::time_point last_measurement = std::chrono::high_resolution_clock::now();

void rx_handler(can_frame_t* frame)
{
   if(!MQManager::getInstance().isConnected())
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

      int totalPower = static_cast<int>(convertByteToInt16(frame->data[6], frame->data[7]));
      int phase1 = static_cast<int>(convertByteToInt16(frame->data[0], frame->data[1]));
      int phase2 = static_cast<int>(convertByteToInt16(frame->data[2], frame->data[3]));
      int phase3 = static_cast<int>(convertByteToInt16(frame->data[4], frame->data[5]));

      // Calculate consumed/produced energy since last measurement
      // power is in W, energy in Wh
      float power = static_cast<float>(convertByteToInt16(frame->data[6], frame->data[7]));
      float energy = power * intervalSeconds / 3600.0;

      //MQManager::getInstance().publish(topic, payload);
      PublishToMQ::publishToAll(new_measurement, totalPower, phase1, phase2, phase3, energy);

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

      MQManager::getInstance().publish(topic_pot, payload);
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
   
   MQManager mq;
   mq.connect(config);

   if(mq.isConnected())
   {
      PublishToMQ mqLive(mq, "can2mq/power/raw", -1);           // Publish all data live to MQ
      PublishToMQ mqOneSecond(mq, "can2mq_1s/power/raw", 1);    // Publish every second
      PublishToMQ mqFiveSeconds(mq, "can2mq_5s/power/raw", 5);  // Publish every 5 seconds
      PublishToMQ mqTenSeconds(mq, "can2mq_10s/power/raw", 10); // Publish every 10 seconds

      PublishToMQ::addPublisher(&mqLive);
      PublishToMQ::addPublisher(&mqOneSecond);
      PublishToMQ::addPublisher(&mqFiveSeconds);
      PublishToMQ::addPublisher(&mqTenSeconds);

      run_socketcan_loop(config);

      mq.disconnect();
   }

   return 0;
}
