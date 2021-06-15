#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void (*mg_telegram_cb_t)(void *ev_data MG_UD_ARG(void *ud));
void mgos_telegram_subscribe(const char *data, mg_telegram_cb_t cb, void *ud);
const struct mjs_c_struct_member *get_response_descr(void);

void mgos_telegram_send_message(int32_t chat_id, const char *text, const char *json, mg_telegram_cb_t cb, void *ud);
void mgos_telegram_edit_message_text(int32_t chat_id, uint32_t msg_id, const char *text, const char *json, mg_telegram_cb_t cb, void *ud);

void mgos_telegram_edit_message_markup(int32_t chat_id, uint32_t msg_id, const char *json, mg_telegram_cb_t cb, void *ud);
void mgos_telegram_answer_edit_message_mjs(const char *json, mg_telegram_cb_t cb, void *ud);

void mgos_telegram_answer_callback_query(int32_t chat_id, const char *callback_id, const char *text, const char *json, mg_telegram_cb_t cb, void *ud);
void mgos_telegram_answer_callback_query_mjs(const char *json, mg_telegram_cb_t cb, void *ud);

#ifdef __cplusplus
}
#endif /* __cplusplus */