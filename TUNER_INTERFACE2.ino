#include <PinChangeInt.h>
//#include <PinChangeIntConfig.h>

#define IC_TSTART 2  //Must be pin 2 or 3 for hardware interrupts 0,1
#define IC_TKEY 4
#define LDG_TUNE 5
#define LDG_KEY 3   //Must be pin 2 or 3 for hardware interrupts 0,1
#define LDG_LED1 8  //Pins 8-13 share an interrupt vector - used to monitor status of tune
#define led 13

#define DISABLE 0
#define ENABLE 1
#define TUNE 2

/* LDG Tuning Modes - determined by milliseconds TUNE is held for*/
#define TOGGLE 250      // < 0.5 sec
#define MEMO_TUNE 1500  // 0.5 to 2.5 sec
#define FULL_TUNE 3000  // > 2.5 sec

/*  Select either MEMO_TUNE or FULL_TUNE for the ACTIVE_TUNE_MODE*/
int ACTIVE_TUNE_MODE = FULL_TUNE;

volatile unsigned long pulse;
volatile unsigned long low_trig;
volatile unsigned long high_trig;
volatile bool trigd =  false;
volatile bool LDG_KEY_ACTIVE;
volatile int LDG_LED1_Blinks = 0;
volatile int mode_actual = DISABLE;
int mode_req;

void SET_IC_TKEY(bool key_active){
  if(key_active)
  {
    digitalWrite(IC_TKEY, LOW);
    pinMode(IC_TKEY, OUTPUT); 
  }
  else
  {
    digitalWrite(IC_TKEY, LOW);
    pinMode(IC_TKEY, INPUT); // This is actually an OUTPUT - but set as an INPUT for HIGH-Z when not active
  }
}

void setup(){
  // put your setup code here, to run once:

  pinMode(IC_TSTART, INPUT_PULLUP);   
  SET_IC_TKEY(false);
  pinMode(LDG_TUNE, OUTPUT);
  pinMode(LDG_KEY, INPUT_PULLUP);
  pinMode(LDG_LED1, INPUT);
  
  LDG_KEY_ACTIVE = !(digitalRead(LDG_KEY));
  digitalWrite(LDG_TUNE, HIGH);

  attachInterrupt(digitalPinToInterrupt(IC_TSTART), pulseLow, FALLING);
  attachInterrupt(digitalPinToInterrupt(LDG_KEY), ldgChange, CHANGE);
  PCintPort::attachInterrupt(LDG_LED1, LDG_LED1_INT, RISING); // attach a PinChange Interrupt to our pin on the rising edge

  Serial.begin(115200);
}

void LDG_LED1_INT(){
  LDG_LED1_Blinks++;
}

void pulseLow(){
  pulse = 0;
  low_trig = millis();
  attachInterrupt(digitalPinToInterrupt(IC_TSTART), pulseHigh, RISING);
}

void pulseHigh(){
  trigd = true;
  high_trig = millis();
  pulse =(unsigned long) high_trig - low_trig;  // unsigned casting redundant, but forces unsigned math, which prevents rollover errors
  attachInterrupt(digitalPinToInterrupt(IC_TSTART), pulseLow, FALLING);
}

void ldgChange(){
  int state = digitalRead(LDG_KEY);
  if(state == LOW){
    Serial.println("LDG_KEY LOW Interrpt");
    Serial.println(mode_actual);
    LDG_KEY_ACTIVE = true;
    if(mode_actual == ENABLE) SET_IC_TKEY(true);
  }
  else{
    Serial.println("LDG_KEY HIGH Interrpt");
    LDG_KEY_ACTIVE = false;
    SET_IC_TKEY(false);
  }
}

void LDG_command(int _cmd){
  Serial.print("LDG_COMMAND> _cmd = "); 
  Serial.println(_cmd); 
      delay(250);
      digitalWrite(LDG_TUNE, LOW);
      delay(_cmd);
      digitalWrite(LDG_TUNE, HIGH);
      delay(250);
}

int getLDG_Mode(){
  int inverse_mode;
  int _blinks;
  Serial.println("=============== READING CURRENT MODE ================");
  LDG_LED1_Blinks = 0;
  LDG_command(TOGGLE);
  delay(250);
  delay(250);
  delay(250);
  _blinks = LDG_LED1_Blinks;
  
  Serial.print(" =============>Pre-Blinks  =  ");
  Serial.println(LDG_LED1_Blinks);
  if(_blinks == 1) inverse_mode = ENABLE;
  else if(_blinks == 3) inverse_mode = DISABLE;
  else return -1;
  
  LDG_LED1_Blinks = 0;
  LDG_command(TOGGLE);
  delay(250);
  delay(250);
  delay(250);
  _blinks = LDG_LED1_Blinks;
  Serial.print(" =============>Actual-Blinks  =  ");
  Serial.println(LDG_LED1_Blinks);
  if(_blinks == 1) return ENABLE;
  else if(_blinks == 3) return DISABLE;
  else return -1;
  
  
   
}

void performResync(){
   Serial.println("+++---+++---+++ Resync +++---+++---+++");
   pinMode(IC_TSTART, INPUT);
   delay(100);
   pinMode(IC_TSTART, INPUT_PULLUP);
   mode_actual = DISABLE; 
}

void loop() {
  // put your main code here, to run repeatedly:
  
    if(trigd && pulse > 55){
         Serial.print("                                                                Triggered Pulse Value:   ");
         Serial.println(pulse);
          if(pulse > 55 && pulse < 85){
            mode_req = DISABLE;
            Serial.println(" ####################   MODE REQ DISABLE");      
          }
          else if (pulse > 475 & pulse < 525){
            mode_req = TUNE;
            Serial.println(" $$$$$$$$$$$$$$$$$$$ MODE REQ ENABLE/TUNE"); 
          }
          trigd = false;

         Serial.println("CASE: ---------------------- ENTER SWITCH --------------------------");
          switch(mode_req){
            
            case DISABLE:
                mode_actual = getLDG_Mode();
                if(mode_actual == -1){
                  mode_actual = getLDG_Mode();
                }
                if(mode_actual == -1) {
                  performResync();
                  break;
                }
                if(mode_actual == ENABLE){
                  Serial.println("CASE: DISABLE> MODE IS ENABLE");
                  // Wait for TUNE to finish, or abort tune (if possible?) by immediately sending disable pulse to LDG //
                  while(LDG_KEY_ACTIVE){
                    Serial.println("LDG_KEY is ACTIVE - awaiting TUNE to complete...");
                    delay(250);
                  }
                  // Send disable pulse to LDG //
                  LDG_command(TOGGLE);
                  mode_actual = DISABLE;
                  Serial.println("CASE: DISABLE> MODE CHANGED TO DISABLE");
                }
                else if(mode_actual == DISABLE) Serial.println("CASE: DISABLE> MODE IS ALREADY DISABLE");
                else {
                  Serial.print("CASE: DISABLE> Exception: MODE IS = ");
                  Serial.print(mode_actual);
                }
            break;
            
            case ENABLE:  /* NOT CURRENTLY USING THIS MODE )*/
                if(mode_actual == DISABLE)
                {
                  // Send enable pulse to LDG (if using enable/disable logic) //
                  LDG_command(TOGGLE);
                  mode_actual == ENABLE;
                  
                  // Send TUNE Request to LDG? (does ICOM differentiate ENABLE from TUNE?) //
                  mode_req = TUNE; 
                }
            break;
            
            case TUNE:
                mode_actual = getLDG_Mode();
                if(mode_actual == -1){ //If error - try again
                  mode_actual = getLDG_Mode();
                }
                if(mode_actual == -1){ //If second error - goto resync routine
                  performResync();
                  break;
                }
                Serial.print("CASE: TUNE>>>>>> actual mode: ");
                Serial.println(mode_actual);
                if(mode_actual == DISABLE){ /* FIRST ENABLE TUNER*/
                  Serial.println("CASE: TUNE> MODE IS CURRENTLY DISABLE"); 
                  // Send enable pulse to LDG (if using enable/disable logic) //
                  LDG_command(TOGGLE);
                  mode_actual = ENABLE;
                  Serial.println("CASE: TUNE> MODE CHANGED TO ENABLE"); 
                }
                if(mode_actual == ENABLE){ /* SEND TUNE REQUEST*/
                  Serial.println("CASE: TUNE> MODE IS CURRENTLY ENABLE");
                  // Send TUNE Request to LDG - Uncomment type of tuning desired //
                  //LDG_command(FULL_TUNE);
                  //LDG_command(MEMO_TUNE);
                  LDG_command(ACTIVE_TUNE_MODE);
                  Serial.println("CASE: TUNE> TUNE HAS NOW BEEN ISSUED"); 
                }
            break;
            
            default:
            break;
          } // END switch case
          Serial.println("CASE: ===================== EXIT SWITCH ====================");

    } // END if(trigd...)
}
