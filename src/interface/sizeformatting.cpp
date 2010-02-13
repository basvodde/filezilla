#include <filezilla.h>
#include "sizeformatting.h"
#include "Options.h"

wxString CSizeFormat::Format(const wxLongLong& size, bool add_bytes_suffix, enum CSizeFormat::_format format, bool thousands_separator, int num_decimal_places)
{
	return CSizeFormatBase::Format(COptions::Get(), size, add_bytes_suffix, format, thousands_separator, num_decimal_places);
}

wxString CSizeFormat::Format(const wxLongLong& size, bool add_bytes_suffix /*=false*/)
{
	return CSizeFormatBase::Format(COptions::Get(), size, add_bytes_suffix);
}

wxString CSizeFormat::FormatUnit(const wxLongLong& size, CSizeFormat::_unit unit, int base /*=1024*/)
{
	return CSizeFormatBase::FormatUnit(COptions::Get(), size, unit, base);
}

wxString CSizeFormat::GetUnitWithBase(_unit unit, int base)
{
	return CSizeFormatBase::GetUnitWithBase(COptions::Get(), unit, base);
}

wxString CSizeFormat::GetUnit(_unit unit, CSizeFormat::_format format /*=formats_count*/)
{
	return CSizeFormatBase::GetUnit(COptions::Get(), unit, format);
}

wxString CSizeFormat::FormatNumber(const wxLongLong& size, bool* thousands_separator /*=0*/)
{
	return CSizeFormatBase::FormatNumber(COptions::Get(), size, thousands_separator);
}
