#include <Homie.h>
#include <Bounce2.h>

const int SHUTTER_PIN_BUTTON_0 = 0;
const int SHUTTER_PIN_BUTTON_1 = 9;
const int SHUTTER_PIN_RELAY_0 = 12;
const int SHUTTER_PIN_RELAY_1 = 5;
const int BUTTON_PIN_CASE = 10;
const int LED_PIN_STATUS = 13;

const int DEBOUNCE_INTERVAL = 60;

Bounce button0Debouncer;
Bounce button1Debouncer;

int publishingButton0 = -1;
int publishingButton1 = -1;
int button0State = -1;
int button1State = -1;

HomieNode myNode("dual", "sonoffdual");

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
  bool valueBool = (value == "true");

  activateRelay(0, valueBool);

  return true;
}

bool relay1Handler(const HomieRange& range, const String& value)
{
  bool valueBool = (value == "true");

  activateRelay(1, valueBool);

  return true;
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

void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial << endl << endl;

  Homie_setFirmware("sonoffdual-simple", "1.0.2");
  Homie.setLoopFunction(loopHandler);
  Homie.setLedPin(LED_PIN_STATUS, LOW).setResetTrigger(BUTTON_PIN_CASE, LOW, 5000);
  Homie.setup();

  myNode.advertise("relay0").settable(relay0Handler);
  myNode.advertise("relay1").settable(relay1Handler);
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
}

void loop() {
  Homie.loop();
  homieIndependentLoop();
}
