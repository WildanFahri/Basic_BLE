#include <Arduino.h>
#include <WiFi.h>
#include "PubSubClient.h"
#include "BLEDevice.h"
// #include "BLEScan.h"

const char *ssid = "JTI-POLINEMA"; // Enter your WiFi name
const char *password = "jtifast!"; // Enter WiFi password
const char *usermqtt = "nqylskjy:nqylskjy";
const char *passmqtt = "C-qA-Iv_y-VrZ4Io9zf2Qgi3ly4ZRWc-";
const char *mqttServer = "armadillo.rmq.cloudamqp.com";
const char *publish = "publish/Atlet1";
const int mqtt_port = 1883;
WiFiClient espClient;
PubSubClient client(espClient);

static BLEUUID serviceUUID("0000180d-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID(BLEUUID((uint16_t)0x2A37));

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static boolean notification = false;
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;

static void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);

  if (length == 2)
  {
    // int data = pData[1], DEC;
    Serial.print("heart Rate: ");
    Serial.print(pData[1], DEC);
    Serial.println("bpm");

    // Serial.print("pData: ");
    // Serial.println(pData[1]);

    char str[8];
    client.publish(publish, itoa(pData[1], str, 10));
  }
  // Serial.print("data: ");
  // Serial.write(pData, length);
  // Serial.println();
}

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
  }

  void onDisconnect(BLEClient *pclient)
  {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer()
{
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  pClient->connect(myDevice); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");
  pClient->setMTU(517); // set client to request maximum MTU from server (default is 23 otherwise)

  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  if (pRemoteCharacteristic->canRead())
  {
    std::string value = pRemoteCharacteristic->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  connected = true;
  return true;
}
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID))
    {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");

  client.setServer(mqttServer, mqtt_port);

  while (!client.connected())
  {
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());

    Serial.printf("The client %s connects to mosquitto mqtt broker\n", client_id.c_str());

    if (client.connect(client_id.c_str(), usermqtt, passmqtt))
    {
      Serial.println("Public emqx mqtt broker connected");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  delay(1000);
}

void loop()
{

  if (doConnect == true)
  {
    if (connectToServer())
    {
      Serial.println("We are now connected to the BLE Server.");
    }
    else
    {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }
  if (connected)
  {
    if (notification == false)
    {
      Serial.println("Turning Notification On");
      // const uint8_t onPacket[] = {0x1, 0x0}; // Modify the packet data if needed
      // pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2092))->writeValue((uint8_t*)onPacket, 2, true);
      // notification = true;
      const uint8_t indicationOn[] = {0x1, 0x0};
      const uint8_t indicationOff[] = {0x0, 0x0};
      BLERemoteDescriptor *p2902 = pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902));
      if (p2902 != nullptr)
      {
        p2902->writeValue((uint8_t *)indicationOn, 2, true);
      }
    }
  }
  delay(1000);
}