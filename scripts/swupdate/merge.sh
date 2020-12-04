#!/bin/sh

#Below function will return value corresponding to requested key
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

#Below method perform merging of config
function performMerge {
while read line; do
if [ "$fileType" = "properties" ]
then
    IFS=$'='
else
    IFS=$':'
fi
read -ra ID <<< "$line"
key="${ID[0]// /}" #remove leading and trailing spaces
value="${ID[1]// /}"
if ! grep -Fx "${key}" $unsafeAttributesFile >/dev/null
then
if [[ $key != \#* ]]; then #Avoiding commented lines or keys
configValue=$(getProperty "$key" "$configFile" "$fileType")
oldSampleValue=$(getProperty "$key" "$oldSampleFile" "$fileType")
newSampleValue=$(getProperty "$key" "$newSampleFile" "$fileType")
#if config and old sample value same or new parameter added then replace config value with new sample value
#else keep config value as it is
if [ "$configValue" = "$oldSampleValue" ] || [ -z "$configValue" ]
then
IFS=$'\n'
if [ "$fileType" = "properties" ]
then
    sed -i "s+$key=$configValue+$key=$newSampleValue+" $configFile
else
    sed -i "s+$key: $configValue+$key: $newSampleValue+" $configFile
fi
fi
fi
fi
done < $newSampleFile
echo "Config merge complete"
}

#Below method will delete temporarily created old files
function performCleanUp {
echo "Removing old sample file backup.."
rm -f /tmp/s3config.yaml.sample.old
rm -f /tmp/config.yaml.sample.old
rm -f /tmp/authserver.properties.sample.old
rm -f /tmp/keystore.properties.sample.old
}

#Script logic starts here
if [ -f /opt/seagate/cortx/s3/conf/s3config.yaml -a -f /tmp/s3config.yaml.sample.old ]; then
configFile=/opt/seagate/cortx/s3/conf/s3config.yaml
oldSampleFile=/tmp/s3config.yaml.sample.old
newSampleFile=/opt/seagate/cortx/s3/conf/s3config.yaml.sample
unsafeAttributesFile=/opt/seagate/cortx/s3/conf/s3config_unsafe_attributes.yaml
fileType=yaml
performMerge
else
    cp /opt/seagate/cortx/s3/conf/s3config.yaml.sample /opt/seagate/cortx/s3/conf/s3config.yaml
fi
if [ -f /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml -a -f /tmp/config.yaml.sample.old ]; then
configFile=/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml
oldSampleFile=/tmp/config.yaml.sample.old
newSampleFile=/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml.sample
unsafeAttributesFile=/opt/seagate/cortx/s3/s3backgrounddelete/s3backgrounddelete_unsafe_attributes.yaml
fileType=yaml
performMerge
else
    cp /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml.sample /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml
fi
if [ -f /opt/seagate/cortx/auth/resources/authserver.properties -a -f /tmp/authserver.properties.sample.old ]; then
configFile=/opt/seagate/cortx/auth/resources/authserver.properties
oldSampleFile=/tmp/authserver.properties.sample.old
newSampleFile=/opt/seagate/cortx/auth/resources/authserver.properties.sample
unsafeAttributesFile=/opt/seagate/cortx/auth/resources/authserver_unsafe_attributes.yaml
fileType=properties
performMerge
else
    cp /opt/seagate/cortx/auth/resources/authserver.properties.sample /opt/seagate/cortx/auth/resources/authserver.properties
fi
if [ -f /opt/seagate/cortx/auth/resources/keystore.properties -a -f /tmp/keystore.properties.sample.old ]; then
configFile=/opt/seagate/cortx/auth/resources/keystore.properties
oldSampleFile=/tmp/keystore.properties.sample.old
newSampleFile=/opt/seagate/cortx/auth/resources/keystore.properties.sample
unsafeAttributesFile=/opt/seagate/cortx/auth/resources/keystore_unsafe_attributes.yaml
fileType=properties
performMerge
else
    cp /opt/seagate/cortx/auth/resources/keystore.properties.sample /opt/seagate/cortx/auth/resources/keystore.properties
fi

performCleanUp
