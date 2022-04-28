/*
 * 2021 Aleksey A. Kotelnikov <kotelnikov.www@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

let tgb_bn = Event.baseNumber("TGB");
let TGB = {
  _sm: ffi('void *mgos_telegram_send_message(int, char *, char *, void (*)(void *, userdata), userdata)'),
  _sb: ffi('void *mgos_telegram_subscribe(char *, void (*)(void *, userdata), userdata)'),
  _rd: ffi('void *get_response_descr(void)'),
  _ef: function(ed, ud) {
    return;
  },
  sub: function(cmd, cb, ud){
    return this._sb(cmd, cb || this._ef, ud);
  },
  pub: function(chat_id, text, obj, cb, ud){
    return this._sm(chat_id, text, JSON.stringify(obj), cb || this._ef, ud);
  },
  parse: function(p){
    let r = s2o(p, this._rd());
    r.result = JSON.parse(r.result_json);
    return r;
  },
  // EVENTS
  DISCONNECTED: tgb_bn,
  CONNECTED: tgb_bn + 1,
};

/* Example of using telegram bot mJS API
load("api_events.js");
load('api_telegram.js');

// Here describe your own handler for managing the receiving data
let tgb_msg_handler = function(ed, ud) {
  
    let resp = TGB.parse(ed);
    
    // Here we will manage the '/start' command
    if (resp.result.text === '/start') {
        print('Telegram: mJS "/start" handler');
        TGB.pub(resp.result.chat_id, 'Received command "/start"', null, null, null);
    }
    // Here we will manage the '/status' command
    else if (resp.result.text === '/status') {
        print('Telegram: mJS "/status" handler');
        TGB.pub(resp.result.chat_id, 'Received command "/status"', null, null, null);
    }
    // Here we will manage all other commands
    // In case we subscribed for all commands
    else {
        print('Telegram: mJS "*" handler');
        TGB.pub(resp.result.chat_id, resp.result.text, null, null, null);
    }
};

// Here describe your handler for managing the subscriptions
let tgb_start_handler = function(ev, ed, ud) {
    
    // Subscribe to all received commands
    TGB.sub('*', tgb_msg_handler, null);
    
    // Subscribe to certain commands
    //TGB.sub('/start', tgb_msg_handler, null);
    //TGB.sub('/status', tgb_msg_handler, null);
};

Event.addHandler(TGB.CONNECTED, tgb_start_handler, null);
*/