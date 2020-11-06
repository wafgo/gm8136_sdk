#!/bin/sh
#
# Spying on the call to cdrecord.
#
# Move $(which cdrecord) to $(dirname $(which cdrecord))/real_cdrecord . 
# Install this sript instead. (Do not forget to revoke this after the test.)
#

# The report target is set in variable rt.
# The default is this file :
rt=/tmp/cdrecord_spy_log

# To use a bystanding xterm as target i find out the pty address by
# executing in that terminal
#   sleep 12345
# and then running in another terminal
#   ps -ef | grep 'sleep 12345'
# which answers something like
#   thomas   21303 30518  0 14:02 pts/23   00:00:00 sleep 12345
#   thomas   21421 30523  0 14:02 pts/24   00:00:00 grep sleep 12345
# from which i learn that pts/23 is sleeping 12345. Now sleep can be aborted.
#
# rt=/dev/pts/23

echo '------------------------------------- cdrecord_spy 0.1.0 -------' >>"$rt"
date >>"$rt"
echo '----------------------------------------------------------------' >>"$rt"
echo "$0" >>"$rt"
for i in "$@"
do
  echo "$i" >>"$rt"
done
echo '------------------------------------- cdrecord_spy 0.1.0 - end -' >>"$rt"

real_cdrecord "$@"


