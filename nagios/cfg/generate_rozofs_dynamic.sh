#!/bin/bash
GRP_FILE_FS="/tmp/group_FS"
rm -f $GRP_FILE_FS

syntax() {
  echo   "$name [-v] -i <input file> [ -o <output file> ]"
  echo "    -i <input file> is a collection of lines :"
  echo "        VOLUME <VIP@> <volume#> <storage1IP@> <storage2IP@> <storage3IP@> <storage4IP@> ..." 
  echo "        STORAGE <hostname>" 
  echo "        FSMOUNT <hostname> <instance#>"
  exit
}
preambule () {
  echo "#########################################################"
  echo "#"
  echo "#   Hosts "
  echo "# "
  echo "#########################################################"  
}
add_group_fs () {
  FILE=$GRP_FILE_FS
  if [ -s $FILE ];
  then
    GRP=`cat $FILE`
    echo "$GRP,$1" > $FILE
  else
    echo $1 > $FILE
  fi  
}
put_group_fs () {
    
  FILE=$GRP_FILE_FS
  if [ -s $FILE ];
  then
    GROUP=`cat $FILE`
    echo "#"
    echo "# RozoFS mount"
    echo "#"
    echo "define hostgroup{"
    echo "  hostgroup_name              rozofs-mount-group"
    echo "  alias                       RozoFS mount group"
    echo "  members                     $GROUP"
    echo "}"  
    rm -f $FILE    
    echo "define hostextinfo{"
    echo "  hostgroup_name   rozofs-mount-group"
    echo "  notes            RozoFS mount group" 
    echo "  icon_image_alt   RozoFS mount"
    echo "  icon_image       base/rozofs-128.png"
    echo "  vrml_image       rozofs-128.png"
    echo "}"   
  fi  


}
STORAGE() {
  echo "#-------------------------------"
  echo "# RozoFS storage ${param[1]}"
  echo "#-------------------------------"
  echo "define service {"
  echo "  name                rozofs_check_storage_${param[1]}"
  echo "  use                 generic-service"
  echo "  host_name            rozofs_storage_${param[1]}"
  echo "  check_interval      1"
  echo "  service_description RozoFS storage ${param[1]} check"
  echo "  check_command       rozofs_storage_check"
  echo "}"
  echo "define host{"
  echo "  host_name                   rozofs_storage_${param[1]}"
  echo "  use                         generic-host"
  echo "  alias                       RozoFS Storage ${param[1]}"
  echo "  address                     ${param[1]}"
  echo "}"
}
VOLUME() {
  echo "#-------------------------------"
  echo "# RozoFS volume ${param[2]}"
  echo "#-------------------------------"
  echo "define service {"
  echo "  name                rozofs_check_volume_""${param[2]}"
  echo "  use                 generic-service"
  echo "  host_name           rozofs_volume_${param[2]}"
  echo "  check_interval      1"
  echo "  service_description RozoFS volume ${param[2]} check"
  echo "  check_command       rozofs_volume_check!8%!20%  "
  echo "}"
  echo "define host{"
  echo "  host_name                   rozofs_volume_${param[2]}"
  echo "  use                         generic-host"
  echo "  _ROZOFS_VOLUME_NUMBER       ${param[2]}"
  echo "  alias                       RozoFS Volume ${param[2]}"
  echo "  address                     ${param[1]}"
  echo "  use                         generic-host"
  echo "}"
  echo "define hostgroup{"
  echo "  hostgroup_name              rozofs-volume-${param[2]}-group"
  echo "  alias                       RozoFS Volume ${param[2]} group"
  GROUP="rozofs_volume_${param[2]}"
  i=3
  while [ ! -z ${param[$i]} ]
  do
    GROUP=`echo "$GROUP,rozofs_storage_${param[$i]}"`
    i=`expr $i + 1`
  done
  echo "  members                     $GROUP"
  echo "}"  
  echo "define hostextinfo{"
  echo "  hostgroup_name   rozofs-volume-${param[2]}-group"
  echo "  notes            Rozofs Volume ${param[2]}" 
  echo "  icon_image_alt   RozoFS"
  echo "  icon_image       base/rozofs-128.png"
  echo "  vrml_image       rozofs-128.png"
  echo "}"    
}
FSMOUNT() {
  echo "#-------------------------------"
  echo "# RozoFS mount ${param[2]} on ${param[1]}"
  echo "#-------------------------------" 
  echo "define service {"
  echo "  name                rozofs_check_mount_${param[2]}"_"${param[1]}"
  echo "  use                 generic-service"
  echo "  host_name           rozofsmount${param[2]}"_"${param[1]}"
  echo "  check_interval      1"
  echo "  service_description  Rozofsmount ${param[2]} on ${param[1]} check"
  echo "  check_command       rozofs_mount_check"
  echo "}"
  echo "define host{"
  echo "  host_name                   rozofsmount${param[2]}"_"${param[1]}"
  echo "  use                         generic-host"
  echo "  _ROZOFSMOUNT_INSTANCE       ${param[2]}"
  echo "  alias                       Rozofsmount ${param[2]} on ${param[1]}"
  echo "  address                     ${param[1]}"
  echo "}"
  add_group_fs rozofsmount${param[2]}"_"${param[1]}
}

write_all () {
  preambule 
  cat $input | while read -a param
  do
    case "${param[0]}" in
      VOLUME|FSMOUNT|STORAGE) ${param[0]};;
    esac
  done 
  put_group_fs
}

name=`basename $0`
case $1 in
 "") syntax;;
esac 

input=""
output="rozofs_dynamic.cfg"
while [ ! -z $1 ];
do
  case "$1" in
   "-v") set -x; shift 1;;
    -i) input=$2; shift 2;;
    *) syntax;;
  esac
done

if [ -z $input ];
then 
  echo "Missing -i !!! "
  syntax
fi

if [ ! -s $input ];
then
  echo "\"$input\" file do not exist !!!"
  syntax
fi

write_all > $output





