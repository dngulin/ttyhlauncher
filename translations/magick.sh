#!/bin/bash

count=`xmlstarlet sel -t -v "count(TS/context/message)" koi7.ts`

for i in `seq 1 $count`;
do
    type=`xmlstarlet sel -T -t -v "(TS/context/message/translation)[$i]/@type" koi7.ts`

    if [ "$type" == "unfinished" ]
    then
        src=`xmlstarlet sel -T -t -v "(TS/context/message/source)[$i]" koi7.ts`
        trs=`echo $src | tr '[:upper:]' '[:lower:]' | iconv -f koi-7 -t utf-8`
        echo "$i = $trs"
        xmlstarlet ed -L -u "(TS/context/message/translation)[$i]" -v "$trs" koi7.ts
    fi
done
