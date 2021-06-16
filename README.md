# Mongoose OS Telegram Bot API library

## Overview
The library representing simple Telegram Bot API for [Mongoose OS](https://mongoose-os.com/)  - IOT framework written by [Cesanta Software Ltd](https://cesanta.com/). 

The library will be useful for projects when you need to managing your IOT devices by sending and receiving commands through the [Telegram Bot API](https://core.telegram.org/bots). The library successfully tested on ESP8266 and ESP32 from [Espressif Systems](https://www.espressif.com/).

For using this library you have to provide firstly the connectivity to the internet for your IOT device by using the connection to the [Wi-Fi network](https://mongoose-os.com/docs/mongoose-os/api/net/wifi.md) or by using the connection to the [Cellular network](https://mongoose-os.com/docs/mongoose-os/api/net/pppos.md).

Please pay attention the library is in "alpha" version, so any issues or suggestions are welcomed!!! )))

## How to use the library
### Include the library and configure it for your project
Firstly enable the library by adding the following to your `mos.yml` file in section `libs`:

```
libs:
  - origin: https://github.com/kotelnikov/mgos_telegram
    name: telegram
```

Next configure the library by adding the following properties to your `mos.yml` in section `config_schema`:

```
config_schema:
  - ["telegram.enable", true]
  - ["telegram.token", "1111222333:YourTokenGoesHere"]
  - ["telegram.acl", "[111222333, -444555666]"]
  - ["telegram.echo_bot", false]
```
Configuration description for `config_schema` section:

Property | Description
------------ | -------------
`telegram.token` | This property stores your telegram token represented by the string. If you don't have your own token yet, you can find the information how to get it here https://core.telegram.org/bots#creating-a-new-bot
`telegram.acl` | This property stores the Access list (ACL) represented by string, containing an array of the chat/group IDs. Pay attention, the correct group id must be represented with mines prefix, see the second element in the example above. If ACL list will be empty, or message arrived from the chat/group id not in the ACL, all such messages will be ignored by the library. If you don't know how to get your chat/group id, you can "ask" the Bot `@myidbot` (just subscribe for the Bot and then sent him the command `/getid`)
`telegram.echo_bot` | This property switches on/off the echo mode. Pay attention this mode is enabled by default^so in work scenarios you have to turn it to "false".  In case you want to play with library you don't have to write absolutely any code in your `init.js` or `main.c` for just checking the library functionality as parrot. Just leave this option as "true" and don't write any other code. But relevant ACL must be present anyway, otherwise all received messages will be ignored

### mJS (restricted Java Script engine) usage
If your prefer to develop in mJS (restricted Java Script engine) add the following code to your `init.js` file:

```js
load("api_events.js");
load('api_telegram.js');

// Here describe your own handler for managing the receiving data
let tgb_msg_handler = function(ed, ud) {
  
    let resp = TGB.parse(ed);
  
    if (resp.result.text === '/start') {
        print('Telegram: mJS "/start" handler');
        TGB.pub(resp.result.chat_id, 'Received command "/start"', null, null, null);
    }
    else if (resp.result.text === '/status') {
        print('Telegram: mJS "/status" handler');
        TGB.pub(resp.result.chat_id, 'Received command "/status"', null, null, null);
    }
    else {
        // If you subscribed for any messages
        print('Telegram: mJS "*" handler');
        TGB.pub(resp.result.chat_id, resp.result.text, null, null, null);
    }
};

// Here describe your handler for managing the subscriptions
let tgb_start_handler = function(ev, ed, ud) {

    //TGB.sub('*', tgb_msg_handler, null);
    TGB.sub('/start', tgb_msg_handler, null);
    TGB.sub('/status', tgb_msg_handler, null);
};

Event.addHandler(TGB.CONNECTED, tgb_start_handler, null);

```
#### API description:


`Event.addHandler(TGB.CONNECTED, callback, user_data)` Before we can subscribe the commands and send the messages, we have to wait for the Telegram Bot library successfully connects to the Telegram server. So when `TGB.CONNECTED` event will fired it calls the callback. 

Argument | Description
------------ | -------------
`callback` | With this argument you have to pass the callback function for handling the subscribing commands.
`user_data` | With this argument you pass the user data with the provided above callback. You can use it for storing some pointer to the object or another data, for later use when callback function will fired, otherwise pass the `null`.


`TGB.sub(command_text, callback, user_data)` This method for subscribing the commands received though the Telegram Bot API (it's like some sort of mqtt subscription). 

Argument | Description
------------ | -------------
`command_text` | With this argument you have to pass the command you need to subscribe and than handle in callback. All the messages which not equal the `command_text` will be ignored. If you want to subscribe for absolutely all commands please use the asterisk sing `'*'` as "command_text" argument.
`callback` | With this argument you have pass the callback, where we will going to handle the received command. 
`user_data` | With this argument you have pass the user data with the provided above callback. You can use it for storing some pointer to the object or another data, for later use when callback function will fired, otherwise pass the `null`.


`TGB.pub(chat_id, message_text, json_data, callback, user_data)` This method we use for sending messages through the Telegram Bot API.

Argument | Description
------------ | -------------
`chat_id` | With this argument you have to pass the chat/group id where you going to send current message, or you can leave it empty `''`, if you prefer to use native Telegram Bot API (see `json_data` argument description).
`message_text` | With this argument you have to pass the message text you need to send, or you can leave it empty `''`, if you prefer to use native Telegram Bot API (see `json_data` argument description).
`json_data` | If you prefer for using the native Telegram Bot API, pass here an object with this argument, according to the API https://core.telegram.org/bots/api#making-requests or otherwise pass the `null` for this argument.
`callback` | If you need to receive back the response from the Telegram Bot API, according to the API https://core.telegram.org/bots/api#sendmessage or otherwise pass the `null`. Usually the Telegram Bot API returns the sent message back in case of success, or error and its description in case of an error. `TGB.parse()` method can parse such responses recieved from the Telegram Bot API.
`user_data` | With this argument you have pass the user data with the provided above callback. You can use it for storing some pointer to the object or another data, for later use when callback function will fired, otherwise pass the `null`.


`TGB.parse(event_data)` This is the method for parse receiving data. Before we can work with receiving data we have to parse it as an JS object. It returns the object, which contains such properties:

in case of success:
```
{
    ok: true, 
    result: {
        chat_id: 111222333, 
        text: 'Text of command'
    }
}
```

or in case of error:
```
{
    ok: false, 
    result: {
        error_code: 500, 
        description: Error description'
    }
}
```

Argument | Description
------------ | -------------
`event_data` | With this argument you have to pass the data for parsing.


### C usage
Description coming a bit later.

## Misc
For additional information look through the code from the library's files.

## TO-DO
We will going to implement some additional functionality a bit later such as `editMessageText`,`editMessageReplyMarkup`,`callbackQuery`,`answerCallbackQuery`.