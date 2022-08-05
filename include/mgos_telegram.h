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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MGOS_EVENT_TGB MGOS_EVENT_BASE('T', 'G', 'B')

enum mgos_telegram_event {
  TGB_EV_DISCONNECTED = MGOS_EVENT_TGB,
  TGB_EV_CONNECTED
};

enum mgos_telegram_request_method {
  NO_METHOD,
  GET_ME,
  SEND_MESSAGE,
  EDIT_MESSAGE_TEXT,
  ANSWER_CALLBACK_QUERY,
  CUSTOM_METHOD
};

enum mgos_telegram_update_type {
  NO_TYPE,
  MESSAGE,
  CALLBACK_QUERY
};

struct mgos_telegram_response {
  bool ok;
  int error_code;
  char *description;
  int message_id;
  int64_t chat_id;
  enum mgos_telegram_request_method method;
};

struct mgos_telegram_update {
  enum mgos_telegram_update_type type;
  uint32_t update_id;
  uint32_t message_id;
  uint64_t user_id;
  int64_t chat_id;
  char *data;
  char *query_id;
  STAILQ_ENTRY(mgos_telegram_update) next;
};

typedef void (*mgos_telegram_cb_t)(void *ev_data, void *userdata);
void mgos_telegram_subscribe(const char *data, mgos_telegram_cb_t callback, void *userdata);

void mgos_telegram_send_message(int64_t chat_id, const char *text);
void mgos_telegram_send_message_json(const char *json);

void mgos_telegram_send_message_with_callback(int64_t chat_id, const char *text, mgos_telegram_cb_t callback, void *userdata);
void mgos_telegram_send_message_json_with_callback(const char *json, mgos_telegram_cb_t callback, void *userdata);

void mgos_telegram_edit_message_text(int64_t chat_id, uint32_t message_id, const char *text);
void mgos_telegram_edit_message_text_json(const char *json);

void mgos_telegram_answer_callback_query(const char *id, const char *text, bool alert);
void mgos_telegram_answer_callback_query_json(const char *json);

void mgos_telegram_execute_custom_method(const char *method, const char *json);
void mgos_telegram_execute_custom_method_with_callback(const char *method, const char *json, mgos_telegram_cb_t callback, void *userdata);

#ifdef __cplusplus
}
#endif /* __cplusplus */