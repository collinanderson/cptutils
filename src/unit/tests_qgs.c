/*
  tests_qgs.h
  Copyright (c) J.J. Green 2014
*/

#include <qgs.h>
#include "tests_qgs.h"

CU_TestInfo tests_qgs[] =
  {
    {"create", test_qgs_create },
    {"set type", test_qgs_set_type },
    {"set name", test_qgs_set_name },
    {"alloc entries",  test_qgs_alloc_entries },
    {"set entry", test_qgs_set_entry },
    CU_TEST_INFO_NULL,
  };

extern void test_qgs_create(void)
{
  qgs_t *qgs = qgs_new();
  CU_ASSERT_PTR_NOT_NULL_FATAL(qgs);
  qgs_destroy(qgs);
}

extern void test_qgs_set_type(void)
{
  qgs_t *qgs;
  CU_TEST_FATAL((qgs = qgs_new()) != NULL);
  CU_ASSERT_EQUAL(qgs_set_type(qgs, QGS_TYPE_DISCRETE), 0);
  CU_ASSERT_EQUAL(qgs->type, QGS_TYPE_DISCRETE);
  qgs_destroy(qgs);
}

extern void test_qgs_set_name(void)
{
  qgs_t *qgs;
  CU_TEST_FATAL((qgs = qgs_new()) != NULL);
  CU_ASSERT_EQUAL_FATAL(qgs_set_name(qgs, "froob"), 0);
  CU_ASSERT_STRING_EQUAL(qgs->name, "froob");
  qgs_destroy(qgs);
}

extern void test_qgs_alloc_entries(void)
{
  qgs_t *qgs;
  CU_TEST_FATAL((qgs = qgs_new()) != NULL);
  CU_ASSERT_EQUAL_FATAL(qgs_alloc_entries(qgs, 3), 0);
  CU_ASSERT_EQUAL(qgs->n, 3);
  CU_ASSERT_NOT_EQUAL(qgs->entries, NULL);
  qgs_destroy(qgs);
}

extern void test_qgs_set_entry(void)
{
  qgs_t *qgs;
  CU_TEST_FATAL((qgs = qgs_new()) != NULL);
  CU_TEST_FATAL(qgs_alloc_entries(qgs, 3) == 0);

  rgb_t rgb = { 3, 4, 5 };
  qgs_entry_t entry = {
    .rgb = rgb,
    .opacity = 255,
    .value = 2.4,
  };

  /* check out of range */

  CU_ASSERT_NOT_EQUAL(qgs_set_entry(qgs, 6, &entry), 0);

  /* check in range */

  CU_ASSERT_EQUAL_FATAL(qgs_set_entry(qgs, 1, &entry), 0);
  CU_ASSERT_EQUAL(qgs->entries[1].rgb.red, entry.rgb.red);
  CU_ASSERT_EQUAL(qgs->entries[1].rgb.green, entry.rgb.green);
  CU_ASSERT_EQUAL(qgs->entries[1].rgb.blue, entry.rgb.blue);
  CU_ASSERT_EQUAL(qgs->entries[1].opacity, entry.opacity);
  CU_ASSERT_DOUBLE_EQUAL(qgs->entries[1].value, entry.value, 1e-16);

  qgs_destroy(qgs);
}
