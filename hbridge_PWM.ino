#define enA 10
#define inA1 6
#define inA2 7

int pctSpeed = 0;//Input % of max speed

bool dirSelected = false;
char selectedDir = 'f';
bool speedSelected = false;
int selectedSpeed = 0;//% of max speed

void setup() {
  // put your setup code here, to run once:
  pinMode(enA, OUTPUT);
  pinMode(inA1, OUTPUT);
  pinMode(inA2, OUTPUT);  
  // Set initial rotation direction
  digitalWrite(inA1, LOW);
  digitalWrite(inA2, HIGH);
  Serial.begin(9600);
  Serial.println("Arduino Ready");
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
    // Set the direction and speed

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
          selectedSpeed=(int) value;
          Serial.println("Control fwd speed to "+String(selectedSpeed)+" % of max.");
          if(value==0){
            analogWrite(enA, 0);
          }
          else{
            int pwmOutput = map(selectedSpeed, 0, 100, 20, 255);//13% and 7.7V VCC was the min needed to overcome the motor's inertia
            //So change map min from 0 to 0.12*255 = 30.6
            //(7.7V VCC was the amount needed to get 6V output to the motor when PWM was at 100% (3.3V)
            analogWrite(enA, pwmOutput);
          }
          digitalWrite(inA1, LOW);
          digitalWrite(inA2, HIGH);
          speedSelected=true;
        }
        else{
          Serial.println("Error. Enter 0-100.");
        }
        break;
      case 'b' :
        value = getVal('b');
        if(value!=-1){
          selectedSpeed=(uint16_t) value;
          Serial.println("Control bkwd speed to "+String(selectedSpeed)+" % of max.");
          if(value==0){
            analogWrite(enA, 0);
          }
          else{
            int pwmOutput = map(selectedSpeed, 0, 100, 20, 255);
            analogWrite(enA, pwmOutput);
          }
          digitalWrite(inA1, HIGH);
          digitalWrite(inA2, LOW);
          speedSelected=true;
        }
        else{
          Serial.println("Error. Enter 0-100.");
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
      maxVal = 255;
      break;
    case 'b' : 
      valLen = 3;
      minVal = 0;
      maxVal = 255;
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
