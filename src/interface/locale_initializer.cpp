#include "FileZilla.h"
#include "locale_initializer.h"
#include "../tinyxml/tinyxml.h"
#include <string>

#ifdef ENABLE_BINRELOC
	#define BR_PTHREADS 0
	#include "prefix.h"
#endif


// Custom main method to initialize proper locale
#ifdef __WXGTK__

bool CInitializer::error = false;

static std::string mkstr(const char* str)
{
	if (!str)
		return "";
	else
		return str;
}

int main(int argc, char** argv)
{
	std::string locale = CInitializer::GetLocaleOption();
	if (locale != "")
	{
		if (!CInitializer::SetLocale(locale.c_str()))
		{
#ifdef __WXDEBUG__
			printf("failed to set locale\n");
#endif
			CInitializer::error = true;
		}
		else
		{
#ifdef __WXDEBUG__
			printf("locale set to %s\n", setlocale(LC_ALL, 0));
#endif
		}
	}

	return wxEntry(argc, argv);
}

bool CInitializer::SetLocale(const char* arg)
{
	char locale[50];

	if (strlen(arg) > 40)
		return false;

	strcpy(locale, arg);

	// First try with utf8 locale
	// Take special care of modifiers
	const char* p = strchr(arg, '@');
	if (p)
	{
		locale[p - arg] = 0;
		strcat(locale, ".utf8");
		strcat(locale, p);
	}
	else
		strcat(locale, ".utf8");

	if (setlocale(LC_ALL, locale))
	{
#ifdef __WXDEBUG__
		printf("setlocale %s successful\n", locale);
#endif
		setenv("LC_ALL", locale, 1);
		return true;
	}

	strcpy(locale, arg);
	if (setlocale(LC_ALL, locale))
	{
		setenv("LC_ALL", locale, 1);
		return true;
	}

	return false;
}

std::string CInitializer::GetDefaultsXmlFile()
{
	std::string home = mkstr(getenv("HOME"));
	if (!home.empty())
	{
		if (home[home.size() - 1] != '/')
			home += '/';
		home += ".filezilla/fzdefaults.xml";

		struct stat buf;
		if (!stat(home.c_str(), &buf))
			return home;
	}

	std::string file("/etc/filezilla/fzdefaults.xml");

	struct stat buf;
	if (!stat(file.c_str(), &buf))
		return file;


	file = mkstr(SELFPATH);
	if (file.empty())
		return "";

	int p = file.rfind('/');
	if (p == -1)
		return "";
	file = file.substr(0, p);

	p = file.rfind('/');
	if (p == -1)
		return "";
	file = file.substr(0, p);

	file += "/share/filezilla/fzdefaults.xml";
	if (!stat(file.c_str(), &buf))
		return file;

	return "";
}

std::string CInitializer::ReadSettingsFromDefaults(std::string file)
{
	std::string dir = CInitializer::GetSettingFromFile(file, "Config Location");

	if (dir.empty())
		return "";

	std::string result;
	while (!dir.empty())
	{
		std::string token;
		int pos = dir.find('/');
		if (pos == -1)
			token.swap(dir);
		else
		{
			token = dir.substr(0, pos);
			dir = dir.substr(pos + 1);
		}

		if (token[0] == '$')
		{
			if (token[1] == '$')
				result += token.substr(1);
			else if (token.size() > 1)
			{
				std::string value = mkstr(getenv(token.substr(1).c_str()));
				result += value;
			}
		}
		else
			result += token;

		result += '/';
	}

	struct stat buf;
	if (stat(result.c_str(), &buf))
		return "";

	if (result[result.size() - 1] != '/')
		result += '/';

	return result;
}

std::string CInitializer::GetSettingFromFile(std::string file, const std::string& name)
{
	TiXmlDocument xmldoc;
	if (!xmldoc.LoadFile(file.c_str()))
		return "";

	TiXmlElement* main = xmldoc.FirstChildElement("FileZilla3");
	if (!main)
		return "";

	TiXmlElement* settings = main->FirstChildElement("Settings");
	if (!main)
		return "";

	for (TiXmlElement* setting = settings->FirstChildElement("Setting"); setting; setting = settings->NextSiblingElement("Setting"))
	{
		const char* nodeVal = setting->Attribute("name");
		if (!nodeVal || strcmp(nodeVal, name.c_str()))
			continue;

		TiXmlNode* textNode = setting->FirstChild();
		if (!textNode || !textNode->ToText())
			continue;

		return mkstr(textNode->Value());
	}

	return "";
}

std::string CInitializer::GetSettingsDir()
{
	std::string defaults = GetDefaultsXmlFile();
	if (!defaults.empty())
	{
		std::string dir = CInitializer::ReadSettingsFromDefaults(defaults);
		if (!dir.empty())
			return dir;
	}

	std::string home = mkstr(getenv("HOME"));
	if (home.empty())
		 return "";

	if (home[home.size() - 1] != '/')
		home += '/';
	home += ".filezilla/";

	return home;
}

std::string CInitializer::GetLocaleOption()
{
	const std::string dir = GetSettingsDir();
	if (dir.empty())
		return "";

	std::string locale = GetSettingFromFile(dir + "filezilla.xml", "Language Code");

	return locale;
}

#endif //__WXGTK__
