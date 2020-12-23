#!/bin/bash
#upgrade on nao
liepa_tts_version=`curl --silent https://api.github.com/repos/liepa-project/liepa2-nao_sintezatorius/releases/latest | grep '"tag_name":'| sed -E 's/.*"([^"]+)".*/\1/'`
echo "Liepa TTS version: $liepa_tts_version "
killall -9 LiepaTTS
rm /home/nao/naoqi/lib/LiepaTTS* -rf
rm /tmp/nao-liepa-tts* -rf
wget https://github.com/liepa-project/liepa2-nao_sintezatorius/releases/download/$liepa_tts_version/nao-liepa-tts.tar.gz -P /tmp
mkdir -p /tmp/nao-liepa-tts
tar xvzf /tmp/nao-liepa-tts.tar.gz -C /tmp/nao-liepa-tts
cp /tmp/nao-liepa-tts/* /home/nao/naoqi/lib -r

