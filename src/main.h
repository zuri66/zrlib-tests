#include <stdio.h>
#include <minunit/minunit.h>

// ============================================================================

extern char MESSAGE_BUFF[];
extern char FUN_PREFIX[];

// ============================================================================

#define FUN_CMP_NAME \
	cmpResult
#define FUN_PRINT_NAME \
	printResult

#define ZRTEST_RESULT(prefix, msg_buffer, result, expected) \
	testResult(prefix, msg_buffer, result, expected, (int (*)(void*, void*))FUN_CMP_NAME, (void (*)(char*, void*))FUN_PRINT_NAME)

#define ZRTEST_BEGIN() \
	sprintf(FUN_PREFIX, "%s:\n", __FUNCTION__)

#define ZRTEST_BEGIN_MSG(MSG) \
	sprintf(FUN_PREFIX, "%s:\n%s\n", __FUNCTION__, MSG)

#define ZRTEST_END(msg_buffer, result, expected) \
	ZRTEST_RESULT(FUN_PREFIX, msg_buffer, result, expected)

// ============================================================================

static void testResult(const char *prefix, char* MESSAGE_BUFF, void *result, void* expected, int (*cmpResult)(void*, void*), void (*print)(char*, void*))
{
	const int cmp = cmpResult(result, expected);

	if (cmp != 0)
	{
		char msg_expected[500] = "";
		char msg_have[500] = "";

		print(msg_expected, expected);
		print(msg_have, result);

		sprintf(MESSAGE_BUFF, "%s (cmp=%d) Expected\n%s but have\n%s\n", prefix, cmp, msg_expected, msg_have);
		fputs(MESSAGE_BUFF, stderr);
		fflush(stderr);
		mu_fail(MESSAGE_BUFF);
	}
}

static void mainTestSetup()
{
	*MESSAGE_BUFF = '\0';
}