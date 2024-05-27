// set pin numbers
const int buttonPin1 = 4; // the number of the first pushbutton pin
const int buttonPin2 = 19; // the number of the second pushbutton pin
const int ledPin =  33;    // the number of the LED pin

// variables for storing the pushbutton status
int buttonState1 = 0;
int buttonState2 = 0;
bool ledState = false;

void setup() {
  Serial.begin(115200);  
  // initialize the pushbutton pins as inputs
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  // initialize the LED pin as an output
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // read the state of the pushbuttons
  buttonState1 = digitalRead(buttonPin1);
  buttonState2 = digitalRead(buttonPin2);

  // check if button 1 is pressed
  if (buttonState1 == HIGH) {
    // if button 1 is pressed, turn LED on
    ledState = true;
  }
  // check if button 2 is pressed
  if (buttonState2 == HIGH) {
    // if button 2 is pressed, turn LED off
    ledState = false;
  }

  // update the LED state
  digitalWrite(ledPin, ledState ? HIGH : LOW);
}
