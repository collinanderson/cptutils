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
  qgs_t *qgs = qgs_new(QGS_TYPE_DISCRETE, 2);

  CU_ASSERT_PTR_NOT_NULL_FATAL(qgs);

  {
    rgb_t rgb = {
      .red   = 0,
      .green = 0,
      .blue  = 0
    };
    
    qgs_entry_t entry = {
      .rgb = rgb,
      .opacity = 255, 
      .value = 0.0,
    };
    CU_ASSERT_EQUAL_FATAL(qgs_set_entry(qgs, 0, &entry), 0);
  }

  {
    rgb_t rgb = {
      .red   = 255,
      .green = 255,
      .blue  = 255,
    };
    
    qgs_entry_t entry = {
      .rgb = rgb,
      .opacity = 255, 
      .value = 1.0,
    };
    CU_ASSERT_EQUAL_FATAL(qgs_set_entry(qgs, 1, &entry), 0);
  }

  char *path = tmpnam(NULL);

  CU_ASSERT_PTR_NOT_NULL_FATAL(path);

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
