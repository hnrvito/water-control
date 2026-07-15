
// =====================================================
// V2 WEB SERVER (AUTO-GENERATED STARTER)
// NOTE:
// - File ini adalah basis migrasi dari Blynk ke WebServer.
// - Endpoint dan dashboard web masih perlu integrasi lanjutan.
// =====================================================


#include <time.h>
const char* ntpServer = "pool.ntp.org";

const long gmtOffset_sec = 8 * 3600; // WITA
const int daylightOffset_sec = 0;
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <Update.h>
#include <WiFiClientSecure.h>
String alarmText = "SISTEM NORMAL";
String alarmColor = "#28a745";
WebServer server(80);
WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

const char* mqtt_server =
"7670300b39ba4d55a0ec6d11ce773787.s1.eu.hivemq.cloud";

const int mqtt_port = 8883;

const char* mqtt_user = "hnrvito";
const char* mqtt_pass = "Jangandiingat1";

unsigned long lastMQTTStatus = 0;
unsigned long lastMQTTPublish = 0;
Preferences prefs;
char ssid[] = "ADEEVA";
char pass[] = "ZHAFRAN01";
bool pumpRunning = false;
bool lowBakAlarm = false;
unsigned long autoPumpStartTime = 0;
unsigned long autoPumpDuration = 0;
// =====================================================
// KONTROL AIR ESP32
// VERSION : V1.1 FINAL
// DATE : 06-06-2026
// =====================================================

// =====================================================
// KONTROL AIR ESP32 V1 FINAL FIXED
// =====================================================

#include <Arduino.h>
// =====================================================
// INPUT
// =====================================================

#define FLOAT_BAK        13
#define FLOAT_TANDON     14

#define FLOW_SENSOR      27
#define TRIG_PIN 32
#define ECHO_PIN 33

float jarakBAK = 0;
float tinggiAirBAK = 0;
float levelBAK = 0;

// =====================================================
// RELAY ACTIVE LOW
// =====================================================

#define RLY_BYPASS_OPEN      16
#define RLY_BYPASS_CLOSE     17

#define RLY_SOURCE_PDAM      18
#define RLY_SOURCE_BAK       19

#define RLY_DIST_BAK         21
#define RLY_DIST_TANDON      22

#define RLY_PUMP             23
#define RLY_STATUS           25
#define RLY_ALARM_LOWBAK     26
#define BTN_AUTO             5
#define BTN_STOP             4
// =====================================================
// TIMER
// =====================================================

const unsigned long VALVE_TIME = 20000UL;

const unsigned long FLOAT_CONFIRM_TIME = 5000UL;

const unsigned long NO_FLOW_PDAM_TIME = 60000UL;

const unsigned long FLOW_RETURN_TIME = 120000UL;

const unsigned long NO_FLOW_TANDON_TIME = 60000UL;

const unsigned long BAK_TIMEOUT = 7200000UL;

const unsigned long TANDON_TIMEOUT = 7200000UL;

// =====================================================
// MODE
// =====================================================

enum Mode
{
  MODE_IDLE,

  MODE_AUTO,

  MODE_MANUAL,

  MODE_ERROR
};

Mode currentMode = MODE_IDLE;
String nextScheduleString = "11/06/2026 21:30";
// =====================================================
// AUTO STATE
// =====================================================

enum AutoState
{
  AUTO_IDLE,

  AUTO_START_1,

  AUTO_START_2,

  AUTO_START_WAIT,

  AUTO_FILL_BAK,

  AUTO_PDAM_FAIL,

  AUTO_WAIT_PDAM_RETURN,

  AUTO_SWITCH_TANDON,

  AUTO_SWITCH_TANDON_WAIT,

  AUTO_FILL_TANDON,

  AUTO_STOP,

  AUTO_FINISHED
};

AutoState autoState = AUTO_IDLE;
// =====================================================
// ERROR STATE
// =====================================================

enum ErrorState
{
  ERR_NONE,

  ERR_BAK_TIMEOUT,

  ERR_TANDON_TIMEOUT,

  ERR_FLOW_ERROR
};

ErrorState errorState = ERR_NONE;
// =====================================================
// BYPASS POSITION
// =====================================================

enum BypassPos
{
  BYPASS_UNKNOWN,

  BYPASS_OPEN,

  BYPASS_CLOSE
};

// =====================================================
// SOURCE POSITION
// =====================================================

enum SourcePos
{
  SOURCE_UNKNOWN,

  SOURCE_PDAM,

  SOURCE_BAK
};

// =====================================================
// DISTRIBUSI POSITION
// =====================================================

enum DistPos
{
  DIST_UNKNOWN,

  DIST_BAK,

  DIST_TANDON
};
BypassPos bypassPos = BYPASS_UNKNOWN;

SourcePos sourcePos = SOURCE_UNKNOWN;
bool countPDAM = false;

unsigned long stopCountTimer = 0;

DistPos distPos = DIST_UNKNOWN;
// =====================================================
// FLOAT LATCH
// =====================================================

bool bakFullLatched = false;

bool tandonFullLatched = false;
// =====================================================
// STARTUP STATUS
// =====================================================

bool startupComplete = false;
// =====================================================
// FLOWMETER YF-B6
// =====================================================

volatile unsigned long pulseCount = 0;

float flowRate = 0.0;

float totalLiter = 0.0;
// =====================================================
// WATER COUNTER
// =====================================================

float dailyLiter = 0.0;

float lifetimeLiter = 0.0;
float lastSavedLiter = 0.0;

int currentDay = -1;

unsigned long lastFlowCalc = 0;
// =====================================================
// GLOBAL TIMER
// =====================================================

unsigned long noFlow45Timer = 0;

unsigned long noFlow60Timer = 0;

unsigned long flow30Timer = 0;
unsigned long flowErrorTimer = 0;

unsigned long bakFillStart = 0;

unsigned long tandonFillStart = 0;

unsigned long bypassCloseStart = 0;
// =====================================================
// FLOW INTERRUPT
// =====================================================
void mqttCallback(
  char* topic,
  byte* payload,
  unsigned int length)
{
  String cmd = "";

  for(unsigned int i=0;i<length;i++)
    cmd += (char)payload[i];

  Serial.print("MQTT CMD : ");
  Serial.println(cmd);

  // AUTO
  if(cmd == "AUTO")
{
    if(currentMode == MODE_IDLE)
    {
        startAutoMode();

        mqttClient.publish(
          "water/mode",
          "AUTO");
    }
}

  // MANUAL
  else if(cmd == "MANUAL")
{
    if(currentMode != MODE_AUTO)
    {
        startManualMode();

        mqttClient.publish(
          "water/mode",
          "MANUAL");
    }
}

  // STOP
  else if(cmd == "STOP")
{
    stopManualMode();

    mqttClient.publish(
      "water/mode",
      "IDLE");
}

  // RESET ESP32
else if(cmd == "RESET")
{
    mqttClient.publish(
      "water/status",
      "ESP32 RESTART");

    delay(1000);

    ESP.restart();
}

  // PUMP ON
  else if(cmd == "PUMP_ON")
{
    if(currentMode == MODE_MANUAL)
    {
        pumpOn();

        mqttClient.publish(
          "water/pump",
          "ON");
    }
}

  // PUMP OFF
  else if(cmd == "PUMP_OFF")
{
    if(currentMode == MODE_MANUAL)
    {
        pumpOff();

        mqttClient.publish(
          "water/pump",
          "OFF");
    }
}

  // BYPASS OPEN
  else if(cmd == "OPEN_BYPASS")
  {
    if(currentMode == MODE_MANUAL)
    {
      startBypassOpen();

      mqttClient.publish(
        "water/status",
        "BYPASS OPEN");
    }
  }

  // BYPASS CLOSE
  else if(cmd == "CLOSE_BYPASS")
  {
    if(currentMode == MODE_MANUAL)
    {
      startBypassClose();

      mqttClient.publish(
        "water/status",
        "BYPASS CLOSE");
    }
  }

  // SOURCE PDAM
  else if(cmd == "FROM_PDAM")
  {
    if(currentMode == MODE_MANUAL)
    {
      startSourcePDAM();

      mqttClient.publish(
        "water/status",
        "SOURCE PDAM");
    }
  }

  // SOURCE BAK
  else if(cmd == "FROM_BAK")
  {
    if(currentMode == MODE_MANUAL)
    {
      startSourceBAK();

      mqttClient.publish(
        "water/status",
        "SOURCE BAK");
    }
  }

  // TO BAK
  else if(cmd == "TO_BAK")
  {
    if(currentMode == MODE_MANUAL)
    {
      startDistribusiBAK();

      mqttClient.publish(
        "water/status",
        "DIST BAK");
    }
  }

  // TO TANDON
  else if(cmd == "TO_TANDON")
  {
    if(currentMode == MODE_MANUAL)
    {
      startDistribusiTandon();

      mqttClient.publish(
        "water/status",
        "DIST TANDON");
    }
  }
}
void reconnectMQTT()
{
  while(!mqttClient.connected())
  {
    Serial.println("MQTT Connecting...");

    if(mqttClient.connect(
          "ESP32Water",
          mqtt_user,
          mqtt_pass,
          "water/online",  // Last Will Topic
          1,               // QoS
          true,            // Retain
          "OFFLINE"))      // Pesan jika ESP32 putus
    {
      Serial.println("MQTT Connected");

      mqttClient.subscribe("water/cmd");

      mqttClient.publish(
          "water/online",
          "ONLINE",
          true);

      mqttClient.publish(
          "water/alarm",
          "ESP32 ONLINE");

      publishMQTTStatus();
    }
    else
    {
      Serial.print("MQTT Failed : ");
      Serial.println(mqttClient.state());

      delay(500);
    }
  }
}
void IRAM_ATTR flowISR()
{
    pulseCount++;
    
}
// =====================================================
// UPDATE FLOWMETER
// =====================================================

void updateFlowmeter()
{
  if (millis() - lastFlowCalc >= 1000)
  {
    noInterrupts();

    unsigned long pulses = pulseCount;
    

    pulseCount = 0;

    interrupts();

    if (pulses <= 1)
{
    flowRate = 0.0;
}
else
{
    flowRate = (pulses + 4.0) / 8.0;
}

    float literThisSecond = flowRate / 60.0;

totalLiter += literThisSecond;

// Hitung hanya saat PDAM aktif
if(countPDAM && pumpRunning)
{
    dailyLiter += literThisSecond;

    lifetimeLiter += literThisSecond;

    if(lifetimeLiter - lastSavedLiter >= 5.0)
    {
        prefs.putFloat(
            "lifetime",
            lifetimeLiter);

        lastSavedLiter =
            lifetimeLiter;
    }
}


    lastFlowCalc = millis();
  }
}

// =====================================================
// FLOW DETECT
// =====================================================

bool flowDetected()
{
  return flowRate > 0.2;
}
// =====================================================
// FLOAT RAW
// =====================================================

bool bakFloatRaw()
{
  return digitalRead(FLOAT_BAK) == LOW;
}

bool tandonFloatRaw()
{
  return digitalRead(FLOAT_TANDON) == LOW;
}
// =====================================================
// FLOAT BAK CONFIRM
// =====================================================

bool bakFullConfirmed()
{
  static unsigned long timer = 0;

  if (bakFloatRaw())
  {
    if (timer == 0)
      timer = millis();

    if (millis() - timer >= FLOAT_CONFIRM_TIME)
      return true;
  }
  else
  {
    timer = 0;
  }

  return false;
}
// =====================================================
// FLOAT TANDON CONFIRM
// =====================================================

bool tandonFullConfirmed()
{
  static unsigned long timer = 0;

  if (tandonFloatRaw())
  {
    if (timer == 0)
      timer = millis();

    if (millis() - timer >= FLOAT_CONFIRM_TIME)
      return true;
  }
  else
  {
    timer = 0;
  }

  return false;
}
// =====================================================
// RELAY MANAGER
// ACTIVE LOW
// =====================================================

void relayOn(uint8_t pin)
{
  digitalWrite(pin, LOW);
}

void relayOff(uint8_t pin)
{
  digitalWrite(pin, HIGH);
}
// =====================================================
// VALVE STRUCT
// =====================================================

struct ValveControl
{
  bool busy;

  uint8_t relayPin;

  unsigned long startTime;

  unsigned long duration;
};
// =====================================================
// VALVE OBJECT
// =====================================================

ValveControl valveBypass;

ValveControl valveSource;

ValveControl valveDist;
// =====================================================
// READY STATUS
// =====================================================

bool bypassReady()
{
  return !valveBypass.busy;
}

bool sourceReady()
{
  return !valveSource.busy;
}

bool distribusiReady()
{
  return !valveDist.busy;
}

bool allValvesReady()
{
  return
    !valveBypass.busy &&
    !valveSource.busy &&
    !valveDist.busy;
}
// =====================================================
// BYPASS OPEN
// =====================================================

void startBypassOpen()
{
  if(bypassPos == BYPASS_OPEN)
  return;
  if (valveBypass.busy)
    return;

  relayOff(RLY_BYPASS_CLOSE);

  relayOn(RLY_BYPASS_OPEN);

  valveBypass.busy = true;

  valveBypass.relayPin = RLY_BYPASS_OPEN;

  valveBypass.startTime = millis();

  valveBypass.duration = VALVE_TIME;
}
// =====================================================
// BYPASS CLOSE
// =====================================================

void startBypassClose()
{
  if(bypassPos == BYPASS_CLOSE)
  return;
  if (valveBypass.busy)
    return;

  relayOff(RLY_BYPASS_OPEN);

  relayOn(RLY_BYPASS_CLOSE);

  valveBypass.busy = true;

  valveBypass.relayPin = RLY_BYPASS_CLOSE;

  valveBypass.startTime = millis();

  valveBypass.duration = VALVE_TIME;
}
// =====================================================
// SOURCE PDAM
// =====================================================

void startSourcePDAM()
{
  countPDAM = true;
  if(sourcePos == SOURCE_PDAM)
  return;
  if (valveSource.busy)
    return;

  relayOff(RLY_SOURCE_BAK);

  relayOn(RLY_SOURCE_PDAM);

  valveSource.busy = true;

  valveSource.relayPin = RLY_SOURCE_PDAM;

  valveSource.startTime = millis();

  valveSource.duration = VALVE_TIME;
}
// =====================================================
// SOURCE BAK
// =====================================================

void startSourceBAK()
{
  stopCountTimer = millis();
  if(sourcePos == SOURCE_BAK)
  return;
  if (valveSource.busy)
    return;

  relayOff(RLY_SOURCE_PDAM);

  relayOn(RLY_SOURCE_BAK);

  valveSource.busy = true;

  valveSource.relayPin = RLY_SOURCE_BAK;

  valveSource.startTime = millis();

  valveSource.duration = VALVE_TIME;
}
// =====================================================
// DISTRIBUSI BAK
// =====================================================

void startDistribusiBAK()
{
  if(distPos == DIST_BAK)
  return;
  if (valveDist.busy)
    return;

  relayOff(RLY_DIST_TANDON);

  relayOn(RLY_DIST_BAK);

  valveDist.busy = true;

  valveDist.relayPin = RLY_DIST_BAK;

  valveDist.startTime = millis();

  valveDist.duration = VALVE_TIME;
}
// =====================================================
// DISTRIBUSI TANDON
// =====================================================

void startDistribusiTandon()
{
  if(distPos == DIST_TANDON)
  return;
  if (valveDist.busy)
    return;

  relayOff(RLY_DIST_BAK);

  relayOn(RLY_DIST_TANDON);

  valveDist.busy = true;

  valveDist.relayPin = RLY_DIST_TANDON;

  valveDist.startTime = millis();

  valveDist.duration = VALVE_TIME;
}
// =====================================================
// UPDATE VALVES
// =====================================================

void updateValves()
{
  // -------------------------
  // BYPASS
  // -------------------------

  if (valveBypass.busy)
  {
    if (millis() - valveBypass.startTime >= valveBypass.duration)
    {
      relayOff(valveBypass.relayPin);

      valveBypass.busy = false;

      if (valveBypass.relayPin == RLY_BYPASS_OPEN)
        bypassPos = BYPASS_OPEN;

      if (valveBypass.relayPin == RLY_BYPASS_CLOSE)
        bypassPos = BYPASS_CLOSE;
      //Serial.print("Bypass Pos = ");
//Serial.println(bypassPos);
    }
  }

  // -------------------------
  // SOURCE
  // -------------------------

  if (valveSource.busy)
  {
    if (millis() - valveSource.startTime >= valveSource.duration)
    {
      relayOff(valveSource.relayPin);

      valveSource.busy = false;

      if (valveSource.relayPin == RLY_SOURCE_PDAM)
        sourcePos = SOURCE_PDAM;

      if (valveSource.relayPin == RLY_SOURCE_BAK)
        sourcePos = SOURCE_BAK;
    }
  }

  // -------------------------
  // DISTRIBUSI
  // -------------------------

  if (valveDist.busy)
  {
    if (millis() - valveDist.startTime >= valveDist.duration)
    {
      relayOff(valveDist.relayPin);

      valveDist.busy = false;

      if (valveDist.relayPin == RLY_DIST_BAK)
        distPos = DIST_BAK;

      if (valveDist.relayPin == RLY_DIST_TANDON)
        distPos = DIST_TANDON;
    }
  }
}
// =====================================================
// PUMP ON
// =====================================================

void pumpOn()
{
    relayOn(RLY_PUMP);

    relayOn(RLY_STATUS);

    pumpRunning = true;

    if(currentMode == MODE_AUTO && autoPumpStartTime == 0)
    {
        autoPumpStartTime = millis();
    }
}
// =====================================================
// PUMP OFF
// =====================================================

void pumpOff()
{
    relayOff(RLY_PUMP);

    relayOff(RLY_STATUS);

    pumpRunning = false;
}
// =====================================================
// HOME POSITION
// =====================================================

void startHomePosition()
{
  startBypassOpen();

  startSourceBAK();

  startDistribusiTandon();
}
// =====================================================
// ERROR POSITION
// =====================================================

void startErrorPosition()
{
  startBypassOpen();

  startSourceBAK();

  startDistribusiTandon();
}
// =====================================================
// INIT VALVE
// =====================================================

void initValveManager()
{
  valveBypass.busy = false;

  valveSource.busy = false;

  valveDist.busy = false;
}
// =====================================================
// RUN AUTO
// =====================================================

void runAuto()
{
  switch(autoState)
  {
        case AUTO_START_1:

      bakFullLatched = false;

      tandonFullLatched = false;

      startBypassClose();

      bypassCloseStart = millis();

      autoState = AUTO_START_2;

    break;
        case AUTO_START_2:

  if(millis() - bypassCloseStart < 5000)
    break;

  startSourcePDAM();

  if(bakFloatRaw())
  {
    bakFullLatched = true;

    startDistribusiTandon();
  }
  else
  {
    startDistribusiBAK();
  }

  pumpOn();

  autoState = AUTO_START_WAIT;

break;
        case AUTO_START_WAIT:

  if(!sourceReady())
    break;

  if(!distribusiReady())
    break;

  if(bakFullLatched)
  {
    tandonFillStart = millis();

    noFlow60Timer = millis();

    autoState = AUTO_FILL_TANDON;
  }
  else
  {
    bakFillStart = millis();

    noFlow45Timer = millis();

    autoState = AUTO_FILL_BAK;
  }

break;
        case AUTO_FILL_BAK:

      // ----------------------
      // Flow Monitoring
      // ----------------------

      if(flowDetected())
      {
        noFlow45Timer = millis();
      }
      else
      {
        if(millis() - noFlow45Timer >= NO_FLOW_PDAM_TIME)
        {
          autoState = AUTO_PDAM_FAIL;

          break;
        }
      }

      // ----------------------
      // Timeout
      // ----------------------

      if(millis() - bakFillStart >= BAK_TIMEOUT)
      {
        errorState = ERR_BAK_TIMEOUT;

        currentMode = MODE_ERROR;

        break;
      }

      // ----------------------
      // Float BAK
      // ----------------------

      if(!bakFullLatched)
      {
        if(bakFullConfirmed())
        {
          bakFullLatched = true;

          autoState = AUTO_SWITCH_TANDON;
        }
      }

    break;
        case AUTO_PDAM_FAIL:

      startBypassOpen();

      startSourceBAK();

      flow30Timer = millis();

      autoState = AUTO_WAIT_PDAM_RETURN;

    break;
        case AUTO_WAIT_PDAM_RETURN:

      if(flowDetected())
      {
        if(millis() - flow30Timer >= FLOW_RETURN_TIME)
        {
          autoState = AUTO_START_1;
        }
      }
      else
      {
        flow30Timer = millis();
      }

    break;
        case AUTO_SWITCH_TANDON:

      startDistribusiTandon();

      autoState = AUTO_SWITCH_TANDON_WAIT;

    break;
        case AUTO_SWITCH_TANDON_WAIT:

      if(!distribusiReady())
        break;

      tandonFillStart = millis();

      noFlow60Timer = millis();

      autoState = AUTO_FILL_TANDON;

    break;
        case AUTO_FILL_TANDON:

      // ----------------------
      // Flow Monitoring
      // ----------------------

      if(flowDetected())
      {
        noFlow60Timer = millis();
      }
      else
      {
        if(millis() - noFlow60Timer >= NO_FLOW_TANDON_TIME)
        {
          autoState = AUTO_STOP;

          break;
        }
      }

      // ----------------------
      // Timeout
      // ----------------------

      if(millis() - tandonFillStart >= TANDON_TIMEOUT)
      {
        errorState = ERR_TANDON_TIMEOUT;

        currentMode = MODE_ERROR;

        break;
      }

      // ----------------------
      // Float TANDON
      // ----------------------

      if(!tandonFullLatched)
      {
        if(tandonFullConfirmed())
        {
          tandonFullLatched = true;

          autoState = AUTO_STOP;
        }
      }

    break;
        case AUTO_STOP:

      startBypassOpen();

      startSourceBAK();

      autoState = AUTO_FINISHED;

    break;
        case AUTO_FINISHED:

      if(!bypassReady())
        break;

      if(!sourceReady())
        break;

      pumpOff();

      currentMode = MODE_IDLE;

      autoState = AUTO_IDLE;

    break;
      }
}
// =====================================================
// START AUTO MODE
// =====================================================

void startAutoMode()
{
  bakFullLatched = false;

  tandonFullLatched = false;

  errorState = ERR_NONE;

  autoState = AUTO_START_1;

  autoPumpDuration = 0;
  autoPumpStartTime = 0;

  currentMode = MODE_AUTO;
  //Blynk.virtualWrite(V0, "AUTO");
}
// =====================================================
// START MANUAL MODE
// =====================================================

void startManualMode()
{
  currentMode = MODE_MANUAL;
  //Blynk.virtualWrite(V0, "MANUAL");
}
// =====================================================
// STOP MANUAL MODE
// =====================================================

void stopManualMode()
{
  currentMode = MODE_IDLE;
  //Blynk.virtualWrite(V0, "IDLE");
}
// =====================================================
// RESET ERROR
// =====================================================

void resetError()
{
  errorState = ERR_NONE;

  bakFullLatched = false;

  tandonFullLatched = false;

  currentMode = MODE_IDLE;
}
// =====================================================
// ERROR MODE
// =====================================================

void errorMode()
{
  static bool errorInit = false;

  if(!errorInit)
  {
    startErrorPosition();

    errorInit = true;
  }

  if(allValvesReady())
  {
    pumpOff();
  }

  if(currentMode != MODE_ERROR)
  {
    errorInit = false;
  }
}
// =====================================================
// MANUAL MODE. TOMBOL BLYNK AKAN MASUK KESINI
// =====================================================

void manualMode()
{
  static unsigned long tandonTimer = 0;

  if(tandonFloatRaw())
  {
    if(tandonTimer == 0)
      tandonTimer = millis();

    if(pumpRunning && (millis() - tandonTimer >= FLOAT_CONFIRM_TIME))
    {
      pumpOff();
    }
  }
  else
  {
    tandonTimer = 0;
  }
}
// =====================================================
// SETUP
// =====================================================
/*
void sendToBlynk()
{
    // Status Pompa
    if(pumpRunning)
        Blynk.virtualWrite(V33, "ON");
    else
        Blynk.virtualWrite(V33, "OFF");

    // Level BAK
    Blynk.virtualWrite(V40, levelBAK);
    Blynk.virtualWrite(V41, tinggiAirBAK);

    // Mode
    switch(currentMode)
    {
        case MODE_IDLE:
            Blynk.virtualWrite(V0, "IDLE");
            break;

        case MODE_AUTO:
            Blynk.virtualWrite(V0, "AUTO");
            break;

        case MODE_MANUAL:
            Blynk.virtualWrite(V0, "MANUAL");
            break;

        case MODE_ERROR:
            Blynk.virtualWrite(V0, "ERROR");
            break;
    }

    // Posisi Bypass
    if(bypassPos == BYPASS_OPEN)
        Blynk.virtualWrite(V30, "OPEN");
    else if(bypassPos == BYPASS_CLOSE)
        Blynk.virtualWrite(V30, "CLOSE");
    else
        Blynk.virtualWrite(V30, "UNKNOWN");

    // Posisi Sumber
    if(sourcePos == SOURCE_PDAM)
        Blynk.virtualWrite(V31, "PDAM");
    else
        Blynk.virtualWrite(V31, "BAK");

    // Posisi Distribusi
    if(distPos == DIST_BAK)
        Blynk.virtualWrite(V32, "BAK");
    else
        Blynk.virtualWrite(V32, "TANDON");

  // Flow
  Blynk.virtualWrite(V1, flowRate);

  // Liter
  Blynk.virtualWrite(V2, dailyLiter);
  Blynk.virtualWrite(V3, lifetimeLiter);

  // Float
  //Blynk.virtualWrite(V4, bakFloatRaw());
  //Blynk.virtualWrite(V5, tandonFloatRaw());
Blynk.virtualWrite(V4, 0);
Blynk.virtualWrite(V5, 0);
  // Error
  switch(errorState)
  {
    case ERR_NONE:
      Blynk.virtualWrite(V6, "NONE");
    break;

    case ERR_BAK_TIMEOUT:
      Blynk.virtualWrite(V6, "BAK TIMEOUT");
    break;

    case ERR_TANDON_TIMEOUT:
      Blynk.virtualWrite(V6, "TANDON TIMEOUT");
    break;

    default:
      Blynk.virtualWrite(V6, "ERROR");
    break;
  }
}
*/
/*
BLYNK_CONNECTED()
{
    Blynk.syncVirtual(V61);
    Blynk.syncVirtual(V62);
    Blynk.syncVirtual(V63);
}

BLYNK_WRITE(V61)
{
    int val = param.asInt();

    if(currentMode != MODE_MANUAL)
        return;

    if(val == 0)
    {
        startSourcePDAM();
        Serial.println("BLYNK -> SUMBER PDAM");
    }
    else
    {
        startSourceBAK();
        Serial.println("BLYNK -> SUMBER BAK");
    }
}
BLYNK_WRITE(V62)
{
    int val = param.asInt();

    if(currentMode != MODE_MANUAL)
        return;

    if(val == 0)
    {
        startDistribusiBAK();
        Serial.println("BLYNK -> DISTRIBUSI BAK");
    }
    else
    {
        startDistribusiTandon();
        Serial.println("BLYNK -> DISTRIBUSI TANDON");
    }
}
BLYNK_WRITE(V63)
{
    int val = param.asInt();

    if(currentMode != MODE_MANUAL)
        return;

    if(val == 0)
    {
        startBypassClose();
        Serial.println("BLYNK -> BYPASS CLOSE");
    }
    else
    {
        startBypassOpen();
        Serial.println("BLYNK -> BYPASS OPEN");
    }
}
BLYNK_WRITE(V10)
{
    if(!param.asInt())
        return;

    startAutoMode();

    Serial.println("MODE -> AUTO");
}
BLYNK_WRITE(V11)
{
    if(!param.asInt())
        return;

    startManualMode();

    Serial.println("MODE -> MANUAL");
}
BLYNK_WRITE(V13)
{
    if(!param.asInt())
        return;

    stopManualMode();
    pumpOff();

    Serial.println("MODE -> IDLE");
}
BLYNK_WRITE(V12)
{
  if(param.asInt())
  {
    resetError();
  }
}
BLYNK_WRITE(V20)
{
  if(!param.asInt()) return;

  if(currentMode != MODE_MANUAL)
    return;

  startBypassOpen();
}
BLYNK_WRITE(V21)
{
  if(!param.asInt()) return;

  if(currentMode != MODE_MANUAL)
    return;

  startBypassClose();
}
BLYNK_WRITE(V22)
{
  if(!param.asInt()) return;

  if(currentMode != MODE_MANUAL)
    return;

  startSourcePDAM();
}
BLYNK_WRITE(V23)
{
  if(!param.asInt()) return;

  if(currentMode != MODE_MANUAL)
    return;

  startSourceBAK();
}
BLYNK_WRITE(V24)
{
  if(!param.asInt()) return;

  if(currentMode != MODE_MANUAL)
    return;

  startDistribusiBAK();
}
BLYNK_WRITE(V25)
{
  if(!param.asInt()) return;

  if(currentMode != MODE_MANUAL)
    return;

  startDistribusiTandon();
}
BLYNK_WRITE(V26)
{
  if(!param.asInt()) return;

  if(currentMode != MODE_MANUAL)
    return;

  pumpOn();
}
BLYNK_WRITE(V27)
{
  if(!param.asInt()) return;

  if(currentMode != MODE_MANUAL)
    return;

  pumpOff();
}
*/

float bacaJSNSR04T()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long durasi = pulseIn(ECHO_PIN, HIGH, 30000);

  if(durasi == 0)
    return jarakBAK;

  float jarak = durasi * 0.0343 / 2.0;

  return jarak;
}
void updateLevelBAK()
{
  jarakBAK = bacaJSNSR04T();

  tinggiAirBAK = 150.0 - jarakBAK;

  if(tinggiAirBAK < 0)
    tinggiAirBAK = 0;

  if(tinggiAirBAK > 130)
    tinggiAirBAK = 130;

  levelBAK = (tinggiAirBAK / 130.0) * 100.0;
  /*
  Serial.print("Jarak BAK: ");
Serial.print(jarakBAK);
Serial.print(" cm | Tinggi Air: ");
Serial.print(tinggiAirBAK);
Serial.print(" cm | Level: ");
Serial.print(levelBAK);
Serial.println(" %");
*/

}
void checkLowBakAlarm()
{
    // ON jika < 15%
    if(!lowBakAlarm && levelBAK < 15.0)
    {
        lowBakAlarm = true;

        relayOn(RLY_ALARM_LOWBAK);
    }

    // OFF jika > 25%
    else if(lowBakAlarm && levelBAK > 25.0)
    {
        lowBakAlarm = false;

        relayOff(RLY_ALARM_LOWBAK);
    }
}
void checkPhysicalButtons()
{
    static bool lastAuto = HIGH;
    static bool lastStop = HIGH;

    bool autoState = digitalRead(BTN_AUTO);
    bool stopState = digitalRead(BTN_STOP);

    // AUTO
    if(lastAuto == HIGH && autoState == LOW)
    {
        if(currentMode == MODE_IDLE)
        {
            startAutoMode();

            //Serial.println("AUTO BUTTON PRESSED");
        }
    }

    // STOP
    if(lastStop == HIGH && stopState == LOW)
{
    stopManualMode();
}

    lastAuto = autoState;
    lastStop = stopState;
}
void handleStatus()
{
    String json = "{";

    json += "\"mode\":\"";

    switch(currentMode)
    {
        case MODE_IDLE:   json += "IDLE"; break;
        case MODE_AUTO:   json += "AUTO"; break;
        case MODE_MANUAL: json += "MANUAL"; break;
        case MODE_ERROR:  json += "ERROR"; break;
    }

    json += "\",";

    json += "\"pump\":\"";
    json += (pumpRunning ? "ON" : "OFF");
    json += "\",";

    json += "\"pumpDuration\":";
    json += String(autoPumpDuration);
    json += ",";

    json += "\"levelBAK\":";
    json += String(levelBAK,1);
    json += ",";

    json += "\"tinggiBAK\":";
    json += String(tinggiAirBAK,1);
    json += ",";

    json += "\"flow\":";
    json += String(flowRate,1);
    json += ",";

    json += "\"daily\":";
    json += String(dailyLiter,1);
    json += ",";

    json += "\"lifetime\":";
json += String(lifetimeLiter,1);
json += ",";

struct tm timeinfo;

json += "\"datetime\":\"";

if(getLocalTime(&timeinfo))
{
    char buf[30];

    sprintf(
        buf,
        "%02d/%02d/%04d %02d:%02d:%02d",
        timeinfo.tm_mday,
        timeinfo.tm_mon + 1,
        timeinfo.tm_year + 1900,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec
    );

    json += buf;
}
else
{
    json += "NTP OFF";
}

json += "\",";

json += "\"nextSchedule\":\"";
json += nextScheduleString;
json += "\"";



json += ",";
json += "\"bypass\":\"";

if(valveBypass.busy)
{
    if(valveBypass.relayPin == RLY_BYPASS_OPEN)
        json += "OPEN";
    else if(valveBypass.relayPin == RLY_BYPASS_CLOSE)
        json += "CLOSE";
    else
        json += "UNKNOWN";
}
else
{
    if(bypassPos == BYPASS_OPEN)
        json += "OPEN";
    else if(bypassPos == BYPASS_CLOSE)
        json += "CLOSE";
    else
        json += "UNKNOWN";
}

json += "\",";
json += "\"source\":\"";

if(valveSource.busy)
{
    if(valveSource.relayPin == RLY_SOURCE_PDAM)
        json += "PDAM";
    else if(valveSource.relayPin == RLY_SOURCE_BAK)
        json += "BAK";
    else
        json += "UNKNOWN";
}
else
{
    if(sourcePos == SOURCE_PDAM)
        json += "PDAM";
    else if(sourcePos == SOURCE_BAK)
        json += "BAK";
    else
        json += "UNKNOWN";
}

json += "\",";
json += "\"alarm\":\"";
json += alarmText;
json += "\",";

json += "\"alarmColor\":\"";
json += alarmColor;
json += "\",";
json += "\"dist\":\"";

if(valveDist.busy)
{
    if(valveDist.relayPin == RLY_DIST_BAK)
        json += "BAK";
    else if(valveDist.relayPin == RLY_DIST_TANDON)
        json += "TANDON";
    else
        json += "UNKNOWN";
}
else
{
    if(distPos == DIST_BAK)
        json += "BAK";
    else if(distPos == DIST_TANDON)
        json += "TANDON";
    else
        json += "UNKNOWN";
}

json += "\"";



json += ",";

json += "\"bypassMoving\":";
json += (valveBypass.busy ? "true" : "false");

json += ",";

json += "\"sourceMoving\":";
json += (valveSource.busy ? "true" : "false");

json += ",";

json += "\"distMoving\":";
json += (valveDist.busy ? "true" : "false");
json += ",";

json += "\"lowBakAlarm\":\"";
json += (lowBakAlarm ? "LOW LEVEL" : "NORMAL");
json += "\"";
    json += "}";

    server.send(200,"application/json",json);
}
void handleRoot()
{
    String html;

    html += "<!DOCTYPE html>";
    html += "<html>";
    html += "<head>";

    html += "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>";

    html += "<title>Kontrol Air</title>";

    html += "<style>";
    html += ".btnAuto{";
html += "background:#FFD700;";
html += "color:black;";
html += "height:52px;";
html += "}";

html += ".btnManual{";
html += "background:#2196F3;";
html += "color:white;";
html += "height:52px;";
html += "}";

html += ".btnStop{";
html += "background:#F44336;";
html += "color:white;";
html += "height:52px;";
html += "}";
html += ".btnReset{";
html += "background:#FF9800;";
html += "color:white;";
html += "height:52px;";
html += "}";

html += ".btnPumpOn{";
html += "background:#4CAF50;";
html += "color:white;";
html += "height:52px;";
html += "}";

html += ".btnPumpOff{";
html += "background:#9E9E9E;";
html += "color:white;";
html += "height:52px;";
html += "}";
html += ".btnBypassOpen{";
html += "background:#4CAF50;";
html += "color:white;";
html += "}";

html += ".btnBypassClose{";
html += "background:#F44336;";
html += "color:white;";
html += "height:52px;";
html += "}";

html += ".btnPDAM{";
html += "background:#0096FF;";
html += "color:white;";
html += "height:52px;";
html += "}";

html += ".btnBAK{";
html += "background:#795548;";
html += "color:white;";
html += "height:52px;";
html += "}";

html += ".btnToBak{";
html += "background:#00BCD4;";
html += "color:white;";
html += "height:52px;";
html += "}";

html += ".btnToTandon{";
html += "background:#673AB7;";
html += "color:white;";
html += "height:52px;";
html += "}";

html += "body{";
html += "margin:0;";
html += "padding:5px;";
html += "height:100vh;";
html += "font-family:Arial;";
html += "background:#f2f2f2;";
html += "overflow:hidden;";
html += "}";

html += "h2{";
html += "margin:5px;";
html += "font-size:24px;";
html += "}";

html += ".card{";
html += "padding:8px;";
html += "margin:5px;";
html += "border-radius:10px;";
html += "background:white;";
html += "}";

html += ".grid{";
html += "display:flex;";
html += "gap:4px;";
html += "}";

html += ".grid2{";
html += "display:grid;";
html += "grid-template-columns:1fr 1fr;";
html += "gap:4px;";
html += "}";

html += ".grid3{";
html += "display:grid;";
html += "grid-template-columns:1fr 1fr 1fr;";
html += "gap:4px;";
html += "}";

html += "<button id='btnBypassOpen' class='btn btnBypassOpen' onclick=\"sendCmd('/bypass/open')\">OPEN<br>BYPASS</button>";

html += "<button id='btnPDAM' class='btn btnPDAM' onclick=\"sendCmd('/source/pdam')\">FROM<br>PDAM</button>";

html += "<button id='btnDistBak' class='btn btnToBak' onclick=\"sendCmd('/dist/bak')\">TO<br>BAK</button>";

html += "<button id='btnBypassClose' class='btn btnBypassClose' onclick=\"sendCmd('/bypass/close')\">CLOSE<br>BYPASS</button>";

html += "<button id='btnBAK' class='btn btnBAK' onclick=\"sendCmd('/source/bak')\">FROM<br>BAK</button>";

html += "<button id='btnDistTandon' class='btn btnToTandon' onclick=\"sendCmd('/dist/tandon')\">TO<br>TANDON</button>";
html += "</div>";

html += "<div class='grid3'>";


html += "</div>";
html += "display:grid;";
html += "grid-template-columns:1fr 1fr;";
html += "gap:4px;";
html += "}";

html += ".btn{";
html += "height:80px;";
html += "width:100%;";
html += "font-size:18px;";
html += "border:none;";
html += "border-radius:8px;";
html += "}";
html += ".grid3{";
html += "display:grid;";
html += "grid-template-columns:1fr 1fr 1fr;";
html += "gap:4px;";
html += "}";

html += "p{";
html += "margin:4px;";
html += "font-size:16px;";
html += "}";
html += ".btn:active{";
html += "transform:scale(0.95);";
html += "}";
html += ".btn:disabled{";
html += "background:#999;";
html += "color:#555;";
html += "opacity:0.6;";
html += "}";
html += ".blink{";  //blinking
html += "animation:blink 1s infinite;";
html += "}";

html += "@keyframes blink{";
html += "50%{opacity:0.2;}";
html += "}";  //blinking

html += "</style>";

html += "<h2>KONTROL AIR ESP32</h2>";
html += "<div id='alarmBox' style='";
html += "margin:5px;";
html += "padding:12px;";
html += "border-radius:12px;";
html += "text-align:center;";
html += "font-weight:bold;";
html += "color:white;";
html += "background:#28a745;";
html += "'>";

html += "<span id='alarmText'>SISTEM NORMAL</span>";

html += "</div>";

html += "<div style='text-align:center;font-size:18px;font-weight:bold;'>";

html += "<span id='datetime'>--:--:--</span>";

html += "</div>";
html += "<div style='background:#111;";
html += "color:white;";
html += "padding:8px;";
html += "margin:5px;";
html += "border-radius:20px;";
html += "text-align:center;'>";

html += "<div style='font-size:16px;'>Jadwal Berikutnya</div>";

html += "<div id='nextSchedule' style='font-size:18px;font-weight:bold;margin-top:5px;'>";

html += "--/--/---- --:--";

html += "</div>";

html += "</div>";

html += "<div class='card'>";

html += "<p>Mode : <span id='mode' style='font-weight:bold'>-</span></p>";
html += "<p>Pompa : <span id='pump'>OFF</span> | <span id='pumpDuration'>00:00:00</span></p>";
html += "<p>Level BAK : <span id='level'>0</span>%</p>";
html += "<div style='width:100%;height:18px;background:#ddd;border-radius:10px;'>";

html += "<div id='barlevel' style='width:0%;height:18px;background:#0096FF;border-radius:10px;'></div>";

html += "</div>";
html += "<p>Flow : <span id='flow'>0</span> L/min</p>";
html += "<p>Hari Ini : <span id='daily'>0</span> Liter</p>";

html += "<p>Total Sistem : <span id='lifetime'>0</span> Liter</p>";
html += "<p>Bypass : <span id='bypass' style='font-weight:bold'>-</span></p>";

html += "<p>Sumber : <span id='source' style='font-weight:bold'>-</span></p>";

html += "<p>Distribusi : <span id='dist' style='font-weight:bold'>-</span></p>";
html += "<p>Alarm BAK : <span id='lowBakAlarm' style='font-weight:bold'>NORMAL</span></p>";

html += "</div>";
// =======================
// TOMBOL DI BAWAH
// =======================

html += "<div class='grid3'>";

html += "<button id='btnAuto' class='btn btnAuto' onclick=\"sendCmd('/auto')\">AUTO</button>";

html += "<button id='btnManual' class='btn btnManual' onclick=\"sendCmd('/manual')\">MANUAL</button>";

html += "<button id='btnStop' class='btn btnStop' onclick=\"sendCmd('/stop')\">STOP</button>";


html += "</div>";

html += "<div class='grid3'>";

html += "<button id='btnPumpOn' class='btn btnPumpOn' onclick=\"sendCmd('/pump/on')\">PUMP ON</button>";

html += "<button id='btnPumpOff' class='btn btnPumpOff' onclick=\"sendCmd('/pump/off')\">PUMP OFF</button>";

html += "<button id='btnReset' class='btn btnReset' onclick=\"sendCmd('/reset')\">RESET</button>";

html += "</div>";
html += "<div class='grid3'>";

html += "<button id='btnBypassOpen' class='btn btnBypassOpen' onclick=\"sendCmd('/bypass/open')\">OPEN<br>BYPASS</button>";

html += "<button id='btnPDAM' class='btn btnPDAM' onclick=\"sendCmd('/source/pdam')\">FROM<br>PDAM</button>";

html += "<button id='btnDistBak' class='btn btnToBak' onclick=\"sendCmd('/dist/bak')\">TO<br>BAK</button>";

html += "</div>";

html += "<div class='grid3'>";

html += "<button id='btnBypassClose' class='btn btnBypassClose' onclick=\"sendCmd('/bypass/close')\">CLOSE<br>BYPASS</button>";

html += "<button id='btnBAK' class='btn btnBAK' onclick=\"sendCmd('/source/bak')\">FROM<br>BAK</button>";

html += "<button id='btnDistTandon' class='btn btnToTandon' onclick=\"sendCmd('/dist/tandon')\">TO<br>TANDON</button>";

html += "</div>";
html += "<script>";

html += "setInterval(function(){";

html += "fetch('/status')";

html += ".then(r=>r.json())";

html += ".then(d=>{";

html += "let m=document.getElementById('mode');";

html += "m.innerHTML=d.mode;";

html += "if(d.mode=='AUTO'){";
html += "m.style.color='green';";
html += "}";

html += "else if(d.mode=='MANUAL'){";
html += "m.style.color='blue';";
html += "}";

html += "else if(d.mode=='ERROR'){";
html += "m.style.color='red';";
html += "}";

html += "else{";
html += "m.style.color='gray';";
html += "}";

html += "document.getElementById('level').innerHTML=d.levelBAK;";

html += "document.getElementById('flow').innerHTML=d.flow;";
html += "document.getElementById('daily').innerHTML=d.daily;";

html += "document.getElementById('lifetime').innerHTML=d.lifetime;";
html += "document.getElementById('datetime').innerHTML=d.datetime;";
html += "document.getElementById('nextSchedule').innerHTML=d.nextSchedule;";
html += "document.getElementById('alarmText').innerHTML=d.alarm;";
html += "let low=document.getElementById('lowBakAlarm');";

html += "low.innerHTML=d.lowBakAlarm;";

html += "if(d.lowBakAlarm=='LOW LEVEL'){";
html += "low.style.color='red';";
html += "}else{";
html += "low.style.color='green';";
html += "}";

html += "document.getElementById('alarmBox').style.background=d.alarmColor;";
html += "let b=document.getElementById('bypass');";
html += "b.innerHTML=d.bypass;";

html += "if(d.bypass=='OPEN'){";
html += "b.style.color='green';";
html += "}else if(d.bypass=='CLOSE'){";
html += "b.style.color='red';";
html += "}else{";
html += "b.style.color='gray';";
html += "}";
html += "if(d.bypassMoving){";
html += "b.classList.add('blink');";
html += "}else{";
html += "b.classList.remove('blink');";
html += "}";

html += "let s=document.getElementById('source');";
html += "s.innerHTML=d.source;";

html += "if(d.source=='PDAM'){";
html += "s.style.color='#0096FF';";
html += "}else if(d.source=='BAK'){";
html += "s.style.color='green';";
html += "}else{";
html += "s.style.color='gray';";
html += "}";
html += "if(d.sourceMoving){";
html += "s.classList.add('blink');";
html += "}else{";
html += "s.classList.remove('blink');";
html += "}";

html += "let dst=document.getElementById('dist');";
html += "dst.innerHTML=d.dist;";

html += "if(d.dist=='TANDON'){";
html += "dst.style.color='green';";
html += "}else if(d.dist=='BAK'){";
html += "dst.style.color='#0096FF';";
html += "}else{";
html += "dst.style.color='gray';";
html += "}";
html += "if(d.distMoving){";
html += "dst.classList.add('blink');";
html += "}else{";
html += "dst.classList.remove('blink');";
html += "}";
html += "let btnAuto=document.getElementById('btnAuto');";
html += "let btnStop=document.getElementById('btnStop');";
html += "let btnManual=document.getElementById('btnManual');";
html += "let btnPumpOn=document.getElementById('btnPumpOn');";
html += "let btnReset=document.getElementById('btnReset');";
html += "let btnPumpOff=document.getElementById('btnPumpOff');";
html += "let btnBypassOpen=document.getElementById('btnBypassOpen');";
html += "let btnBypassClose=document.getElementById('btnBypassClose');";

html += "let btnPDAM=document.getElementById('btnPDAM');";
html += "let btnBAK=document.getElementById('btnBAK');";

html += "let btnDistBak=document.getElementById('btnDistBak');";
html += "let btnDistTandon=document.getElementById('btnDistTandon');";

html += "if(d.mode=='AUTO'){";

html += "btnAuto.disabled=true;";
html += "btnManual.disabled=true;";

html += "btnPumpOn.disabled=true;";
html += "btnPumpOff.disabled=true;";

html += "btnBypassOpen.disabled=true;";
html += "btnBypassClose.disabled=true;";

html += "btnPDAM.disabled=true;";
html += "btnBAK.disabled=true;";

html += "btnDistBak.disabled=true;";
html += "btnDistTandon.disabled=true;";

html += "btnStop.disabled=false;";
html += "btnReset.disabled=false;";
html += "btnReset.disabled=false;";
html += "btnReset.disabled=false;";
html += "}else if(d.mode=='MANUAL'){";

html += "btnAuto.disabled=true;";
html += "btnManual.disabled=true;";

html += "btnPumpOn.disabled=false;";
html += "btnPumpOff.disabled=false;";

html += "btnBypassOpen.disabled=false;";
html += "btnBypassClose.disabled=false;";

html += "btnPDAM.disabled=false;";
html += "btnBAK.disabled=false;";

html += "btnDistBak.disabled=false;";
html += "btnDistTandon.disabled=false;";

html += "btnStop.disabled=false;";

html += "}else{";
html += "btnAuto.disabled=false;";
html += "btnManual.disabled=false;";

html += "btnPumpOn.disabled=true;";
html += "btnPumpOff.disabled=true;";

html += "btnBypassOpen.disabled=true;";
html += "btnBypassClose.disabled=true;";

html += "btnPDAM.disabled=true;";
html += "btnBAK.disabled=true;";

html += "btnDistBak.disabled=true;";
html += "btnDistTandon.disabled=true;";

html += "btnStop.disabled=true;";

html += "btnAuto.disabled=false;";
html += "btnManual.disabled=false;";
html += "btnPumpOn.disabled=true;";
html += "btnPumpOff.disabled=true;";
html += "btnStop.disabled=true;";

html += "}";
html += "document.getElementById('barlevel').style.width=d.levelBAK+'%';";

html += "let sec=d.pumpDuration;";
html += "let h=Math.floor(sec/3600);";
html += "let mn=Math.floor((sec%3600)/60);";
html += "let sc=sec%60;";
html += "document.getElementById('pumpDuration').innerHTML=(h<10?'0':'')+h+':' +(mn<10?'0':'')+mn+':' +(sc<10?'0':'')+sc;";
html += "let p=document.getElementById('pump');";

html += "p.innerHTML=d.pump;";

html += "if(d.pump=='ON'){";

html += "p.style.color='green';";

html += "}else{";

html += "p.style.color='red';";

html += "}";

html += "});";

html += "},1000);";
html += "function sendCmd(url){";

html += "if(url=='/reset'){";
html += "if(!confirm('Restart ESP32 ?')) return;";
html += "}";

html += "if(url=='/auto'){";
html += "if(!confirm('Jalankan Mode AUTO ?')) return;";
html += "}";

html += "if(url=='/stop'){";
html += "if(!confirm('Hentikan Sistem ?')) return;";
html += "}";

html += "fetch(url)";

html += ".then(r=>r.text())";

html += ".then(d=>console.log(d));";

html += "}";


html += "</script>";
html += "</body>";
html += "</html>";

    server.send(200,"text/html",html);
}
void handleAuto()
{
    if(currentMode != MODE_IDLE)
    {
        server.send(403,"text/plain","NOT IDLE");
        return;
    }

    startAutoMode();

    server.send(200,"text/plain","AUTO");
}

void handleManual()
{
    if(currentMode == MODE_AUTO)
    {
        server.send(403,"text/plain","AUTO RUNNING");
        return;
    }

    startManualMode();

    server.send(200,"text/plain","MANUAL");
}

void handleStop()
{
    stopManualMode();
    server.send(200,"text/plain","STOP");
}

void handleReset()
{
    server.send(
        200,
        "text/plain",
        "ESP32 RESTART");

    delay(1000);

    ESP.restart();
}
void handlePumpOn()
{
    if(currentMode != MODE_MANUAL)
    {
        server.send(403,"text/plain","DENIED");
        return;
    }

    pumpOn();

    server.send(200,"text/plain","OK");
}

void handlePumpOff()
{
    if(currentMode != MODE_MANUAL)
    {
        server.send(403,"text/plain","DENIED");
        return;
    }

    pumpOff();

    server.send(200,"text/plain","OK");
}
void handleBypassOpen()
{
    if(currentMode == MODE_MANUAL)
        startBypassOpen();

    server.send(200,"text/plain","BYPASS OPEN");
}

void handleBypassClose()
{
    if(currentMode == MODE_MANUAL)
        startBypassClose();

    server.send(200,"text/plain","BYPASS CLOSE");
}
void handlePDAM()
{
    if(currentMode == MODE_MANUAL)
        startSourcePDAM();

    server.send(200,"text/plain","PDAM");
}

void handleBAK()
{
    if(currentMode == MODE_MANUAL)
        startSourceBAK();

    server.send(200,"text/plain","BAK");
}
void handleDistBak()
{
    if(currentMode == MODE_MANUAL)
        startDistribusiBAK();

    server.send(200,"text/plain","DIST BAK");
}

void handleDistTandon()
{
    if(currentMode == MODE_MANUAL)
        startDistribusiTandon();

    server.send(200,"text/plain","DIST TANDON");
}
void printESP32Info()
{
    Serial.println();

    Serial.print("IP ESP32 = ");
    Serial.println(WiFi.localIP());

    time_t now = time(nullptr);

    if(now > 100000)
    {
        struct tm *timeinfo = localtime(&now);

        Serial.printf(
            "WAKTU ESP32 = %02d:%02d:%02d %02d/%02d/%04d\n",
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec,
            timeinfo->tm_mday,
            timeinfo->tm_mon + 1,
            timeinfo->tm_year + 1900
        );
    }
    else
    {
        Serial.println("NTP BELUM SINKRON");
    }
}
void checkAutoSchedule();
//void printESP32Info();

void publishMQTTStatus()
{
    if(currentMode == MODE_AUTO)
        mqttClient.publish("water/mode", "AUTO");
    else if(currentMode == MODE_MANUAL)
        mqttClient.publish("water/mode", "MANUAL");
    else if(currentMode == MODE_IDLE)
        mqttClient.publish("water/mode", "IDLE");
    else if(currentMode == MODE_ERROR)
        mqttClient.publish("water/mode", "ERROR");

    mqttClient.publish("water/pump", pumpRunning ? "ON" : "OFF");

    if(valveBypass.busy)
    {
        if(valveBypass.relayPin == RLY_BYPASS_OPEN)
            mqttClient.publish("water/bypass","OPEN");
        else if(valveBypass.relayPin == RLY_BYPASS_CLOSE)
            mqttClient.publish("water/bypass","CLOSE");
    }
    else if(bypassPos == BYPASS_OPEN)
        mqttClient.publish("water/bypass","OPEN");
    else if(bypassPos == BYPASS_CLOSE)
        mqttClient.publish("water/bypass","CLOSE");
    else
        mqttClient.publish("water/bypass","UNKNOWN");

    if(valveSource.busy)
    {
        if(valveSource.relayPin == RLY_SOURCE_PDAM)
            mqttClient.publish("water/source","PDAM");
        else if(valveSource.relayPin == RLY_SOURCE_BAK)
            mqttClient.publish("water/source","BAK");
    }
    else if(sourcePos == SOURCE_PDAM)
        mqttClient.publish("water/source","PDAM");
    else if(sourcePos == SOURCE_BAK)
        mqttClient.publish("water/source","BAK");
    else
        mqttClient.publish("water/source","UNKNOWN");

    if(valveDist.busy)
    {
        if(valveDist.relayPin == RLY_DIST_BAK)
            mqttClient.publish("water/dist","BAK");
        else if(valveDist.relayPin == RLY_DIST_TANDON)
            mqttClient.publish("water/dist","TANDON");
    }
    else if(distPos == DIST_BAK)
        mqttClient.publish("water/dist","BAK");
    else if(distPos == DIST_TANDON)
        mqttClient.publish("water/dist","TANDON");
    else
        mqttClient.publish("water/dist","UNKNOWN");

    mqttClient.publish("water/bypassMoving", valveBypass.busy ? "true":"false");
    mqttClient.publish("water/sourceMoving", valveSource.busy ? "true":"false");
    mqttClient.publish("water/distMoving", valveDist.busy ? "true":"false");

    mqttClient.publish("water/alarm", alarmText.c_str());

    String bakText = String((int)levelBAK);
    mqttClient.publish("water/bak", bakText.c_str());

    String flowText = String(flowRate,1);
    mqttClient.publish("water/flow", flowText.c_str());

    String todayText = String((long)dailyLiter);
    mqttClient.publish("water/today", todayText.c_str());

    String lifetimeText = String((long)lifetimeLiter);
    mqttClient.publish("water/lifetime", lifetimeText.c_str());

    struct tm timeinfo;
    if(getLocalTime(&timeinfo))
    {
        char buf[30];
        sprintf(buf,"%02d/%02d/%04d %02d:%02d:%02d",
        timeinfo.tm_mday,timeinfo.tm_mon+1,timeinfo.tm_year+1900,
        timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);

        mqttClient.publish("water/datetime", buf);
    }

    mqttClient.publish("water/nextSchedule", nextScheduleString.c_str());
}

void handleOTA()
{
    server.send(
        200,
        "text/html",
        "<form method='POST' action='/update' enctype='multipart/form-data'>"
        "<input type='file' name='update'>"
        "<input type='submit' value='Upload'>"
        "</form>"
    );
}
void handleOTAUpload()
{
    HTTPUpload& upload =
        server.upload();

    if(upload.status ==
       UPLOAD_FILE_START)
    {
        Serial.printf(
            "Update: %s\n",
            upload.filename.c_str());

        Update.begin(
            UPDATE_SIZE_UNKNOWN);
    }
    else if(upload.status ==
            UPLOAD_FILE_WRITE)
    {
        Update.write(
            upload.buf,
            upload.currentSize);
    }
    else if(upload.status ==
            UPLOAD_FILE_END)
    {
        if(Update.end(true))
        {
            Serial.println(
                "Update Success");

            server.send(
                200,
                "text/plain",
                "Update Success. Rebooting...");

            delay(1000);

            ESP.restart();
        }
        else
        {
            Update.printError(
                Serial);
        }
    }
}
void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, pass);

while (WiFi.status() != WL_CONNECTED)
{
    delay(500);
    Serial.print(".");
}

//Serial.println();
Serial.println("WiFi Connected");
Serial.println(WiFi.localIP());
  server.on("/", handleRoot);

server.on("/status", handleStatus);
server.on(
    "/update",
    HTTP_GET,
    handleOTA);

server.on(
    "/update",
    HTTP_POST,
    [](){},
    handleOTAUpload);
server.on("/auto", handleAuto);
server.on("/manual", handleManual);
server.on("/stop", handleStop);
server.on("/reset", handleReset);

server.on("/pump/on", handlePumpOn);
server.on("/pump/off", handlePumpOff);

server.on("/bypass/open", handleBypassOpen);
server.on("/bypass/close", handleBypassClose);

server.on("/source/pdam", handlePDAM);
server.on("/source/bak", handleBAK);

server.on("/dist/bak", handleDistBak);
server.on("/dist/tandon", handleDistTandon);

server.begin();

//Serial.println("WEB SERVER START");
  pinMode(TRIG_PIN, OUTPUT);
pinMode(ECHO_PIN, INPUT);
  
  secureClient.setInsecure();

mqttClient.setServer(
  mqtt_server,
  mqtt_port);

mqttClient.setCallback(
  mqttCallback);
  prefs.begin("water", false);
  

lifetimeLiter = prefs.getFloat("lifetime", 0.0);
lastSavedLiter = lifetimeLiter;
    pinMode(FLOAT_BAK, INPUT_PULLUP);

  pinMode(FLOAT_TANDON, INPUT_PULLUP);
  pinMode(BTN_AUTO, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);

  pinMode(FLOW_SENSOR, INPUT_PULLUP);
    pinMode(RLY_BYPASS_OPEN, OUTPUT);
  pinMode(RLY_BYPASS_CLOSE, OUTPUT);

  pinMode(RLY_SOURCE_PDAM, OUTPUT);
  pinMode(RLY_SOURCE_BAK, OUTPUT);

  pinMode(RLY_DIST_BAK, OUTPUT);
  pinMode(RLY_DIST_TANDON, OUTPUT);

  pinMode(RLY_PUMP, OUTPUT);
  pinMode(RLY_STATUS, OUTPUT);
  pinMode(RLY_ALARM_LOWBAK, OUTPUT);
    relayOff(RLY_BYPASS_OPEN);
  relayOff(RLY_BYPASS_CLOSE);

  relayOff(RLY_SOURCE_PDAM);
  relayOff(RLY_SOURCE_BAK);

  relayOff(RLY_DIST_BAK);
  relayOff(RLY_DIST_TANDON);

  relayOff(RLY_PUMP);
  relayOff(RLY_STATUS);
  relayOff(RLY_ALARM_LOWBAK);
    initValveManager();
      attachInterrupt(
      digitalPinToInterrupt(FLOW_SENSOR),
      flowISR,
      FALLING);
        startHomePosition();
  
  configTime(
    gmtOffset_sec,
    daylightOffset_sec,
    ntpServer
);
  //Blynk.virtualWrite(V30, "BOOT");
         
}
// =====================================================
// LOOP
// =====================================================

void loop()
{
  
  if(!mqttClient.connected())
{
    reconnectMQTT();
}

mqttClient.loop();

if(millis() - lastMQTTPublish > 1000)
{
    lastMQTTPublish = millis();

    publishMQTTStatus();
}

if(millis() - lastMQTTStatus > 30000)
{
    lastMQTTStatus = millis();

    mqttClient.publish(
        "water/status",
        "SISTEM NORMAL");
}

server.handleClient();
  static unsigned long lastInfoPrint = 0;

if(millis() - lastInfoPrint >= 5000)
{
    lastInfoPrint = millis();

    printESP32Info();
}
//timer.run();
checkPhysicalButtons();
updateNextSchedule();
checkAutoSchedule();
updateLevelBAK();
checkLowBakAlarm();

  if(stopCountTimer != 0)
{
    if(millis() - stopCountTimer >= 10000)
    {
        countPDAM = false;

        stopCountTimer = 0;
    }
}
  updateFlowmeter();
  if(pumpRunning)
{
    if(flowRate < 0.1)
    {
        if(flowErrorTimer == 0)
        {
            flowErrorTimer = millis();
        }

        if(millis() - flowErrorTimer >= 10000)
        {
            alarmText = "FLOW ERROR";
            alarmColor = "#dc3545";
        }
    }
    else
    {
        flowErrorTimer = 0;

        alarmText = "SISTEM NORMAL";
        alarmColor = "#28a745";
    }
}
else
{
    flowErrorTimer = 0;

    alarmText = "SISTEM NORMAL";
    alarmColor = "#28a745";
}

  if(currentMode == MODE_AUTO && pumpRunning && autoPumpStartTime > 0)
  {
      autoPumpDuration = (millis() - autoPumpStartTime) / 1000;
  }

  updateValves();

  if(!startupComplete)
  {
    if(allValvesReady())
    {
      startupComplete = true;

      currentMode = MODE_IDLE;
    }

    return;
  }
    

    switch(currentMode)
  {
        case MODE_IDLE:

    break;
        case MODE_AUTO:

      runAuto();

    break;
        case MODE_MANUAL:

      manualMode();

    break;
        case MODE_ERROR:

      errorMode();

    break;
      }
}
// =====================================================
// RESET DAILY COUNTER
// =====================================================

void resetDailyCounter()
{
  dailyLiter = 0;
}
// =====================================================
// RESET LIFETIME COUNTER
// =====================================================
void updateNextSchedule()
{
    time_t now = time(nullptr);

    // 09/06/2026 21:30:00
    struct tm startTm = {};
    startTm.tm_year = 2026 - 1900;
    startTm.tm_mon  = 6 - 1;
    startTm.tm_mday = 9;
    startTm.tm_hour = 21;
    startTm.tm_min  = 30;
    startTm.tm_sec  = 0;

    time_t startTime = mktime(&startTm);

    const time_t interval = 48L * 60L * 60L; // 48 jam

    time_t nextTime = startTime;

    while(nextTime <= now)
    {
        nextTime += interval;
    }

    struct tm *tmNext = localtime(&nextTime);

    char buf[30];

    sprintf(
        buf,
        "%02d/%02d/%04d %02d:%02d",
        tmNext->tm_mday,
        tmNext->tm_mon + 1,
        tmNext->tm_year + 1900,
        tmNext->tm_hour,
        tmNext->tm_min
    );

    nextScheduleString = buf;
}
void resetLifetimeCounter()
{
  lifetimeLiter = 0;
}

void checkAutoSchedule()
{
    static bool triggeredToday = false;

    struct tm timeinfo;

    if(!getLocalTime(&timeinfo))
        return;

    // Reference:
    // 09 Juni 2026 = hari ke-160 tahun 2026
    // tm_yday dimulai dari 0

    if(timeinfo.tm_year != (2026 - 1900))
        return;

    int referenceDay = 159; // 09 Juni 2026

    int daysPassed = timeinfo.tm_yday - referenceDay;

    if(daysPassed < 0)
        return;

    bool scheduleDay = (daysPassed % 2 == 0);

    if(scheduleDay)
    {
        if(timeinfo.tm_hour == 21 &&
           timeinfo.tm_min  == 30)
        {
            if(!triggeredToday)
            {
                if(currentMode == MODE_IDLE)
                {
                    startAutoMode();

                    //Serial.println("AUTO SCHEDULE 48 JAM");
                }

                triggeredToday = true;
            }
        }
    }

    // reset flag setelah jam berganti
    if(timeinfo.tm_hour == 0 &&
       timeinfo.tm_min == 0)
    {
        triggeredToday = false;
    }
}
