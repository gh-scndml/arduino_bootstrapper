/*
  QueueManager.cpp - Managing MQTT queue
  
  Copyright (C) 2020 - 2021  Davide Perini
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of 
  this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
  copies of the Software, and to permit persons to whom the Software is 
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in 
  all copies or substantial portions of the Software.
  
  You should have received a copy of the MIT License along with this program.  
  If not, see <https://opensource.org/licenses/MIT/>.
*/

#include "QueueManager.h"


PubSubClient mqttClient(espClient);

/********************************** SETUP MQTT QUEUE **********************************/
void QueueManager::setupMQTTQueue(void (*callback)(char*, byte*, unsigned int)) {

  mqttClient.setServer(IPAddress(helper.getValue(mqttIP,'.',0).toInt(),
                                 helper.getValue(mqttIP,'.',1).toInt(),
                                 helper.getValue(mqttIP,'.',2).toInt(),
                                 helper.getValue(mqttIP,'.',3).toInt()), mqttPort.toInt());
  mqttClient.setCallback(callback);
  mqttClient.setBufferSize(MQTT_MAX_PACKET_SIZE);
  mqttClient.setKeepAlive(MQTT_KEEP_ALIVE);

}

/********************************** MQTT RECONNECT **********************************/
void QueueManager::mqttReconnect(void (*manageDisconnections)(), void (*manageQueueSubscription)(), void (*manageHardwareButton)()) {

  // how many attemps to MQTT connection
  mqttReconnectAttemp = 0;

  // Loop until we're reconnected
  while (!mqttClient.connected() && WiFi.status() == WL_CONNECTED) {

    #if (DISPLAY_ENABLED) 
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0,0);
    #endif
    if (mqttReconnectAttemp <= 20) {
      helper.smartPrintln(F("Connecting to"));
      helper.smartPrintln(F("MQTT Broker..."));
    }
    helper.smartDisplay();

    // Manage hardware button if any
    manageHardwareButton();

    // Attempt to connect to MQTT server with QoS = 1 (pubsubclient supports QoS 1 for subscribe only, published msg have QoS 0 this is why I implemented a custom solution)
    boolean mqttSuccess;
    if (mqttuser.isEmpty() || mqttpass.isEmpty()) {
      mqttSuccess = mqttClient.connect(helper.string2char(deviceName), 0, 1, 0, 0);
    } else {
      mqttSuccess = mqttClient.connect(helper.string2char(deviceName), helper.string2char(mqttuser), helper.string2char(mqttpass), 0, 1, 0, 0, 1);
    }
    if (mqttSuccess) {

      helper.smartPrintln(F(""));
      helper.smartPrintln(F("CONNECTED"));
      helper.smartPrintln(F(""));
      helper.smartPrintln(F("Reading data from"));
      helper.smartPrintln(F("the network..."));
      helper.smartDisplay();

      // Subscribe to MQTT topics
      manageQueueSubscription();

      delay(DELAY_2000);
      mqttReconnectAttemp = 0;

      // reset the lastMQTTConnection to off, will be initialized by next time update
      lastMQTTConnection = OFF_CMD;

    } else {

      helper.smartPrintln(F("MQTT attempts="));
      helper.smartPrintln(mqttReconnectAttemp);
      helper.smartDisplay();

      if (mqttReconnectAttemp > 15) {
        // if fastDisconnectionManagement we need to execute the callback immediately, 
        // example: power off a watering system can't wait MAX_RECONNECT attemps
        if (fastDisconnectionManagement) {
          manageDisconnections();
          helper.smartPrintln(F("Disconnecting WiFi."));
          WiFi.reconnect();
        }
      }

      // after MAX_RECONNECT attemps all peripherals are shut down
      if (mqttReconnectAttemp >= MAX_RECONNECT) {
        helper.smartPrintln(F("Max retry reached, powering off peripherals."));
        helper.smartDisplay();
        // Manage disconnections, powering off peripherals
        manageDisconnections();
        helper.smartPrintln(F("Disconnecting WiFi."));
        WiFi.reconnect();
      } else if (mqttReconnectAttemp > 10000) {
        mqttReconnectAttemp = 0;
      }
      mqttReconnectAttemp++;
      // Wait 500 millis before retrying
      delay(DELAY_500);

    }
  }

}

void QueueManager::queueLoop(void (*manageDisconnections)(), void (*manageQueueSubscription)(), void (*manageHardwareButton)()) {

  if (!mqttClient.connected()) {
    mqttReconnect(manageDisconnections, manageQueueSubscription, manageHardwareButton);
  }
  mqttClient.loop();

}

/********************************** SEND A MESSAGE ON THE QUEUE **********************************/
void QueueManager::publish(const char *topic, const char *payload, boolean retained) {

  mqttClient.publish(topic, payload, retained);

}

/********************************** SUBSCRIBE TO A QUEUE TOPIC **********************************/
void QueueManager::unsubscribe(const char *topic) {

  mqttClient.unsubscribe(topic);

}

/********************************** SUBSCRIBE TO A QUEUE TOPIC **********************************/
void QueueManager::subscribe(const char *topic) {

  mqttClient.subscribe(topic);

}

/********************************** SUBSCRIBE TO A QUEUE TOPIC **********************************/
void QueueManager::subscribe(const char *topic, uint8_t qos) {

  mqttClient.subscribe(topic, qos);

}

