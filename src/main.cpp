#include <Homie.h>
#include <Bounce2.h>
#include <EEPROM.h>

const int SHUTTER_PIN_BUTTON_0 = 3;
const int SHUTTER_PIN_BUTTON_1 = 9;
const int SHUTTER_PIN_RELAY_0 = 12;
const int SHUTTER_PIN_RELAY_1 = 5;
const int BUTTON_PIN_CASE = 10;
const int LED_PIN_STATUS = 13;

const int DEBOUNCE_INTERVAL = 60;
const int RELAY_MOMENTARY_INTERVAL = 1000;

Bounce button0Debouncer;
Bounce button1Debouncer;

int publishingButton0 = -1;
int publishingButton1 = -1;
int button0State = -1;
int button1State = -1;

unsigned long momentary0 = 0;
unsigned long momentary1 = 0;

uint8_t rebootCount = 0;
unsigned long disconnectedTime = 0;
unsigned long const maxDisconnectedTime = 20000;

HomieNode myNode("dual", "sonoffdual", "dual");

void activateRelay(int relay, bool activated)
{
  if(relay < 0 || relay > 1)
  {
    return;
  }

  int relayPin = relay == 0 ? SHUTTER_PIN_RELAY_0 : SHUTTER_PIN_RELAY_1;

  digitalWrite(relayPin, activated ? HIGH : LOW);
}

bool relay0Handler(const HomieRange& range, const String& value)
{
  bool valueBool = (value == "1");

  activateRelay(0, valueBool);

  return true;
}

bool relay1Handler(const HomieRange& range, const String& value)
{
  bool valueBool = (value == "1");

  activateRelay(1, valueBool);

  return true;
}

bool relay0momentaryHandler(const HomieRange& range, const String& value)
{
  activateRelay(0, true);
  momentary0 = millis();

  return true;
}

bool relay1momentaryHandler(const HomieRange& range, const String& value)
{
  activateRelay(1, true);
  momentary1 = millis();

  return true;
}

void readRebootCount() {
  rebootCount = EEPROM.read(0);
}

void writeRebootCount() {
  EEPROM.write(0, rebootCount);
  EEPROM.commit();
}

void loopHandler() {
  if(publishingButton0 != -1)
  {
    myNode.setProperty("button0").send(String(publishingButton0 == 1));
    publishingButton0 = -1;
  }

  if(publishingButton1 != -1)
  {
    myNode.setProperty("button1").send(String(publishingButton1 == 1));
    publishingButton1 = -1;
  }
}

void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::MQTT_READY:
      rebootCount = 0;
      writeRebootCount();
      Serial.println("Connect back to normal");
      break;
    default:
      break;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial << endl << endl;

  readRebootCount();
  rebootCount++;
  writeRebootCount();

  Homie_setFirmware("EnemHomieSonoffDualSimple", "1.1.0");
  Homie.setLoopFunction(loopHandler);
  Homie.onEvent(onHomieEvent);
  Homie.setLedPin(LED_PIN_STATUS, LOW).setResetTrigger(BUTTON_PIN_CASE, LOW, 5000);
  Homie.setup();

  myNode.advertise("relay0").settable(relay0Handler);
  myNode.advertise("relay1").settable(relay1Handler);
  myNode.advertise("relay0momentary").settable(relay0momentaryHandler);
  myNode.advertise("relay1momentary").settable(relay1momentaryHandler);
  myNode.advertise("button0");
  myNode.advertise("button1");

  digitalWrite(SHUTTER_PIN_RELAY_0, LOW);
  digitalWrite(SHUTTER_PIN_RELAY_1, LOW);
  pinMode(SHUTTER_PIN_RELAY_0, OUTPUT);
  pinMode(SHUTTER_PIN_RELAY_1, OUTPUT);

  pinMode(SHUTTER_PIN_BUTTON_0, INPUT_PULLUP);
  pinMode(SHUTTER_PIN_BUTTON_1, INPUT_PULLUP);
  button0Debouncer.attach(SHUTTER_PIN_BUTTON_0);
  button1Debouncer.attach(SHUTTER_PIN_BUTTON_1);
  button0Debouncer.interval(DEBOUNCE_INTERVAL);
  button1Debouncer.interval(DEBOUNCE_INTERVAL);
}

void homieIndependentLoop()
{
  button0Debouncer.update();
  button1Debouncer.update();

  int currentButton0 = !button0Debouncer.read();
  int currentButton1 = !button1Debouncer.read();

  if(currentButton0 != button0State)
  {
    button0State = currentButton0;
    publishingButton0 = currentButton0;
  }

  if(currentButton1 != button1State)
  {
    button1State = currentButton1;
    publishingButton1 = currentButton1;
  }

  if(momentary0 != 0)
  {
    if(millis() - momentary0 >= RELAY_MOMENTARY_INTERVAL)
    {
      momentary0 = 0;
      activateRelay(0, false);
    }
  }

  if(momentary1 != 0)
  {
    if(millis() - momentary1 >= RELAY_MOMENTARY_INTERVAL)
    {
      momentary1 = 0;
      activateRelay(1, false);
    }
  }
}

void loop() {
  Homie.loop();

  if(rebootCount >= 3)
  {
    if(Homie.isConfigured())
    {
      if (Homie.isConnected())
      {
        disconnectedTime = 0;
      }
      else
      {
        if(disconnectedTime == 0)
        {
          disconnectedTime = millis();
        }
        else
        {
          if(millis() - disconnectedTime >= maxDisconnectedTime)
          {
            rebootCount = 0;
            writeRebootCount();
            Homie.reset();
          }
        }
      }
    }
  }

  homieIndependentLoop();
}
