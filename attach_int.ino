//Modified from http://heliosoph.mit-links.info/rotary-encoder-software-for-arduino/
// The pins the quadrature encoder is connected.
// We connect the second signal of the encoder to pin 4.
// int.1 is free for other use.
int in_a = 2;
int in_b = 3;

// The number of pulses per revolution
// depends on your index disc!!
unsigned int pulsesperturn = 816;//34 * 12 * 2(Gear ratio: 34; 12 rising edges per motor revolution; 2 encoder signals)

// The total number of revolutions
double revolutions = 0;
double lastRevolutions = 0;
double RPM = 0;

// Initialize the counter
volatile int pulses = 0;

void count() {
  // This function is called by the interrupt
  // If in_b is HIGH increment the counter
  // otherwise decrement it
    pulses++;
}

void setup() {
  pinMode(in_a, INPUT);
  pinMode(in_b, INPUT);
  attachInterrupt(digitalPinToInterrupt(in_a), count, RISING);
  attachInterrupt(digitalPinToInterrupt(in_b), count, RISING);
  Serial.begin(9600);
}

void loop() {
  revolutions = 1.0 * pulses / pulsesperturn;
  // Here you can output the revolutions, e. g. once a second
  //
  RPM = (revolutions-lastRevolutions)*60;
  Serial.println(RPM);
  //
  delay(1000);
  lastRevolutions=revolutions;
  
}
