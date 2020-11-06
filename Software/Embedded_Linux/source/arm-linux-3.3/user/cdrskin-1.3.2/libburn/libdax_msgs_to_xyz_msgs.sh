#!/bin/sh

# libdax_msgs_to_iso_msgs.sh
# generates ${xyz}_msgs.[ch] from libdax_msgs.[ch]
# To be executed within  ./libburn-*  resp ./cdrskin-*

# The module name for the generated sourcecode in several
# uppercase-lowercase forms
xyz="libiso"
Xyz="Libiso"
XYZ="LIBISO"

# The project name for which the generated code shall serve
project="libisofs"


for i in libburn/libdax_msgs.[ch]
do
  target_adr=$(echo "$i" | sed -e "s/libdax_/${xyz}_/")

  echo "$target_adr"

  sed \
    -e "s/^\/\* libdax_msgs/\/* ${xyz}_msgs   (generated from XYZ_msgs : $(date))/" \
    -e "s/Message handling facility of libdax/Message handling facility of ${project}/" \
    -e "s/libdax_/${xyz}_/g" \
    -e "s/libdax:/${xyz}:/g" \
    -e "s/Libdax_/${Xyz}_/g" \
    -e "s/LIBDAX_/${XYZ}_/g" \
    -e "s/generated from XYZ_msgs/generated from libdax_msgs/" \
    -e "s/${xyz}_msgs is designed to serve in libraries/libdax_msgs is designed to serve in libraries/" \
    -e "s/Owner of ${xyz}_msgs is libburn/Owner of libdax_msgs is libburn/" \
  \
  <"$i" >"$target_adr"

done

