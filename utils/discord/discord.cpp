#include <main.hpp>

#include <utils/console/console.hpp>
#include <utils/discord/discord.hpp>
#include <utils/format/format.hpp>

#include <discord.h>

discord::Core* core{};
discord::Activity activity{};
bool enabled;

namespace mm_template::utils
{
	void drpc::init()
	{
		if (const auto result = discord::Core::Create(795636858255048715, ::DiscordCreateFlags_NoRequireDiscord, &core); result == discord::Result::Ok)
		{
			if (result == discord::Result::Ok) enabled = true;
			else enabled = false;

			core->SetLogHook(discord::LogLevel::Info, [](auto, auto message)
				{
					PRINT_INFO("%s", message);
				});

			core->SetLogHook(discord::LogLevel::Warn, [](auto, auto message)
				{
					PRINT_WARNING("%s", message);
				});

			core->SetLogHook(discord::LogLevel::Error, [](auto, auto message)
				{
					PRINT_ERROR("%s", message);
				});

			//drpc::change_large_image("", "");
			//drpc::change_status("");
			activity.GetTimestamps().SetStart(std::time(nullptr));

			drpc::update();

			if (result == discord::Result::Ok)
			{
				boost::thread([]() -> void
					{
						char PlayerLevel[256];
						char MapName[256];
						char PlayerName[256];
						char PlayerJob[256];
						char SelectIcon[256];

						for (;;)
						{
							if (mm_template::main::ML_RPC_GetJobID != 600200)
							{
								switch (mm_template::main::ML_RPC_GetJobID)
								{

								case 800: sprintf(SelectIcon,"icon"); sprintf(PlayerJob, "Manager "); break;
								case 900: sprintf(SelectIcon,"icon"); sprintf(PlayerJob, "GM "); break;
								case 910: sprintf(SelectIcon,"icon"); sprintf(PlayerJob, "Super GM "); break;
								case 0:	  sprintf(SelectIcon,"icon"); sprintf(PlayerJob, "Beginner"); break;
								case 100: sprintf(SelectIcon,"1"); sprintf(PlayerJob, "Warrior"); break;
								case 110: sprintf(SelectIcon,"1"); sprintf(PlayerJob, "Fighter"); break;
								case 111: sprintf(SelectIcon,"1"); sprintf(PlayerJob, "Crusader "); break;
								case 112: sprintf(SelectIcon,"1"); sprintf(PlayerJob, "Hero "); break;
								case 120: sprintf(SelectIcon,"1"); sprintf(PlayerJob, "Page"); break;
								case 121: sprintf(SelectIcon,"1"); sprintf(PlayerJob, "White Knight"); break;
								case 122: sprintf(SelectIcon,"1"); sprintf(PlayerJob, "Paladin"); break;
								case 130: sprintf(SelectIcon,"1"); sprintf(PlayerJob, "Spearman"); break;
								case 131: sprintf(SelectIcon,"1"); sprintf(PlayerJob, "Dragon Knight"); break;
								case 132: sprintf(SelectIcon,"1"); sprintf(PlayerJob, "Dark Knight"); break;
								case 200: sprintf(SelectIcon,"2"); sprintf(PlayerJob, "Magician"); break;
								case 210: sprintf(SelectIcon,"2"); sprintf(PlayerJob, "Fire/Poison Wizard  "); break;
								case 211: sprintf(SelectIcon,"2"); sprintf(PlayerJob, "Fire/Poison Mage"); break;
								case 212: sprintf(SelectIcon,"2"); sprintf(PlayerJob, "Fire/Poison Archmage"); break;
								case 220: sprintf(SelectIcon,"2"); sprintf(PlayerJob, "Ice/Lightning Wizard"); break;
								case 221: sprintf(SelectIcon,"2"); sprintf(PlayerJob, "Ice/Lightning Mage"); break;
								case 222: sprintf(SelectIcon,"2"); sprintf(PlayerJob, "Ice/Lightning Archmage"); break;
								case 230: sprintf(SelectIcon,"2"); sprintf(PlayerJob, "Cleric"); break;
								case 231: sprintf(SelectIcon,"2"); sprintf(PlayerJob, "Priest"); break;
								case 232: sprintf(SelectIcon,"2"); sprintf(PlayerJob, "Bishop"); break;
								case 300: sprintf(SelectIcon,"3"); sprintf(PlayerJob, "Bowman"); break;
								case 310: sprintf(SelectIcon,"3"); sprintf(PlayerJob, "Hunter"); break;
								case 311: sprintf(SelectIcon,"3"); sprintf(PlayerJob, "Ranger"); break;
								case 312: sprintf(SelectIcon,"3"); sprintf(PlayerJob, "Bow Master"); break;
								case 320: sprintf(SelectIcon,"3"); sprintf(PlayerJob, "Crossbowman"); break;
								case 321: sprintf(SelectIcon,"3"); sprintf(PlayerJob, "Sniper"); break;
								case 322: sprintf(SelectIcon,"3"); sprintf(PlayerJob, "Marksman"); break;
								case 400: sprintf(SelectIcon,"4"); sprintf(PlayerJob, "Thief"); break;
								case 410: sprintf(SelectIcon,"4"); sprintf(PlayerJob, "Assassin"); break;
								case 411: sprintf(SelectIcon,"4"); sprintf(PlayerJob, "Hermit "); break;
								case 412: sprintf(SelectIcon,"4"); sprintf(PlayerJob, "Night Lord"); break;
								case 420: sprintf(SelectIcon,"4"); sprintf(PlayerJob, "Bandit"); break;
								case 421: sprintf(SelectIcon,"4"); sprintf(PlayerJob, "Chief Bandit"); break;
								case 422: sprintf(SelectIcon,"4"); sprintf(PlayerJob, "Shadower"); break;
								case 500: sprintf(SelectIcon,"6"); sprintf(PlayerJob, "Pirate"); break;
								case 510: sprintf(SelectIcon,"6"); sprintf(PlayerJob, "Brawler"); break;
								case 511: sprintf(SelectIcon,"6"); sprintf(PlayerJob, "Marauder"); break;
								case 512: sprintf(SelectIcon,"6"); sprintf(PlayerJob, "Buccaneer"); break;
								case 520: sprintf(SelectIcon,"6"); sprintf(PlayerJob, "Gunslinger"); break;
								case 521: sprintf(SelectIcon,"6"); sprintf(PlayerJob, "Outlaw"); break;
								case 522: sprintf(SelectIcon,"6"); sprintf(PlayerJob, "Corsair"); break;
								case 1000:  sprintf(SelectIcon, "6"); sprintf(PlayerJob, "Noblesse"); break;
								}

							}
							else
							{
								sprintf(SelectIcon, "icon");
							}
							if (mm_template::main::ML_RPC_PlayerName != 0)
							{
								//So now we convert that dword to a pointer of the wchar_t
								//and then you convert it back to a char for discord rpc
								char* GlobalPlayerName = (char*)mm_template::main::ML_RPC_PlayerName;
								sprintf(PlayerName, "IGN : %s ", GlobalPlayerName);
								
							}
							else
							{
								sprintf(PlayerName, "Not logged in.");
							}
							if (mm_template::main::ML_RPC_GetMapName != 0)
							{
								//So now we convert that dword to a pointer of the wchar_t
								//and then you convert it back to a char for discord rpc
								char* GlobalMapName = (char*)mm_template::main::ML_RPC_GetMapName;
								sprintf(MapName, "At %s", GlobalMapName);
							}
							else
							{
								sprintf(MapName, "No Location. ");
								
							}
							if (mm_template::main::ML_RPC_GetLevel != 0)
							{
								sprintf(PlayerLevel, "Level %u - ", mm_template::main::ML_RPC_GetLevel);
							}
							else
							{
								sprintf(PlayerLevel, "Level N/A ");
							}

							strcat(PlayerLevel, PlayerJob);
							strcat(PlayerName, PlayerLevel);

							drpc::change_status(MapName);
							drpc::change_large_image("icon", PlayerName);
							drpc::change_small_image(SelectIcon, PlayerName);
							::core->RunCallbacks();

							std::this_thread::sleep_for(64ms);
						}
					});
			}
		}
	}

	void drpc::change_status(const std::string& text)
	{
		if (enabled)
		{
			activity.SetState(&text[0]);

			drpc::update();
		}
	}

	void drpc::change_large_image(const std::string& image, const std::string& text)
	{
		if (enabled)
		{
			activity.GetAssets().SetLargeImage(&image[0]);
			activity.GetAssets().SetLargeText(&text[0]);

			drpc::update();
		}
	}

	void drpc::change_small_image(const std::string& image, const std::string& text)
	{
		if (enabled)
		{
			activity.GetAssets().SetSmallImage(&image[0]);
			activity.GetAssets().SetSmallText(&text[0]);

			drpc::update();
		}
	}

	void drpc::update()
	{
		if (enabled)
		{
			core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {});
		}
	}
}
