#include <stdlib.h>
#include "mgos.h"
#include "mgos_telegram.h"
#include "mjs.h"

#define MGOS_EVENT_TGB MGOS_EVENT_BASE('T', 'G', 'B')

enum mgos_telegram_event {
  TGB_EV_DISCONNECTED = MGOS_EVENT_TGB,
  TGB_EV_CONNECTED
};

enum mgos_telegram_method {
  //NO METHODS  
  noMethod,
  //TX METHODS
  getMe,
  sendMessage,
  //RX METHODS
  newMessage,

  //TODO: Implement some methods later
  //editMessageText,
  //editMessageReplyMarkup,
  //callbackQuery,
  //answerCallbackQuery,

};

typedef struct mgos_telegram_error_resp {
  bool ok;
  uint16_t error_code;
  char *description;
} mgos_telegram_error_resp_t;

typedef struct mgos_telegram_getme_resp {
  bool ok;
  bool is_bot;  
  uint32_t id;
  char *username;
} mgos_telegram_getme_resp_t;

typedef struct mgos_telegram_send_message_resp {
  bool ok;
  uint32_t message_id;
} mgos_telegram_send_message_resp_t;

typedef struct mgos_telegram_new_message_resp {
  bool ok;
  int32_t chat_id;
  uint32_t user_id;
  uint16_t message_id;
  char *text;
} mgos_telegram_new_message_resp_t;

struct mgos_telegram_subscription {
  char *data;
  mg_telegram_cb_t cb;
  void *ud;
  SLIST_ENTRY(mgos_telegram_subscription) next;
};

struct mgos_telegram_response {
  bool ok;
  char *result;

  enum mgos_telegram_method method;
  void *resp;
};


static const struct mjs_c_struct_member response_descr[] = {
  {"ok", offsetof(struct mgos_telegram_response, ok), MJS_STRUCT_FIELD_TYPE_BOOL, NULL},
  {"result", offsetof(struct mgos_telegram_response, result), MJS_STRUCT_FIELD_TYPE_CHAR_PTR, NULL},
  {NULL, 0, MJS_STRUCT_FIELD_TYPE_INVALID, NULL},
};

struct mgos_telegram_queue_element {

  uint32_t msg_id;
  uint32_t user_id;
  int32_t chat_id;

  char *data;
  char *json;
  enum mgos_telegram_method method;
  
  struct mgos_telegram_response response;
  mg_telegram_cb_t cb;
  void *ud;

  STAILQ_ENTRY(mgos_telegram_queue_element) next;
};

struct mgos_telegram {

  uint32_t update_id;
  const struct mgos_config_telegram *cfg;

  struct mg_connection *nc_poll;
  bool poll_connected;

  struct mg_connection *nc_out;
  bool out_connected;

  SLIST_HEAD(subscriptions, mgos_telegram_subscription) subscriptions;
  STAILQ_HEAD(rx_queue, mgos_telegram_queue_element) rx_queue;
  STAILQ_HEAD(tx_queue, mgos_telegram_queue_element) tx_queue;
};

static struct mgos_telegram *tg = NULL;

static void mgos_telegram_network_cb(int ev, void *ed, void *arg);
static void mgos_telegram_start_cb(void *response, void *ud);
static void mgos_telegram_poll_cb(struct mg_connection *nc, int ev, void *ed, void *arg);
static void mgos_telegram_out_cb(struct mg_connection *nc, int ev, void *ed, void *arg);

static void mgos_telegram_rx_queue_handler(void *arg);
static void mgos_telegram_tx_queue_handler(void *arg);
static uint32_t mgos_telegram_parse_poll(void *source, void *dest);
static void mgos_telegram_parse_method(void *source, void *dest);

static uint32_t mgos_telegram_parse_newMessage(void *source, void *dest);
uint32_t mgos_telegram_parse_callbackQuery(void *source, void *dest);

static void mgos_telegram_parse_response(void *source, void *dest);
static void mgos_telegram_parse_response_Error(void *source, void *dest);
static void mgos_telegram_parse_response_getMe(void *source, void *dest);
static void mgos_telegram_parse_response_messageID(void *source, void *dest);

static bool mgos_telegram_acl_check(uint32_t chat_id);
static bool mgos_telegram_is_tx_queue_overflow();
static bool mgos_telegram_is_rx_queue_overflow();

static struct mgos_telegram_queue_element *mgos_telegram_queue_el_new(void);
mgos_telegram_getme_resp_t *mgos_telegram_getme_resp_new(void);
static void mgos_telegram_queue_el_free(struct mgos_telegram_queue_element *el);


static void mgos_telegram_tx_queue_add(uint8_t method, int32_t chat_id, const char *data, const char *json, mg_telegram_cb_t cb, void *ud);


static void mgos_telegram_poll_worker();
static void mgos_telegram_out_worker(struct mgos_telegram_queue_element *el);
static struct mgos_telegram *mgos_telegram_create(const struct mgos_config_telegram *cfg);


// TELEGRAM CALLBACKS
static void mgos_telegram_network_cb(int ev, void *ed, void *arg) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_network_cb() ->> MGOS_NET_EV_IP_ACQUIRED event fired"));
  
  if (tg == NULL) return;

  if (tg->nc_poll != NULL) {
    tg->nc_poll->flags |= MG_F_CLOSE_IMMEDIATELY;
    tg->nc_poll = NULL;
    tg->poll_connected = false;
  }
  if (tg->nc_out != NULL) {
    tg->nc_out->flags |= MG_F_CLOSE_IMMEDIATELY;
    tg->nc_out = NULL;
    tg->nc_out = false;
  }

  struct mgos_telegram_queue_element *el;
  el = mgos_telegram_queue_el_new();
  el->method = getMe;
  el->cb = mgos_telegram_start_cb;
  el->ud = NULL;
  STAILQ_INSERT_TAIL(&tg->tx_queue, el, next);  

  mgos_set_timer(500, 0, mgos_telegram_tx_queue_handler, NULL);

  (void) ev;
  (void) ed;
  (void) arg;
}

static void mgos_telegram_start_cb(void *response, void *ud) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_start_cb fn"));

  if (tg == NULL) return;

  struct mgos_telegram_response *resp = (struct mgos_telegram_response *) response;
  
  if (resp->ok) {
    LOG(LL_INFO, ("TELEGRAM ->> Initializing done, bot is active!"));
    mgos_event_trigger(TGB_EV_CONNECTED, resp);
    mgos_set_timer(100, MGOS_TIMER_REPEAT, mgos_telegram_rx_queue_handler, NULL);
    mgos_set_timer(100, MGOS_TIMER_REPEAT, mgos_telegram_tx_queue_handler, NULL);
    mgos_telegram_poll_worker();
  }
  else {
    LOG(LL_WARN, ("TELEGRAM ->> Initializing fail!"));
  }

  (void) response;
  (void) ud;
}

static void mgos_telegram_poll_cb(struct mg_connection *nc, int ev, void *ed, void *arg) {
  if (tg == NULL) return;

  switch (ev) {
    case MG_EV_CONNECT: {
      LOG(LL_DEBUG, ("TELEGRAM ->> Poll connected"));
      break;
    }
    case MG_EV_HTTP_REPLY: {
      LOG(LL_DEBUG, ("TELEGRAM ->> Poll HTTP reply")); 
      
      // IF RX QUEUE OVERFLOW WILL TRY NEXT TIME      
      if ( mgos_telegram_is_rx_queue_overflow() ) {
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;        
        break;
      }
      
      struct http_message *hm = (struct http_message *) ed;
      struct mgos_telegram_queue_element *el;
      el = mgos_telegram_queue_el_new();
      
      uint32_t update_id = mgos_telegram_parse_poll(hm, el);
      if (update_id > 0) {
        tg->update_id = update_id;
        STAILQ_INSERT_TAIL(&tg->rx_queue, el, next);  
      }
      else {
        LOG(LL_DEBUG, ("TELEGRAM ->> No data yet or unsupported response"));
        mgos_telegram_queue_el_free(el);
      }

      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      break;
    }
    case MG_EV_CLOSE: {
      LOG(LL_DEBUG, ("TELEGRAM ->> Poll closed"));
      tg->poll_connected = false;
      mgos_telegram_poll_worker();
      break;
    }
    default: {
      break;
    }
  }

  (void) ev;
  (void) ed;
  (void) arg;
}

static void mgos_telegram_out_cb(struct mg_connection *nc, int ev, void *ed, void *arg) {
  if (tg == NULL) return;

  switch (ev) {
    case MG_EV_CONNECT: {
      LOG(LL_DEBUG, ("TELEGRAM ->> Out connected"));
      break;
    }
    case MG_EV_HTTP_REPLY: {
      LOG(LL_DEBUG, ("TELEGRAM ->> Out HTTP reply")); 

      struct http_message *hm = (struct http_message *) ed;
      struct mgos_telegram_queue_element *el = (struct mgos_telegram_queue_element *) arg;

      mgos_telegram_parse_response(hm, el);

      if (el->cb != NULL) el->cb(&el->response, el->ud);
      STAILQ_REMOVE(&tg->tx_queue, el, mgos_telegram_queue_element, next);
      mgos_telegram_queue_el_free(el);

      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      break;
    }
    case MG_EV_CLOSE: {
      LOG(LL_DEBUG, ("TELEGRAM ->> Out closed"));
      tg->out_connected = false;
      break;
    }
    default: {
      break;
    }
  }

  (void) ev;
  (void) ed;
  (void) arg;
}


// TELEGRAM QUEUE HANDLERS
static void mgos_telegram_rx_queue_handler(void *arg) {
  
  if (tg == NULL) return;
  if (STAILQ_EMPTY(&tg->rx_queue)) return;

  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_rx_queue_handler fn"));
  
  struct mgos_telegram_queue_element *el;
  el = STAILQ_FIRST(&tg->rx_queue);

  LOG(LL_INFO, ("TELEGRAM ->> received message from User: %d, Chat: %d, Message: %s", el->user_id, el->chat_id, el->data));

  //CHECK ACCESS LIST
  if ( !mgos_telegram_acl_check(el->chat_id) ) {
    STAILQ_REMOVE(&tg->rx_queue, el, mgos_telegram_queue_element, next);
    mgos_telegram_queue_el_free(el);
    return;    
  }

  //CHECK IS ECHO MODE ENABLED
  if (tg->cfg->echo_bot && el->method == newMessage) {
    LOG(LL_INFO, ("TELEGRAM ->> Echo bot mode enabled"));

    // IF TX QUEUE OVERFLOW WILL TRY NEXT TIME
    if ( mgos_telegram_is_tx_queue_overflow() ) {
      mgos_telegram_queue_el_free(el);
      return;
    }
    // OTHERWISE ADD ELEMENT TO TX QUEUE
    STAILQ_INSERT_TAIL(&tg->tx_queue, el, next);
    el->method = sendMessage;
    STAILQ_REMOVE(&tg->rx_queue, el, mgos_telegram_queue_element, next);
    return;
  }

  //SEARCH FOR SUBSCRIPTION
  struct mgos_telegram_subscription *s;
  LOG(LL_DEBUG, ("TELEGRAM ->> Check the subscription..."));
  
  bool res = false;
  
  SLIST_FOREACH(s, &tg->subscriptions, next) {
    if ( strcmp(s->data, "*") == 0 || strcasecmp(s->data, el->data) == 0 ) {
      res = true;
      LOG(LL_INFO, ("TELEGRAM ->> Subscription found: %s", s->data));   
      if (s->cb != NULL) {
        LOG(LL_DEBUG, ("TELEGRAM ->> Call the callback function stored in subscription..."));           
        s->cb(&el->response, s->ud);
      }
      break;
    }
  }

  if (!res) {
    LOG(LL_INFO, ("TELEGRAM ->> Subscription: %s not found, ignore message", el->data));
  }
  
  STAILQ_REMOVE(&tg->rx_queue, el, mgos_telegram_queue_element, next);
  mgos_telegram_queue_el_free(el);
  (void) arg;
}

static void mgos_telegram_tx_queue_handler(void *arg) {
  if (tg == NULL) return;
  if (STAILQ_EMPTY(&tg->tx_queue)) return;
  if (tg->out_connected) return;

  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_tx_queue_handler"));
  
  struct mgos_telegram_queue_element *el;
  el = STAILQ_FIRST(&tg->tx_queue);
  mgos_telegram_out_worker(el);

  (void) arg;
}


// TELEGRAM SERVICE FN
static bool mgos_telegram_acl_check(uint32_t chat_id) {
  
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_acl_check() fn"));
  bool res = false;
  
  if (mgos_sys_config_get_telegram_acl() == NULL) {
    LOG(LL_INFO, ("TELEGRAM ->> mgos_telegram_acl_check() ->> ACL list is empty, ignore message"));
    return res;
  }
  
  char *acl = NULL;
  mg_asprintf(&acl, 0, mgos_sys_config_get_telegram_acl());
  
  struct json_token t;
  int i;
  int32_t acl_el;
  
  for (i = 0; json_scanf_array_elem(acl, strlen(acl), "", i, &t) > 0; i++) {
    json_scanf(t.ptr, t.len, "%d", &acl_el);
    if (acl_el != 0 && acl_el == chat_id) {
      res = true;
      break;
    }
  }
  
  if (res) {
    LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_acl_check() ->> Chat ID: %d found in ACL list, accept message", chat_id));
  }
  else {
    LOG(LL_INFO, ("TELEGRAM ->> mgos_telegram_acl_check() ->> Chat ID: %d not found in ACL list, ignore message", chat_id));
  }

  free(acl);
  return res;
}

static bool mgos_telegram_is_tx_queue_overflow() {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_is_tx_queue_overflow fn"));
  
  bool res = false;
  size_t qlen = 0;      
  struct mgos_telegram_queue_element *el;

  STAILQ_FOREACH(el, &tg->tx_queue, next) { qlen++; }
  if (qlen >= tg->cfg->tx_queue_len) res = true;
  LOG(LL_DEBUG, ("TELEGRAM ->> tx_queue %s", res ? "overflow" : "not overflow"));  

  return res;
}

static bool mgos_telegram_is_rx_queue_overflow() {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_is_rx_queue_overflow fn"));
  
  bool res = false;
  size_t qlen = 0;      
  struct mgos_telegram_queue_element *el;
  
  STAILQ_FOREACH(el, &tg->rx_queue, next) { qlen++; }
  if (qlen >= tg->cfg->rx_queue_len) res = true;
  LOG(LL_DEBUG, ("TELEGRAM ->> rx_queue %s", res ? "overflow" : "not overflow"));

  return res;
}

void mgos_telegram_subscribe(const char *data, mg_telegram_cb_t cb, void *ud) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_subscribe fn"));
  if (tg == NULL) return;

  struct mgos_telegram_subscription *s = calloc(1, sizeof(*s));
  mg_asprintf(&s->data, 0, data);
  s->cb = cb;
  s->ud = ud;
  LOG(LL_INFO, ("TELEGRAM ->> Subscribe to command: %s", s->data));
  SLIST_INSERT_HEAD(&tg->subscriptions, s, next);
}

static struct mgos_telegram_queue_element *mgos_telegram_queue_el_new(void) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_queue_el_new"));
  struct mgos_telegram_queue_element *el;
  el = calloc(1, sizeof(*el));
  el->method = noMethod;
  el->data = NULL;
  el->json = NULL;
  el->response.result = NULL;
  //el->response.error_description = NULL;
  return el;
}

mgos_telegram_getme_resp_t *mgos_telegram_getme_resp_new(void) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_resp_new"));
  mgos_telegram_getme_resp_t *r;
  r = calloc(1, sizeof(*r));
  r->username = NULL;
  return r;
}

static void mgos_telegram_queue_el_free(struct mgos_telegram_queue_element *el) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_queue_el_free fn"));
  free(el->response.result);
  free(el->data);
  free(el->json);
  free(el);
}

const struct mjs_c_struct_member *get_response_descr(void) {
  return response_descr;
}


// TELEGRAM PARSE POLL JSON FN
static uint32_t mgos_telegram_parse_poll(void *source, void *dest) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_parse_poll()"));

  uint32_t update_id = 0;

  struct http_message *hm = (struct http_message *) source;
  struct mgos_telegram_queue_element *el = (struct mgos_telegram_queue_element *) dest;
  
  json_scanf(hm->body.p, hm->body.len, "{ok: %B}", &el->response.ok);

  if (!el->response.ok) {
    mgos_telegram_parse_response_Error(hm, el);
    return update_id;    
  }

  mgos_telegram_parse_method(hm, el);
  
  switch (el->method) {
    case newMessage: {
      update_id = mgos_telegram_parse_newMessage(hm, el);
      break;      
    }
    default: {
      break;
    }
  }

  return update_id;
}

static void mgos_telegram_parse_method(void *source, void *dest) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_parse_method()"));
  
  uint8_t i;
  struct json_token t;

  uint32_t tmp_int;

  struct http_message *hm = (struct http_message *) source;
  struct mgos_telegram_queue_element *el = (struct mgos_telegram_queue_element *) dest;

  for (i = 0; json_scanf_array_elem(hm->body.p, hm->body.len, ".result", i, &t) > 0; i++) {
    if ( json_scanf(t.ptr, t.len, "{message: {message_id: %d}}", &tmp_int) > 0) el->method = newMessage;
  }

  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_parse_method() ->> Method: %d", el->method)); 
}

static uint32_t mgos_telegram_parse_newMessage(void *source, void *dest) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_parse_newMessage fn"));
  
  struct http_message *hm = (struct http_message *) source;
  struct mgos_telegram_queue_element *el = (struct mgos_telegram_queue_element *) dest;

  int i;
  struct json_token t;
  uint32_t update_id = 0;
  
  json_scanf(hm->body.p, hm->body.len, "{ok: %B}", &el->response.ok);

  for (i = 0; json_scanf_array_elem(hm->body.p, hm->body.len, ".result", i, &t) > 0; i++) {
    json_scanf(t.ptr, t.len, "{update_id: %d}", &update_id);
    json_scanf(t.ptr, t.len, "{message: {message_id: %d}}", &el->msg_id);
    json_scanf(t.ptr, t.len, "{message: {chat: {id: %d}}}", &el->chat_id);
    json_scanf(t.ptr, t.len, "{message: {from: {id: %d}}}", &el->user_id);
    json_scanf(t.ptr, t.len, "{message: {text: %Q}}", &el->data);
    
    if (el->data == NULL) {
      LOG(LL_WARN, ("TELEGRAM ->> JSON contain's unsupported characters, or data type."));
      mg_asprintf(&el->data, 0, "Unsupported characters, or data type");
      el->response.ok = false;
    }

    el->response.result = json_asprintf("{message_id: %d, user_id: %d, chat_id: %d, text: %Q}}",
      el->msg_id, el->user_id, el->chat_id, el->data);
  }
  
  return update_id;
}


// TELEGRAM PARSE RESPONSE JSON FN
static void mgos_telegram_parse_response(void *source, void *dest) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_parse_response fn"));
  
  struct http_message *hm = (struct http_message *) source;
  struct mgos_telegram_queue_element *el = (struct mgos_telegram_queue_element *) dest;

  json_scanf(hm->body.p, hm->body.len, "{ok: %B}", &el->response.ok);

  if (!el->response.ok) {
    mgos_telegram_parse_response_Error(hm, el);
    return;    
  }

  switch (el->method) {
    case getMe: {
      mgos_telegram_parse_response_getMe(hm, el);
      break;      
    }
    case sendMessage: {
      mgos_telegram_parse_response_messageID(hm, el);
      break;      
    }
    default: {
      break;
    }
  }

}

static void mgos_telegram_parse_response_Error(void *source, void *dest) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_parse_response_Error fn"));

  int32_t error_code;
  char *description = NULL;

  struct http_message *hm = (struct http_message *) source;
  struct mgos_telegram_queue_element *el = (struct mgos_telegram_queue_element *) dest;

  json_scanf(hm->body.p, hm->body.len, "{error_code: %d, description: %Q}", &error_code, &description);
  el->response.result = json_asprintf("{error_code: %d, description: %Q}", error_code, description);
  
  free(description);
}

static void mgos_telegram_parse_response_getMe(void *source, void *dest) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_parse_response_getMe fn"));

  int32_t id;
  bool is_bot;
  char *username = NULL;

  struct http_message *hm = (struct http_message *) source;
  struct mgos_telegram_queue_element *el = (struct mgos_telegram_queue_element *) dest;

  json_scanf(hm->body.p, hm->body.len, "{result: {id: %d, is_bot: %B, username: %Q}}", &id, &is_bot, &username);
  el->response.result = json_asprintf("{id: %d, is_bot: %B, username: %Q}", id, is_bot, username);
  el->response.method = el->method;

  mgos_telegram_getme_resp_t *r;
  r = mgos_telegram_getme_resp_new();
  json_scanf(hm->body.p, hm->body.len, "{result: {id: %d, is_bot: %B, username: %Q}}", &r->id, &r->is_bot, &r->username);
  el->response.resp = r;

  mgos_telegram_getme_resp_t *tmp = (mgos_telegram_getme_resp_t*) el->response.resp;
  LOG(LL_DEBUG, ("TELEGRAM ->> GETME ->> %s", tmp->username));
  LOG(LL_DEBUG, ("TELEGRAM ->> GETME ->> METHOD ->> %i", el->response.method));
  free(username);
}

static void mgos_telegram_parse_response_messageID(void *source, void *dest) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_parse_response_messageID fn"));

  int32_t message_id;

  struct http_message *hm = (struct http_message *) source;
  struct mgos_telegram_queue_element *el = (struct mgos_telegram_queue_element *) dest;

  json_scanf(hm->body.p, hm->body.len, "{result: {message_id: %d}}", &message_id);
  el->response.result = json_asprintf("{message_id: %d}", message_id);
}

// TELEGRAM SEND FN
static void mgos_telegram_tx_queue_add(uint8_t method, int32_t chat_id, const char *data, const char *json, mg_telegram_cb_t cb, void *ud) {
  if (tg == NULL) return;  
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_tx_queue_add fn"));

  struct mgos_telegram_queue_element *el;
  el = mgos_telegram_queue_el_new();

  el->method = method;
  el->chat_id = chat_id;

  if (json != NULL) mg_asprintf(&el->json, 0, json);
  if (data != NULL) mg_asprintf(&el->data, 0, data);
  if (cb != NULL) el->cb = cb;
  if (ud != NULL) el->ud = ud;

  STAILQ_INSERT_TAIL(&tg->tx_queue, el, next);
}

void mgos_telegram_send_message(int32_t chat_id, const char *text, const char *json, mg_telegram_cb_t cb, void *ud) {
  if (tg == NULL) return;
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_send_message fn"));
  mgos_telegram_tx_queue_add(sendMessage, chat_id, text, json, cb, ud);
}


// TELEGRAM HTTP FN
static void mgos_telegram_poll_worker() {
  if (tg == NULL) return;
  if (tg->poll_connected) return;

  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_poll_worker fn"));
  tg->poll_connected = true;
  
  char *url = NULL;
  char *eh = NULL;
  char *pd = NULL;

  mg_asprintf(&url, 0, "%s/bot%s/getUpdates", tg->cfg->server, tg->cfg->token);
  mg_asprintf(&eh, 0, "Content-Type: application/json\r\n");
  pd = json_asprintf("{limit: 1, timeout: %d, offset: %d, allowed_updates: [%Q, %Q]}",
    tg->cfg->timeout > 0 ? tg->cfg->timeout : 60, 
    tg->update_id > 0 ? tg->update_id + 1 : 0,
    "message", "callback_query");

  tg->nc_poll = mg_connect_http(mgos_get_mgr(), mgos_telegram_poll_cb, NULL, url, eh, pd);
  
  free(url);
  free(eh);
  free(pd);
}

static void mgos_telegram_out_worker(struct mgos_telegram_queue_element *el) {
  if (tg == NULL) return;
  if (tg->out_connected) return;

  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_out_worker fn"));
  tg->out_connected = true;

  char *url = NULL;
  char *eh = NULL;
  char *pd = NULL;

  mg_asprintf(&eh, 0, "Content-Type: application/json\r\n");
  
  switch (el->method) {
    case getMe: {
      mg_asprintf(&url, 0, "%s/bot%s/%s", tg->cfg->server, tg->cfg->token, "getMe");
      break;
    }
    case sendMessage: {
      mg_asprintf(&url, 0, "%s/bot%s/%s", tg->cfg->server, tg->cfg->token, "sendMessage");
      if (el->data != NULL) {
        pd = json_asprintf("{chat_id: %d, text: %Q}", el->chat_id, el->data);
        LOG(LL_INFO, ("TELEGRAM ->> send message to User: %d, Message: %s", el->chat_id, el->data));
      }
      else mg_asprintf(&pd, 0, el->json);
      break;
    }
    default: {
      free(url);
      free(eh);
      free(pd);
      return;
      break;
    }
  }

  tg->nc_out = mg_connect_http(mgos_get_mgr(), mgos_telegram_out_cb, el, url, eh, pd);

  free(url);
  free(eh);
  free(pd);
}


// TELEGRAM CREATE FN

bool mgos_telegram_check_config(const struct mgos_config_telegram *cfg) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_check_config() ->> Checking configuration..."));
  bool res = true;
  if (cfg->server == NULL || cfg->token == NULL) res = false;
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_check_config() ->> Configuration seems to be %s", res ? "OK": "ERROR"));  
  return res;
}

static struct mgos_telegram *mgos_telegram_create(const struct mgos_config_telegram *cfg) {
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_create() ->> Create tg object"));
  



  struct mgos_telegram *tg = (struct mgos_telegram *) calloc(1, sizeof(*tg));
  if (tg == NULL || cfg->server == NULL || cfg->token == NULL) {
    free(tg);
    return NULL;
  }

  tg->cfg = cfg;

  STAILQ_INIT(&tg->rx_queue);
  STAILQ_INIT(&tg->tx_queue);
  
  LOG(LL_DEBUG, ("TELEGRAM ->> mgos_telegram_create() ->> Add handler for MGOS_NET_EV_IP_ACQUIRED event"));
  mgos_event_add_handler(MGOS_NET_EV_IP_ACQUIRED, mgos_telegram_network_cb, tg);
  mgos_event_register_base(MGOS_EVENT_TGB, "Telegram bot events");
  return tg;
}

// TELEGRAM INIT FN
bool mgos_telegram_init(void) {
  LOG(LL_INFO, ("TELEGRAM ->> mgos_telegram_init() ->> Initializing telegram bot library"));  
  if (mgos_sys_config_get_telegram_enable()) {
    LOG(LL_INFO, ("TELEGRAM ->> mgos_telegram_init() ->> Starting telegram bot"));
    tg = mgos_telegram_create( mgos_sys_config_get_telegram() );
    LOG(LL_INFO, ("TELEGRAM ->> mgos_telegram_init() ->> Telegram bot %s", tg != NULL ? "successfully started" : "starting unsuccessful"));
  }
  else {
    LOG(LL_INFO, ("TELEGRAM ->> mgos_telegram_init() ->> Telegram bot switched off in configuration files (see mos.yml)"));
  }

  return true;
}