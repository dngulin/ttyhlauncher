#!/bin/sh
set -eu

TARGET='ttyhlauncher_koi7.ts'
COUNT="$(xmlstarlet sel -t -v "count(TS/context/message)" "$TARGET")"

for i in $(seq 1 "$COUNT"); do
    TYPE=$(xmlstarlet sel -T -t -v "(TS/context/message/translation)[$i]/@type" "$TARGET")

    if [ "$TYPE" = 'unfinished' ]; then
        SRC="$(xmlstarlet sel -T -t -v "(TS/context/message/source)[$i]" "$TARGET")"
        TRS="$(printf '%s' "$SRC" | tr '[:upper:]' '[:lower:]' | iconv -f koi-7 -t utf-8)"
        printf "%s = %s\n" "$i" "$TRS"
        xmlstarlet ed -L -u "(TS/context/message/translation)[$i]" -v "$TRS" "$TARGET"
    fi
done
