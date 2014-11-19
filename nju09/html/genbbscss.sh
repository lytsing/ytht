#!/bin/sh
p=bbsproto.css
cmd=./replaceStrings
for i in lg sg lm sm l99 s99; do
	cat $p | $cmd bbs$i.css.def > bbs$i.css
done
