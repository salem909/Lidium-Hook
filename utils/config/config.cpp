#include <utils/config/config.hpp>
#include <utils/format/format.hpp>
#include <utils/console/console.hpp>
#include <filesystem>

#pragma warning(push, 0)	//Removed deprecation warnings
#include <Utils/INI/IniReader.h>
#pragma warning(pop)

namespace mm_template
{
	int  config::CPUMaxPerformance;
	void config::init()
	{
		CIniReader iniReader("settings.ini");

		if (!std::filesystem::exists("settings.ini"))
		{
			iniReader.WriteInteger("Settings", "CPUMaxPerformance", 0);
		}

		CPUMaxPerformance = iniReader.ReadInteger("Other", "CPUMaxPerformance", 0);
		
	}
}