SBC_SMALLC99 regression tests
=============================

LANGTEST_LANGUAGE.C
  Broad implemented-language regression for SBC_SMALLC99/TMS9900.
  It intentionally tests language semantics without parser/stack torture.

  Current expected result:
      tests: 133  passed: 133  failed: 0
      ALL LANGUAGE TESTS PASSED

  Relevant unsigned arithmetic regressions:
      311  unsigned 0xffff >> 1
      312  unsigned 0xffff / 2
      313  unsigned 0xffff % 2
      315  unsigned 0x8000 >> 1
      316-321 compound/variable/zero-count unsigned arithmetic checks

The tests directory is the canonical home for language regression sources.
Legacy root/build copies may remain for compatibility with existing workflows,
but new or updated regression tests should be maintained here first.
