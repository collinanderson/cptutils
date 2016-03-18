/*
  tests_qgs.h
  Copyright (c) J.J. Green 2014
*/

#ifndef TESTS_QGS_H
#define TESTS_QGS_H

#include <CUnit/CUnit.h>

extern CU_TestInfo tests_qgs[];

extern void test_qgs_create(void);
extern void test_qgs_set_type(void);
extern void test_qgs_set_name(void);
extern void test_qgs_alloc_entries(void);
extern void test_qgs_set_entry(void);

#endif
