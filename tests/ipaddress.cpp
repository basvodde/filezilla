#include "FileZilla.h"
#include "directorylistingparser.h"
#include <cppunit/extensions/HelperMacros.h>
#include <list>

/*
 * This testsuite asserts the correctness of the
 * functions handling IP addresses
 */

class CIPAddressTest : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(CIPAddressTest);
	CPPUNIT_TEST(testIPV6LongForm);
	CPPUNIT_TEST(testIsRoutableAddress4);
	CPPUNIT_TEST(testIsRoutableAddress6);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {}
	void tearDown() {}

	void testIPV6LongForm();
	void testIsRoutableAddress4();
	void testIsRoutableAddress6();

protected:
};

CPPUNIT_TEST_SUITE_REGISTRATION(CIPAddressTest);

void CIPAddressTest::testIPV6LongForm()
{
	// Valid addresses
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("::1")) == _T("0000:0000:0000:0000:0000:0000:0000:0001"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("1234::1")) == _T("1234:0000:0000:0000:0000:0000:0000:0001"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("4::1")) == _T("0004:0000:0000:0000:0000:0000:0000:0001"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("1234:abcd::1234:ef01")) == _T("1234:abcd:0000:0000:0000:0000:1234:ef01"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("1234:ABCD::1234:ef01")) == _T("1234:abcd:0000:0000:0000:0000:1234:ef01"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("0:0:0:0:0:0:0:1")) == _T("0000:0000:0000:0000:0000:0000:0000:0001"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("0:0:0::0:0:0:0:1")) == _T("0000:0000:0000:0000:0000:0000:0000:0001"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("::0:0:0:0:0:0:0:1")) == _T("0000:0000:0000:0000:0000:0000:0000:0001"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("0000:0000:0000:0000:0000:0000:0000:0001")) == _T("0000:0000:0000:0000:0000:0000:0000:0001"));

	CPPUNIT_ASSERT(GetIPV6LongForm(_T("[::1]")) == _T("0000:0000:0000:0000:0000:0000:0000:0001"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("[1234::1]")) == _T("1234:0000:0000:0000:0000:0000:0000:0001"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("[4::1]")) == _T("0004:0000:0000:0000:0000:0000:0000:0001"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("[1234:abcd::1234:ef01]")) == _T("1234:abcd:0000:0000:0000:0000:1234:ef01"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("[1234:ABCD::1234:ef01]")) == _T("1234:abcd:0000:0000:0000:0000:1234:ef01"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("[0:0:0:0:0:0:0:1]")) == _T("0000:0000:0000:0000:0000:0000:0000:0001"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("[0:0:0::0:0:0:0:1]")) == _T("0000:0000:0000:0000:0000:0000:0000:0001"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("[::0:0:0:0:0:0:0:1]")) == _T("0000:0000:0000:0000:0000:0000:0000:0001"));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("[0000:0000:0000:0000:0000:0000:0000:0001]")) == _T("0000:0000:0000:0000:0000:0000:0000:0001"));

	// Invalid ones
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("::")) == _T(""));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T(":::")) == _T(""));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("1234:abcd::1234::ef01")) == _T(""));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("[1234:abcd::1234::ef01")) == _T(""));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("1234:abcd::1234::ef01]")) == _T(""));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("[[1234:abcd::1234::ef01]]")) == _T(""));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("1234:abcde:1234::ef01")) == _T(""));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("1234:abcg:1234::ef01")) == _T(""));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T(":::1")) == _T(""));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("0:0:0:0:0:0:0:1:2")) == _T(""));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("0:0:0:0:0:0:0:1:2:0:0:0:0:0:0:1:2")) == _T(""));
	CPPUNIT_ASSERT(GetIPV6LongForm(_T("0::0:0:0:0:0:0:1:2:0:0:0:0:0:0:1")) == _T(""));
}

void CIPAddressTest::testIsRoutableAddress4()
{
	// 127.0.0.0/8
	CPPUNIT_ASSERT(IsRoutableAddress(_T("126.255.255.255"), CSocket::ipv4));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("127.0.0.0"), CSocket::ipv4));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("127.255.255.255"), CSocket::ipv4));
	CPPUNIT_ASSERT(IsRoutableAddress(_T("128.0.0.0"), CSocket::ipv4));

	// 10.0.0.0/8
	CPPUNIT_ASSERT(IsRoutableAddress(_T("9.255.255.255"), CSocket::ipv4));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("10.0.0.0"), CSocket::ipv4));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("10.255.255.255"), CSocket::ipv4));
	CPPUNIT_ASSERT(IsRoutableAddress(_T("11.0.0.0"), CSocket::ipv4));

	// 169.254.0.0/16
	CPPUNIT_ASSERT(IsRoutableAddress(_T("169.253.255.255"), CSocket::ipv4));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("169.254.0.0"), CSocket::ipv4));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("169.254.255.255"), CSocket::ipv4));
	CPPUNIT_ASSERT(IsRoutableAddress(_T("169.255.0.0"), CSocket::ipv4));

	// 192.168.0.0/16
	CPPUNIT_ASSERT(IsRoutableAddress(_T("192.167.255.255"), CSocket::ipv4));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("192.168.0.0"), CSocket::ipv4));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("192.168.255.255"), CSocket::ipv4));
	CPPUNIT_ASSERT(IsRoutableAddress(_T("102.169.0.0"), CSocket::ipv4));

	// 172.16.0.0/20
	CPPUNIT_ASSERT(IsRoutableAddress(_T("172.15.255.255"), CSocket::ipv4));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("172.16.0.0"), CSocket::ipv4));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("172.31.255.255"), CSocket::ipv4));
	CPPUNIT_ASSERT(IsRoutableAddress(_T("172.32.0.0"), CSocket::ipv4));
}

void CIPAddressTest::testIsRoutableAddress6()
{
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("::1"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("::0"), CSocket::ipv6));
	CPPUNIT_ASSERT(IsRoutableAddress(_T("::2"), CSocket::ipv6));
	CPPUNIT_ASSERT(IsRoutableAddress(_T("1234:ABCD::1234:ef01"), CSocket::ipv6));

	// fe80::/10 (link local)
	CPPUNIT_ASSERT(IsRoutableAddress(_T("fe7f:ffff:ffff:ffff:ffff:ffff:ffff:ffff"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("fe80:0000:0000:0000:0000:0000:0000:0000"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("febf:ffff:ffff:ffff:ffff:ffff:ffff:ffff"), CSocket::ipv6));
	CPPUNIT_ASSERT(IsRoutableAddress(_T("fec0:0000:0000:0000:0000:0000:0000:0000"), CSocket::ipv6));

	// fc00::/7 (site local)
	CPPUNIT_ASSERT(IsRoutableAddress(_T("fbff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("fc00:0000:0000:0000:0000:0000:0000:0000"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("fdff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"), CSocket::ipv6));
	CPPUNIT_ASSERT(IsRoutableAddress(_T("fe00:0000:0000:0000:0000:0000:0000:0000"), CSocket::ipv6));

	// IPv4 mapped

	// 127.0.0.0/8
	CPPUNIT_ASSERT(IsRoutableAddress(_T("::ffff:7eff:ffff"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("::ffff:7f00:0000"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("::ffff:7fff:ffff"), CSocket::ipv6));
	CPPUNIT_ASSERT(IsRoutableAddress(_T("::ffff:8000:0000"), CSocket::ipv6));

	// 10.0.0.0/8
	CPPUNIT_ASSERT(IsRoutableAddress(_T("::ffff:9ff:ffff"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("::ffff:0a00:0000"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("::ffff:0aff:ffff"), CSocket::ipv6));
	CPPUNIT_ASSERT(IsRoutableAddress(_T("::ffff:0b00:0000"), CSocket::ipv6));

	// 169.254.0.0/16
	CPPUNIT_ASSERT(IsRoutableAddress(_T("::ffff:a9fd:ffff"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("::ffff:a9fe:0000"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("::ffff:a9fe:ffff"), CSocket::ipv6));
	CPPUNIT_ASSERT(IsRoutableAddress(_T("::ffff:a9ff:0000"), CSocket::ipv6));

	// 192.168.0.0/16
	CPPUNIT_ASSERT(IsRoutableAddress(_T("::ffff:c0a7:ffff"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("::ffff:c0a8:0000"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("::ffff:c0a8:ffff"), CSocket::ipv6));
	CPPUNIT_ASSERT(IsRoutableAddress(_T("::ffff:c0a9:0000"), CSocket::ipv6));

	// 172.16.0.0/20
	CPPUNIT_ASSERT(IsRoutableAddress(_T("::ffff:ac0f:ffff"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("::ffff:ac10:0000"), CSocket::ipv6));
	CPPUNIT_ASSERT(!IsRoutableAddress(_T("::ffff:ac1f:ffff"), CSocket::ipv6));
	CPPUNIT_ASSERT(IsRoutableAddress(_T("::ffff:ac20:0000"), CSocket::ipv6));
}
