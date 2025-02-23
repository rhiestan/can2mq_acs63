/**
 * MQManager manages the interface to MQ.
 */

#ifndef MQ_MANAGER_H
#define MQ_MANAGER_H

struct mosquitto;
class Config;

/**
 * Manages the interface to MQ.
 */
class MQManager
{
public:
   MQManager();
   virtual ~MQManager();
   void connect(const Config &config);
   void disconnect();
   void publish(const char *topic, const char *payload);

   bool isConnected() const { return isConnected_; }

   static MQManager &getInstance() { return instance_; }

protected:
   static void on_connect(struct mosquitto *mosq, void *obj, int reason_code);

private:
   struct mosquitto *mosq_{nullptr};
   bool isConnected_{false};

   static MQManager instance_;
};

#endif // !MQ_MANAGER_H
