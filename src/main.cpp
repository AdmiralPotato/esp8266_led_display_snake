#include <Arduino.h>
#include <esp8266_pins.h>
#include <max7219.h>
#include <fonts.h>

//default brightness - can be from 0x0 to 0xF
//#define DEFAULT_BRIGHTNESS 0x6U
#define DEFAULT_BRIGHTNESS 0xFU

uint8_t goatCount = 0;
char message[16];

// ref: https://stackoverflow.com/a/2603254/1053092
static unsigned char lookup[16] = {
0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf, };

uint8_t reverse(uint8_t n) {
   // Reverse the top and bottom nibble then swap them.
   return (lookup[n&0b1111] << 4) | lookup[n>>4];
}

void render_font_char_to_buffer (char *string, int x_offset, uint8_t *buffer)
{
  uint16_t row_count = NUM_MAX*8;
  uint8_t font_data_width = pgm_read_byte(font);
  uint8_t font_char_width;
  uint8_t font_char_column;
  size_t font_data_offset;
  size_t destination_address;
  size_t character_offset = 0;
  char character;
  while ((character = string[character_offset]) != '\0')
  {
    font_data_offset = 1 + (font_data_width * (character - 32));
    font_char_width = pgm_read_byte(font + font_data_offset);
    font_char_column = 0;
    while (font_char_column < font_char_width)
    {
      destination_address = row_count - (x_offset + font_char_column + 1);
      // don't write out of screen buffer
      if (destination_address < SCR_LENGTH)
      {
        scr[destination_address] = reverse(pgm_read_byte(font + font_data_offset + 1 + font_char_column));
      }
      font_char_column++;
    }
    x_offset += font_char_width + 1;
    character_offset++;
  }
}

void setup() {
  //init displays:
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1); //turn shutdown mode off
  sendCmdAll(CMD_INTENSITY, DEFAULT_BRIGHTNESS); //set brightness

  //print an init message to the display:
  render_font_char_to_buffer("HELLO", 0x00, scr);
  refreshAllRot90();

  Serial.begin(9600);
  // space past the garbage that for some reason always comes out on startup
  Serial.println("                ");
  Serial.println();
}

void loop() {
  sprintf(
    message,
    "GOAT %d",
    goatCount
  );
  Serial.println(message);
  goatCount++;
  clr();
  render_font_char_to_buffer(message, 0x00, scr);
  refreshAllRot90();
  delay(200);
}
