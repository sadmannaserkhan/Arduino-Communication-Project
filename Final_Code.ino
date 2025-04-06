/*
  Communications v5

  Transmits data using a white LED and receives it using a photoresistor
*/

#define STX 0x70
#define ETX 0x71

char txButton, txTilt, txPot, txA, txB, txC, txD;
char txBuffer[3] = {0, 0, ETX};
char rxBuffer[2];
char rxButton, rxTilt, rxPot, rxA, rxB, rxC, rxD;
int rx_index;

int buttonPin = 2;
int buzzer = 12; // The pin of the active buzzer
int tiltPin = 4;
int LED = 7;

const int forwardpin = 8;  // Pin that enables clockwise motion
const int backwardpin = 13;  // Pin that enables anticlockwise motion
const int joyXpin = A1;  // Pin that detects joysick e-axis postion
const int speedpin = 11;  // Pin that will outputs the speed of the motor
int joyX;
int speed;

void readInputs() {
  // Reads the inputs in the mini-projects
  // Uses txButton, txTilt, txPot, txA, txB, txC, txD
  if (digitalRead(buttonPin) == LOW) {
    txButton = 1;
  } else {
    txButton = 0;
  }
  if (digitalRead(tiltPin) == HIGH) {
    txTilt = 1;  
  } else {
    txTilt = 0;
  }
}

void writeOutputs() {
  // Writes the outputs in the mini-projects
  // Uses rxButton, rxTilt, rxPot, rxA, rxB, rxC, rxD
  digitalWrite(buzzer, rxButton);
  digitalWrite(LED, rxTilt);
 
}

int ledState = LOW; // ledState used to set the LED

char encrypt(char in_char) {
  char out_char;
  out_char = in_char;
  return out_char;
}

char decrypt(char in_char) {
  char out_char;
  out_char = in_char;
  return out_char;
}

void JOYSTICK_FUNCTION() {
  // put your main code here, to run repeatedly:
  int joyX = analogRead(joyXpin);
  speed = map(joyX, 0, 1023, 0, 255);

  analogWrite(speedpin, speed);

  if (joyX < 510) {
    digitalWrite(forwardpin, LOW);
    digitalWrite(backwardpin, HIGH);
  } else if (joyX > 520) {
    digitalWrite(forwardpin, HIGH);
    digitalWrite(backwardpin, LOW);
  } else {
    digitalWrite(forwardpin, LOW);
    digitalWrite(backwardpin, LOW);
  }
}


// The setup routine runs once when you press reset
void setup() {
  // Set the digital pin as output
  pinMode(3, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT); // Initialize the buzzer pin as an output
  pinMode(tiltPin, INPUT_PULLUP);
  pinMode(LED, OUTPUT); // Initialize the LED pin as an output
 

  // Initialize serial communication at 9600 bits per second
  Serial.begin(9600);
}

const long txInterval = 200; // Interval at which to tx bit (milliseconds)
int tx_state = 0;
char chr;
unsigned long previousTxMillis = 0; // Will store last time LED was updated

#define TX_START_OF_TEXT -1
int tx_string_state = TX_START_OF_TEXT;

char getTxChar() {
  char chr;
  switch (tx_string_state) {
    case TX_START_OF_TEXT:
      tx_string_state = 0;
      txBuffer[0] = txButton;
      txBuffer[1] = txTilt;
      return STX;
    default:
      chr = txBuffer[tx_string_state];
      tx_string_state++;
      if (chr == ETX) { // End of string?
        tx_string_state = TX_START_OF_TEXT; // Update the tx string state to start sending the string again
        return ETX; // Send End of Text character
      } else {
        return chr; // Send a character in the string
      }
  }
}

void txChar() {
  unsigned long currentTxMillis = millis();
  if (currentTxMillis - previousTxMillis >= txInterval) {
    // Save the last time you blinked the LED (improved)
    previousTxMillis = previousTxMillis + txInterval; // This version catches up with itself if a delay was introduced
    switch (tx_state) {
      case 0:
        chr = encrypt(getTxChar());
        digitalWrite(3, HIGH); // Transmit Start bit
        tx_state++;
        break;
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
        if ((chr & 0x40) != 0) { // Transmit each bit in turn
          digitalWrite(3, HIGH);
        } else {
          digitalWrite(3, LOW);
        }
        chr = chr << 1; // Shift left to present the next bit
        tx_state++;
        break;
      case 8:
        digitalWrite(3, HIGH); // Transmit Stop bit
        tx_state++;
        break;
      default:
        digitalWrite(3, LOW);
        tx_state++;
        if (tx_state > 20) tx_state = 0; // Start resending the character
        break;
    }
  }
}

const long rxInterval = 20; // Interval at which to read bit (milliseconds)
int rx_state = 0;
char rx_char;
unsigned long previousRxMillis = 0; // Will store last time LED was updated
int rx_bits[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void rxChar() {
  unsigned long currentRxMillis = millis();
  int sensorValue;
  int i;

  if (currentRxMillis - previousRxMillis >= rxInterval) {
    previousRxMillis = previousRxMillis + rxInterval; // This version catches up with itself if a delay was introduced
    sensorValue = analogRead(A0);
    // Serial.println(sensorValue);
    switch (rx_state) {
      case 0:
        if (sensorValue >= 980) {
          rx_bits[0]++;
          rx_state++;
        }
        break;
      case 100:
        if ((rx_bits[0] >= 6) && (rx_bits[8] >= 6)) { // Valid start and stop bits
          rx_char = 0;
          for (i = 1; i < 8; i++) {
            rx_char = rx_char << 1;
            if (rx_bits[i] >= 6) rx_char = rx_char | 0x01;
          }
          rx_char = decrypt(rx_char);
          Serial.println(rx_char + 0);
          switch (rx_char) {
            case STX:
              rx_index = 0;
              break;
            case ETX:
              rxButton = rxBuffer[0];
              rxTilt = rxBuffer[1];
              rx_index = 0;
              break;
            default:
              rxBuffer[rx_index] = rx_char;
              rx_index++;
              break;
          }
        } else {
          Serial.println("Rx error");
        }
        for (i = 0; i < 10; i++) {
          rx_bits[i] = 0;
        }
        rx_state = 0;
        break;
      default:
        if (sensorValue >= 980) {
          rx_bits[rx_state / 10]++;
        }
        rx_state++;
        break;
    }
  }
}

// The loop routine runs over and over again forever
void loop() {
  readInputs();
  JOYSTICK_FUNCTION();
  txChar();
  rxChar();
  writeOutputs();
}