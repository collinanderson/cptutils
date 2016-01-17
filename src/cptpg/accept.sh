#!/bin/bash

source "$2/accept-setup.sh"

for base in subtle bad-whitespace GMT_nighttime
do
    fixture="$TESTFIX/cpt/$base.cpt"

    pg="$base.pg"
    assert_raises "./cptpg -o $pg $fixture" 0
    assert "equal-pg $pg accept/$pg" true
    rm -f $pg

    pg="$base-percent.pg"
    assert_raises "./cptpg -p -o $pg $fixture" 0
    assert "equal-pg $pg accept/$pg" true
    rm -f $pg
done

source accept-teardown.sh
