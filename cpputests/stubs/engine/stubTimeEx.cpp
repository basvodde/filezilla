
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "filezilla.h"
#include "timeex.h"

CTimeEx::CTimeEx()
{
	FAIL("CTimeEx::CTimeEx");
}

CTimeEx CTimeEx::Now()
{
	FAIL("CTimeEx::Now");
	return CTimeEx();
}

bool CTimeEx::operator== (const CTimeEx& op) const
{
	FAIL("CTimeEx::operator==");
	return true;
}
