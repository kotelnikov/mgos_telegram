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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MGOS_EVENT_TGB MGOS_EVENT_BASE('T', 'G', 'B')

enum mgos_telegram_event {
  TGB_EV_DISCONNECTED = MGOS_EVENT_TGB,
  TGB_EV_CONNECTED
};

enum mgos_telegram_method {
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

struct mgos_telegram_response {
  bool ok;
  int msg_id;
  int user_id;
  int chat_id;
  char *data;
  char *result_json;
  enum mgos_telegram_method method;
};

typedef void (*mg_telegram_cb_t)(void *ev_data MG_UD_ARG(void *userdata));
const struct mjs_c_struct_member *get_response_descr(void);
void mgos_telegram_subscribe(const char *data, mg_telegram_cb_t callback, void *userdata);
void mgos_telegram_send_message(int32_t chat_id, const char *text, const char *json, mg_telegram_cb_t callback, void *userdata);

#ifdef __cplusplus
}
#endif /* __cplusplus */