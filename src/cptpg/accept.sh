#!/bin/bash

source "$2/accept-setup.sh"

for base in subtle bad-whitespace GMT_nighttime
do
    pg="$base.pg"
    fixture="$TESTFIX/cpt/$base.cpt"

    assert_raises "./cptpg -o $pg $fixture" 0
    assert "equal-pg $pg accept/$pg" true

    rm -f $pg
done

source accept-teardown.sh
