#! /bin/bash

# MUON COINCIDENCE COUNTER START SCRIPT

MUON_HOME="/home/$(whoami)/downloadedSources/muon_timer/muon_timer/coincidences/"
COUNTER_PATH="coincidence_counter.py"
CONFIG_PATH="coincidence_counter.conf"
USER=$(whoami)
EXEC_PATH=$(which python3)


read_config()
{
    while read -r line; do
        if [[ "$line" != "#"* ]]; then
            if [[ "$LINE" == *"#"* ]]; then
                initialSplit=($(echo $line | tr "#" "$IFS"))
                line=${initialSplit[0]}
            fi
            pieces=($(echo $line | tr "=" "$IFS"))
            readonly muon_${pieces[0]}=${pieces[1]}
        fi
    done < $1
}

echo "Starting coincidence counter..."
read_config "$MUON_HOME$CONFIG_PATH"
# set default values for anything not included
if [ -z "$muon_urlTop" ]; then
    muon_urlTop="muon4.hepnet:8090"
fi
if [ -z "$muon_urlBottom" ]; then
    muon_urlBottom="muon3.hepnet:8090"
fi
if [ -z "$muon_dataHome" ]; then
    muon_dataHome="/home/$USER/muon_timer_data"
fi
if [ -z "$muon_logfile" ]; then
    muon_logfile="/var/log/muon_timer/coincidence_counter.log"
fi
# make sure muon_dataHome exists
if [ ! -d $muon_dataHome ]; then
    mkdir $muon_dataHome
fi

$EXEC_PATH $MUON_HOME$COUNTER_PATH $muon_urlTop $muon_urlBottom $muon_dataHome $muon_logfile