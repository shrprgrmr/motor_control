#define en1 10 //h-bridge enable 1
#define in1 6 //h-bridge input 1
#define in2 7 //h-bridge input 2
#define encA 2 //rotary encoder A output
#define encB 3 //rotary encoder B output

//--------------Motor Characteristics----------------//
// The number of pulses per revolution
// depends on your index disc!!
unsigned int pulsesperturn = 816;//34 * 12 * 2(Gear ratio: 34 encoder revolutions per wheel revolution; 12 rising edges per encoder signal per motor revolution; Rising and falling edges)//2 encoder signals)

unsigned int maxRPM = 180;//No-load RPM speed at 6V
//--------------End Motor Characteristics-----------//

//--------------User inputs-------------------------//
int pctSpeed = 0;//Input % of max speed
int targetRPM = 0;//user input; RPM setpoint

bool dirSelected = false;
char selectedDir = 'f';
bool speedSelected = false;

//--------------End User inputs--------------------//

//--------------Encoder readings and calcs---------//
int sampleInterval = 10;//[ms]
unsigned long sampleClk = 0;//millis() at last sample
int serialInterval = 1000;//[ms]; log speed to serial port at 1Hz
unsigned long serialClk = 0;

volatile int pulses = 0;//encoder pulses
volatile int lastPulses = 0;//store the last sample count for debugging
void pulseCount() {
  // This function is called by the interrupt
  // If in_b is HIGH increment the counter
  // otherwise decrement it
    pulses++;
}

//Calculate the motor speed in RPM as
//  (pulses/(sampleInterval)) x (1/(pulses/rev)) x (sampleIntervals/min

//Consolidate the latter two factors for quick use.
double rpmFactor = (1.0/pulsesperturn) * (60000.0/sampleInterval);


double mtrRPM = 0.0;
//-----------End Encoder readings and calcs-------//

//----------- PWM Output to H-Bridge Enable-------//
int pwmMin = 20; //13% and 7.7V VCC was the min needed to overcome the motor's inertia.
                 //So change map min from 0 to 0.12*255 = 30.6
                 //(7.7V VCC was the amount needed to get 6V output to the motor when PWM was at 100% (3.3V))
int pwmMax = 255;
int pwmOutput = 0;//Set the duty cycle for the H-bridge enable. 3.3V, 0-100% mapped to 0-255
int pwmStep = 1;

//------- End PWM Output to H-Bridge Enable-------//

void setup() {
  // put your setup code here, to run once:
  pinMode(en1, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);  
  // Set initial rotation direction
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);

  //Use interrupts to count both encoder outputs' pulses
  attachInterrupt(digitalPinToInterrupt(encA), pulseCount, RISING);
  attachInterrupt(digitalPinToInterrupt(encB), pulseCount, RISING);
  
  Serial.begin(9600);
  Serial.print("Encoder Sample Interval [ms]: ");
  Serial.println(sampleInterval);
  Serial.print("RPM Factor: ");
  Serial.println(rpmFactor);
  Serial.println("Arduino Ready");
  sampleClk = millis();
  serialClk = sampleClk;
}

//Handle keyboard input over serial
const byte msgSize = 32;
char inputMsg[msgSize];
bool newInputs = false;
bool newInputsSent = true;

void loop() {
  receiveInputs();
  if(newInputs){
    processMsg();
  }
  
  if(millis()-sampleClk>sampleInterval){
    //Calculate average motor speed over the last sample interval
    mtrRPM = (double)(lastPulses+pulses)/2 * rpmFactor;
    lastPulses = pulses;
    pulses=0;
    sampleClk=millis();

    //Adjust the PWM output to reduce error in the motor speed
    if(targetRPM!=0){
      if(mtrRPM<(targetRPM-5)){
        pwmOutput+=pwmStep;
        if(pwmOutput>pwmMax){
          pwmOutput=pwmMax;
        }
      }
      else if(mtrRPM>(targetRPM+5)){
        pwmOutput-=pwmStep; 
        if(pwmOutput<pwmMin){
          pwmOutput=pwmMin;
        }
      }
      analogWrite(en1, pwmOutput);
    }
    

    if(millis()-serialClk>serialInterval){
      Serial.print("SerClk: ");
      Serial.println(serialClk);
      Serial.print("Millis: ");
      Serial.println(millis());
      Serial.print(lastPulses);
      Serial.println(" pulses");
      Serial.print(mtrRPM);
      Serial.println(" RPM");
      Serial.print(pwmOutput);
      Serial.println(" Cts");
      
      serialClk=millis();
    }
  }

}

void receiveInputs(){
  static byte i = 0;
  char terminate = '\n';
  char rxIP;

  while(Serial.available() > 0 && newInputs == false){
    rxIP = Serial.read();

    if(rxIP != terminate){
      inputMsg[i] = rxIP;
      i++;
      if(i >= msgSize){
        i = msgSize-1;
      }
    }
    else{
      i=0;
      newInputs=true;
    }
  }
}


void processMsg(){
  if(!dirSelected){
    selectedDir = inputMsg[0];
    switch(selectedDir){
      case 'f' :
        dirSelected=true;
        break;
      case 'b' :
        dirSelected=true;
        break;
      default :
        Serial.println("Invalid direction. Select f or b.");
        break;
    }    
  }
  if(dirSelected){
    int value = 0;
    static int valLen = 0;
    switch(selectedDir){
      case 'f' :
        value = getVal('f');
        if(value!=-1){
          //targetRPM=(int) value;
          Serial.println("Control fwd speed to "+String(value)+" RPM.");
          setRPM(value);
          digitalWrite(in1, LOW);
          digitalWrite(in2, HIGH);
          speedSelected=true;
        }
        else{
          Serial.println("Error. Enter 0-"+String(maxRPM)+".");
        }
        break;
      case 'b' :
        value = getVal('b');
        if(value!=-1){
          Serial.println("Control bkwd speed to "+String(value)+" RPM.");
          setRPM(value);
          digitalWrite(in1, HIGH);
          digitalWrite(in2, LOW);
          speedSelected=true;
        }
        else{
          Serial.println("Error. Enter 0-"+String(maxRPM)+".");
        }
        break;
      default :
        Serial.println("Invalid field. Select f or b");
        break;
    }
    if(speedSelected){
      newInputsSent = false;
    }
    dirSelected = false;
    speedSelected = false;
  }
  memset(inputMsg, 0, sizeof(inputMsg));
  newInputs = false;
}

int getVal(char field){
  int value = 0;
  uint16_t valLen, minVal, maxVal;
  bool validRx = false;
  switch(field){
    case 'f' :
      valLen = 3;
      minVal = 0;
      maxVal = maxRPM;
      break;
    case 'b' : 
      valLen = 3;
      minVal = 0;
      maxVal = maxRPM;
      break;
  }
  for(int n=1; n<valLen+1; n++){
    char minChar = '0';
    if(inputMsg[n]>=minChar && inputMsg[n]<='9'){
      value = (value*10) + (inputMsg[n] - '0');
      validRx = true;
    }
    else{
      break;
    }
  }
  if(validRx){
    if(value>=minVal && value <= maxVal){
      return value;
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }
}

void setRPM(int rpmSetpoint){
  targetRPM = rpmSetpoint;
  if(targetRPM==0){
    pwmOutput=0;
  }
  else{
    pwmOutput = map(targetRPM, 0, maxRPM, pwmMin, 255);//13% and 7.7V VCC was the min needed to overcome the motor's inertia
    //So change map min from 0 to 0.12*255 = 30.6
    //(7.7V VCC was the amount needed to get 6V output to the motor when PWM was at 100% (3.3V)    
  }
  analogWrite(en1, pwmOutput);
}
