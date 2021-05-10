#include <Arduino.h>
#include <esp8266_pins.h>
#include <max7219.h>
#include <fonts.h>

//default brightness - can be from 0x0 to 0xF
//#define DEFAULT_BRIGHTNESS 0x6U
#define DEFAULT_BRIGHTNESS 0xFU
#define DIRECTION_COUNT 4
#define SNAKE_MAX_TAIL_LENGTH 255

typedef struct {
  int8_t x;
  int8_t y;
} Vec;

Vec head = {};
Vec directions[DIRECTION_COUNT] = {
  {  1,  0 }, // east
  {  0,  1 }, // south
  { -1,  0 }, // west
  {  0, -1 }, // north
};
Vec direction = directions[0];
char previousInput = 'd';
uint8_t currentTailLength = 6;
Vec tailPositions[SNAKE_MAX_TAIL_LENGTH];

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

void set_pixel(uint8_t x, uint8_t y, bool state)
{
  uint8_t columnIndex = (
    (COLUMN_COUNT - 1) // column order is actually right to left? WTF?
    - (x % COLUMN_COUNT)
  );
  uint8_t columnValue = scr[columnIndex];
  uint8_t shiftAmount = (
    (DISPLAY_PIXEL_HEIGHT - 1) - // row order is bottom to top
    (y % DISPLAY_PIXEL_HEIGHT)
  );
  uint8_t shiftedMask = ~ (1 << shiftAmount);
  columnValue = (
    (columnValue & shiftedMask) |
    (state << shiftAmount)
  );
  scr[columnIndex] = columnValue;
}

void display_bounds() {
  set_pixel(
    0,
    0,
    true
  );
  set_pixel(
    0,
    DISPLAY_PIXEL_HEIGHT - 1,
    true
  );
  set_pixel(
    COLUMN_COUNT -1,
    0,
    true
  );
  set_pixel(
    COLUMN_COUNT -1,
    DISPLAY_PIXEL_HEIGHT - 1,
    true
  );
}

void reset_game (char *message) {
  for(uint8_t i = 0; i < SNAKE_MAX_TAIL_LENGTH; i++) {
    tailPositions[i] = {
      .x = 0,
      .y = 0
    };
  }
  head = {
    .x = COLUMN_COUNT / 2,
    .y = DISPLAY_PIXEL_HEIGHT / 2
  };
  direction = directions[0];
  clr();
  render_font_char_to_buffer(message, 0x00, scr);
  refreshAllRot90();
  delay(1000);
}

bool push_current_tail_segment_and_check_collision (Vec head) {
  for(uint8_t i = SNAKE_MAX_TAIL_LENGTH - 1; i > 0; i--) {
    Vec segment = tailPositions[i - 1];
    if (
      i <= currentTailLength &&
      (segment.x == head.x) &&
      (segment.y == head.y)
    ) {
      return true;
    }
    tailPositions[i] = segment;
  }
  tailPositions[0] = head;
  return false;
}

void draw_tail () {
  for(uint8_t i = 0; i < currentTailLength; i++) {
    Vec segment = tailPositions[i];
    set_pixel(
      segment.x,
      segment.y,
      true
    );
  }
}

void handle_input () {
  if (Serial.available() > 0) {
    char input = Serial.read();
    bool handleInput = false;
    Serial.printf(
      "input: %c\n",
      input
    );
    switch (input) {
      case 'd': handleInput = previousInput != 'a'; break;
      case 's': handleInput = previousInput != 'w'; break;
      case 'a': handleInput = previousInput != 'd'; break;
      case 'w': handleInput = previousInput != 's'; break;
      default : handleInput = false; break;
    }
    if (handleInput) {
      switch (input) {
        case 'd': direction = directions[0]; break;
        case 's': direction = directions[1]; break;
        case 'a': direction = directions[2]; break;
        case 'w': direction = directions[3]; break;
        default : direction = directions[0]; break;
      }
      previousInput = input;
    }
  }
}

void move_snek () {
  head.x += direction.x;
  head.y += direction.y;
  head.x %= COLUMN_COUNT;
  head.y %= DISPLAY_PIXEL_HEIGHT;
}

void setup() {
  //init displays:
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1); //turn shutdown mode off
  sendCmdAll(CMD_INTENSITY, DEFAULT_BRIGHTNESS); //set brightness

  reset_game("SNEK!!");

  Serial.begin(9600);
  // space past the garbage that for some reason always comes out on startup
  Serial.println("                ");
  Serial.println("Keyboard input here pls");
}

void loop() {
  handle_input();
  move_snek();
  clr();
  display_bounds();
  bool collided = push_current_tail_segment_and_check_collision(head);
  if (!collided) {
    draw_tail();
    refreshAllRot90();
    delay(100);
  } else {
    reset_game("DEAD :(");
  }
}
