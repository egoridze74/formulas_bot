#include <tgbot/tgbot.h>

#include <cstdio>

#include "../include/constants.h"
#include "../include/db_utils.h"
#include "../include/models/group.h"
#include "../include/state_utils.h"
using namespace keyboards;
std::unordered_map<int64_t, Group> userGroup;

int main() {
  db::init_conn();

  TgBot::Bot bot(consts::TOKEN);

  init_keyboards();
  
  /*bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
    printf("User wrote %s\n", message->text.c_str());
    if (StringTools::startsWith(message->text, '/'+commands::START)) {
      return;
    }
    const auto query_result = db::basic_where(message->text);
    bot.getApi().sendMessage(message->chat->id,
                             std::to_string(query_result.size()));
    for (const std::string& res : query_result) {
      bot.getApi().sendMessage(message->chat->id, res);
    }
    return;
  });*/

  // Stepan's piece

  bot.getEvents().onCallbackQuery([&bot, create_group_keyboard, delete_group_keyboard, edit_group_keyboard](
                                      TgBot::CallbackQuery::Ptr query) {
    if (query->data == button_datas::create_group) {
      int64_t userId = query->message->chat->id;
      userGroup[userId].set_owner_id(userId);
      bot.getApi().sendMessage(userId, messages::PrintGroupName);
      setState(userId, Group_State::WAITING_FOR_GROUP_NAME);
    } else if (query->data == button_datas::delete_group) {
      int64_t userId = query->message->chat->id;
      // удаление группы из базы данных
      bot.getApi().sendMessage(userId, messages::DeletedGroup);
      // вернуться в меню группы
    } else if (query->data == button_datas::edit_group) {
      int64_t userId = query->message->chat->id;
      //изменение в базе данных имени группы
      bot.getApi().sendMessage(userId, messages::PrintNewGroupName);
      setState(userId, Group_State::WAITING_FOR_NEW_GROUP_NAME);
      // вернуться в меню группы
    }
  });

  bot.getEvents().onCommand(commands::START, [&bot](TgBot::Message::Ptr message) {
    if (message->chat->type == TgBot::Chat::Type::Private) {
      bot.getApi().sendMessage(message->chat->id, messages::HI);
    }
  });

  bot.getEvents().onCommand(
      commands::CREATEGROUP, [&bot, create_group_keyboard](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Меню.", NULL, 0, create_group_keyboard);
      });

  bot.getEvents().onCommand(
      commands::DELETEGROUP, [&bot, delete_group_keyboard](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Меню", NULL, 0, delete_group_keyboard);
      });

  bot.getEvents().onCommand(commands::EDITGROUP, [&bot, edit_group_keyboard](TgBot::Message::Ptr message) {
      bot.getApi().sendMessage(message->chat->id, "Меню.", NULL, 0, edit_group_keyboard);  
  });

  bot.getEvents().onAnyMessage(
      [&bot](TgBot::Message::Ptr message) {
        int64_t userId = message->chat->id;
        Group_State state = getState(userId);

        if (state == Group_State::WAITING_FOR_GROUP_NAME) {
          // запись названия в базу данных
          printf("Name of group: %s\n", message->text.c_str());
          userGroup[userId].set_group_name(message->text);
          bot.getApi().sendMessage(userId, messages::SavedNameGroup);
          bot.getApi().sendMessage(userId, messages::CreatedGroup);
          setState(userId, Group_State::NONE);
          // вернуться в меню группы
        } else if (state == Group_State::WAITING_FOR_NEW_GROUP_NAME) {
          // вводим значит new name и изменяем в базе данных
          userGroup[userId].set_group_name(message->text);
          bot.getApi().sendMessage(userId, messages::SavedNewNameGroup);
          setState(userId, Group_State::NONE);
          // вернуться в меню группы
        } else {
          printf("User wrote %s\n", message->text.c_str());
          if (StringTools::startsWith(message->text, '/'+commands::START) ||
              StringTools::startsWith(message->text, '/'+commands::CREATEGROUP) ||
              StringTools::startsWith(message->text, '/'+commands::DELETEGROUP) ||
              StringTools::startsWith(message->text, '/'+commands::EDITGROUP)) {
            return;
          }
        }
      });

  try {
    printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
    TgBot::TgLongPoll longPoll(bot);
    while (true) {
      printf("Long poll started\n");
      longPoll.start();
    }
  } catch (TgBot::TgException& e) {
    printf("error: %s\n", e.what());
  }

  return 0;
}
