#ifndef __LOCALE_INITIALIZER_H__

#ifdef __WXGTK__

#include <string>

class CInitializer
{
public:
	static bool SetLocale(const char* arg);

	static std::string GetLocaleOption();

	static bool error;

protected:
	static std::string GetSettingsDir();
	static std::string ReadSettingsFromDefaults(std::string file);
	static std::string GetSettingFromFile(std::string file, const std::string& name);
	static std::string GetDefaultsXmlFile();
};

#endif //__WXGTK__

#endif //__LOCALE_INITIALIZER_H__
