#!/bin/sh

mkdir -p failures
dots='............................................................'

####

name="europe-forecast-fi"
printf "%s %s " $name "${dots:${#name}}"
../frontier -w woml/europe-forecast.woml -s tpl/europe-forecast.tpl -p stereographic,5,90,60:-12.24349761,31.83310001,74.66294552,54.86671043:458,-1 -d -t conceptualmodelanalysis -l FI-fi > failures/${name}.svg 2>/dev/null
result=failures/${name}.svg
expected=output/${name}.svg

if [[ -z  "$result" ]]; then
    echo "FAIL - NO OUTPUT"
else
    ./CompareImages.sh $result $expected
fi

#####

name="europe-forecast-sv"
printf "%s %s " $name "${dots:${#name}}"
../frontier -w woml/europe-forecast.woml -s tpl/europe-forecast.tpl -p stereographic,5,90,60:-12.24349761,31.83310001,74.66294552,54.86671043:458,-1 -d -t conceptualmodelanalysis -l SV-se > failures/${name}.svg 2>/dev/null
result=failures/${name}.svg
expected=output/${name}.svg

if [[ -z  "$result" ]]; then
    echo "FAIL - NO OUTPUT"
else
    ./CompareImages.sh $result $expected
fi
