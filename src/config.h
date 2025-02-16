/**
 * config.h: Contains the config class for can2mq
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <filesystem>


/**
 * Contains configuration parameters.
 */
class Config
{
public:
   Config() {}
   bool load(const std::filesystem::path &p);

   std::string mqttURI;
   int mqttPort;

   std::string prepareCommand;
   std::string publishTopic;

   std::string canDevice;

private:
   Config(const Config&) = delete;
   Config(const Config&&) = delete;
};

#endif // !CONFIG_H
