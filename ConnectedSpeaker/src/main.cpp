#include <Arduino.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>


//STATE CONSTANTS
#define CONNECTED 3
#define PAIRING  2
#define BTDISABLED 1
#define STANDBY 0
#define STERROR   -1

//PinDefinitions
#define BTLED A1 //Sensor going to the bluetooth module
#define BTPOWER 5
#define RELAYPIN 6
#define PIXELPIN 3


bool btPower = true;
bool relayPower = true;
int state = PAIRING;
bool initialValueSent = false;

//--------- MySensor Shinanigans -------------

// Enable debug prints to serial monitor
#define MY_DEBUG

#define MY_RADIO_RF24
#define MY_RF24_CHANNEL 107
#define MY_REPEATER_FEATURE
#define RELAYID  1
#define BTID  2
#define BTSTATUSID 3

#define LONG_WAIT 500
#define SHORT_WAIT 50

#include <MySensors.h>

MyMessage relay_msg(RELAYID, V_STATUS);
MyMessage bt_power_msg(BTID, V_STATUS);
MyMessage bt_connected_msg(BTSTATUSID, V_TRIPPED);

void before()
{
    //pinMode(LED_BUILTIN, OUTPUT); //Testing. represents global power

		// Then set relay pins in output mode
		pinMode(RELAYPIN, OUTPUT);
    pinMode(BTPOWER, OUTPUT);
    pinMode(BTLED, INPUT);
		// Set relay to last known state (using eeprom storage)
    relayPower = true; //loadState(RELAYID); //may or may not use it
    btPower = true; //loadState(BTID);
    
}


void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("c3_speaker", "1.0");
		// Register all sensors to gw (they will be created as child devices)
	
  present(BTID, S_BINARY, "Bluetooth");
  wait(SHORT_WAIT);
  present(BTSTATUSID, S_DOOR, "Bluetooth Connected");
  wait(SHORT_WAIT);
  present(RELAYID, S_BINARY, "Power");
  wait(LONG_WAIT);

  Serial.println("Sending bluetooth initial value");
  send(bt_power_msg.set(btPower));
  Serial.println("Requesting bluetooth initial value from controller");
  request(BTID, V_STATUS);
  wait(2000, C_SET, V_STATUS);

  Serial.println("Sending relay initial value");
  send(relay_msg.set(relayPower));
  Serial.println("Requesting relay initial value from controller");
  request(RELAYID, V_STATUS);
  wait(2000, C_SET, V_STATUS);

  
  digitalWrite(RELAYPIN, relayPower);
  digitalWrite(BTPOWER, btPower);
}

void receive(const MyMessage &message)
{
	// We only expect one type of message from controller. But we better check anyway.
	if (message.getType()==V_STATUS) {
		// Change relay state
    auto node_id = message.getSensor();
    bool messageValue = (bool) message.getInt();
    switch (node_id)
    {
    case BTID:
      btPower = messageValue;
      break;
    case RELAYID:
      relayPower = messageValue;
      break;
    default:
      break;
    }
    
    // Store state in eeprom
		//saveState(message.getSensor(), message.getBool());
		// Write some debug info
		Serial.print("Incoming change for sensor:");
		Serial.print(message.getSensor());
		Serial.print(", New status: ");
		Serial.println(message.getBool());
	}
}
//----------------------------------------

//---------- NEOPIXEL THINGIES ------------

//Color Definitions
#define CONNECTED_COLOR RgbwColor(0,0,100,0)
#define BTDISABLED_COLOR  RgbwColor(0,100,0,0) 
#define STANDBY_COLOR   RgbwColor(150,0,0)
#define BLACK           RgbColor(0)
#define PIXELCOUNT 1
NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(PIXELCOUNT, PIXELPIN);
NeoPixelAnimator animations(1); // NeoPixel animation management object
bool fadeToColor = true;  // general purpose variable used to store effect state
struct MyAnimationState
{
    RgbwColor StartingColor;
    RgbwColor EndingColor;
};
MyAnimationState animationState[1];
// simple blend function
void BlendAnimUpdate(const AnimationParam& param)
{
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    RgbwColor updatedColor = RgbwColor::LinearBlend(
        animationState[param.index].StartingColor,
        animationState[param.index].EndingColor,
        param.progress);

    // apply the color to the strip
    for (uint16_t pixel = 0; pixel < PIXELCOUNT; pixel++)
    {
        strip.SetPixelColor(pixel, updatedColor);
    }
}

//-------------------------


/*
Ligou lá atrás: 
  - NÃO IMPORTA OQ TA NA NET. LIGA TUDO KRL
  Se a pessoa quiser desligar o BT e só usar o P2,
  ela precisa de uma maneira de desligá-lo - Checar se dá pra usar
  touch em algum daqueles parafusos. Depois
  
*/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  strip.Begin();
  strip.SetPixelColor(0, STANDBY_COLOR);
  strip.Show();

}

int lastState = CONNECTED;
unsigned long lastBlink;
int lastLedBt = 0;

int getBtState(){
  bool ledbt = digitalRead(BTLED);
  unsigned long now = millis();
  if(ledbt != lastLedBt){ //Store last time it blinked
    lastLedBt = ledbt;
    if(!ledbt){
      lastBlink = now;
    }
  }
  if(now < lastBlink){ //check for an STERROR
    lastBlink = now;
    return STERROR;
  }else{
    auto blinkTime = now - lastBlink;
    if(blinkTime < 2000){

      return PAIRING;
    }else if(ledbt){
        return CONNECTED;
    }else{
      return STERROR;
    } 
  }
}

void loop() {
  digitalWrite(BTPOWER, btPower && relayPower);
  digitalWrite(RELAYPIN, relayPower);

  if(relayPower){
    if(btPower){
      int i = getBtState();
      if(i != STERROR){
        state = i;
      }
      //checkbtstatus
      //Ou seja, seta CONNECTED ou PAIRING
    }else{
      state = BTDISABLED;
    }
  }else{    
    state = STANDBY;
  }

  //If animating, go for it!
  if (animations.IsAnimating()){
    animations.UpdateAnimations();
    strip.Show();
  }else{ //Else: See if the state we're in requires some other action
    if(state == PAIRING){
      animationState[0].StartingColor = strip.GetPixelColor(0);
      if(animationState[0].StartingColor.CalculateBrightness() < 2){
        animationState[0].EndingColor = CONNECTED_COLOR;
      }else{
        animationState[0].EndingColor = BLACK;
      }
      animations.StartAnimation(0, 600, BlendAnimUpdate);
    }
  }


  if(state != lastState){
    lastState = state;
    animationState[0].StartingColor = strip.GetPixelColor(0);
    switch (state)
    {
    case STANDBY:
      Serial.println("STANDBY");
      animationState[0].EndingColor = STANDBY_COLOR;
      break;
    case BTDISABLED:
      Serial.println("BTDISABLED");
      animationState[0].EndingColor = BTDISABLED_COLOR;
      break;
    case CONNECTED:
      Serial.println("CONNECTED");
      send(bt_connected_msg.set(1));
      animationState[0].EndingColor = CONNECTED_COLOR;
      break;
    case PAIRING:
      send(bt_connected_msg.set(0));
      break;
    default:
      break;
    }
    animations.StartAnimation(0, 400, BlendAnimUpdate);
  }

}