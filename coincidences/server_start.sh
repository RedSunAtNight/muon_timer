#! /bin/bash

# MUON EVENT SERVER START/STOP SCRIPT

MUON_HOME="/home/$(whoami)/muon_timer/coincidences/"
SERVER_PATH="event_server.py"
CONFIG_PATH="server.conf"
PID_PATH="/opt/muon_timer/server.pid"
USER=$(whoami)
EXEC_PATH=$(which python)


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

# the actual things to do

case "$1" in
    start)
        echo "Starting event_server.py..."
        set +e
        if [ -f "$PID_PATH" ]; then
            echo "(already running)"
        else
            read_config "$MUON_HOME$CONFIG_PATH"
            # set defaults if necessary
            if [ -z "$muon_port" ]; then
                muon_port="8090"
            fi
            if [ -z "$muon_logfile" ]; then
                muon_logfile="/var/log/muon_timer/server.log"
            fi
            start-stop-daemon --background --make-pidfile --pidfile "$PID_PATH" --exec "$EXEC_PATH" --start -- "$MUON_HOME$SERVER_PATH" "$muon_port" "$muon_logfile"
            # make sure it actually started
            PID="`cat $PID_PATH`"
            #the following line would TOTALLY work in a newer version of start-stop-daemon :(
            #start-stop-daemon --pid "$PID" --status
            sleep 1
            foundPID=$(ps -o pid= -p $PID)
            if [[ $foundPID -ne $PID ]]; then
                echo "(Failed)"
                rm -f "$PID_PATH"
                exit 1
            else
                echo "OK"
            fi
        fi
        set -e
        ;;
    stop)
        echo "Stopping event_server.py..."

        set +e
        if [ -f "$PID_PATH" ]; then
                # stop the app
                start-stop-daemon --stop --pidfile "$PID_PATH" --retry=TERM/20/KILL/5 >/dev/null 2>&1
                stop_result=$?
                if [ $stop_result -eq 1 ]; then
                        echo "event_server.py is not running but pid file exists, cleaning up"
                        stop_result=0
                elif [ $stop_result -eq 3 ]; then
                        PID="`cat $PID_PATH`"
                        echo "Failed to stop event_server.py (pid $PID)"
                        exit $stop_result
                elif [ $stop_result -ne 0 ]; then
                        echo "Unexpected failure"
                        exit $stop_result
                fi
                rm -f "$PID_PATH"
                echo "OK"
        else
                echo "(not running)"
        fi
        set -e
        ;;
    restart)
        if [ -f "$PID_PATH" ]; then
                $0 stop
                sleep 1
        fi
        $0 start
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
        ;;
esac
exit 0