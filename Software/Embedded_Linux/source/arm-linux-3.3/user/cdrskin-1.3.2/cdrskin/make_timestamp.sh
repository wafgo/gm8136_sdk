#!/bin/sh

# Create version timestamp cdrskin/cdrskin_timestamp.h
# to be executed within  ./libburn-*  resp ./cdrskin-*

timestamp="$(date -u '+%Y.%m.%d.%H%M%S')"
echo "Version timestamp :  $timestamp"
echo '#define Cdrskin_timestamP "'"$timestamp"'"' >cdrskin/cdrskin_timestamp.h

