#!/bin/sh
set -eu

TARGET='ttyhlauncher_koi7.ts'
COUNT="$(xmlstarlet sel -t -v "count(TS/context/message)" "$TARGET")"

echo "$COUNT"

for i in $(seq 1 "$COUNT"); do
    TYPE="$(xmlstarlet sel -T -t -v "(TS/context/message/translation)[$i]/@type" "$TARGET" || true)"

    if [ "$TYPE" = 'unfinished' ]; then
        SRC="$(xmlstarlet sel -T -t -v "(TS/context/message/source)[$i]" "$TARGET")"
        TRS="$(printf '%s' "$SRC" | tr '[:upper:]' '[:lower:]' | iconv -f koi-7 -t utf-8)"
        printf "%s = %s\n" "$i" "$TRS"
        
        NUMERUS="$(xmlstarlet sel -T -t -v "(TS/context/message)[$i]/@numerus" "$TARGET" || true)"
        if [ "$NUMERUS" = 'yes' ]; then
            NTRS=$(sed 's/%Н/%n/g' <<< "$TRS")
            xmlstarlet ed -L -u "((TS/context/message/translation)[$i]/numerusform)[1]" -v "$NTRS" "$TARGET"
            xmlstarlet ed -L -u "((TS/context/message/translation)[$i]/numerusform)[2]" -v "$NTRS" "$TARGET"
            xmlstarlet ed -L -u "((TS/context/message/translation)[$i]/numerusform)[3]" -v "$NTRS" "$TARGET"
        else
            xmlstarlet ed -L -u "(TS/context/message/translation)[$i]" -v "$TRS" "$TARGET"
        fi
    fi
done
