
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "filezilla.h"
#include "sizeformatting_base.h"


wxString CSizeFormatBase::FormatNumber(COptionsBase* pOptions, const wxLongLong& size, bool* thousands_separator)
{
	FAIL("CSizeFormatBase::FormatNumber");
	return L"";
}

wxString CSizeFormatBase::GetUnitWithBase(COptionsBase* pOptions, _unit unit, int base)
{
	FAIL("CSizeFormatBase::GetUnitWithBase");
	return L"";
}

wxString CSizeFormatBase::GetUnit(COptionsBase* pOptions, _unit unit, enum _format)
{
	FAIL("CSizeFormatBase::GetUnit");
	return L"";
}

wxString CSizeFormatBase::FormatUnit(COptionsBase* pOptions, const wxLongLong& size, _unit unit, int base)
{
	FAIL("CSizeFormatBase::FormatUnit");
	return L"";
}

wxString CSizeFormatBase::Format(COptionsBase* pOptions, const wxLongLong& size, bool add_bytes_suffix, enum _format format, bool thousands_separator, int num_decimal_places)
{
	FAIL("CSizeFormatBase::Format");
	return L"";
}

wxString CSizeFormatBase::Format(COptionsBase* pOptions, const wxLongLong& size, bool add_bytes_suffix)
{
	FAIL("CSizeFormatBase::Format");
	return L"";
}

