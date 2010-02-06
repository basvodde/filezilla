#ifndef __SIZEFORMATTING_H__
#define __SIZEFORMATTING_H__

class CSizeFormat
{
public:
	enum _format
	{
		bytes,
		iec,
		si1024,
		si1000,

		formats_count
	};

	// We stop at Exa. If someone has files bigger than that, he can afford to
	// make a donation to have this changed ;)
	enum _unit
	{
		byte,
		kilo,
		mega,
		giga,
		tera,
		peta,
		exa
	};

	static wxString FormatNumber(const wxLongLong& size, bool* thousands_separator = 0);

	static wxString GetUnit(_unit unit, enum _format = formats_count);
	static wxString FormatUnit(const wxLongLong& size, _unit unit, int base = 1024);

	static wxString Format(const wxLongLong& size, bool add_bytes_suffix, enum _format format, bool thousands_separator, int num_decimal_places);
	static wxString Format(const wxLongLong& size, bool add_bytes_suffix = false);

	static const wxString& GetThousandsSeparator();
	static const wxString& GetRadixSeparator();
};

#endif //__SIZEFORMATTING_H__
