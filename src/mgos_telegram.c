/*
 * 2025 Aleksey A. Kotelnikov <kotelnikov.www@gmail.com>
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

#include "common/cs_dbg.h"
#include "common/str_util.h"
#include "mgos_sys_config.h"
#include "mgos_mongoose.h"
#include "mgos_net.h"
#include "mgos_timers.h"
#include "mgos_telegram.h"

#ifdef MGOS_HAVE_MJS
#include "mjs.h"
#endif

#define LIB_NAME "TELEGRAM"

struct mgos_telegram_subscription {
  char *data;
  mgos_telegram_cb_t callback;
  void *userdata;
  SLIST_ENTRY(mgos_telegram_subscription) next;
};

struct mgos_telegram_request {
  enum mgos_telegram_request_method method;
  char *custom_method;
  char *json;
  mgos_telegram_cb_t callback;
  void *userdata;
  struct mgos_telegram_response *response;
  STAILQ_ENTRY(mgos_telegram_request) next;
};

struct mgos_telegram {
  uint32_t update_id;
  bool auth_token_tested;
  const struct mgos_config_telegram *cfg;
  bool poll_connected;
  bool out_connected;
  struct mg_connection *nc_poll;
  struct mg_connection *nc_out;
  SLIST_HEAD(subscriptions, mgos_telegram_subscription) subscriptions;
  STAILQ_HEAD(update_queue, mgos_telegram_update) update_queue;
  STAILQ_HEAD(request_queue, mgos_telegram_request) request_queue;
  mgos_timer_id update_queue_timer;
  mgos_timer_id request_queue_timer;
};

struct mgos_telegram *tg = NULL;

#ifdef MGOS_HAVE_MJS
const struct mjs_c_struct_member *get_update_descr(void *ptr);
const struct mjs_c_struct_member *get_response_descr(void *ptr);
#endif

static void mgos_telegram_update_queue_handler(void *userdata);
static void mgos_telegram_request_queue_handler(void *userdata);

struct mgos_telegram_request *mgos_telegram_request_alloc(void);
static void mgos_telegram_request_free(struct mgos_telegram_request *request);

struct mgos_telegram_update *mgos_telegram_update_alloc(void);
static void mgos_telegram_update_free(struct mgos_telegram_update *update);

struct mgos_telegram_response *mgos_telegram_response_alloc(void);
static void mgos_telegram_response_free(struct mgos_telegram_response *response);

static bool mgos_telegram_is_request_queue_overflow();
static bool mgos_telegram_is_update_queue_overflow();
static bool mgos_telegram_request_queue_add(struct mgos_telegram_request *request);
static bool mgos_telegram_check_user_access(uint64_t user_id);
struct mgos_telegram_subscription *mgos_telegram_subscription_search(const char *data);

static void mgos_telegram_parse_update(void *source, void *dest, uint32_t *id);
static void mgos_telegram_parse_response(void *source, void *dest);

static void mgos_telegram_http_poll_once();
static void mgos_telegram_http_send_request(struct mgos_telegram_request *request);
static void mgos_telegram_http_update_handler(struct mg_connection *nc, int ev, void *ev_data, void *userdata);
static void mgos_telegram_http_request_handler(struct mg_connection *nc, int ev, void *ev_data, void *userdata);

static void mgos_telegram_close_all_connections(void);
static void mgos_telegram_check_token(void);
static void mgos_telegram_network_cb(int ev, void *ev_data, void *userdata);
static void mgos_telegram_connection_cb(void *ev_data, void *userdata);
bool mgos_telegram_check_config(const struct mgos_config_telegram *cfg);
static struct mgos_telegram *mgos_telegram_create(const struct mgos_config_telegram *cfg);

// MJS STRUCT WRAPPERS & GETTERS
#ifdef MGOS_HAVE_MJS
static const struct mjs_c_struct_member update_descr_message[] = {
  {"update_id", offsetof(struct mgos_telegram_update, update_id), MJS_STRUCT_FIELD_TYPE_INT, NULL},
  {"type", offsetof(struct mgos_telegram_update, type), MJS_STRUCT_FIELD_TYPE_INT, NULL},
  {"message_id", offsetof(struct mgos_telegram_update, message_id), MJS_STRUCT_FIELD_TYPE_INT, NULL},
  {"chat_id", offsetof(struct mgos_telegram_update, chat_id), MJS_STRUCT_FIELD_TYPE_INT, NULL},
  {"user_id", offsetof(struct mgos_telegram_update, user_id), MJS_STRUCT_FIELD_TYPE_INT, NULL},
  {"data", offsetof(struct mgos_telegram_update, data), MJS_STRUCT_FIELD_TYPE_CHAR_PTR, NULL},
  {NULL, 0, MJS_STRUCT_FIELD_TYPE_INVALID, NULL},
};

static const struct mjs_c_struct_member update_descr_callback_query[] = {
  {"update_id", offsetof(struct mgos_telegram_update, update_id), MJS_STRUCT_FIELD_TYPE_INT, NULL},
  {"type", offsetof(struct mgos_telegram_update, type), MJS_STRUCT_FIELD_TYPE_INT, NULL},
  {"callback_query_id", offsetof(struct mgos_telegram_update, query_id), MJS_STRUCT_FIELD_TYPE_CHAR_PTR, NULL},
  {"data", offsetof(struct mgos_telegram_update, data), MJS_STRUCT_FIELD_TYPE_CHAR_PTR, NULL},
  {"message_id", offsetof(struct mgos_telegram_update, message_id), MJS_STRUCT_FIELD_TYPE_INT, NULL},
  {"chat_id", offsetof(struct mgos_telegram_update, chat_id), MJS_STRUCT_FIELD_TYPE_INT, NULL},
  {"user_id", offsetof(struct mgos_telegram_update, user_id), MJS_STRUCT_FIELD_TYPE_INT, NULL},
  {NULL, 0, MJS_STRUCT_FIELD_TYPE_INVALID, NULL},
};

static const struct mjs_c_struct_member update_descr_other[] = {
  {"update_id", offsetof(struct mgos_telegram_update, update_id), MJS_STRUCT_FIELD_TYPE_INT, NULL},
  {NULL, 0, MJS_STRUCT_FIELD_TYPE_INVALID, NULL},
};

const struct mjs_c_struct_member *get_update_descr(void *ptr) {
  struct mgos_telegram_update *update = (struct mgos_telegram_update *) ptr;
  
  switch (update->type) {
    case MESSAGE:
      return update_descr_message;
      break;
    case CALLBACK_QUERY:
      return update_descr_callback_query;
      break;
    default:
      return update_descr_other;
      break;
  }
}


static const struct mjs_c_struct_member response_descr_error[] = {
  {"ok", offsetof(struct mgos_telegram_response, ok), MJS_STRUCT_FIELD_TYPE_BOOL, NULL},
  {"error_code", offsetof(struct mgos_telegram_response, error_code), MJS_STRUCT_FIELD_TYPE_INT, NULL},  
  {"description", offsetof(struct mgos_telegram_response, description), MJS_STRUCT_FIELD_TYPE_CHAR_PTR, NULL},
  {NULL, 0, MJS_STRUCT_FIELD_TYPE_INVALID, NULL},
};

static const struct mjs_c_struct_member response_descr_message[] = {
  {"ok", offsetof(struct mgos_telegram_response, ok), MJS_STRUCT_FIELD_TYPE_BOOL, NULL},
  {"message_id", offsetof(struct mgos_telegram_response, message_id), MJS_STRUCT_FIELD_TYPE_INT, NULL},
  {"chat_id", offsetof(struct mgos_telegram_response, chat_id), MJS_STRUCT_FIELD_TYPE_INT, NULL},
  {NULL, 0, MJS_STRUCT_FIELD_TYPE_INVALID, NULL},
};

static const struct mjs_c_struct_member response_descr_other[] = {
  {"ok", offsetof(struct mgos_telegram_response, ok), MJS_STRUCT_FIELD_TYPE_BOOL, NULL},
  {NULL, 0, MJS_STRUCT_FIELD_TYPE_INVALID, NULL},
};

const struct mjs_c_struct_member *get_response_descr(void *ptr) {
  struct mgos_telegram_response *response = (struct mgos_telegram_response *) ptr;
  if (!response->ok) return response_descr_error;
  switch (response->method) {
    case SEND_MESSAGE:
    case EDIT_MESSAGE_TEXT:
      return response_descr_message;
      break;
    default:
      return response_descr_other;
      break;
  }
}
#endif

// TELEGRAM QUEUE HANDLERS
static void mgos_telegram_update_queue_handler(void *userdata) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));

  if (STAILQ_EMPTY(&tg->update_queue)) return;

  struct mgos_telegram_update *update = STAILQ_FIRST(&tg->update_queue);
  struct mgos_telegram_subscription *subscription;

  switch (update->type) {
    case MESSAGE: {
      LOG(LL_INFO, ("%s ->> New message in chat: %lld, from user: %llu, text: %s", 
                    LIB_NAME, update->chat_id, update->user_id, update->data));
      //Check echo bot mode
      if (tg->cfg->echo_bot) {
        LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Echo bot mode enabled"));
        //Send message back to chat
		    mgos_telegram_send_message(update->chat_id, update->data);
        break;
      }
      //Check permissions for user_id in access list
      if (!mgos_telegram_check_user_access(update->user_id)) break;
      //Search for subscription
      subscription = mgos_telegram_subscription_search(update->data);
      //If subscribed invoke callback stored in subscription
      if (subscription) {
        LOG(LL_DEBUG, ("%s ->> %s %s", LIB_NAME, "Subscription found:", update->data));
        subscription->callback(update, subscription->userdata);
      }
      else LOG(LL_WARN, ("%s ->> %s %s", LIB_NAME, "Subscription not found:", update->data));
      break;
    }
    case CALLBACK_QUERY: {
      LOG(LL_INFO, ("%s ->> New callback query id: %s, in chat: %lld, from user: %llu, data: %s", 
                    LIB_NAME, update->query_id, update->chat_id, update->user_id, update->data));
      //Check permissions for user_id in access list
      if (!mgos_telegram_check_user_access(update->user_id)) break;
      //Search for subscription
      subscription = mgos_telegram_subscription_search(update->data);
	    //If subscribed call callback stored in subscription
      if (subscription) {
        LOG(LL_DEBUG, ("%s ->> %s  %s", LIB_NAME, "Subscription found:", update->data));
        subscription->callback(update, subscription->userdata);
      }
      else LOG(LL_DEBUG, ("%s ->> %s  %s", LIB_NAME, "Subscription not found:", update->data));
      break;
    } 
    default: {
      break;
    }
  }

  STAILQ_REMOVE(&tg->update_queue, update, mgos_telegram_update, next);
  mgos_telegram_update_free(update);

  (void) userdata;
}

static void mgos_telegram_request_queue_handler(void *userdata) {
  if (STAILQ_EMPTY(&tg->request_queue) || tg->out_connected) return;
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  struct mgos_telegram_request *request = STAILQ_FIRST(&tg->request_queue);
  mgos_telegram_http_send_request(request);
  (void) userdata;
}


// TELEGRAM QUEUE SERVICE FN
struct mgos_telegram_update *mgos_telegram_update_alloc(void) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  struct mgos_telegram_update *update = calloc(1, sizeof(*update));
  update->type = NO_TYPE;
  update->update_id = 0;
  update->data = NULL;
  update->query_id = NULL;
  return update;
}

static void mgos_telegram_update_free(struct mgos_telegram_update *update) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  if (update->data != NULL) free(update->data);
  if (update->query_id != NULL) free(update->query_id);
  free(update);
}

struct mgos_telegram_request *mgos_telegram_request_alloc(void) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  struct mgos_telegram_request *request = calloc(1, sizeof(*request));
  request->response = mgos_telegram_response_alloc();
  request->method = NO_METHOD;
  request->json = NULL;
  request->custom_method = NULL;
  return request;
}

static void mgos_telegram_request_free(struct mgos_telegram_request *request) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  mgos_telegram_response_free(request->response);
  if (request->json != NULL) free(request->json);
  if (request->custom_method != NULL) free(request->custom_method);
  free(request);
}

struct mgos_telegram_response *mgos_telegram_response_alloc(void) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  struct mgos_telegram_response *response = calloc(1, sizeof(*response));
  response->description = NULL;
  return response;
}

static void mgos_telegram_response_free(struct mgos_telegram_response *response) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  if (response->description != NULL) free(response->description);
  free(response);
}


static bool mgos_telegram_is_request_queue_overflow() {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));

  bool overflow = false;
  size_t qlen = 0;      
  struct mgos_telegram_request *request;

  STAILQ_FOREACH(request, &tg->request_queue, next) { qlen++; }
  if (qlen >= tg->cfg->request_queue_len) overflow = true;

  return overflow;
}

static bool mgos_telegram_is_update_queue_overflow() {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));

  bool overflow = false;
  size_t qlen = 0;      
  struct mgos_telegram_update *update;

  STAILQ_FOREACH(update, &tg->update_queue, next) { qlen++; }
  if (qlen >= tg->cfg->update_queue_len) overflow = true;
  
  return overflow;
}

static bool mgos_telegram_request_queue_add(struct mgos_telegram_request *request) {
  bool success = false;
  if (!mgos_telegram_is_request_queue_overflow()) {
    LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
    STAILQ_INSERT_TAIL(&tg->request_queue, request, next);
    success = true;
  }
  return success;
}


// TELEGRAM SERVICE FN
static bool mgos_telegram_check_user_access(uint64_t user_id) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  bool allowed = false;

  if (tg->cfg->acl == NULL) {
    LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Access list is empty, ignore update"));
    return allowed;
  }
  
  struct json_token t;
  int64_t acl_el;

  for (int i = 0; json_scanf_array_elem(tg->cfg->acl, strlen(tg->cfg->acl), "", i, &t) > 0; i++) {
    json_scanf(t.ptr, t.len, "%llu", &acl_el);
    if (acl_el != 0 && acl_el == user_id) {
      allowed = true;
      break;
    }
  }
  
  if (allowed) LOG(LL_DEBUG, ("%s ->> Access allowed for user_id: %llu, accept update", LIB_NAME, user_id));
  else LOG(LL_INFO, ("%s ->> Access denied for user_id: %llu, ignore update", LIB_NAME, user_id));

  return allowed;
}

struct mgos_telegram_subscription *mgos_telegram_subscription_search(const char *data) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  if (SLIST_EMPTY(&tg->subscriptions)) return NULL;

  struct mgos_telegram_subscription *subscription;
  SLIST_FOREACH(subscription, &tg->subscriptions, next) {
    if ( strcmp(subscription->data, "*") == 0 || strcasecmp(subscription->data, data) == 0 ) {
      return subscription;
    }
  }

  return NULL;
}


// TELEGRAM PARSERS
static void mgos_telegram_parse_update(void *source, void *dest, uint32_t *update_id) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  
  struct http_message *hm = (struct http_message *) source;
  struct mgos_telegram_update *update = (struct mgos_telegram_update *) dest;
  
  struct json_token t;
  json_scanf_array_elem(hm->body.p, hm->body.len, ".result", 0, &t);
  json_scanf(t.ptr, t.len, "{update_id: %u}", &update->update_id);

  if (update->update_id) {
    json_scanf(t.ptr, t.len, "{message: {message_id: %u}}", &update->message_id);
    json_scanf(t.ptr, t.len, "{callback_query: {id: %Q}}", &update->query_id);
    if (update->message_id > 0)         update->type = MESSAGE;
    else if (update->query_id != NULL)  update->type = CALLBACK_QUERY;

    switch (update->type) {
      case MESSAGE: {
        json_scanf(t.ptr, t.len, "{message: {chat: {id: %lld}}}", &update->chat_id);
        json_scanf(t.ptr, t.len, "{message: {from: {id: %llu}}}", &update->user_id);
        json_scanf(t.ptr, t.len, "{message: {text: %Q}}", &update->data);
        if (update->data == NULL) mg_asprintf(&update->data, 0, "Unsupported characters");
        break;      
      }
      case CALLBACK_QUERY: {
        json_scanf(t.ptr, t.len, "{callback_query: {from: {id: %llu}}}", &update->user_id);
        json_scanf(t.ptr, t.len, "{callback_query: {message: {chat: {id: %lld}}}}", &update->chat_id);
        json_scanf(t.ptr, t.len, "{callback_query: {message: {message_id: %u}}}", &update->message_id);
        json_scanf(t.ptr, t.len, "{callback_query: {data: %Q}}", &update->data);
        if (update->data == NULL) mg_asprintf(&update->data, 0, "Unsupported characters");
        break;      
      }
      default: {
        break;
      }
    }
  }

  *update_id = update->update_id;
}

static void mgos_telegram_parse_response(void *source, void *dest) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));

  struct http_message *hm = (struct http_message *) source;
  struct mgos_telegram_request *request = (struct mgos_telegram_request *) dest;

  request->response->method = request->method;
  json_scanf(hm->body.p, hm->body.len, "{ok: %B}", &request->response->ok);

  if (request->response->ok) {
    switch (request->method) {
      case SEND_MESSAGE:
      case EDIT_MESSAGE_TEXT:
      case CUSTOM_METHOD: {
        json_scanf(hm->body.p, hm->body.len, "{result: {message_id: %u} }", &request->response->message_id);
        json_scanf(hm->body.p, hm->body.len, "{result: {chat: {id: %lld}}}", &request->response->chat_id);
        break;
      }
      default: {
        break;
      }
    }
  }
  else {
    json_scanf(hm->body.p, hm->body.len, "{error_code: %d, description: %Q}", 
      &request->response->error_code, &request->response->description);    
  }
}


// TELEGRAM HTTP FN
static void mgos_telegram_http_poll_once() {
  if (tg->poll_connected) return;

  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
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

  tg->nc_poll = mg_connect_http(mgos_get_mgr(), mgos_telegram_http_update_handler, NULL, url, eh, pd);

  if (url !=NULL) free(url);
  if (eh !=NULL) free(eh);
  if (pd !=NULL) free(pd);
}

static void mgos_telegram_http_update_handler(struct mg_connection *nc, int ev, void *ev_data, void *userdata) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));

  switch (ev) {
    case MG_EV_CONNECT: {
      int connect_status = *(int *) ev_data;
      if (connect_status != 0) {
        LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Update HTTP connection error"));
        mgos_telegram_close_all_connections();
        mgos_telegram_check_token();
        break;
      }
      LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, "Poll HTTP connection opened"));
      break;
    }
    case MG_EV_HTTP_REPLY: {
      LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, "Poll HTTP connection got data"));
      
      // IF RX QUEUE OVERFLOW WILL TRY NEXT TIME      
      if ( mgos_telegram_is_update_queue_overflow() ) {
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;        
        break;
      }
      
      struct http_message *hm = (struct http_message *) ev_data;
      struct mgos_telegram_update *update = mgos_telegram_update_alloc();

      uint32_t update_id = 0;
      mgos_telegram_parse_update(hm, update, &update_id);
      if (update_id > 0) {
        tg->update_id = update_id;
        STAILQ_INSERT_TAIL(&tg->update_queue, update, next);  
      }
      else {
        LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, "Poll HTTP connection empty data"));
        mgos_telegram_update_free(update);
      }
      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      break;
    }
    case MG_EV_CLOSE: {
      LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, "Poll HTTP connection closed"));
      tg->poll_connected = false;
      tg->nc_poll = NULL;
      if ( !SLIST_EMPTY(&tg->subscriptions) ) {
        mgos_telegram_http_poll_once();
      }
      break;
    }
    default: {
      break;
    }
  }

  (void) ev_data;
  (void) userdata;
}

static void mgos_telegram_http_send_request(struct mgos_telegram_request *request) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  tg->out_connected = true;
  char *url = NULL;
  char *eh = NULL;
  char *pd = NULL;

  mg_asprintf(&eh, 0, "Content-Type: application/json\r\n");
  
  switch (request->method) {
    case GET_ME: {
      mg_asprintf(&url, 0, "%s/bot%s/%s", tg->cfg->server, tg->cfg->token, "getMe");
      break;
    }
    case SEND_MESSAGE: {
      mg_asprintf(&url, 0, "%s/bot%s/%s", tg->cfg->server, tg->cfg->token, "sendMessage");
      mg_asprintf(&pd, 0, request->json);
      break;
    }
    case EDIT_MESSAGE_TEXT: {
      mg_asprintf(&url, 0, "%s/bot%s/%s", tg->cfg->server, tg->cfg->token, "editMessageText");
      mg_asprintf(&pd, 0, request->json);
      break;
    }
    case ANSWER_CALLBACK_QUERY: {
      mg_asprintf(&url, 0, "%s/bot%s/%s", tg->cfg->server, tg->cfg->token, "answerCallbackQuery");
      mg_asprintf(&pd, 0, request->json);
      break;
    }
    case CUSTOM_METHOD: {
      mg_asprintf(&url, 0, "%s/bot%s/%s", tg->cfg->server, tg->cfg->token, request->custom_method);
      mg_asprintf(&pd, 0, request->json);
      break;
    }
    default: {
      return;
    }
  }

  tg->nc_out = mg_connect_http(mgos_get_mgr(), mgos_telegram_http_request_handler, request, url, eh, pd);

  if (url !=NULL) free(url);
  if (eh !=NULL) free(eh);
  if (pd !=NULL) free(pd);
}

static void mgos_telegram_http_request_handler(struct mg_connection *nc, int ev, void *ev_data, void *userdata) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));

  switch (ev) {
    case MG_EV_CONNECT: {
      int connect_status = *(int *) ev_data;
      if (connect_status != 0) {
        LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Request HTTP connection error"));
        mgos_telegram_close_all_connections();
        mgos_telegram_check_token();
        break;
      }
      LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, "Request HTTP connection opened"));
      break;
    }
    case MG_EV_HTTP_REPLY: {
      LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, "Request HTTP connection got data"));
      struct http_message *hm = (struct http_message *) ev_data;
      struct mgos_telegram_request *request = (struct mgos_telegram_request *) userdata;
      if (request->callback != NULL) {
        mgos_telegram_parse_response(hm, request);
        request->callback(request->response, request->userdata);
      }
      STAILQ_REMOVE(&tg->request_queue, request, mgos_telegram_request, next);
      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      mgos_telegram_request_free(request);
      break;
    }
    case MG_EV_CLOSE: {
      LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, "Request HTTP connection closed"));
      tg->out_connected = false;
      tg->nc_out = NULL;
      break;
    }
    default: {
      break;
    }
  }

  (void) ev_data;
  (void) userdata;
}


// TELEGRAM PUBLIC FN
void mgos_telegram_subscribe(const char *data, mgos_telegram_cb_t callback, void *userdata) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  if (!tg || !tg->auth_token_tested) {
    LOG(LL_WARN, ("%s ->> %s", LIB_NAME, "Telegram bot is not active, unable execute method"));
    return;
  }

  // Check if already subscribed
  struct mgos_telegram_subscription *subscription = mgos_telegram_subscription_search(data);
  if (subscription) {
    LOG(LL_INFO, ("%s ->> %s %s", LIB_NAME, "Subscription already exist:", subscription->data));
	  return;
  }
  // Subscribe the command
  struct mgos_telegram_subscription *new_subscription = calloc(1, sizeof(*new_subscription));
  mg_asprintf(&new_subscription->data, 0, data);
  new_subscription->callback = callback;
  new_subscription->userdata = userdata;
  SLIST_INSERT_HEAD(&tg->subscriptions, new_subscription, next);
  LOG(LL_INFO, ("%s ->> %s %s", LIB_NAME, "Subscription success:", new_subscription->data));
  // If update queue timer not set yet (e.g. it is first subscription), set this timer
  if (tg->update_queue_timer == MGOS_INVALID_TIMER_ID) {
    LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Subscription added, start polling"));
    tg->update_queue_timer = mgos_set_timer(500, MGOS_TIMER_REPEAT, mgos_telegram_update_queue_handler, NULL);
    mgos_telegram_http_poll_once();
  }
}


void mgos_telegram_send_message_with_callback(int64_t chat_id, const char *text, mgos_telegram_cb_t callback, void *userdata) {  
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  if (!tg || !tg->auth_token_tested) {
    LOG(LL_WARN, ("%s ->> %s", LIB_NAME, "Telegram bot is not active, unable execute method"));
    return;
  }

  struct mgos_telegram_request *request = mgos_telegram_request_alloc();
  request->method = SEND_MESSAGE;
  request->callback = callback;
  request->userdata = userdata;
  request->json = json_asprintf("{chat_id: %lld, text: %Q}", chat_id, text);
  
  LOG(LL_DEBUG, ("%s: %s %s", LIB_NAME, "Send message ->>", request->json));
  bool is_added = mgos_telegram_request_queue_add(request);
  if (!is_added) {
  LOG(LL_WARN, ("%s ->> %s", LIB_NAME, "Error while sending message"));
    mgos_telegram_request_free(request);
  }
}

void mgos_telegram_send_message(int64_t chat_id, const char *text) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  mgos_telegram_send_message_with_callback(chat_id, text, NULL, NULL);
}

void mgos_telegram_send_message_json_with_callback(const char *json, mgos_telegram_cb_t callback, void *userdata){
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  if (!tg || !tg->auth_token_tested) {
    LOG(LL_WARN, ("%s ->> %s", LIB_NAME, "Telegram bot is not active, unable execute method"));
    return;
  }

  struct mgos_telegram_request *request = mgos_telegram_request_alloc();
  request->method = SEND_MESSAGE;
  request->callback = callback;
  request->userdata = userdata;
  mg_asprintf(&request->json, 0, json);
  
  LOG(LL_DEBUG, ("%s: %s %s", LIB_NAME, "Send message ->>", request->json));
  bool is_added = mgos_telegram_request_queue_add(request);
  if (!is_added) {
    LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Error while sending message"));
    mgos_telegram_request_free(request);
  }
}

void mgos_telegram_send_message_json(const char *json){
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  mgos_telegram_send_message_json_with_callback(json, NULL, NULL);
}


void mgos_telegram_edit_message_text(int64_t chat_id, uint32_t message_id, const char *text) {  
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  if (!tg || !tg->auth_token_tested) {
    LOG(LL_WARN, ("%s ->> %s", LIB_NAME, "Telegram bot is not active, unable execute method"));
    return;
  }

  struct mgos_telegram_request *request = mgos_telegram_request_alloc();
  request->method = EDIT_MESSAGE_TEXT;
  request->json = json_asprintf("{chat_id: %lld, message_id: %u, text: %Q}", chat_id, message_id, text);
  
  LOG(LL_DEBUG, ("%s: %s %s", LIB_NAME, "Edit message text ->>", request->json));
  bool is_added = mgos_telegram_request_queue_add(request);
  if (!is_added) {
    LOG(LL_INFO, ("%s ->> %s", LIB_NAME, " Error while edit message text"));
    mgos_telegram_request_free(request);
  }
}

void mgos_telegram_edit_message_text_json(const char *json) {  
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  if (!tg || !tg->auth_token_tested) {
    LOG(LL_WARN, ("%s ->> %s", LIB_NAME, "Telegram bot is not active, unable execute method"));
    return;
  }

  struct mgos_telegram_request *request = mgos_telegram_request_alloc();
  request->method = EDIT_MESSAGE_TEXT;
  mg_asprintf(&request->json, 0, json);
  
  LOG(LL_DEBUG, ("%s: %s %s", LIB_NAME, "Edit message text ->>", request->json));
  bool is_added = mgos_telegram_request_queue_add(request);
  if (!is_added) {
    LOG(LL_INFO, ("%s ->> %s", LIB_NAME, " Error while edit message text"));
    mgos_telegram_request_free(request);
  }
}


void mgos_telegram_answer_callback_query(const char *id, const char *text, bool alert) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  if (!tg || !tg->auth_token_tested) {
    LOG(LL_WARN, ("%s ->> %s", LIB_NAME, "Telegram bot is not active, unable execute method"));
    return;
  }

  struct mgos_telegram_request *request = mgos_telegram_request_alloc();
  request->method = ANSWER_CALLBACK_QUERY;
  request->json = json_asprintf("{callback_query_id: %Q, text: %Q, show_alert: %B}", id, text, alert);
  
  LOG(LL_DEBUG, ("%s: %s %s", LIB_NAME, "Answer callback query ->>", request->json));
  bool is_added = mgos_telegram_request_queue_add(request);
  if (!is_added) {
    LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Error while sending answer callback query"));
    mgos_telegram_request_free(request);
  }
}

void mgos_telegram_answer_callback_query_json(const char *json) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  if (!tg || !tg->auth_token_tested) {
    LOG(LL_WARN, ("%s ->> %s", LIB_NAME, "Telegram bot is not active, unable execute method"));
    return;
  }

  struct mgos_telegram_request *request = mgos_telegram_request_alloc();
  request->method = ANSWER_CALLBACK_QUERY;
  mg_asprintf(&request->json, 0, json);

  LOG(LL_DEBUG, ("%s ->> %s %s", LIB_NAME, "Answer callback query ->>", request->json));
  bool is_added = mgos_telegram_request_queue_add(request);
  if (!is_added) {
    LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Error with sending answer callback query"));
    mgos_telegram_request_free(request);
  }
}


void mgos_telegram_execute_custom_method_with_callback(const char *method, const char *json, mgos_telegram_cb_t callback, void *userdata) {  
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  if (!tg || !tg->auth_token_tested) {
    LOG(LL_WARN, ("%s ->> %s", LIB_NAME, "Telegram bot is not active, unable execute method"));
    return;
  }

  struct mgos_telegram_request *request = mgos_telegram_request_alloc();
  request->method = CUSTOM_METHOD;
  mg_asprintf(&request->custom_method, 0, method);
  mg_asprintf(&request->json, 0, json);
  request->callback = callback;
  request->userdata = userdata;
  
  LOG(LL_INFO, ("%s ->> %s %s %s", LIB_NAME, "Execute custom method:", method, json));
  bool is_added = mgos_telegram_request_queue_add(request);
  if (!is_added) {
    LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Error while exec custom method"));
    mgos_telegram_request_free(request);
  }
}

void mgos_telegram_execute_custom_method(const char *method, const char *json){
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  if (!tg || !tg->auth_token_tested) {
    LOG(LL_WARN, ("%s ->> %s", LIB_NAME, "Telegram bot is not active, unable execute method"));
    return;
  }
  mgos_telegram_execute_custom_method_with_callback(method, json, NULL, NULL);
}


// LIB INIT FN
static void mgos_telegram_close_all_connections(void) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  // If token tested flag is false there is no needed to do anything
  if (!tg->auth_token_tested) return;
  // Otherwise reset token tested flag
  tg->auth_token_tested = false;
  // Stop all timers
  if (tg->update_queue_timer != MGOS_INVALID_TIMER_ID) {
    mgos_clear_timer(tg->update_queue_timer);
    tg->update_queue_timer = MGOS_INVALID_TIMER_ID;
  }
  if (tg->request_queue_timer != MGOS_INVALID_TIMER_ID) {
    mgos_clear_timer(tg->request_queue_timer);
    tg->request_queue_timer = MGOS_INVALID_TIMER_ID;
  }
  // Close all active connections
  if (tg->nc_poll != NULL) {
    tg->nc_poll->flags |= MG_F_CLOSE_IMMEDIATELY;
    tg->nc_poll = NULL;
    tg->poll_connected = false;
  }
  if (tg->nc_out != NULL) {
    tg->nc_out->flags |= MG_F_CLOSE_IMMEDIATELY;
    tg->nc_out = NULL;
    tg->out_connected = false;
  }
  // Trigger TGB_EV_DISCONNECTED event
  mgos_event_trigger(TGB_EV_DISCONNECTED, NULL);
}

static void mgos_telegram_check_token(void) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Testing telegram token"));

  struct mgos_telegram_request *request;
  
  // Check if GET_ME request is already in the head of the queue
  if (!STAILQ_EMPTY(&tg->request_queue)) {
    request = STAILQ_FIRST(&tg->request_queue);
    if (request->method == GET_ME) {
      mgos_set_timer(3000, 0, mgos_telegram_request_queue_handler, NULL);
      return;
    }
  }
  // Otherwise generate new GET_ME request
  request = mgos_telegram_request_alloc();
  request->method = GET_ME;
  request->callback = mgos_telegram_connection_cb;
  request->userdata = NULL;
  // And insert it in the head of the queue
  STAILQ_INSERT_HEAD(&tg->request_queue, request, next);
  mgos_set_timer(3000, 0, mgos_telegram_request_queue_handler, NULL);
}

static void mgos_telegram_network_cb(int ev, void *ev_data, void *userdata) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));

  switch (ev) {
    case MGOS_NET_EV_IP_ACQUIRED: {
      mgos_telegram_close_all_connections();
      mgos_telegram_check_token();
      break;
    }
    case MGOS_NET_EV_DISCONNECTED:
    case MGOS_NET_EV_CONNECTING:
    case MGOS_NET_EV_CONNECTED: 
    default: {
      mgos_telegram_close_all_connections();
      break;
    }
  }

  (void) ev_data;
  (void) userdata;
}

static void mgos_telegram_connection_cb(void *ev_data, void *userdata) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  struct mgos_telegram_response *response = (struct mgos_telegram_response *) ev_data;

  if (response->ok) {
    LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Testing auth token successful"));
    tg->request_queue_timer = mgos_set_timer(500, MGOS_TIMER_REPEAT, mgos_telegram_request_queue_handler, NULL);
    if ( !SLIST_EMPTY(&tg->subscriptions) || tg->cfg->echo_bot) {
      LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Starting update handler"));
      tg->update_queue_timer = mgos_set_timer(500, MGOS_TIMER_REPEAT, mgos_telegram_update_queue_handler, NULL);
      mgos_telegram_http_poll_once();
    }
    LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Telegram bot is active"));
    tg->auth_token_tested = true;
    mgos_event_trigger(TGB_EV_CONNECTED, NULL);
  }
  else {
    LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Testing auth token unsuccessful, check token or internet connection issues"));
    mgos_telegram_check_token();
  }
  (void) userdata;
}

bool mgos_telegram_check_config(const struct mgos_config_telegram *cfg) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  bool success = false;
  
  if (cfg->server != NULL && cfg->token != NULL) success = true;
  if (success) LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Configuration OK"));
  else LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Configuration Error, check configuration files (see mos.yml)"));

  return success;
}

static struct mgos_telegram *mgos_telegram_create(const struct mgos_config_telegram *cfg) {
  LOG(LL_DEBUG, ("%s ->> %s", LIB_NAME, __FUNCTION__));
  if (!mgos_telegram_check_config(cfg)) return NULL;
  struct mgos_telegram *tg = (struct mgos_telegram *) calloc(1, sizeof(*tg));
  tg->cfg = cfg;
  tg->auth_token_tested = false;
  STAILQ_INIT(&tg->update_queue);
  STAILQ_INIT(&tg->request_queue);
  tg->update_queue_timer = MGOS_INVALID_TIMER_ID;
  tg->request_queue_timer = MGOS_INVALID_TIMER_ID;
  mgos_event_register_base(MGOS_EVENT_TGB, "Telegram bot events");
  mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, mgos_telegram_network_cb, NULL);
  LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Waiting for internet connection"));
  return tg;
}


bool mgos_telegram_init(void) {
  if ( !mgos_sys_config_get_telegram_enable() ) {
    LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Library switched off in configuration files (see mos.yml or conf[1-9].json)"));
    return true;
  }
  LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Initializing library"));
  tg = mgos_telegram_create(mgos_sys_config_get_telegram());
  if (!tg) {
    LOG(LL_INFO, ("%s ->> %s", LIB_NAME, "Initializing telegram bot library unsuccessful"));
  }
  return true;
}