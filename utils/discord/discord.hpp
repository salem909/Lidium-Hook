#pragma once
#include <discord.h>

namespace mm_template::utils
{
	class drpc final
	{
    public:
        static void init();

		static void update();

		static void change_status(const std::string& text);
		static void change_large_image(const std::string& image, const std::string& text);
		static void change_small_image(const std::string& image, const std::string& text);
	};
}
