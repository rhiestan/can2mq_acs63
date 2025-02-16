/**
* config.cpp: Contains the config class for can2mq
*/
#include "config.h"

#include <nlohmann/json.hpp>
#include <fstream>

bool Config::load(const std::filesystem::path &p)
{
   if(!std::filesystem::exists(p))
      return false;
   std::ifstream configFileStream(p);
   if(!configFileStream.is_open())
      return false;
   try
   {
      nlohmann::json j = nlohmann::json::parse(configFileStream);

      mqttURI = j["MQTT_URI"].get<std::string>();
      mqttPort = j["MQTT_Port"].get<int>();
      prepareCommand = j["prepareCommand"].get<std::string>();
      publishTopic = j["publishTopic"].get<std::string>();
      canDevice = j["canDevice"].get<std::string>();
   }
   catch(nlohmann::json::parse_error &e)
   {
      return false;
   }

   return true;
}
