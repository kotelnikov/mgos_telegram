/*
 * 2022 Aleksey A. Kotelnikov <kotelnikov.www@gmail.com>
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
  _sb: ffi('void *mgos_telegram_subscribe(char *, void (*)(void *, userdata), userdata)'),

  _sm: ffi('void *mgos_telegram_send_message(int, char *)'),
  _smc: ffi('void *mgos_telegram_send_message_with_callback(int, char *, void (*)(void *, userdata), userdata)'),
  _smj: ffi('void *mgos_telegram_send_message_json(char *)'),
  _smjc: ffi('void *mgos_telegram_send_message_json_with_callback(char *, void (*)(void *, userdata), userdata)'),

  _um: ffi('void *mgos_telegram_edit_message_text(int, int, char *)'),
  _umj: ffi('void *mgos_telegram_edit_message_text_json(char *)'),

  _aq: ffi('void *mgos_telegram_answer_callback_query(char *, char *, bool)'),
  _aqj: ffi('void *mgos_telegram_answer_callback_query_json(char *)'),

  _cmj: ffi('void *mgos_telegram_execute_custom_method(char *, char *)'),
  _cmjc: ffi('void *mgos_telegram_execute_custom_method_with_callback(char *, char *, void (*)(void *, userdata), userdata)'),

  _ud: ffi('void *get_update_descr(void *)'),
  _rd: ffi('void *get_response_descr(void *)'),

  subscribe: function(data, cb, ud){
    return this._sb(data, cb, ud);
  },
  send: function(chat_id, text){
    return this._sm(chat_id, text);
  },
  send_cb: function(chat_id, text, cb, ud){
    return this._smc(chat_id, text, cb, ud);
  },
  send_js: function(js_obj){
    return this._smj(JSON.stringify(js_obj));
  },
  send_js_cb: function(js_obj, cb, ud){
    return this._smjc(JSON.stringify(js_obj), cb, ud);
  },
  update: function(chat_id, message_id, text){
    return this._um(chat_id, message_id, text);
  },
  update_js: function(js_obj){
    return this._umj(JSON.stringify(js_obj));
  },
  answer: function(id, text, alert){
    return this._aq(id, text, alert);
  },
  answer_js: function(js_obj){
    return this._aqj(JSON.stringify(js_obj));
  },
  custom: function(method, js_obj){
    return this._cmj(method, JSON.stringify(js_obj));
  },
  custom_cb: function(method, js_obj, cb, ud){
    return this._cmjc(method, JSON.stringify(js_obj), cb, ud);
  },
  parse_update: function(ptr){
    let u = s2o(ptr, this._ud(ptr));
    return u;
  },
  parse_response: function(ptr){
    let r = s2o(ptr, this._rd(ptr));
    return r;
  },
  // EVENTS
  DISCONNECTED: tgb_bn,
  CONNECTED: tgb_bn + 1,
  // UPDATES  
  MESSAGE: 1,
  CALLBACK_QUERY: 2
};