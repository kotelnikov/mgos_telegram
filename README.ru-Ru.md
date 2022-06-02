[English](https://github.com/kotelnikov/mgos_telegram/blob/main/README.md) |
[Русский](https://github.com/kotelnikov/mgos_telegram/blob/main/README.ru-Ru.md)

# Библиотека Telegram Bot API для Mongoose OS 

## Краткое описание

Библиотека предназначена для простого взаимодействия с [Telegram Bot API](https://core.telegram.org/bots) с использованием фреймворка [Mongoose OS](https://mongoose-os.com/), разработанного [Cesanta Software Ltd](https://cesanta.com/). Библиотека подойдет для приложений, где требуется управлять устройствами интернета вещей, путем обмена сообщениями через мессенджер Telegram. Библиотека будет работать на микроконтроллерах ESP8266 и ESP32 от [Espressif Systems](https://www.espressif.com/).

Для функционирования библиотеки в вашем проекте должно быть настроено подключение к интернет через [Wi-Fi](https://mongoose-os.com/docs/mongoose-os/api/net/wifi.md) или через GSM модем [PPPOS](https://mongoose-os.com/docs/mongoose-os/api/net/pppos.md).

Библиотека поддерживает указанные обновления: [Message](https://core.telegram.org/bots/api#message), [CallbackQuery](https://core.telegram.org/bots/api#callbackquery)

Библиотека поддерживает указанные запросы: [sendMessage](https://core.telegram.org/bots/api#sendmessage), [editMessageText](https://core.telegram.org/bots/api#editmessagetext), [answerCallbackQuery](https://core.telegram.org/bots/api#answercallbackquery).

Любые замечания и рекомендации приветствуются! )))

Демонстрация работы:

<img src="https://github.com/kotelnikov/mgos_telegram/blob/master/control_panel.gif" height="500"/>

## Конфигурация

В первую очередь необходимо добавить библиотеку в проект, для этого требуется добавить в секцию `libs` в файле `mos.yml` следующий код:

```yaml
libs:
  - origin: https://github.com/kotelnikov/mgos_telegram
    name: telegram
```

Далее необходимо сконфигурировать библиотеку, путем добавления в секцию `config_schema` в файле `mos.yml` следующих параметров:

```yaml
config_schema:
  - ["telegram.enable", true]
  - ["telegram.token", "1111222333:YourTokenGoesHere"]
  - ["telegram.echo_bot", false]
  - ["telegram.acl", "[555777888, 222333444]"]
```
Описание конфигурации `config_schema`:

Параметр | Тип | Описание
------------ | ------------- | -------------
`telegram.enable` | `boolean` | Данный параметр включает и выключает использование библиотеки, по умолчанию библиотека выключена, поэтому требуется установить значение `true`. 
`telegram.token` | `string` | Данный параметр хранит токен для подключения к серверу Telegram и представляет собой строку. Если у Вас нет своего токена, вы можете получить его воспользовавшись инструкцией по [ссылке](https://core.telegram.org/bots#creating-a-new-bot).
`telegram.echo_bot` | `boolean` | Данный параметр включает/выключает режим эхо бота. Обратите внимание, что по умолчанию данный параметр установлен в значение "true", т.е. в рабочей конфигурации вы должны выключить данный режим, установив значение "false". Иначе принятые сообщения не попадут в функции обратного вызова, поскольку в этом режиме входящая очередь сразу копируется в исходящую.  Данный режим можно использовать для начальной проверки работоспособности библиотеки или корректности вашего токена, достаточно оставить данный режим включенным и не писать вообще никакого кода в `init.js` или `main.c`, в таком случае библиотека будет работать как попугай, присылая вам в ответ все, что вы отправляете сами.
`telegram.acl` | `string` | Данный параметр хранит список пользователей от которых разрешено принимать сообщения. Параметр представлен в виде строки в JSON нотации содержащей массив ID пользователей. Если список пустой или пришедшее сообщение от пользователя, который не внесен в списке, то такие сообщения будут игнорироваться. Узнать свой ID, можно подписавшись на бота `@myidbot` и спросив ID командой `/getid`. Также id пользователя можно посмотреть в консоли вывода библиотеки, поскольку информация о принятых данных и отправителях выводится в терминал.

### Описание JS API

Перед тем как будет возможно получать и отправлять данные, необходимо подписаться на событие `TGB.CONNECTED`, которое срабатывает при успешном подключении библиотеки к серверу Telegram Bot API. В обработчике события, как правило, осуществляется подписка на текстовые команды и/или события нажатия на кнопки инлайн клавиатуры. Работа с событиями является функциональностью ядра Mongoose OS, при необходимости Вы можете получить дополнительную информацию по ссылке: [Event](https://mongoose-os.com/docs/mongoose-os/api/core/mgos_event.h.md).

```js
Event.addHandler(ev, callback, userdata);

// Сигнатура обработчика событий:
// function(ev, evdata, userdata) { /* Выполняем какие-либо действия здесь */ }

// Обработчик события TGB.CONNECTED, в котором осуществляется подписка на команды
let app_start_handler = function(ev, ed, ud) {
	// Подписка на команду '/start'
    TGB.subscribe('/start', updates_handler, null);
};

// Добавляем обработчик события TGB.CONNECTED
Event.addHandler(TGB.CONNECTED, app_start_handler, null);
```

## TGB.subscribe()

Используйте данный метод для выполнения подписки на обновления от Telegram Bot API (обновлениями в терминологии Telegram Bot API называются входящие данные отправленные нашему боту через мессенджер, это могут быть текстовые сообщения или события нажатия на кнопки инлайн клавиатуры). Подписка на текстовые сообщения и нажатия на кнопки выполняются одним методом. Возможно подписаться как на всё данные сразу, путем подписки на команду '*', так и на конкретные команды, которые необходимы. Все обновления, на которые не оформлена подписка будут игнорироваться библиотекой. При выполнении подписки передается функция обработчик, которая будет вызвана при срабатывании подписки.

```js
TGB.subscribe(data, callback, userdata);

// Сигнатура функции обработчика данных telegram
// function(ev_data, userdata) { /* Выполняем какие-либо действия здесь */ }

// Пример подписки на все команды сразу
TGB.subscribe('*', app_updates_handler, null);
// Пример подписки на конкретную команду
TGB.subscribe('/start', app_updates_handler, null);
```

## TGB.send(), TGB.send(), TGB.send_cb(), TGB.send_js(), TGB.send_js_cb()

Используйте данные методы для отправки сообщений в чаты и группы. Возможно отправлять как простые текстовые сообщения, так и более сложные, содержащие инлайн клавиатуру. Более подробная информация о методе Telegram Bot API: [sendMessage](https://core.telegram.org/bots/api#sendmessage).

```js
TGB.send(chat_id, text);
TGB.send_cb(chat_id, text, cb, ud);
TGB.send_js(js_obj);
TGB.send_js_cb(js_obj, cb, ud);

// Обработчик ответов на запросы к Telegram
let response_handler = function(ed, ud) {
    // Парсинг ответа в JS объект
    let resp = TGB.parse_response(ed);
    // Вывод данных в терминал
    print(JSON.stringify(resp));
};

// Объект JS, с ключами, соответствующими родному API, для метода sendMessage
let js_odj = {
  chat_id: 111222333,
  text: 'Hello from Mongoose OS'
};

// Пример отправки простого текстового сообщения
TGB.send(111222333, 'Hello from Mongoose OS');
// Тоже самое, но с обработкой ответа
TGB.send_cb(111222333, 'Hello from Mongoose OS', response_handler, null);
// Пример  отправки сообщения, с использованием родного API
TGB.send_js(js_odj);
// Тоже самое, но с обработкой ответа
TGB.send_js_cb(js_odj, response_handler, null);
```

## TGB.update(), TGB.update_js()

Используйте данные методы для обновления сообщений в чатах или группах. Возможно обновить как простые текстовые сообщения, так и более сложные, содержащие инлайн клавиатуру. Более подробная информация о методе Telegram Bot API: [editMessageText](https://core.telegram.org/bots/api#editmessagetext).

```js
TGB.update(chat_id, message_id, text);
TGB.update_js(js_obj);

// Объект JS, с ключами, соответствующими родному API, для метода editMessageText
let js_odj = {
  chat_id: 111222333,
  message_id: 1122,
  text: 'Updated message by Mongoose OS'
};

// Пример  обновления текстового сообщения
TGB.update(111222333, 1122, 'Updated message by Mongoose OS');
// Пример  обновления сообщения, с использованием родного API (передаем объект)
TGB.update_js(js_odj);
```

## TGB.answer(), TGB.answer_js()

Используйте данные методы для отправки ответа о получении события нажатия кнопки инлайн клавиатуры. Ответ может быть показан в виде всплывающего сообщения вверху чата или показан в отдельном модальном окне (alert). Более подробная информация о методе Telegram Bot API: [answerCallbackQuery](https://core.telegram.org/bots/api#answercallbackquery).

```js
TGB.answer(id, text, alert);
TGB.answer_js(js_obj);

// Объект JS, с ключами, соответствующими родному API, для метода answerCallbackQuery
let js_odj = {
  callback_query_id: '933786469216003032',
  text: '⚠ Command received',
  show_alert: false
};

// Пример отправки ответа на нажатие кнопки инлайн клавиатуры
TGB.answer('933786469216003032', '⚠ Command received', false);
// Пример отправки ответа на нажатие кнопки инлайн клавиатуры, с использованием родного API
TGB.answer_js(js_odj);
```

## TGB.custom(), TGB.custom_cb()

Используйте данные методы для выполнения произвольных запросов, не имеющих специальных функций в данной библиотеке. Подробная информация о доступных методах Telegram Bot API: [Telegram Bot API](https://core.telegram.org/bots/api#available-methods).

```js
TGB.custom(method, js_obj);
TGB.custom_cb(method, js_obj, cb, ud);

// Обработчик ответа на запрос
let response_handler = function(ed, ud) {
    // Парсинг ответа
    let resp = TGB.parse_response(ed);
    // Вывод в терминал данных
    print(JSON.stringify(resp));
};

// Объект JS, с ключами, соответствующими родному API, как пример для метода sendMessage
let js_odj = {
  chat_id: 111222333,
  text: 'Hello from Mongoose OS'
};

// Пример отправки произвольного метода
TGB.custom('sendMessage', js_odj);
// Тоже самое, но с обработкой ответа
TGB.custom_cb('sendMessage', js_odj, response_handler, null);
```

## TGB.parse_update(), TGB.parse_response()

Перед тем как мы сможем обрабатывать данные от Telegram требуется их преобразовать в JS объект. Используйте данные методы для парсинга обновлений и ответов от Telegram Bot API.

```js
// Пример обработчика входящих обновлений от Telegram Bot API
let update_handler = function(ed, ud) {
    // Парсинг данных обновления
    let upd = TGB.parse_update(ed);
    // Вывод в терминал
    print(JSON.stringify(upd));
};

// Пример обработчика ответов на запросы, выполненные к Telegram Bot API
let response_handler = function(ed, ud) {
    // Парсинг данных ответа
    let resp = TGB.parse_response(ed);
    // Вывод в терминал
    print(JSON.stringify(resp));
};

// Пример обновления при поступлении входящего Сообщения
{
  update_id: 532671789,
  type:1, // 1 -> СООБЩЕНИЕ
  message_id: 1464,
  data: "/menu",
  user_id: 111222333, 
  chat_id: -444555666 // Если отрицательное значение, то это группа
}

// Пример входящего обновления при нажатия на кнопку инлайн клавиатуры
{
  update_id: 532671790,
  type:2, // 2 -> CALLBACK QUERY (НАЖАТИЕ НА КНОПКУ)
  callback_query_id: "933786467781980781",
  data: "/alarm_toggle",
  message_id: 1464,
  user_id: 111222333,
  chat_id: 111222333
}

// Пример ответа на запрос, в случае ошибки
{
  ok: false, 
  error_code: 420, 
  description: 'Flood'
}

// Пример ответа на запрос sendMessage
{
  ok: true, 
  message_id: 1464, 
  chat_id: 111222333 // Если отрицательное значение, то это группа
}

// Пример ответа на все остальные запросы
{
  ok: true
}
```

## Примеры приложений на JS

#### Пример 1. Получение и отправка текстовых сообщений.

```js
load("api_events.js");
load('api_telegram.js');

// Обработчик входящих обновлений от Telegram
let updates_handler = function(ed, ud) {
    // Парсинг данных обновления
    let upd = TGB.parse_update(ed);
    // Вывод в терминал полученных данных
    print(JSON.stringify(upd));
    // Обработка подписки на команду '/start'
    if (upd.data === '/start') {
        // Отправка сообщения отправителю
        TGB.send(upd.chat_id, 'Received command "/start"');
    }
    // // Обработка подписки на команду '/status'
    else if (upd.data === '/status') {
		// Отправка сообщения отправителю
        TGB.send(upd.chat_id, 'Received command "/status"');
    }
    // Обработка других подписок
    // В случае если подписаны на все
    else {
        // Отправка сообщения отправителю (отправляется тот же текст, что и получен)
        TGB.send(upd.chat_id, upd.data);
    }
};

// Обработчик для выполнения подписки на команды
let app_start_handler = function(ev, ed, ud) {
    // Подписка на все команды одновременно "*"
    TGB.subscribe('*', updates_handler, null);
    // При необходимости можно подписаться на конкретные команды
    //TGB.subscribe('/start', updates_handler, null);
    //TGB.subscribe('/status', updates_handler, null);
};

// Подписка на событие TGB.CONNECTED
Event.addHandler(TGB.CONNECTED, app_start_handler, null);
```

#### Пример 2. Многопользовательская панель управления с автоматическим обновлением состояния.

```js
load('api_sys.js');
load("api_events.js");
load('api_telegram.js');

// Некие переменные, хранящие состояния реле
let alarm = false;
let light = false;

// Объект формирующий панель управления
let control_panel = {
    // Скрытый метод, формирующий текстовое состояние
    _get_state: function() {
        let l1 = 'FREE MEM: ' + JSON.stringify(Sys.free_ram()) + '\r\n';
        let l2 = 'UPTIME: ' + JSON.stringify(Sys.uptime()) + '\r\n';
        let l3 = alarm ? 'ALARM: ✅ ON\r\n' : 'ALARM: ❌ OFF\r\n';
        let l4 = light ? 'LIGHT: ✅ ON' : 'LIGHT: ❌ OFF';
        return l1 + l2 + l3 + l4;
    },
    // Скрытый метод, формирующий кнопки инлайн клавиатуры
    _get_keyboard: function() {
        return [
            [{text: alarm ? 'SWITCH OFF ALARM' : 'SWITCH ON ALARM', callback_data: '/alarm_toggle'}],
            [{text: light ? 'SWITCH OFF LIGHT' : 'SWITCH ON LIGHT', callback_data: '/light_toggle'}]
        ];
    },
    // Метод возвращающий объект с панелью в актуальном состоянии
    get_panel: function() {
        return {
            text: this._get_state(),
            reply_markup: { 
                inline_keyboard: this._get_keyboard()
            }
        };
    },
    // Массив для хранения панелей во всех чатах
    all_panels: []
};

// Обработчик входящих обновлений от Telegram
let updates_handler = function(ed, ud) {
    // Парсинг данных обновления
    let upd = TGB.parse_update(ed);
    // Печать в терминал полученных данных
    print(JSON.stringify(upd));
    // Обработка подписки на команду '/menu'
    if (upd.data === '/menu') {
	    // Итерируем массив с существующими панелями управления
        for (let i = 0; i < control_panel.all_panels.length; i++) {
            // Если панель уже есть в чате, то завершаем обработчик  
            if (control_panel.all_panels[i].chat_id === upd.chat_id) { return; }
        }
        // Если панели еще нет, то формируем новую панель
        // Формируем объект с панелью управления в актуальном состоянии
        let p = control_panel.get_panel();
        // Добавляем в объект ключ "chat_id" из полученного обновления
        p.chat_id = upd.chat_id;
        // Отправляем панель в чат
        TGB.send_js_cb(p, response_handler, null);
		// Далее завершаем выполнение обработчика
        return;
    }
    // Обработка нажатия на кнопку '/alarm_toggle'
    else if (upd.data === '/alarm_toggle') {
		// Инвертируем состояние реле alarm
		alarm = !alarm; 
	}
    // Обработка нажатия на кнопку '/light_toggle'
    else if (upd.data === '/light_toggle') {
		// Инвертируем состояние реле light
		light = !light; 
	}
	// Если пришла другая команда то завершаем выполнение обработчика
    else {
		return; 
	}
    // Отправляем подтверждение о получении команды
    TGB.answer(upd.callback_query_id, '⚠ Command received', false);
    // Обновляем все существующие панели во всех чатах
	// Формируем актуальную панель
    let p = control_panel.get_panel();
	// Итерируем массив с существующими панелями управления
    for (let i = 0; i < control_panel.all_panels.length; i++) {
		// Добавляем ключи
        p.chat_id = control_panel.all_panels[i].chat_id;
        p.message_id = control_panel.all_panels[i].message_id;
        // Обновляем экземпляр панели в чате       
        TGB.update_js(p);
    }
};

// Обработчик ответов на запросы (в нем получаем id сообщения)
let response_handler = function(ed, ud) {
    // Парсинг данных ответа
    let resp = TGB.parse_response(ed);
    // Печать в терминал
    print(JSON.stringify(resp));
    // Проверка наличия панели в списке панелей
    for (let i = 0; i < control_panel.all_panels.length; i++) {
        // Если панель уже есть в этом чате, то прекращаем выполнение функции
        if (resp.chat_id === control_panel.all_panels[i].chat_id) { return; }
    }
    // Если панели еще нет в списке добавляем новую панель в массив панелей
    control_panel.all_panels.push({chat_id: resp.chat_id, message_id: resp.message_id});
};

// Обработчик подписок
let app_start_handler = function(ev, ed, ud) {
    // Подписка на команду '/menu'
    TGB.subscribe('/menu', updates_handler, null);
    // Подписка на нажатия кнопок выполняется аналогично
    // Ниже подписки на нажатия кнопок инлайн клавиатуры
    TGB.subscribe('/alarm_toggle', updates_handler, null);
    TGB.subscribe('/light_toggle', updates_handler, null);
};

// Подписка на событие TGB.CONNECTED (срабатывает когда библиотека подключилась к Telegram Bot API)
Event.addHandler(TGB.CONNECTED, app_start_handler, null);
```

# C API reference

## mgos_event_add_handler()

Перед тем как будет возможно получать и отправлять данные, необходимо дождаться события `TGB_EV_CONNECTED`, которое срабатывает при успешном подключении библиотеки к Telegram Bot API. При наступлении данного события вызывается функция обратного вызова, в которой необходимо подписаться на команды, отправляемые нашему боту. Работа с событиями является функциональностью ядра Mongoose OS, при необходимости Вы можете ознакомиться с данной функциональностью по ссылке: [Event](https://mongoose-os.com/docs/mongoose-os/api/core/mgos_event.h.md).

```C
bool mgos_event_add_handler(int ev, mgos_event_handler_t cb, void *userdata);

// Сигнатура функции обработчика событий Mongoose OS
typedef void (*mgos_event_handler_t)(int ev, void *ev_data, void *userdata);

// Обработчик для выполнения подписки на обновления от Telegram
void subscription_callback(void *ev_data, void *userdata) {
  // Подписка на команду "/start"
  mgos_telegram_subscribe("/start", app_telegram_updates_handler, NULL);
}

// Добавление обработчика события TGB_EV_CONNECTED
mgos_event_add_handler(TGB_EV_CONNECTED, subscription_callback, NULL);
```

## mgos_telegram_subscribe()

Используйте данную функцию для выполнения подписки на обновления от Telegram Bot API (обновлениями в терминологии Telegram Bot API называются входящие данные отправленные нашему боту через мессенджер, это могут быть текстовые сообщения или события нажатия кнопки инлайн клавиатуры). Подписка на сообщения и события нажатия на кнопки выполняются одинаково. Возможно подписаться как на всё сразу, путем подписки на команду '*', так и на конкретные команды, которые Вам необходимы. Все обновления, на которые не оформлена подписка будут игнорироваться. При выполнении подписки передается функция обратного вызова, которая будет вызвана при срабатывании подписки. 

```C
void mgos_telegram_subscribe(const char *data, mgos_telegram_cb_t callback, void *userdata);

// Сигнатура обработчика подписок данной библиотеки
typedef void (*mgos_telegram_cb_t)(void *ev_data MG_UD_ARG(void *userdata));

// Пример подписки на любые команды за один раз
mgos_telegram_subscribe("*", app_telegram_updates_handler, NULL);

// Пример подписки на конкретную команду, в данном случае "/start"
mgos_telegram_subscribe("/start", app_telegram_updates_handler, NULL);
```

## mgos_telegram_send_message(), mgos_telegram_send_message_with_callback(), mgos_telegram_send_message_json(), mgos_telegram_send_message_json_with_callback()

Используйте данные функции для отправки сообщений в чаты и группы. Возможно отправлять как простые текстовые сообщения, так и более сложные, содержащие инлайн клавиатуру. Более подробная информация о методе Telegram Bot API: [sendMessage](https://core.telegram.org/bots/api#sendmessage).

```C
void mgos_telegram_send_message(int32_t chat_id, const char *text);
void mgos_telegram_send_message_with_callback(int32_t chat_id, const char *text, mgos_telegram_cb_t callback, void *userdata);
void mgos_telegram_send_message_json(const char *json);
void mgos_telegram_send_message_json_with_callback(const char *json, mgos_telegram_cb_t callback, void *userdata);

// Обработчик ответов на запросы к Telegram Bot API
void callback(void *ev_data, void *userdata) {
    // Параметр ev_data будет содержать ответ от telegram API на метод sendMessage
    struct mgos_telegram_response *response = (struct mgos_telegram_response *) ev_data;
    // Параметр userdata будет содержать пользовательские данные отправленные при выполнении запроса
    (void) userdata;
}

// Отправка простого текстового сообщения в персональный чат или группу
mgos_telegram_send_message(111222333, "Hello from Mongoose OS");
// Тоже самое но с обработкой ответа
mgos_telegram_send_message_with_callback(111222333, "Hello from Mongoose OS", callback, NULL);

// JSON строка с параметрами родного API (для генерации JSON используется библиотека frozen https://github.com/cesanta/frozen)
char *json = json_asprintf("{chat_id: %d, text: %Q}", 111222333, "Hello from Mongoose OS");
// Отправка сообщения в персональный чат или группу с использованием родного API
mgos_telegram_send_message_json(json);
// Тоже самое но с обработкой ответа
mgos_telegram_send_message_json_with_callback(json, callback, NULL)
free(json);
```

## mgos_telegram_edit_message_text(), mgos_telegram_edit_message_text_json()

Используйте данные функции для обновления (изменения) ранее отправленных сообщений в чатах или группах. Возможно обновить как простые текстовые сообщения, так и более сложные, содержащие инлайн клавиатуру. Более подробная информация о методе Telegram Bot API: [editMessageText](https://core.telegram.org/bots/api#editmessagetext).

```C
void mgos_telegram_edit_message_text(int32_t chat_id, int32_t message_id, const char *text);
void mgos_telegram_edit_message_text_json(const char *json);

// Пример обновления простого текстового сообщения
mgos_telegram_edit_message_text(111222333, 1237, "Updated message from Mongoose OS");

// JSON строка с параметрами родного API (для генерации JSON используется библиотека frozen https://github.com/cesanta/frozen)
char *json = json_asprintf("{chat_id: %d, message_id: %d, text: %Q}", 111222333, 1237, "Updated message from Mongoose OS");
// Пример обновления сообщения с использованием родного API
mgos_telegram_edit_message_text_json(json);
```
## mgos_telegram_answer_callback_query(), mgos_telegram_answer_callback_query_json()

Используйте данные функции для отправки ответа на получение события о нажатии кнопки инлайн клавиатуры. Ответ может быть показан в виде всплывающего сообщения вверху чата или показан в отдельном модальном окне (alert). Более подробная информация о методе Telegram Bot API: [answerCallbackQuery](https://core.telegram.org/bots/api#answercallbackquery).

```C
void mgos_telegram_answer_callback_query(const char *id, const char *text, bool alert);
void mgos_telegram_answer_callback_query_json(const char *json);

// Отправка текстового ответа на нажатие кнопки инлайн клавиатуры
mgos_telegram_answer_callback_query("933786469216003032", "⚠ Command received", false);

// JSON строка с параметрами родного API (для генерации JSON используется библиотека frozen https://github.com/cesanta/frozen)
char *json = json_asprintf("{callback_query_id: %s, text: %s, show_alert: %B}", "933786469216003032", "⚠ Command received", false);
// Пример ответа на нажатие кнопки инлайн клавиатуры с использованием родного API
mgos_telegram_answer_callback_query_json(json);
free(json);
```

## mgos_telegram_execute_custom_method(), mgos_telegram_execute_custom_method_with_callback()

Используйте данные функции для выполнения произвольных запросов, не имеющих специальных функций в данной библиотеке. Подробная информация о доступных методах Telegram Bot API: [Telegram Bot API](https://core.telegram.org/bots/api#available-methods).

```C
void mgos_telegram_execute_custom_method(const char *method, const char *json);
void mgos_telegram_execute_custom_method_with_callback(const char *method, const char *json, mgos_telegram_cb_t callback, void *userdata);

// Пример обработчика ответа на запросы
void callback(void *ev_data, void *userdata) {
    // Параметр ev_data будет содержать ответ от telegram API на метод sendMessage
    struct mgos_telegram_response *response = (struct mgos_telegram_response *) ev_data;
    // Параметр userdata будет содержать пользовательские данные отправленные при выполнении запроса
    (void) userdata;
}

// JSON строка с параметрами родного API (для генерации JSON используется библиотека frozen https://github.com/cesanta/frozen)
char *json = json_asprintf("{chat_id: %d, text: %Q}", 111222333, "Hello from telegram bot lib");

//Пример отправки произвольного метода (для примера sendMessage) 
mgos_telegram_execute_custom_method("sendMessage", json);

//Тоже самое, но с обработкой ответа
mgos_telegram_execute_custom_method_with_callback("sendMessage", json, callback, NULL);
```

## Примеры приложений на C

#### Пример 1. Отправка и получение текстовых сообщений.

```C
#include "mgos.h"
#include "mgos_telegram.h"

void app_start_handler(int ev, void *ev_data, void *userdata);
void app_telegram_updates_handler(void *ev_data, void *userdata);

// Обработчик для выполнения подписок на обновления от Telegram
void app_start_handler(int ev, void *ev_data, void *userdata) {
  (void) ev;
  (void) ev_data;
  (void) userdata;
  // Подписка на любый обновления за один раз
  mgos_telegram_subscribe("*", app_telegram_updates_handler, NULL);
  // Или можно подписаться на конкретные обновления
  // mgos_telegram_subscribe("/start", app_telegram_updates_handler, NULL);
}

// Обработчик входящих обновлений от Telegram
void app_telegram_updates_handler(void *ev_data, void *userdata) {
    (void) userdata;
    // Данные обновления будут переданы с параметром ev_data
    // Объясняем Системе, что это за тип (придет структура struct mgos_telegram_update)
    struct mgos_telegram_update *update = (struct mgos_telegram_update *) ev_data;
	// Выводим в терминал информацию об обновлении
    LOG(LL_INFO, ("APP ->> chat: %li, data: %s", (long)update->chat_id, update->data));
    // Обработка команды "/start"
    if ( strcmp(update->data, "/start") == 0 ) {
		// Отправляем текстовое сообщение отправителю
        mgos_telegram_send_message(update->chat_id, "Hello from mongoose OS");
    }
	// Обработка всех остальных команд
    else {
		// Отправляем текстовое сообщение отправителю (отправляем полученный текст без изменений)
        mgos_telegram_send_message(update->chat_id, update->data);
    }
    return;
}

// APP INIT FN
enum mgos_app_init_result mgos_app_init(void) {
    // Подписка на событие TGB_EV_CONNECTED
    mgos_event_add_handler(TGB_EV_CONNECTED, app_start_handler, NULL);
    return MGOS_APP_INIT_SUCCESS;
};
```

#### Пример 2. Многопользовательская панель управления с автоматическим обновлением состояния.

```C
#include "mgos.h"
#include "mgos_telegram.h"

void app_start_handler(int ev, void *ev_data, void *userdata);
char* get_panel_state();
char* get_panel_keyboard();
void update_all_panels();
void app_telegram_updates_handler(void *ev_data, void *userdata);
void app_telegram_response_handler(void *ev_data, void *userdata);

// Структура для элемента панели управления
struct panel_el {
  uint32_t chat_id;
  uint32_t message_id;
  SLIST_ENTRY(panel_el) next;
};

// Структура для хранения состояния
struct app_state {
  // Состояние тестовых реле
  bool alarm;
  bool light;
  // Список для кранения информации о панелях управления в чатах
  SLIST_HEAD(panels, panel_el) panels;
};

// Переменная для хранения состояния
struct app_state *state = NULL;

// Функция получения актуального состояния для панели
char* get_panel_state() {
  char *res = NULL;
  mg_asprintf(&res, 0, "%s%u\r\n%s%f\r\n%s%s\r\n%s%s",
              "FREE MEM: ", mgos_get_heap_size(),
              "UPTIME: ", mgos_uptime(),              
              "ALARM: ", state->alarm ? "✅ ON" : "❌ OFF",
              "LIGHT: ", state->light ? "✅ ON" : "❌ OFF"
              );
  return res;
}
// Функция получения актуального состояния кнопок
char* get_panel_keyboard() {
  char *res = json_asprintf("{inline_keyboard: [[{text: %Q, callback_data: %Q}],[{text: %Q, callback_data: %Q}]]}", 
                state->alarm ? "SWITCH OFF ALARM" : "SWITCH ON ALARM", "/alarm_toggle",
                state->light ? "SWITCH OFF LIGHT" : "SWITCH ON LIGHT", "/light_toggle"
                );
  return res;
}

// Функция обновления всех панелей управления во всех чатах
void update_all_panels() {
  // Если список панелей пуст, прекращаем выполнение
  if (SLIST_EMPTY(&state->panels)) return;
  // Получаем актуальное состояние панели
  char *text = get_panel_state();
  // Получаем актуальное состояние кнопок панели
  char *keyboard = get_panel_keyboard();
  char *json = NULL;
  struct panel_el *panel;
  // Итерируем список панелей
  SLIST_FOREACH(panel, &state->panels, next) {
	// Формируем JSON с обновленной панелью
    json = json_asprintf("{chat_id: %d, message_id: %d, text: %Q, reply_markup: %s}", 
      panel->chat_id, panel->message_id, text, keyboard);
	// Отправляем обновленную панель в чат
    mgos_telegram_edit_message_text_json(json);
  }
  // Не забываем освободить динамическую память
  free(json);
  free(text);
  free(keyboard);
}

// Обработчик для выполнения подписки на обновления от Telegram
void app_start_handler(int ev, void *ev_data, void *userdata) {
  (void) ev;
  (void) ev_data;
  (void) userdata;
  // Подписываемся на все команды за один раз
  mgos_telegram_subscribe("*", app_telegram_updates_handler, NULL);
  // Или можно подписаться на конкретные команды
  // mgos_telegram_subscribe("/menu", app_telegram_updates_handler, NULL);
  // mgos_telegram_subscribe("/alarm_toggle", app_telegram_updates_handler, NULL);
  // mgos_telegram_subscribe("/light_toggle", app_telegram_updates_handler, NULL);
}

// Обработчик ответов на запросы к Telegram
void app_telegram_response_handler(void *ev_data, void *userdata) {
  // Данные ответа будут переданы с параметром ev_data
  // Объясняем Системе, что это за тип (придет структура struct mgos_telegram_response)
  struct mgos_telegram_response *response = (struct mgos_telegram_response *) ev_data;
  // Новый элемент списка панелей будет передан в пользовательских данных userdata
  struct panel_el *new_panel = (struct panel_el *) userdata;
  // Добавляем в элемент новой панели id сообщения
  new_panel->message_id = response->message_id;
  // Добавляем элемент панели в список панелей
  SLIST_INSERT_HEAD(&state->panels, new_panel, next);
}

// Обработчик обновлений от Telegram
void app_telegram_updates_handler(void *ev_data, void *userdata) {
    (void) userdata;
    // Данные обновления будут переданы с параметром ev_data
    // Объясняем Системе, что это за тип (придет структура struct mgos_telegram_update)
    struct mgos_telegram_update *update = (struct mgos_telegram_update *) ev_data;
    // Вывод данных в терминал
    LOG(LL_INFO, ("APP ->> chat: %li, data: %s", (long)update->chat_id, update->data));
    // Обработка команды "/menu"
    if ( strcmp(update->data, "/menu") == 0 ) {
      // Проверяем, существует ли уже панель в чате
      if (!SLIST_EMPTY(&state->panels)) {
        struct panel_el *panel;
        SLIST_FOREACH(panel, &state->panels, next) {
			// Если панель уже есть, завершаем выполнение функции
			if (panel->chat_id == update->chat_id) return;
        };        
      }
      // Если дошли сюда, значит панели в этом чате еще нет
	  // Создаем новый элемент панели 
      struct panel_el *new_panel = (struct panel_el *) calloc(1, sizeof(*new_panel));
      new_panel->chat_id = update->chat_id;
	  // Получаем актуальное состояние панели
      char *text = get_panel_state();
	  // Получаем актуальное состояние кнопок
      char *keyboard = get_panel_keyboard();
	  // Генерируем JSON с сообщением в котором содержится панель управления
      char *json = json_asprintf("{chat_id: %d, text: %Q, reply_markup: %s}", update->chat_id, text, keyboard);
	  // Отправляем сообщение с обработкой ответа в чат
	  // Также в качестве пользовательских данных передаем ссылку на новую панель
	  // Туда в обработчике ответа добавим id отправленного сообщения и добавим в список всех панелей
      mgos_telegram_send_message_json_with_callback(json, app_telegram_response_handler, new_panel);
	  // Не забываем освободить динамическую память
      free(text);
      free(keyboard);
      free(json);
      return;
    }
	// Обработка команды "/alarm_toggle"
    if ( strcmp(update->data, "/alarm_toggle") == 0 ) {
	  // Инвертируем состояние реле
      state->alarm = !state->alarm;
	  // Отправляем ответ на нажатие кнопки
      mgos_telegram_answer_callback_query(update->query_id, "⚠ Command received", false);
	  // Состояние реле изменилось, вызываем функцию обновления всех панелей во всех чатах
      update_all_panels();
    }
	// Обработка команды "/light_toggle"
    if ( strcmp(update->data, "/light_toggle") == 0 ) {
	  // Инвертируем состояние реле
      state->light = !state->light;
	  // Отправляем ответ на нажатие кнопки
      mgos_telegram_answer_callback_query(update->query_id, "⚠ Command received", false);
	  // Состояние реле изменилось, вызываем функцию обновления всех панелей во всех чатах
      update_all_panels();
    }
}

// Функция инициализации приложения
enum mgos_app_init_result mgos_app_init(void) {
  // Выделяем память для состояния
  state = (struct app_state *) calloc(1, sizeof(*state));
  // Указываем начальные состояния реле
  state->alarm = false;
  state->light = false;
  // Подписываемся на событие TGB_EV_CONNECTED
  mgos_event_add_handler(TGB_EV_CONNECTED, app_start_handler, NULL);
  return MGOS_APP_INIT_SUCCESS;
};
```