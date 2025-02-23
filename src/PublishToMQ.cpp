
#include "PublishToMQ.h"
#include "MQManager.h"

std::vector<PublishToMQ*> PublishToMQ::publisherList_;

PublishToMQ::PublishToMQ(MQManager &mq, const char *topic, int interval)
   : mq_{mq}, topic_(topic), interval_{interval}
{
   nextPublishTime_ = std::chrono::high_resolution_clock::now();
}

PublishToMQ::~PublishToMQ()
{
}

void PublishToMQ::newEnergyData(const std::chrono::high_resolution_clock::time_point &tp, int totalPower, int phase1, int phase2, int phase3, float energy)
{
   if(interval_ <= 0)
      publishData(totalPower, phase1, phase2, phase3, energy);
   else
   {
      totalPowerSum_ += totalPower;
      phase1Sum_ += phase1;
      phase2Sum_ += phase2;
      phase3Sum_ += phase3;
      numberOfMeasurements_++;

      // Formula see https://math.stackexchange.com/questions/106700/incremental-averaging
      //energyAverage_ += (static_cast<double>(energy) - energyAverage_) / static_cast<double>(numberOfMeasurements_);
      energySum_ += static_cast<double>(energy);

      if(tp >= nextPublishTime_)
      {
         publishData(totalPowerSum_ / numberOfMeasurements_, phase1Sum_ / numberOfMeasurements_,
            phase2Sum_ / numberOfMeasurements_, phase3Sum_ / numberOfMeasurements_, static_cast<float>(energySum_));

         numberOfMeasurements_ = 0;
         totalPowerSum_ = phase1Sum_ = phase2Sum_ = phase3Sum_ = 0;
         energySum_ = 0;
         nextPublishTime_ += std::chrono::seconds{interval_};
      }
   }
}

void PublishToMQ::publishToAll(const std::chrono::high_resolution_clock::time_point &tp, int totalPower, int phase1, int phase2, int phase3, float energy)
{
   for(auto &publisher : publisherList_)
      publisher->newEnergyData(tp, totalPower, phase1, phase2, phase3, energy);
}

void PublishToMQ::addPublisher(PublishToMQ *publisher)
{
   publisherList_.push_back(publisher);
}

void PublishToMQ::publishData(int totalPower, int phase1, int phase2, int phase3, float energy)
{
   char payload[1024];

   const char *topic = "can2mq/power/raw";
   snprintf(payload, 1024, "{\"total_power\": %d,\"phase1\": %d,\"phase2\": %d,\"phase3\":%d,\"energy\": %f}",
      totalPower, phase1, phase2, phase3, energy);

   mq_.publish(topic_.c_str(), payload);
}
