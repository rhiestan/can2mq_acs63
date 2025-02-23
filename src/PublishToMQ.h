
#ifndef PUBLISH_TO_MQ_H
#define PUBLISH_TO_MQ_H

class MQManager;

#include <string>
#include <chrono>
#include <vector>

/**
 * Publish energy data to MQ.
 */
class PublishToMQ
{
public:
   PublishToMQ(MQManager &mq, const char *topic, int interval);
   virtual ~PublishToMQ();

   void newEnergyData(const std::chrono::high_resolution_clock::time_point &tp, int totalPower, int phase1, int phase2, int phase3, float energy);

   static void publishToAll(const std::chrono::high_resolution_clock::time_point &tp, int totalPower, int phase1, int phase2, int phase3, float energy);
   static void addPublisher(PublishToMQ *publisher);

protected:
   void publishData(int totalPower, int phase1, int phase2, int phase3, float energy);

private:
   MQManager &mq_;
   std::string topic_;
   int interval_;
   std::chrono::high_resolution_clock::time_point nextPublishTime_;
   int numberOfMeasurements_{0};
   int totalPowerSum_{0};
   int phase1Sum_{0}, phase2Sum_{0}, phase3Sum_{0};
   double energySum_{0};

   static std::vector<PublishToMQ*> publisherList_;
};

#endif // !PUBLISH_TO_MQ_H
