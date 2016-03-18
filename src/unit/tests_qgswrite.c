/*
  tests_qgswrite.h
  Copyright (c) J.J. Green 2014
*/

#include <unistd.h>
#include <stdio.h>

#include <qgswrite.h>
#include "tests_qgswrite.h"

CU_TestInfo tests_qgswrite[] =
  {
    {"minimal", test_qgswrite_minimal},
    CU_TEST_INFO_NULL,
  };

extern void test_qgswrite_minimal(void)
{
  qgs_t *qgs;

  CU_TEST_FATAL((qgs = qgs_new()) != NULL);
  CU_TEST_FATAL(qgs_set_type(qgs, QGS_TYPE_INTERPOLATED) == 0);
  CU_TEST_FATAL(qgs_set_name(qgs, "froob") == 0);
  CU_TEST_FATAL(qgs_alloc_entries(qgs, 2) == 0);

  {
    rgb_t rgb = { 0, 0, 0 };
    qgs_entry_t entry = {
      .rgb = rgb,
      .opacity = 255,
      .value = 0.0,
    };
    CU_TEST_FATAL(qgs_set_entry(qgs, 0, &entry) == 0);
  }

  {
    rgb_t rgb = { 255, 255, 255 };
    qgs_entry_t entry = {
      .rgb = rgb,
      .opacity = 255,
      .value = 1.0,
    };
    CU_TEST_FATAL(qgs_set_entry(qgs, 1, &entry) == 0);
  }

  char *path = tmpnam(NULL);

  CU_TEST_FATAL(path != NULL);

  CU_ASSERT_EQUAL(qgs_write(path, qgs), 0);
  CU_ASSERT_EQUAL(access(path, F_OK), 0);

  /*
    libxml allows one to validate a document against a DTD,
    so we could do that here, but it would be rather involved
    so for the present we leave this to acceptance tests,
    which are easy
  */

  unlink(path);
  qgs_destroy(qgs);
}
