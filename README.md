[English](https://github.com/kotelnikov/mgos_telegram/blob/main/README.md) |
[Русский](https://github.com/kotelnikov/mgos_telegram/blob/main/README.ru-Ru.md)

# Telegram Bot API for Mongoose OS

## Overview

This library represents a simple [Telegram Bot API](https://core.telegram.org/bots) for [Mongoose OS](https://mongoose-os.com/) - IOT framework developed by [Cesanta Software Ltd](https://cesanta.com/). The library could be useful for applications for control and manage IOT devices through the Telegram Bot API. The library successfully tested on ESP8266/ESP32 from [Espressif Systems](https://www.espressif.com/). Any issues or suggestions are welcomed!

For using the library you have to provide the internet connection for your IOT device by using the [Wi-Fi](https://mongoose-os.com/docs/mongoose-os/api/net/wifi.md), by using the cellular connection using [PPPOS](https://mongoose-os.com/docs/mongoose-os/api/net/pppos.md) or [Ethernet](https://mongoose-os.com/docs/mongoose-os/api/net/ethernet.md).

The library supports Telegram Bot API updates: 
  - [Message](https://core.telegram.org/bots/api#message) 
  - [CallbackQuery](https://core.telegram.org/bots/api#callbackquery)

The library supports Telegram Bot API requests: 
  - [sendMessage](https://core.telegram.org/bots/api#sendmessage)
  - [editMessageText](https://core.telegram.org/bots/api#editmessagetext)
  - [answerCallbackQuery](https://core.telegram.org/bots/api#answercallbackquery)

If you found this library useful for your project, please give a star for repository! ⭐

### Demonstration example:

<img src="https://github.com/kotelnikov/mgos_telegram/blob/master/control_panel.gif" height="500"/>

## Configuration

Firstly include the library to your application by adding the following to your `mos.yml` file in section `libs`:

```yaml
libs:
  - location: https://github.com/kotelnikov/mgos_telegram
    name: telegram
```

Next configure the library by adding the following properties to your `mos.yml` in section `config_schema`:

```yaml
config_schema:
  - ["telegram.enable", true]
  - ["telegram.token", "1111222333:YourTokenGoesHere"]
  - ["telegram.echo_bot", false]
  - ["telegram.acl", "[555777888, 222333444]"]
```
Description for `config_schema` section:

Property | Type | Description
------------ | ------------- | -------------
`telegram.enable` | `boolean` | This property enables the library. By default the library disabled, you have to enable it by setting value `true`.
`telegram.token` | `string` | This property stores your telegram token represented by the string. If you don't have your own token yet, you can find How-to instructions here - [Creating a new bot](https://core.telegram.org/bots#creating-a-new-bot).
`telegram.echo_bot` | `boolean` | Property switches on/off echo mode. Pay attention - this mode enabled by default, so in productive you have to turn it to `false`.  In case you want to test the library, you don't have to write absolutely any code just leave this option as `true`. In this case all received messages will be immediately send back to the sender.
`telegram.acl` | `string` | Property stores the User access list (ACL) represented by JSON serialized string containing an array of the User IDs. If `telegram.echo_bot` property will be `false` and ACL list will be empty or new update arrived from the user not included in the ACL, all incoming updates (messages) will be ignored by the library. If you don't know how to get your user id, you can "ask" the Bot `@myidbot` (just subscribe for the Bot and then sent him the command `/getid`). Also you can find your user id by analyzing the serial monitor output. Information about received updates and whom it comes from will be shown in the console.


# JS API reference

## Event.addHandler()

Before we can receive an update or make requests to/from Telegram Bot API, we have to wait when the library successfully connects to the Telegram Bot API server. For this purpose add handler for `TGB.CONNECTED` event. The event handler usually contains subscribing functions for text commands and callback queries. `Events` it is a Core functionality of the Mongoose OS, you can find additional information here: [Event](https://mongoose-os.com/docs/mongoose-os/api/core/mgos_event.h.md).

```js
Event.addHandler(ev, callback, userdata);

// Callback should look like:
// function(ev, evdata, userdata) { /* Do some stuff here */ }

// Connection handler
let tgb_connect_handler = function(ev, ed, ud) {
  print("Connected to Telegram API");
  TGB.subscribe('/start', updates_handler, null);
};

// Add handler for TGB.CONNECTED event
Event.addHandler(TGB.CONNECTED, tgb_connect_handler, null);
```

## TGB.subscribe()

Use this method for subscribing for updates from the Telegram Bot API. You can subscribe for all commands at one subscription "*" or subscribe for certain commands you need. All updates not in the subscription list will be ignored by the library.

```js
TGB.subscribe(data, callback, userdata);

// Callback should look like:
// function(ev_data, userdata) { /* Do some stuff here */ }

// Subscribe to all received data by one subscription "*"
TGB.subscribe('*', app_updates_handler, null);
// Or you can subscribe to certain commands
TGB.subscribe('/start', app_updates_handler, null);
```

## TGB.send(), TGB.send(), TGB.send_cb(), TGB.send_js(), TGB.send_js_cb()

Use this methods for send messages to the Telegram chat or group. You can send simple text messages or you can also send messages by using native Telegram bot API for [sendMessage](https://core.telegram.org/bots/api#sendmessage) method.

```js
TGB.send(chat_id, text);
TGB.send_cb(chat_id, text, cb, ud);
TGB.send_js(js_obj);
TGB.send_js_cb(js_obj, cb, ud);

// Response callback example
let response_handler = function(ed, ud) {
    // Parse response data
    let resp = TGB.parse_response(ed);
    // Print to console parsed response data
    print(JSON.stringify(resp));
};

// JS object with keys according to the native API
let js_odj = {
  chat_id: 111222333,
  text: 'Hello from Mongoose OS'
};

TGB.send(111222333, 'Hello from Mongoose OS');
TGB.send_cb(111222333, 'Hello from Mongoose OS', response_handler, null);
TGB.send_js(js_odj);
TGB.send_js_cb(js_odj, response_handler, null);
```

## TGB.update(), TGB.update_js()

Use these methods to update certain messages. You can send the update as simple text message, or you can also update message by using native Telegram bot API for [editMessageText](https://core.telegram.org/bots/api#editmessagetext) method.

```js
TGB.update(chat_id, message_id, text);
TGB.update_js(js_obj);

// JS object with keys according to the native API
let js_odj = {
  chat_id: 111222333,
  message_id: 1122,
  text: 'Updated message by Mongoose OS'
};

TGB.update(111222333, 1122, 'Updated message by Mongoose OS');
TGB.update_js(js_odj);
```

## TGB.answer(), TGB.answer_js()

Use these methods to send answers to callback queries sent from inline keyboards. The answer will be displayed to the user as a notification at the top of the chat screen or as an alert. You can send the simple text answer, or you can use the native Telegram bot API for [answerCallbackQuery](https://core.telegram.org/bots/api#answercallbackquery) method.

```js
TGB.answer(id, text, alert);
TGB.answer_js(js_obj);

// JS object with keys according to the native API
let js_odj = {
  callback_query_id: '933786469216003032',
  text: '⚠ Command received',
  show_alert: false
};

TGB.answer('933786469216003032', '⚠ Command received', false);
TGB.answer_js(js_odj);
```

## TGB.custom(), TGB.custom_cb()

Use these methods to execute the custom Telegram Bot API method according to the [API](https://core.telegram.org/bots/api#available-methods).

```js
TGB.custom(method, js_obj);
TGB.custom_cb(method, js_obj, cb, ud);

// Response callback example
let response_handler = function(ed, ud) {
    // Parse response data
    let resp = TGB.parse_response(ed);
    // Print to console parsed response data
    print(JSON.stringify(resp));
};

// JS object with keys for sendMessage method, according to the native API
let js_odj = {
  chat_id: 111222333,
  text: 'Hello from Mongoose OS'
};

TGB.custom('sendMessage', js_odj);
TGB.custom_cb('sendMessage', js_odj, response_handler, null);
```

## TGB.parse_update(), TGB.parse_response()

Use these methods for parsing update and response data from Telegram Bot API. Before we can work with receiving data we have to parse it to JS object. These methods return JS object.

```js
TGB.parse_update(ed);
TGB.parse_response(ed);

// Update callback example
let update_handler = function(ed, ud) {
    // Parse update data
    let upd = TGB.parse_update(ed);
    // Print to console parsed update data
    print(JSON.stringify(upd));
};

// Response callback example
let response_handler = function(ed, ud) {
    // Parse response data
    let resp = TGB.parse_response(ed);
    // Print to console parsed response data
    print(JSON.stringify(resp));
};

// Update data example in case of an Error
{
  ok: false, 
  error_code: 420, 
  description: 'Flood'
}

// Update data example in case of Message
{
  update_id: 532671789,
  type:1, // 1 -> MESSAGE
  message_id: 1464,
  data: "/menu",
  user_id: 111222333, 
  chat_id: -444555666 // If negative it is a group chat
}

// Update data example in case of Callback query
{
  update_id: 532671790,
  type:2, // 2 -> CALLBACK QUERY
  callback_query_id: "933786467781980781",
  data: "/alarm_toggle",
  message_id: 1464,
  user_id: 111222333,
  chat_id: -444555666 // If negative it is a group chat
}

// Response data example in case of an Error
{
  ok: false, 
  error_code: 420, 
  description: 'Flood'
}

// Response data example in case of response to sendMessage
{
  ok: true, 
  message_id: 1464, 
  chat_id: -444555666 // If negative it is a group chat
}

// Response data example in case of all other methods
{
  ok: true
}
```

## Complete JS examples

#### Example 1. Text messaging.

```js
load("api_events.js");
load('api_telegram.js');

// Handler for managing updates
let updates_handler = function(ed, ud) {
    // Parse update data
    let upd = TGB.parse_update(ed);
    // Print to console parsed update data
    print(JSON.stringify(upd));
    // Handling the '/start' command
    if (upd.data === '/start') {
        print('Telegram: mJS "/start" handler');
        TGB.send(upd.chat_id, 'Received command "/start"');
    }
    // Handling the '/status' command
    else if (upd.data === '/status') {
        print('Telegram: mJS "/status" handler');
        TGB.send(upd.chat_id, 'Received command "/status"');
    }
    // Handling all other commands in case we subscribed for all
    else {
        print('Telegram: mJS "*" handler');
        TGB.send(upd.chat_id, upd.data);
    }
};

// Handler for make subscriptions
let app_start_handler = function(ev, ed, ud) {
    // Subscribe to all received data by one subscription "*"
    TGB.subscribe('*', updates_handler, null);
    // Or you can subscribe to certain commands
    //TGB.subscribe('/start', updates_handler, null);
    //TGB.subscribe('/status', updates_handler, null);
};

// Add handler for TGB.CONNECTED event
Event.addHandler(TGB.CONNECTED, app_start_handler, null);
```

#### Example 2. Multi-user control panel with inline control buttons and state updates.

```js
load('api_sys.js');
load("api_events.js");
load('api_telegram.js');

// Fake relay variables
let alarm = false;
let light = false;

// Control panel builder object
let control_panel = {
	// Hidden method for getting current state
    _get_state: function() {
        let l1 = 'FREE MEM: ' + JSON.stringify(Sys.free_ram()) + '\r\n';
        let l2 = 'UPTIME: ' + JSON.stringify(Sys.uptime()) + '\r\n';
        let l3 = alarm ? 'ALARM: ✅ ON\r\n' : 'ALARM: ❌ OFF\r\n';
        let l4 = light ? 'LIGHT: ✅ ON' : 'LIGHT: ❌ OFF';
        return l1 + l2 + l3 + l4;
    },
	// Hidden method for getting current buttons state
    _get_keyboard: function() {
        return [
            [{text: alarm ? 'SWITCH OFF ALARM' : 'SWITCH ON ALARM', callback_data: '/alarm_toggle'}],
            [{text: light ? 'SWITCH OFF LIGHT' : 'SWITCH ON LIGHT', callback_data: '/light_toggle'}]
        ];
    },
	// Method for getting JS object with current control panel 
    get_panel: function() {
        return {
            text: this._get_state(),
            reply_markup: { 
                inline_keyboard: this._get_keyboard()
            }
        };
    },
    // Array for store all panels in all chats
    all_panels: []
};

// Telegram updates handler
let updates_handler = function(ed, ud) {
    // Parse update
    let upd = TGB.parse_update(ed);
    // Print to console parsed update data
    print(JSON.stringify(upd));
    // Handle the '/menu' command
    if (upd.data === '/menu') {
	    // Check the existence of the control panel in the chat
        for (let i = 0; i < control_panel.all_panels.length; i++) {
            // If the panel exist return 
            if (control_panel.all_panels[i].chat_id === upd.chat_id) { return; }
        }
        // Otherwise build new control panel for the chat
        // Build control panel to the JS object
        let p = control_panel.get_panel();
        // Add to this object key "chat_id" from update
        p.chat_id = upd.chat_id;
        // Then send panel to the chat
        TGB.send_js_cb(p, response_handler, null);
        return;
    }
    // Handle the '/alarm_toggle' callback query
    else if (upd.data === '/alarm_toggle') { alarm = !alarm; }
    // Handle the '/light_toggle' callback query
    else if (upd.data === '/light_toggle') { light = !light; }
    // Handle the '/light_toggle' callback query    
    else { return; }
    // Next we send back answer callback query (command received acknowledgment by app)
    TGB.answer(upd.callback_query_id, '⚠ Command received', false);
    // Update control panels in all chats
    let p = control_panel.get_panel();
    for (let i = 0; i < control_panel.all_panels.length; i++) {
        p.chat_id = control_panel.all_panels[i].chat_id;
        p.message_id = control_panel.all_panels[i].message_id;
        // Update panel instance        
        TGB.update_js(p);
    }
};

// Telegram response handler
let response_handler = function(ed, ud) {
    // Parse response data
    let resp = TGB.parse_response(ed);
    // Print to console parsed response data
    print(JSON.stringify(resp));
    // Search the control panel already exist in the that
    for (let i = 0; i < control_panel.all_panels.length; i++) {
        // If it already exist nothing to do, return
        if (resp.chat_id === control_panel.all_panels[i].chat_id) { return; }
    }
    // If there is no control panel in this chat add to array information about new panel
    control_panel.all_panels.push({chat_id: resp.chat_id, message_id: resp.message_id});
};

// Telegram subscribing handler
let app_start_handler = function(ev, ed, ud) {
    // Subscribe for text command '/menu'
    TGB.subscribe('/menu', updates_handler, null);
    // Subscription for text command and callback query is the same
    // Subscribe for callback query
    TGB.subscribe('/alarm_toggle', updates_handler, null);
    TGB.subscribe('/light_toggle', updates_handler, null);
};

// Adding the handler for TGB.CONNECTED event
Event.addHandler(TGB.CONNECTED, app_start_handler, null);
```

# C API reference

## mgos_event_add_handler()

Before we can receive updates and make requests to/from Telegram Bot API, we have to wait when the library successfully connects to the Telegram Bot API server. For this purpose add handler for `TGB_EV_CONNECTED` event. The handler usually contains subscribing functions to the text commands or/and callback queries. `Events` it is a Core functionality of the Mongoose OS, you can find additional information here: [Event](https://mongoose-os.com/docs/mongoose-os/api/core/mgos_event.h.md).

```C
bool mgos_event_add_handler(int ev, mgos_event_handler_t cb, void *userdata);

// Event handler signature
typedef void (*mgos_event_handler_t)(int ev, void *ev_data, void *userdata);

// Subscription callback
void subscription_callback(void *ev_data, void *userdata) {
  // Subscribing for command "/start"
  mgos_telegram_subscribe("/start", app_telegram_updates_handler, NULL);
}

// Adding handler for TGB_EV_CONNECTED event
mgos_event_add_handler(TGB_EV_CONNECTED, subscription_callback, NULL);
```

## mgos_telegram_subscribe()

Use this function for subscribing for updates from the Telegram Bot API. You can subscribe for all commands at one subscription "*" or subscribe for certain commands you need. All updates not in the subscription list will be ignored by the library.

```C
void mgos_telegram_subscribe(const char *data, mgos_telegram_cb_t callback, void *userdata);

// Telegram handler signature
typedef void (*mgos_telegram_cb_t)(void *ev_data MG_UD_ARG(void *userdata));

// Subscribing for all received commands at one subscription
mgos_telegram_subscribe("*", app_telegram_updates_handler, NULL);

// Subscribing for command "/start"
mgos_telegram_subscribe("/start", app_telegram_updates_handler, NULL);
```

## mgos_telegram_send_message(), mgos_telegram_send_message_with_callback(), mgos_telegram_send_message_json(), mgos_telegram_send_message_json_with_callback()

Use this functions for send messages to the Telegram chat or group. You can send simple text messages or you can also send messages by using native Telegram bot API for [sendMessage](https://core.telegram.org/bots/api#sendmessage) method.

```C
void mgos_telegram_send_message(int32_t chat_id, const char *text);
void mgos_telegram_send_message_with_callback(int32_t chat_id, const char *text, mgos_telegram_cb_t callback, void *userdata);
void mgos_telegram_send_message_json(const char *json);
void mgos_telegram_send_message_json_with_callback(const char *json, mgos_telegram_cb_t callback, void *userdata);

// Callback example
void callback(void *ev_data, void *userdata) {
    // With ev_data param it will be the response from telegram API for sendMessage method
    struct mgos_telegram_response *response = (struct mgos_telegram_response *) ev_data;
    // With userdata param it will be userdata passed earlier to the send message function
    (void) userdata;
}

// Sending simple text messages to the chat/group
mgos_telegram_send_message(111222333, "Hello from Mongoose OS");
// The same with callback
mgos_telegram_send_message_with_callback(111222333, "Hello from Mongoose OS", callback, NULL);

// Build json string according to the native API (here we using frozen parser/emitter lib https://github.com/cesanta/frozen)
char *json = json_asprintf("{chat_id: %d, text: %Q}", 111222333, "Hello from Mongoose OS");
// Sending messages to the chat/group according to the native API
mgos_telegram_send_message_json(json);
// The same with callback
mgos_telegram_send_message_json_with_callback(json, callback, NULL)
free(json);
```

## mgos_telegram_edit_message_text(), mgos_telegram_edit_message_text_json()

Use this functions to update certain messages. You can send the update as simple text message or you can also update message by using native Telegram bot API for [editMessageText](https://core.telegram.org/bots/api#editmessagetext) method.

```C
void mgos_telegram_edit_message_text(int32_t chat_id, int32_t message_id, const char *text);
void mgos_telegram_edit_message_text_json(const char *json);

// Update as simple text message
mgos_telegram_edit_message_text(111222333, 1237, "Updated message from Mongoose OS");

// Build json string according to the native API (here we using frozen parser/emitter lib https://github.com/cesanta/frozen)
char *json = json_asprintf("{chat_id: %d, message_id: %d, text: %Q}", 111222333, 1237, "Updated message from Mongoose OS");
// Update message according to the native API
mgos_telegram_edit_message_text_json(json);
```
## mgos_telegram_answer_callback_query(), mgos_telegram_answer_callback_query_json()

Use this functions to send answers to callback queries sent from inline keyboards. The answer will be displayed to the user as a notification at the top of the chat screen or as an alert. You can send the simple text answer or you can use the native Telegram bot API for [answerCallbackQuery](https://core.telegram.org/bots/api#answercallbackquery) method.

```C
void mgos_telegram_answer_callback_query(const char *id, const char *text, bool alert);
void mgos_telegram_answer_callback_query_json(const char *json);

// Send simple answer
mgos_telegram_answer_callback_query("933786469216003032", "⚠ Command received", false);

// Build json string according to the native API (here we using frozen parser/emitter lib https://github.com/cesanta/frozen)
char *json = json_asprintf("{callback_query_id: %s, text: %s, show_alert: %B}", "933786469216003032", "⚠ Command received", false);
// Send answer according to the native API
mgos_telegram_answer_callback_query_json(json);
free(json);
```

## mgos_telegram_execute_custom_method(), mgos_telegram_execute_custom_method_with_callback()

Use this functions to call custom Telegram Bot API methods, according to the [Telegram Bot API](https://core.telegram.org/bots/api#available-methods).

```C
void mgos_telegram_execute_custom_method(const char *method, const char *json);
void mgos_telegram_execute_custom_method_with_callback(const char *method, const char *json, mgos_telegram_cb_t callback, void *userdata);

// Callback example
void callback(void *ev_data, void *userdata) {
    // With ev_data param it will be the response from telegram API for sendMessage method
    struct mgos_telegram_response *response = (struct mgos_telegram_response *) ev_data;
    // With userdata param it will be userdata passed earlier to the send message function
    (void) userdata;
}

// Build json string according to the native Telegram Bot API (here we using frozen parser/emitter lib https://github.com/cesanta/frozen)
char *json = json_asprintf("{chat_id: %d, text: %Q}", 111222333, "Hello from telegram bot lib");

//For example execute sendMessage method 
mgos_telegram_execute_custom_method("sendMessage", json);

//The same with callback
mgos_telegram_execute_custom_method_with_callback("sendMessage", json, callback, NULL);
```

## Complete C code examples

#### Example 1. Text messaging.

```C
#include "mgos.h"
#include "mgos_telegram.h"

void app_start_handler(int ev, void *ev_data, void *userdata);
void app_telegram_updates_handler(void *ev_data, void *userdata);

// Application start handler
void app_start_handler(int ev, void *ev_data, void *userdata) {
  (void) ev;
  (void) ev_data;
  (void) userdata;
  // Subscribe to all received commands at one subscription "*"
  mgos_telegram_subscribe("*", app_telegram_updates_handler, NULL);
  // Or subscribe to certain commands
  // mgos_telegram_subscribe("/start", app_telegram_updates_handler, NULL);
  // mgos_telegram_subscribe("/status", app_telegram_updates_handler, NULL);
}

// Telegram updates handler
void app_telegram_updates_handler(void *ev_data, void *userdata) {
    (void) userdata;
    // The message data will be passed with ev_data param
    // We have to explain what kind of data it is (it will be type of struct mgos_telegram_response)
    struct mgos_telegram_update *update = (struct mgos_telegram_update *) ev_data;
	// Print to the terminal update details 
    LOG(LL_INFO, ("APP ->> chat: %li, data: %s", (long)update->chat_id, update->data));
    // Handle the "/start" command
    if ( strcmp(update->data, "/start") == 0 ) {
        // Send message to the chat
        mgos_telegram_send_message(update->chat_id, "Hello from Mongoose OS");
    }
    // Handle all another commands
    else {
        // Send message back with received text
        mgos_telegram_send_message(update->chat_id, update->data);
    }
    return;
}

// APP INIT FN
enum mgos_app_init_result mgos_app_init(void) {
    // Adding the handler for TGB_EV_CONNECTED event
    mgos_event_add_handler(TGB_EV_CONNECTED, app_start_handler, NULL);
    return MGOS_APP_INIT_SUCCESS;
};
```

#### Example 2. Multi user control panel with inline keyboard and state updates.

```C
#include "mgos.h"
#include "mgos_telegram.h"

void app_start_handler(int ev, void *ev_data, void *userdata);
char* get_panel_state();
char* get_panel_keyboard();
void update_all_panels();
void app_telegram_updates_handler(void *ev_data, void *userdata);
void app_telegram_response_handler(void *ev_data, void *userdata);

// Control panel element prototype
struct panel_el {
  uint32_t chat_id;
  uint32_t message_id;
  SLIST_ENTRY(panel_el) next;
};

// State struct prototype
struct app_state {
  // Fake relays
  bool alarm;
  bool light;
  // List for storing all panels in all chat
  SLIST_HEAD(panels, panel_el) panels;
};

// Variable for storing the state
struct app_state *state = NULL;

// Function returning current state 
char* get_panel_state() {
  char *res = NULL;
  mg_asprintf(&res, 0, "%s%u\r\n%s%f\r\n%s%s\r\n%s%s",
              "FREE MEM: ", mgos_get_heap_size(),
              "UPTIME: ", mgos_uptime(),              
              "ALARM: ", state->alarm ? "✅ ON" : "❌ OFF",
              "LIGHT: ", state->light ? "✅ ON" : "❌ OFF"
              );
  return res;
}
// Function returning current button state 
char* get_panel_keyboard() {
  char *res = json_asprintf("{inline_keyboard: [[{text: %Q, callback_data: %Q}],[{text: %Q, callback_data: %Q}]]}", 
                state->alarm ? "SWITCH OFF ALARM" : "SWITCH ON ALARM", "/alarm_toggle",
                state->light ? "SWITCH OFF LIGHT" : "SWITCH ON LIGHT", "/light_toggle"
                );
  return res;
}
// Function for update all control panels in all chat
void update_all_panels() {
  // If panels list is empty return
  if (SLIST_EMPTY(&state->panels)) return;
  // Getting current state
  char *text = get_panel_state();
  // Getting current button state 
  char *keyboard = get_panel_keyboard();
  char *json = NULL;
  struct panel_el *panel;
  // Iterate panels list and update each panel
  SLIST_FOREACH(panel, &state->panels, next) {
    json = json_asprintf("{chat_id: %d, message_id: %d, text: %Q, reply_markup: %s}", 
      panel->chat_id, panel->message_id, text, keyboard);
    mgos_telegram_edit_message_text_json(json);
  }
  // Do not forget to free allocated memory
  free(json);
  free(text);
  free(keyboard);
}
// Telegram subscribing handler
void app_start_handler(int ev, void *ev_data, void *userdata) {
  (void) ev;
  (void) ev_data;
  (void) userdata;
  // Subscribe to all received commands at one subscription
  mgos_telegram_subscribe("*", app_telegram_updates_handler, NULL);
  // Or subscribe to certain commands
  // mgos_telegram_subscribe("/menu", app_telegram_updates_handler, NULL);
  // mgos_telegram_subscribe("/alarm_toggle", app_telegram_updates_handler, NULL);
  // mgos_telegram_subscribe("/light_toggle", app_telegram_updates_handler, NULL);
}
// Telegram response handler
void app_telegram_response_handler(void *ev_data, void *userdata) {
  // The message data will be passed with ev_data param
  // We have to explain what kind of data it is (it will be struct mgos_telegram_response)
  struct mgos_telegram_response *response = (struct mgos_telegram_response *) ev_data;
  struct panel_el *new_panel = (struct panel_el *) userdata;
  // Adding message_id to new panel
  new_panel->message_id = response->message_id;
  // Adding new panel to panels list for future updating
  SLIST_INSERT_HEAD(&state->panels, new_panel, next);
}
// Telegram updates handler
void app_telegram_updates_handler(void *ev_data, void *userdata) {
    (void) userdata;
    // The message data will be passed with ev_data param
    // We have to explain what kind of data it is (it will be struct mgos_telegram_update)
    struct mgos_telegram_update *update = (struct mgos_telegram_update *) ev_data;
	// Print to terminal data
    LOG(LL_INFO, ("APP ->> chat: %li, data: %s", (long)update->chat_id, update->data));
    // Handle the "/menu" command
    if ( strcmp(update->data, "/menu") == 0 ) {
	  // Check for existence of the control panel in this chat
      if (!SLIST_EMPTY(&state->panels)) {
        struct panel_el *panel;
        SLIST_FOREACH(panel, &state->panels, next) {
		  // If panel is already exist in this chat - return from function
          if (panel->chat_id == update->chat_id) return;
        };        
      }
      // Else make new panel
      struct panel_el *new_panel = (struct panel_el *) calloc(1, sizeof(*new_panel));
      new_panel->chat_id = update->chat_id;
	  // Getting current state
      char *text = get_panel_state();
	  // Getting current button state
      char *keyboard = get_panel_keyboard();
	  // Generating the message with actual control panel      
      char *json = json_asprintf("{chat_id: %d, text: %Q, reply_markup: %s}", update->chat_id, text, keyboard);
	  // Send message with control panel to the chat
	  // Here we have to handle the response for getting meaasage id for future updating
	  // Also push the new_panel pointer for response handler as userdata
      mgos_telegram_send_message_json_with_callback(json, app_telegram_response_handler, new_panel);
	  // Do not forget to free dynamically allocated memory
      free(text);
      free(keyboard);
      free(json);
      return;
    }
	// Handle the "/alarm_toggle" command
    if ( strcmp(update->data, "/alarm_toggle") == 0 ) {
      state->alarm = !state->alarm;
	  // Send acknowledgment for pushing the inline keyboard button
      mgos_telegram_answer_callback_query(update->query_id, "⚠ Command received", false);
	  // Relay state was changed, call update all panels function
      update_all_panels();
    }
	// Handle the "/light_toggle" command
    if ( strcmp(update->data, "/light_toggle") == 0 ) {
	  // Invert light relay state
      state->light = !state->light;
	  // Send acknowledgment for pushing the inline keyboard button
      mgos_telegram_answer_callback_query(update->query_id, "⚠ Command received", false);
	  // Relay state was changed, call update all panels function
      update_all_panels();
    }
}

// APP INIT FN
enum mgos_app_init_result mgos_app_init(void) {
  // Allocate memory for the state variable	
  state = (struct app_state *) calloc(1, sizeof(*state));
  // Add initial state for the fake relays
  state->alarm = false;
  state->light = false;
  // Add handler for TGB_EV_CONNECTED event
  mgos_event_add_handler(TGB_EV_CONNECTED, app_start_handler, NULL);
  return MGOS_APP_INIT_SUCCESS;
};
```