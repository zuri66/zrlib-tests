#include <stdlib.h>
#include <minunit/minunit.h>
#include <stdbool.h>

#include <zrlib/base/Map/HashTable.h>
#include <zrlib/base/Map/VectorMap.h>
#include <zrlib/base/Vector/Vector2SideStrategy.h>
#include <zrlib/base/macro.h>
#include <zrlib/base/Allocator/CAllocator.h>
#include "../main.h"

// ============================================================================

ZRAllocator *ALLOCATOR;

typedef ZRMap* (*fmap_create)(size_t keySize, size_t keyA, size_t objSize, size_t objA);

// ============================================================================

static size_t fhash(void *key, void *map)
{
	return *(size_t*)key;
}

static ZRMap* HashTable_create(size_t keySize, size_t keyA, size_t objSize, size_t objA)
{
	zrfuhash fhash_a[] =
		{ fhash };

	return ZRHashTable_create(ZROBJINFOS_DEF(keyA, keySize), ZROBJINFOS_DEF(objA, objSize), fhash_a, ZRCARRAY_NBOBJ(fhash_a), ALLOCATOR);
}

int charcmp(void *a, void *b, void *data)
{
	return *(size_t*)a - *(size_t*)b;
}

static ZRMap* VectorMap_create(size_t keySize, size_t keyA, size_t objSize, size_t objA)
{
	return ZRVectorMap_create(ZROBJINFOS_DEF(keyA, keySize), ZROBJINFOS_DEF(objA, objSize), charcmp, NULL, ALLOCATOR, ZRVectorMap_modeOrder);
}

static ZRMap* VectorMapEq_create(size_t keySize, size_t keyA, size_t objSize, size_t objA)
{
	return ZRVectorMap_create(ZROBJINFOS_DEF(keyA, keySize), ZROBJINFOS_DEF(objA, objSize), charcmp, NULL, ALLOCATOR, ZRVectorMap_modeEq);
}

static ZRMap* StaticVectorMap_create(size_t keySize, size_t keyA, size_t objSize, size_t objA)
{
	ZRInitInfos_t infos, vectorInfos;

	ZRVectorMapIInfos(&infos, ZROBJINFOS_DEF(keyA, keySize), ZROBJINFOS_DEF(objA, objSize));
	ZRVectorMapIInfos_allocator(&infos, ALLOCATOR);
	ZRVectorMapIInfos_fucmp(&infos, charcmp, ZRVectorMap_modeOrder);

	ZRVector2SideStrategyIInfos(&vectorInfos, ZROBJINFOS_DEF0());
	ZRVector2SideStrategyIInfos_initialArraySize(&vectorInfos, 10000);
	ZRVector2SideStrategyIInfos_allocator(&vectorInfos, ALLOCATOR);
	ZRVector2SideStrategyIInfos_oneSide(&vectorInfos);

	ZRVectorMapIInfos_staticVector(&infos, &vectorInfos,
		ZRVector2SideStrategyIInfos_objInfos,
		ZRVector2SideStrategy_objInfos,
		ZRVector2SideStrategy_init
		);

	return ZRVectorMap_new(&infos);
}

// ============================================================================

#define XLIST \
	X(HashTable) \
	X(VectorMap) \
	X(VectorMapEq)
	X(StaticVectorMap) \

#define X(item) #item, ZRCONCAT(item, _create),
static struct
{
	char *name;
	fmap_create fmap_create;
} TEST_CONFIG[] = { XLIST }, *CONFIG;
#undef X

#define ZRMAPTEST_BEGIN() ZRTEST_PRINTF("config: %d, ", (int)(CONFIG - TEST_CONFIG))

MU_TEST(testGet)
{
	ZRMAPTEST_BEGIN();
	ZRMap *map = CONFIG->fmap_create(ZRTYPE_SIZE_ALIGNMENT(size_t), ZRTYPE_SIZE_ALIGNMENT(int));
	size_t const nb = 15;

	size_t key = 'a';
	int val = 0;

	for (int i = 0; i < nb; i++)
	{
		ZRMap_put(map, &key, &val);
		key++;
		val += 115;
	}
	ZRTEST_ASSERT_INT_EQ(0, *(int* )ZRMap_get(map, &((size_t ) { 'a' } )));
	ZRTEST_ASSERT_INT_EQ(115, *(int* )ZRMap_get(map, &((size_t ) { 'b' } )));
	ZRTEST_ASSERT_INT_EQ(230, *(int* )ZRMap_get(map, &((size_t ) { 'c' } )));

	ZRTEST_ASSERT_INT_EQ(false, ZRMap_delete(map, &((size_t ) { 'Z' } )));

	ZRTEST_ASSERT_INT_EQ(true, ZRMap_delete(map, &((size_t ) { 'd' } )));
	ZRTEST_ASSERT_PTR_EQ(NULL, ZRMap_get(map, &((size_t ) { 'd' } )));

	ZRMap_destroy(map);
}

MU_TEST(testDeleteAll)
{
	ZRMAPTEST_BEGIN();
	ZRMap *map = CONFIG->fmap_create(ZRTYPE_SIZE_ALIGNMENT(size_t), ZRTYPE_SIZE_ALIGNMENT(int));
	size_t const nb = 100;
	int val = 0;

	for (size_t i = 0; i < nb; i++, val++)
		ZRMap_put(map, &i, &val);

	ZRTEST_ASSERT_INT_EQ(nb, ZRMAP_NBOBJ(map));

	ZRTEST_CHECK(NULL != ZRMAP_GET(map, (size_t[] ) { 0 }));
	ZRTEST_CHECK(NULL != ZRMAP_GET(map, (size_t[] ) { 99 }));

	ZRMAP_DELETEALL(map);

	ZRTEST_ASSERT_INT_EQ(0, ZRMAP_NBOBJ(map));
	ZRTEST_CHECK(NULL == ZRMAP_GET(map, (size_t[] ) { 0 }));
	ZRTEST_CHECK(NULL == ZRMAP_GET(map, (size_t[] ) { 99 }));

	ZRMap_destroy(map);
}

MU_TEST(cpyKeyValPtr)
{
	ZRMAPTEST_BEGIN();
	ZRMap *map = CONFIG->fmap_create(ZRTYPE_SIZE_ALIGNMENT(size_t), ZRTYPE_SIZE_ALIGNMENT(int));
	size_t const bufferSize = 13;
	size_t const nb = 500;
	size_t const nbCpyLoop = nb / bufferSize;
	size_t const cpyLoopRest = nb % bufferSize;
	ZRMapKeyVal buffer[bufferSize];
	size_t nbLoop = 0;
	size_t lastNbCpy;

	for (size_t i = 0; i < nb; i++)
		ZRMap_put(map, &i, (int[]) { i });

	for (;;)
	{
		lastNbCpy = ZRMap_cpyKeyValPtr(map, buffer, nbLoop * bufferSize, bufferSize);

		// All must be 0 because of NULL
		for (size_t i = 0; i < lastNbCpy; i++)
		{
			ZRTEST_ASSERT_INT_RANGE(0, nb - 1, *(int* )buffer[i].val);
		}

		if (lastNbCpy < bufferSize)
			break;

		nbLoop++;
	}
	ZRTEST_ASSERT_INT_EQ(nbCpyLoop, nbLoop);

	if (cpyLoopRest)
		ZRTEST_ASSERT_INT_EQ(lastNbCpy, cpyLoopRest);

	ZRMap_destroy(map);
}

MU_TEST(testStress)
{
	ZRMAPTEST_BEGIN();
	ZRMap *map = CONFIG->fmap_create(ZRTYPE_SIZE_ALIGNMENT(size_t), ZRTYPE_SIZE_ALIGNMENT(int));
	size_t const nb = 10000;
	size_t const nb_rest = 15;
	int val = 0;

	for (size_t i = 0; i < nb; i++)
	{
		ZRMap_put(map, &i, &val);
		val++;
	}
	ZRTEST_ASSERT_INT_EQ(nb, ZRMAP_NBOBJ(map));

	for (size_t i = 0; i < nb - nb_rest; i++)
	{
		ZRMap_delete(map, &i);
	}
	ZRTEST_ASSERT_INT_EQ(nb_rest, ZRMAP_NBOBJ(map));
	ZRMap_destroy(map);
}

// ============================================================================
// TESTS
// ============================================================================

MU_TEST_SUITE( AllTests)
{
	MU_RUN_TEST(testGet);
	MU_RUN_TEST(testDeleteAll);
	MU_RUN_TEST(cpyKeyValPtr);
	MU_RUN_TEST(testStress);
}

int MapTests(void)
{
	puts(__FUNCTION__);

	ALLOCATOR = malloc(sizeof(*ALLOCATOR));
	ZRCAllocator_init(ALLOCATOR);

	for (size_t c_i = 0; c_i < ZRCARRAY_NBOBJ(TEST_CONFIG); c_i++)
	{
		CONFIG = &TEST_CONFIG[c_i];
		MU_RUN_SUITE(AllTests);
	}
	MU_REPORT()
				;

	free(ALLOCATOR);
	return minunit_status;
}
