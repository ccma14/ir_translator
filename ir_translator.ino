// ir_translator.ino
//
// (C) 2020, Christian Czezatke
//
// This sketch enables an FX Audio D802 receiver to be remote-controlled with an
// IR remote that only supports a pre-defined set of IR commands (as opposed to a
// "learning remote".)
//
// Manufacturer: http://www.szfxaudio.com/en/index.html
// Developed/tested with this amp: https://www.amazon.com/D802-192KHz-Digital-Remote-Amplifier/dp/B00WU6JU9Y
//
// By default this sketch uses the Volume up/down/mute command codes that are emitted
// by the remote for Google's "Chromecast with Google TV" when choosing the "Panasonic
// Sound Bar" command code. -- If such commands are recognized, they are translated
// into the corresponding IR commands understood by the FX Audio D802 receiver, allowing
// you to remote-control it as if it were a Panasonic Sound Bar.
//
// Any IR command that is received and not recognized is passed through unmodified.
//
// NOTE: There is now a newer version of the FX Audio D802 available (FX Audio D802C) that adds
//       Bluetooth support. It is not known whether this amp uses the same IR codes, since I
//       do not have access to one.

#include <Arduino.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>


// If non-zero, each captured IR command will be dumped to serial.
#define DUMP_CAPTURED_IR_COMMAND 0


// Speed for serial communication
#define SERIAL_BAUD_RATE 115200


// Settings for the input code that is supposed to trigger volume up/down/mute
// on the target FX Audio D802 amp. -- We use the "Panasonic Sound Bar" setting from
// Google's "Chromecast with Google TV" remote, which is nice and simple as it
// does not use any repeat codes and IRRremoteESP8266 can decode Panasonic remote
// protocol.
#define SB_TYPE PANASONIC
#define SB_ADDRESS 0x4004
#define SB_VOLUME_UP 0x5000401
#define SB_VOLUME_DOWN 0x5008481
#define SB_VOLUME_MUTE 0x5004c49


// Settings for the target FX Audio D802 commands to emit. It expects NEC style
// commands, hence we only need to hang on to it's device address and the single
// byte command code for each command.
#define FX_AUDIO_ADDRESS 0x6b86
#define FX_AUDIO_NOOP 0  // Indicates no command was recognized; not actually used by remote.
#define FX_AUDIO_VOLUME_UP 0x6
#define FX_AUDIO_VOLUME_DOWN 0x4
#define FX_AUDIO_MUTE 0x12


// Time window within which an identical recognized input command causes us to transmit
// the NEC repeat code rather than the command itself again. With the Pansonic sound bar
// used above, we have seen an initial total delay of ~260ms before the first re-send of
// a command when pushing and holding a button, so this needs to be larger than this value.
// 500ms seems reasonable...
#define MAX_REPEAT_DELAY_MS 500


// IR Send/Receive pins. The recv pin must support interrupts, so any
// pin except for GPIO16 is fine.
#define IR_RECV_PIN 14
#define IR_SEND_PIN 4

// The default on/off frequency for IR remotes is 38kHz, which is what
// we use to send our messages.
#define IR_FREQUENCY 38000


// Raw send buffer representing a NEC repeat command.
const uint16_t nec_repeat_raw[3] = {9248, 2228,  620};  // NEC (Repeat) FFFFFFFFFFFFFFFF


// The IR transmitter.
IRsend irsend(IR_SEND_PIN);
// The IR receiver.
IRrecv irrecv(IR_RECV_PIN);

// Timestamp when the previous command was sent.
unsigned long prev_timestamp;

// Previous FX Audio 802 specific NEC command that was sent, if any.
uint8_t prev_nec_command;

// Somewhere to store the captured message. -- This needs to be outside "loop" even
// tho it is not used anywhere else.
decode_results results;



// This section of code runs only once at start-up.
void setup() {
  irrecv.enableIRIn();  // Start up the IR receiver.
  irsend.begin();       // Start up the IR sender.

  Serial.begin(SERIAL_BAUD_RATE, SERIAL_8N1);
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();

  Serial.printf("FX Audio IR translator receiving on GPIO%d, and transmitting on GPIO%d is ready.\n",
                IR_RECV_PIN, IR_SEND_PIN); 
  
  prev_timestamp = millis();
  prev_nec_command = FX_AUDIO_NOOP;
}


// The repeating section of the code
void loop() {  
  // Check if an IR message has been received.
  if (irrecv.decode(&results)) {  // We have captured something.
    unsigned long now = millis();
    uint8_t nec_command = FX_AUDIO_NOOP;

    if (DUMP_CAPTURED_IR_COMMAND) {
      Serial.print(resultToHumanReadableBasic(&results));
      Serial.print(resultToSourceCode(&results));
    }

    if (results.decode_type == SB_TYPE && results.address == SB_ADDRESS) {
      switch(results.command) {
        case SB_VOLUME_UP:
            nec_command = FX_AUDIO_VOLUME_UP;
            Serial.printf("IN: Volume UP!\n");
            break;
        case SB_VOLUME_DOWN:
            Serial.printf("IN: Volume DOWN!\n");
            nec_command = FX_AUDIO_VOLUME_DOWN;
            break;
        case SB_VOLUME_MUTE:
            Serial.printf("IN: MUTE\n");
            nec_command = FX_AUDIO_MUTE;
            break;
        default:
            nec_command = FX_AUDIO_NOOP;
            break;
      }
    }

    if (nec_command == FX_AUDIO_NOOP) {
      // No command recognized. -- Re-send raw data.
      uint16_t *raw_send = resultToRawArray(&results);
      uint16_t raw_size = getCorrectedRawLength(&results);
      irsend.sendRaw(raw_send, raw_size, IR_FREQUENCY);
      delete[] raw_send;
      Serial.printf("PASSTHRU\n");
    } else {
      // See if we have to send a repeat command.
      if ((now - prev_timestamp) < MAX_REPEAT_DELAY_MS && prev_nec_command == nec_command) {
         // Previous command code matches and was sent within repeat time window.
         irsend.sendRaw(nec_repeat_raw, sizeof(nec_repeat_raw)/sizeof(uint16_t), IR_FREQUENCY);
         Serial.printf("OUT: NEC REPEAT\n");
      } else {
         // Send the NEC remote command the FX Audio 802 amp expects.
         uint32_t nec_send = irsend.encodeNEC(FX_AUDIO_ADDRESS, nec_command);
         irsend.sendNEC(nec_send);
         Serial.printf("OUT NEC: %x\n", nec_send);
      }
    }
    prev_nec_command = nec_command;
    prev_timestamp = now;
    Serial.printf("===\n");
    irrecv.resume();  // Receive the next command.
  }
  yield();  // Allow ESP8266 to run its housekeeping tasks.
}
