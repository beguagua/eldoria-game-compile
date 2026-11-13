#!/bin/bash
RES="1280x720"
DISPLAY_NUM=":99"
OUTPUT="/home/ubuntu/eldoria_complete_demo.mp4"

killall Xvfb ffmpeg eldoria 2>/dev/null
Xvfb $DISPLAY_NUM -screen 0 ${RES}x24 &
sleep 2

ffmpeg -y -f x11grab -s $RES -i $DISPLAY_NUM.0 -c:v libx264 -preset ultrafast -t 100 $OUTPUT &
FFMPEG_PID=$!

export DISPLAY=$DISPLAY_NUM
cd /home/ubuntu/eldoria/native
./bin/eldoria &
GAME_PID=$!

wait $FFMPEG_PID
kill $GAME_PID
killall Xvfb
echo "Gravação concluída em $OUTPUT"
