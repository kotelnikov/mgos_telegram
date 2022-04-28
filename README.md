[English](https://github.com/kotelnikov/mgos_telegram/blob/main/README.md) |
[Русский](https://github.com/kotelnikov/mgos_telegram/blob/main/README.ru-Ru.md)

# Telegram Bot API for Mongoose OS

## Overview
This library represent a simple [Telegram Bot API](https://core.telegram.org/bots) for [Mongoose OS](https://mongoose-os.com/) - IOT framework developed by [Cesanta Software Ltd](https://cesanta.com/). 

The library will be useful for applications where you have to control and/or manage IOT devices by sending/receiving text through the Telegram messenger. The library successfully tested on ESP8266/ESP32 from [Espressif Systems](https://www.espressif.com/).

For using the library you have to provide the internet connection firstly for your IOT device by using the [Wi-Fi](https://mongoose-os.com/docs/mongoose-os/api/net/wifi.md) or by using the cellular connection using [PPPOS](https://mongoose-os.com/docs/mongoose-os/api/net/pppos.md).

At this stage the library supports only text messages on English. In case of receiving messages on other languages or messages with unsupported data type such messages will be changed forcibly to simple text message with placeholder text "Unsupported characters, or data type". Later I will update the library with new additional functionality.

Any issues or suggestions are welcomed! )))

## How to use the library
### Include the library and configure it for your project

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
  - ["telegram.acl", "[111222333, -444555666]"]
  - ["telegram.echo_bot", false]
```
Configuration description for `config_schema` section:

Property | Description
------------ | -------------
`telegram.enable` | This property enables the library. By default the library is disabled, so you have to enable it by passing `true`.
`telegram.token` | This property stores your telegram token represented by the string. If you don't have your own token yet, you can find the information how to get it here - [Creating a new bot](https://core.telegram.org/bots#creating-a-new-bot).
`telegram.acl` | This property stores the Access list (ACL) represented by string and containing an array of the User and Group IDs. Pay attention, the correct Group id must be represented with mines prefix, see the second element in the example above. If ACL list will be empty, or message arrived from the chat/group id not in the ACL, all such messages will be ignored by the library. If you don't know how to get your chat/group id, you can "ask" the Bot `@myidbot` (just subscribe for the Bot and then sent him the command `/getid`). Also you can find the id by analyzing the console logs. Information about received messages and whom it comes from will be shown in serial terminal.
`telegram.echo_bot` | This property switches on/off the echo mode. Pay attention this mode is enabled by default, so in productive scenarios you have to turn it to "false".  In case you want to play with library you don't have to write absolutely any code in your `init.js` or `main.c` for just checking the library functionality as parrot. Just leave this option as "true" and don't write any other code. But relevant ACL must be present anyway, otherwise all received messages will be ignored

### mJS (restricted Java Script engine) usage
If your prefer to develop in mJS (restricted Java Script engine) add the following code to your `init.js` file:

```js
load("api_events.js");
load('api_telegram.js');

// Handler for managing the receiving text
let tgb_msg_handler = function(ed, ud) {
    
    // We have to parse message here firstly
    let resp = TGB.parse(ed);
    
    // Here we will manage the '/start' command
    if (resp.result.text === '/start') {
        print('Telegram: mJS "/start" handler');
        TGB.pub(resp.result.chat_id, 'Received command "/start"', null, null, null);
        // Some other staff here
    }
    // Here we will manage the '/status' command
    else if (resp.result.text === '/status') {
        print('Telegram: mJS "/status" handler');
        TGB.pub(resp.result.chat_id, 'Received command "/status"', null, null, null);
        // Some other staff here
    }
    // Here we will manage all other commands
    // In case we subscribed for all commands
    else {
        print('Telegram: mJS "*" handler');
        TGB.pub(resp.result.chat_id, resp.result.text, null, null, null);
        // Some other staff here
    }
};

// Here describe your handler for managing the subscriptions
let tgb_start_handler = function(ev, ed, ud) {
    
    // Here we subscribe to certain commands
    TGB.sub('/start', tgb_msg_handler, null);
    TGB.sub('/status', tgb_msg_handler, null);
    
    // You can also subscribe to all received commands by one subscription "*"
    TGB.sub('*', tgb_msg_handler, null);
};

Event.addHandler(TGB.CONNECTED, tgb_start_handler, null);

```

#### API description:

`Event.addHandler(TGB.CONNECTED, callback, userdata)` Before we can subscribe the commands and send the messages, we have to wait for the Telegram Bot library successfully connects to the Telegram server. So when `TGB.CONNECTED` event will fired it calls the callback. 

Argument | Description
------------ | -------------
`callback` | With this argument you have to pass the callback function for handling the subscribing commands.
`userdata` | With this argument you pass the user data with the provided callback. You can use it for storing some pointer to the object or another data, for later use when callback function will fired, otherwise pass the `null`.


`TGB.sub(text, callback, user_data)` This method for subscribing the commands received though the Telegram Bot API (it's like some sort of mqtt subscription). 

Argument | Description
------------ | -------------
`text` | With this argument you have to pass the command you need to subscribe and than handle in callback. All the messages which not equal the `text` will be ignored. If you want to subscribe for absolutely all commands please use the string with asterisk sing `'*'` with this argument.
`callback` | With this argument you have pass the callback, where we will going to handle the received command. 
`user_data` | With this argument you have pass the user data with the provided above callback. You can use it for storing some pointer to the object or another data, for later use when callback function will fired, otherwise pass the `null`.


`TGB.pub(chat_id, text, json_data, callback, user_data)` This method we use for sending messages through the Telegram Bot API.

Argument | Description
------------ | -------------
`chat_id` | With this argument you have to pass the User or Group id where you are going to send current message, or you can leave it empty `''`, if you prefer to use native Telegram Bot API (see `json_data` argument description).
`text` | With this argument you have to pass the message text you need to send, or you can leave it empty `''`, if you prefer to use native Telegram Bot API (see `json_data` argument description).
`json_data` | If you want to try the native API, you can pass here the stringified object with parameters, according to the Telegram Bot API for method [SendMessage](https://core.telegram.org/bots/api#sendmessage) or otherwise pass `null`. In this case you can leave `chat_id` and `text` as empty strings `''`.
`callback` | If you need to receive back the response from the Telegram Bot API for the current request of SendMessage method, according to the [SendMessage API](https://core.telegram.org/bots/api#sendmessage), pass here the callback function. Otherwise pass `null`. Usually the Telegram Bot API returns the sent message back in case of success, or error and its description in case of an error. `TGB.parse()` method can parse such responses received from the Telegram Bot API.
`user_data` | With this argument you have pass the user data with the provided above callback. You can use it for storing some pointer to the object or another data, for later use when callback function will fired, otherwise pass the `null`.


`TGB.parse(event_data)` This is the method for parse receiving data. Before we can work with receiving data we have to parse it to JS object. The function returns the JS object with following properties:

in case of success:
```js
{
    ok: true,
    result: {
        message_id:1872,
        user_id: 222444555,
        chat_id: 222444555,
        text: 'Test message'
    }
} 
```

or in case of error:
```js
{
    ok: false, 
    result: {
        error_code: 420, 
        description: 'Flood'
    }
}
```

Argument | Description
------------ | -------------
`event_data` | With this argument you have to pass the massage data for parsing.


### C usage
If your prefer to develop in С add the following test code to your `main.c` file:

```C
#include "mgos_telegram.h"

void app_initializing_handler(int ev, void *ev_data, void *userdata) {
  (void) ev;
  (void) ev_data;
  (void) userdata;
  
  // Subscribe to certain commands
  mgos_telegram_subscribe("/start", app_telegram_message_handler, NULL);
  mgos_telegram_subscribe("/status", app_telegram_message_handler, NULL);
  
  // Or subscribe to all received commands
  mgos_telegram_subscribe("*", app_telegram_message_handler, NULL);
}

void app_telegram_message_handler(void *ev_data, void *userdata) {
    LOG(LL_INFO, ("APP ->> App telegram message handler"));
    (void) userdata;
    
    // The message data will be passed with ev_data
    // So we have to explain what kind of data it is (it will be struct mgos_telegram_response)
    struct mgos_telegram_response *response = (struct mgos_telegram_response *) ev_data;
    
    // Here we can check for an errors
    if (!response->ok) {
        LOG(LL_INFO, ("APP ->> Error code: %s description: %s", ));        
        return;
    };
    
  
    // Catch commands here
    if ( strcmp(response->data, "/start") == 0 ) {
        LOG(LL_INFO, ("APP ->> chat: %d, data: %s", response->chat_id, response->data));
        // Do some staff hire
    }
    if ( strcmp(response->data, "/status") == 0 ) {
        LOG(LL_INFO, ("APP ->> chat: %d, data: %s", response->chat_id, response->data));
        // Do some staff hire
    }
    else return;
}


enum mgos_app_init_result mgos_app_init(void) {
    
    // Adding handler for event TGB_EV_CONNECTED (waiting for telegram will be successfully connected)
    mgos_event_add_handler(TGB_EV_CONNECTED, app_initializing_handler, NULL);
    return MGOS_APP_INIT_SUCCESS;
};

```

## Misc
For additional information look through the code from the library's files.

## TO-DO
We will going to implement some additional functionality a bit later, such as:

- [ ] ReplyKeyboardMarkup
- [ ] InlineKeyboardMarkup