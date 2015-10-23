/*
  tests_qmlwrite.h
  Copyright (c) J.J. Green 2014
*/

#include <unistd.h>
#include <stdio.h>

#include <qmlwrite.h>
#include "tests_qmlwrite.h"

CU_TestInfo tests_qmlwrite[] =
  {
    {"minimal", test_qmlwrite_minimal},
    CU_TEST_INFO_NULL,
  };

extern void test_qmlwrite_minimal(void)
{
  qml_t *qml = qml_new(QML_TYPE_DISCRETE, 2);

  CU_ASSERT_PTR_NOT_NULL_FATAL(qml);

  {
    qml_entry_t entry = {
      .red   = 0,
      .green = 0,
      .blue  = 0,
      .value = 0.0,
      .label = "start"
    };
    CU_ASSERT_EQUAL_FATAL(qml_set_entry(qml, 0, &entry), 0);
  }

  {
    qml_entry_t entry = {
      .red   = 255,
      .green = 255,
      .blue  = 255,
      .value = 1.0,
      .label = "end"
    };
    CU_ASSERT_EQUAL_FATAL(qml_set_entry(qml, 1, &entry), 0);
  }

  char *path = tmpnam(NULL);

  CU_ASSERT_PTR_NOT_NULL_FATAL(path);

  CU_ASSERT_EQUAL(qml_write(path, qml), 0);
  CU_ASSERT_EQUAL(access(path, F_OK), 0);

  /*
    libxml allows one to validate a document against a DTD,
    so we could do that here, but it would be rather involved
    so for the present we leave this to acceptance tests,
    which are easy
  */

  unlink(path);
  qml_destroy(qml);
}
