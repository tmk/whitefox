/*
Copyright 2015 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "keymap_common.h"

#include "led_controller.h"

const uint8_t keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* Layer 0: Default Layer
     * ,---------------------------------------------------------------.
     * |Esc|  1|  2|  3|  4|  5|  6|  7|  8|  9|  0|  -|  =|  \|  `|Ins|
     * |---------------------------------------------------------------|
     * |Tab  |  Q|  W|  E|  R|  T|  Y|  U|  I|  O|  P|  [|  ]|Backs|Del|
     * |---------------------------------------------------------------|
     * | Ctrl |  A|  S|  D|  F|  G|  H|  J|  K|  L|  ;|  '|Enter   |PgU|
     * |---------------------------------------------------------------|
     * |Shif|   |  Z|  X|  C|  V|  B|  N|  M|  ,|  .|  /|Shift |Up |PgD|
     * |---------------------------------------------------------------|
     * |FN0 |Alt |Gui |         Space         |Gui |Alt |  |Lef|Dow|Rig|
     * `---------------------------------------------------------------'
     */
    [0] = KEYMAP( \
        ESC, 1,   2,   3,   4,   5,   6,   7,   8,   9,   0,   MINS,EQL, BSLS,GRV, INS, \
        TAB, Q,   W,   E,   R,   T,   Y,   U,   I,   O,   P,   LBRC,RBRC,BSPC,     DEL, \
        LCTL,A,   S,   D,   F,   G,   H,   J,   K,   L,   SCLN,QUOT,NUHS,ENT,      PGUP,\
        LSFT,NUBS,Z,   X,   C,   V,   B,   N,   M,   COMM,DOT, SLSH,RSFT,     UP,  PGDN,\
        FN0, LALT,LGUI,               SPC,           RGUI,RALT,NO,       LEFT,DOWN,RGHT \
    ),
    /* Layer 1: HHKB-style FN-layer plus extras
     * ,---------------------------------------------------------------.
     * | ` | F1| F2| F3| F4| F5| F6| F7| F8| F9|F10|F11|F12|Ins|Del|Btl|
     * |---------------------------------------------------------------|
     * |Caps |   |Wak|Slp|   |   |   |   |PSc|SLk|Pau| Up|   |     |FN1|
     * |---------------------------------------------------------------|
     * |      |Vo-|Vo+|VoM|Ejc|   | * | / |Hom|PgU|Lef|Rgt| Enter  |FN2|
     * |---------------------------------------------------------------|
     * |    |   |FN5|FN6|   |   |   | + | - |End|PgD|Dow|      |   |FN3|
     * |---------------------------------------------------------------|
     * |    |    |    |                  |    |    |    |  |   |   |   |
     * `---------------------------------------------------------------'
     */
    [1] = KEYMAP( \
        GRV, F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9,  F10, F11, F12, INS, DEL, BTLD,\
        CAPS,TRNS,WAKE,SLEP,TRNS,TRNS,TRNS,TRNS,PSCR,SLCK,PAUS,UP,  TRNS,TRNS,     FN1, \
        TRNS,VOLD,VOLU,MUTE,EJCT,TRNS,PAST,PSLS,HOME,PGUP,LEFT,RGHT,TRNS,PENT,     FN2, \
        TRNS,TRNS,FN5 ,FN6 ,TRNS,TRNS,TRNS,PPLS,PMNS,END, PGDN,DOWN,TRNS,     TRNS,FN3, \
        TRNS,TRNS,TRNS,               TRNS,          TRNS,TRNS,TRNS,     TRNS,TRNS,TRNS \
    ),
};

/* Give numbers some descriptive names */
#define ACTION_LEDS_ALL 1
#define ACTION_LEDS_GAME 2

const uint16_t fn_actions[] = {
    [0] = ACTION_LAYER_MOMENTARY(1),
    // The following two will call action_function with parameter
    [1] = ACTION_FUNCTION(ACTION_LEDS_ALL),
    [2] = ACTION_FUNCTION(ACTION_LEDS_GAME),
};

/* custom action function */
void action_function(keyrecord_t *record, uint8_t id, uint8_t opt) {
  (void)opt;
  switch(id) {
    case ACTION_LEDS_ALL:
        if(record->event.pressed) {
          // signal the LED controller thread
          chMBPost(&led_mailbox, LED_MSG_GAME_TOGGLE, TIME_IMMEDIATE);
        }
      break;
    case ACTION_LEDS_GAME:
      if(record->event.pressed) {
        // signal the LED controller thread
        chMBPost(&led_mailbox, LED_MSG_ALL_TOGGLE, TIME_IMMEDIATE);
      }
      break;
  }
}
