#!/bin/sh

function getProperty {
   PROP_KEY=$1
   PROPERTY_FILE=$2
   FILE_TYPE=$3
   if [ "$fileType" = "properties" ]
   then
       PROP_VALUE=`cat $PROPERTY_FILE | grep "$PROP_KEY=" | cut -d'=' -f2`
   else
       PROP_VALUE=`cat $PROPERTY_FILE | grep "$PROP_KEY:" | cut -d':' -f2`
   fi
   IFS=$'#'
   read -ra prop_val <<< "$PROP_VALUE"
   echo "${prop_val[0]// /}"
}
configFile=$1
oldSampleFile=$2
newSampleFile=$3
safeAttributesFile=$4
fileType=$5
while read line; do
#[[ "$line" =~ ^#.*$ ]] && continue
#Reading each line
if [ "$fileType" = "properties" ]
then
    IFS=$'='
else
    IFS=$':'
fi
read -ra ID <<< "$line"
key="${ID[0]// /}" #remove leading and trailing spaces
value="${ID[1]// /}"
if grep -Fx "${key}" $safeAttributesFile >/dev/null
then
configValue=$(getProperty "$key" "$configFile" "$fileType")
oldSampleValue=$(getProperty "$key" "$oldSampleFile" "$fileType")
newSampleValue=$(getProperty "$key" "$newSampleFile" "$fileType")
if [ "$configValue" = "$oldSampleValue" ] || [ -z "$configValue" ]
then
IFS=$'\n'
if [ "$fileType" = "properties" ]
then
    sed -i "s/$key=$configValue/$key=$newSampleValue/g" $configFile
else
    sed -i "s/$key: $configValue/$key: $newSampleValue/g" $configFile
fi
fi
fi
done < $newSampleFile
