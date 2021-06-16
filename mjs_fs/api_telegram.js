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
    r.result = JSON.parse(r.result);
    return r;
  },
  // EVENTS
  DISCONNECTED: tgb_bn,
  CONNECTED: tgb_bn + 1,
};

/* Example of using telegram bot mJS API
load("api_events.js");
load('api_telegram.js');

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
};

let tgb_start_handler = function(ev, ed, ud) {
  // If you want to subscribe for all messages and use '*' as subscription text, see example line below
  TGB.sub('*', tgb_msg_handler, null);
  
  // If you want to subscribe for certain commands use them for subscription text, see example lines below
  TGB.sub('/start', tgb_msg_handler, null);
  TGB.sub('status', tgb_msg_handler, null);
};

Event.addHandler(TGB.CONNECTED, tgb_start_handler, null);
*/
