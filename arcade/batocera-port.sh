#!/bin/sh
/userdata/system/backyard-games/BackyardCornhole-linux/run-cornhole.sh > /userdata/system/logs/cornhole-native.log 2>&1
game_status=$?
# Also cover a renderer crash or menu exit while a virtual exit key is held.
# Run before configgen restores the frontend context. Never touch physical keys.
python3 /userdata/system/backyard-games/BackyardCornhole-linux/resources/app/release-exit-keys.py >> /userdata/system/logs/cornhole-native.log 2>&1
exit "$game_status"
