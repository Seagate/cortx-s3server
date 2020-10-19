usage() {
    echo "Usage: $0 -a <IP_address> -b <bucket_name>" 
    echo "-b is an optional arguement" 
}

rm /tmp/m0kvtest.log > /dev/null 2>&1

get_object_metadata() {
    OBJECTS=()
    declare -a MOTR_OIDS
    declare -a OBJECT_VERSION

    NO_OF_OBJECTS=$(aws s3 ls s3://$2 --recursive --summarize | aws s3 ls s3://$2 --recursive --summarize | grep "Total Objects: [0-9]*" | cut -c16-)
    if [ $NO_OF_OBJECTS -ne 0 ]
    then 
        
        rm /tmp/m0kvbucket.log > /dev/null 2>&1
        rm /tmp/m0kvobjver.log > /dev/null 2>&1
        ./m0kv -l $3@tcp:12345:33:905 -h $3@tcp:12345:34:1 -p '<0x7000000000000001:0>' -f '<0x7200000000000000:0>' index next $1 '0' $NO_OF_OBJECTS -s >> /tmp/m0kvbucket.log
        ./m0kv -l $3@tcp:12345:33:905 -h $3@tcp:12345:34:1 -p '<0x7000000000000001:0>' -f '<0x7200000000000000:0>' index next $4 '0' $NO_OF_OBJECTS -s >> /tmp/m0kvobjver.log
        i=0
        while read data
        do 
            OBJECTS[i]=$(echo $data | cut -c6-)
            i=$((i+1))
        done < <(grep -Ei -o 'KEY: .*[\.][a-z]*$' /tmp/m0kvbucket.log)

        i=0
        while read data
        do 
            OBJECT_VERSION[i]=$(echo $data | cut -c6-)
            i=$((i+1))
        done < <( grep -Ei -o '[\/][0-9][0-9]*[0-9]' /tmp/m0kvobjver.log | cut -c2-)

        i=0
        for line in $(grep -Ei -o "\"motr_oid\":[\"0-9a-zA-Z+=-]*" /tmp/m0kvbucket.log)
        do
            MOTR_OIDS[i]=$(./base64_encoder_decoder -d $(echo $line | cut -c13-$((${#line}-1))))
            i=$((i+1))
        done

        echo ""
        echo "Objects metadata in Bucket"
        echo " "
        for j in $(seq 0 $((NO_OF_OBJECTS-1)))
        do
            echo objectname : ${OBJECTS[j]}
            echo motr_oid : ${MOTR_OIDS[j]}
            echo object_version: ${OBJECT_VERSION[j]}
            echo ""
        done
    else 
        echo "NO OBJECTS IN THE BUCKET!!"
    fi
}

get_all_buckets_metadata() {
    declare -a MOTR_MULTIPART_INDEX_OID
    declare -a MOTR_OBJECT_LIST_INDEX_OID
    declare -a MOTR_OBJECTS_VERSION_LIST_INDEX_OID
    declare -a BUCKETS
    NO_OF_BUCKETS=$(aws s3 ls | wc -l)
    ./m0kv -l $1@tcp:12345:33:905 -h $1@tcp:12345:34:1 -p '<0x7000000000000001:0>' -f '<0x7200000000000000:0>' index next '0x7800000000000000:0x100002' '0' $NO_OF_BUCKETS -s >> /tmp/m0kvtest.log
    i=0
    for line in $(aws s3 ls | grep -Ei -o '[^a-z0-9][0-9a-z]*[a-z0-9]$')
    do
        BUCKETS[i]=$line
        i=$((i+1))
    done
    
    i=0
    for line in $(grep -Ei -o "\"motr_multipart_index_oid\":[\"0-9a-zA-Z+=-]*" /tmp/m0kvtest.log)
    do
        MOTR_MULTIPART_INDEX_OID_VAL=$(echo $line | cut -c29-$((${#line}-1)))
        MOTR_MULTIPART_INDEX_OID[i]=$(./base64_encoder_decoder -d $MOTR_MULTIPART_INDEX_OID_VAL)
        i=$((i+1))
    done

    i=0
    for line in $(grep -Ei -o "\"motr_object_list_index_oid\":[\"0-9a-zA-Z+=-]*" /tmp/m0kvtest.log)
    do
        MOTR_OBJECT_LIST_INDEX_OID_VAL=$(echo $line | cut -c31-$((${#line}-1)))
        MOTR_OBJECT_LIST_INDEX_OID[i]=$(./base64_encoder_decoder -d $MOTR_OBJECT_LIST_INDEX_OID_VAL)
        i=$((i+1))
    done

    i=0
    for line in $(grep -Ei -o "\"motr_objects_version_list_index_oid\":[\"0-9a-zA-Z+=-]*" /tmp/m0kvtest.log)
    do
        MOTR_OBJECTS_VERSION_LIST_INDEX_OID_VAL=$(echo $line | cut -c40-$((${#line}-1)))
        MOTR_OBJECTS_VERSION_LIST_INDEX_OID[i]=$(./base64_encoder_decoder -d $MOTR_OBJECTS_VERSION_LIST_INDEX_OID_VAL)
        i=$((i+1))
    done

    echo -e "\nMetadata of all buckets"
    echo -e "------------------------------------------------------------------------\n"
    for j in $(seq 1 ${#BUCKETS[@]})
    do
        echo bucketname : ${BUCKETS[j-1]}
        echo motr_multipart_index_oid : ${MOTR_MULTIPART_INDEX_OID[j-1]}
        echo motr_object_list_index_oid : ${MOTR_OBJECT_LIST_INDEX_OID[j-1]}
        echo motr_objects_version_list_index_oid : ${MOTR_OBJECTS_VERSION_LIST_INDEX_OID[j-1]}
        get_object_metadata ${MOTR_OBJECT_LIST_INDEX_OID[j-1]} ${BUCKETS[j-1]} $1 ${MOTR_OBJECTS_VERSION_LIST_INDEX_OID[j-1]}
        
        echo -e "------------------------------------------------------------------------"
    done
}
echo ${MOTR_OBJECT_LIST_INDEX_OID[*]}

get_metadata_of_bucket() {
    rm /tmp/m0kvtest1.log > /dev/null 2>&1
    NO_OF_BUCKETS=$(aws s3 ls | wc -l)
    ./m0kv -l $2@tcp:12345:33:905 -h $2@tcp:12345:34:1 -p '<0x7000000000000001:0>' -f '<0x7200000000000000:0>' index next '0x7800000000000000:0x100002' '0' $NO_OF_BUCKETS -s >> /tmp/m0kvtest1.log
    BUCKET_GREP=$(grep -Ei -o "test[\"].*" /tmp/m0kvtest1.log)
    MOTR_MULTIPART_INDEX_OID_VAL=$(echo $BUCKET_GREP | grep -Ei -o "\"motr_multipart_index_oid\":[\"0-9a-zA-Z+=-]*")
    MOTR_OBJECT_LIST_INDEX_OID_VAL=$(echo $BUCKET_GREP | grep -Ei -o "\"motr_object_list_index_oid\":[\"0-9a-zA-Z+=-]*")
    MOTR_OBJECTS_VERSION_LIST_INDEX_OID_VAL=$(echo $BUCKET_GREP | grep -Ei -o "\"motr_objects_version_list_index_oid\":[\"0-9a-zA-Z+=-]*")

    MOTR_MULTIPART_INDEX_OID=$(./base64_encoder_decoder -d $(echo $MOTR_MULTIPART_INDEX_OID_VAL | cut -c29-$((${#MOTR_MULTIPART_INDEX_OID_VAL}-1))))
    MOTR_OBJECT_LIST_INDEX_OID=$(./base64_encoder_decoder -d $(echo $MOTR_OBJECT_LIST_INDEX_OID_VAL | cut -c31-$((${#MOTR_OBJECT_LIST_INDEX_OID_VAL}-1))))
    MOTR_OBJECTS_VERSION_LIST_INDEX_OID=$(./base64_encoder_decoder -d $(echo $MOTR_OBJECTS_VERSION_LIST_INDEX_OID_VAL | cut -c40-$((${#MOTR_OBJECTS_VERSION_LIST_INDEX_OID_VAL}-1))))
    echo bucketname : $1
    echo motr_multipart_index_oid : $MOTR_MULTIPART_INDEX_OID
    echo motr_object_list_index_oid : $MOTR_OBJECT_LIST_INDEX_OID
    echo motr_objects_version_list_index_oid : $MOTR_OBJECTS_VERSION_LIST_INDEX_OID
    get_object_metadata $MOTR_OBJECT_LIST_INDEX_OID $1 $2 $MOTR_OBJECTS_VERSION_LIST_INDEX_OID
}

while getopts "a:b:" opt
do
   case "$opt" in
      a ) IP_ADDRESS="$OPTARG" ;;
      b ) BUCKET_NAME="$OPTARG" ;;
      ? ) usage ;;
   esac 
done

if [ -z "$BUCKET_NAME" ] 
then
    echo "By default all buckets are selected!"
    get_all_buckets_metadata $IP_ADDRESS
else
    get_metadata_of_bucket $BUCKET_NAME $IP_ADDRESS
fi   

