red=`tput setaf 1`
green=`tput setaf 2`
reset=`tput sgr0`
rm base64_encoder_decoder -f /dev/null 2>&1
g++ base64_encoder_decoder.cc -o base64_encoder_decoder -std=c++11 > /dev/null 2>&1

m0kv_PATH="third_party/motr/utils/m0kv"

usage() {
    echo "${red}Usage:${green} $0 -l 10.230.247.176@tcp:12345:33:905 -h 10.230.247.176@tcp:12345:34:1 -p '<0x7000000000000001:0>' -f '<0x7200000000000000:0>' -b 0 -d 10${reset}" 
    echo "-----------------------------------------------------------------------------------"
    echo "${green}-b is flag and required arguement (can be set 0 or 1) ${reset}"
    echo "${green}-b 0 -> Get metadata of bucket and objects${reset}"
    echo "${green}-b 1 -> Get only bucket level metadata ${reset}"
    echo "-----------------------------------------------------------------------------------"
    echo "${green}-d is number of entries for probable delete index (should be integer val)${reset}"
    echo "${green}-d is required arguement${reset}"
    echo "-----------------------------------------------------------------------------------"
    echo "${green}-n is Optional arguement to get metadata specific to bucket${reset}"
}

binary_error() {
    echo "${red}Binary Error : Please run build_motr.sh to generate m0kv file${reset}"
}

get_probable_delete_index() {
    cd ..
    m0kv_flag=$(find third_party/motr/utils/ -name m0kv)
    cd m0kv_metadata_parsing_tool
    if [[ ! -z "$m0kv_flag" ]]
    then
        declare -a KEYS
        declare -a OBJECTS
        declare -a OBJECT_LIST_INDEX_OID
        declare -a OBJECT_VERSION_LIST_INDEX_OID
        declare -a PART_LIST_INDEX_OID
        rm /tmp/m0kvprobable.log > /dev/null 2>&1
        cd ..
        ./$m0kv_PATH -l $1 -h $2 -p $3 -f $4 index next '0x7800000000000000:0x100003' '0' $5 -s >> /tmp/m0kvprobable.log
        echo "----------------------------------------------------------------------------------------------------------------" >> /var/log/m0kv_metadata.log
        cd m0kv_metadata_parsing_tool
        echo -e "./$m0kv_PATH -l $1 -h $2 -p $3 -f $4 index next '0x7800000000000000:0x100003' '0' $5 -s\n" >> /var/log/m0kv_metadata.log
        cat /tmp/m0kvprobable.log >> /var/log/m0kv_metadata.log

        
        i=0
        for line in $(grep "KEY: [\"0-9a-zA-Z+=-]*" /tmp/m0kvprobable.log | cut -c7-)
        do
            KEYS[i]=$(./base64_encoder_decoder -d $(echo $line))
            i=$((i+1))
        done

        i=0
        for line in $(grep -Ei -o "\"object_key_in_index\":[\"0-9a-zA-Z\"]*(.[a-zA-Z0-9]*)?" /tmp/m0kvprobable.log | cut -c24-)
        do
            OBJECTS[i]=$line
            i=$((i+1))
        done

        i=0
        for line in $(grep -Ei -o "\"object_list_index_oid\":\"[0-9a-zA-Z=+-]*" /tmp/m0kvprobable.log | cut -c26-)
        do
            OBJECT_LIST_INDEX_OID[i]=$(./base64_encoder_decoder -d $(echo $line))
            i=$((i+1))
        done

        i=0
        for line in $(grep -Ei -o "\"objects_version_list_index_oid\":\"[0-9a-zA-Z=+-]*" /tmp/m0kvprobable.log | cut -c35-)
        do
            OBJECT_VERSION_LIST_INDEX_OID[i]=$(./base64_encoder_decoder -d $(echo $line))
            i=$((i+1))
        done

        i=0
        for line in $(grep -Ei -o "\"part_list_idx_oid\":\"[0-9a-zA-Z=+-]*" /tmp/m0kvprobable.log | cut -c22-)
        do
            PART_LIST_INDEX_OID[i]=$(./base64_encoder_decoder -d $(echo $line))
            i=$((i+1))
        done

        echo -e "\n${green}##########################################################################################${reset}"
        echo "Probable Delete List Index"
        echo -e "${green}##########################################################################################${reset}\n"
        for j in $(seq 0 ${#KEYS})
        do
            if [[ ! -z ${KEYS[j]} ]]
            then
                echo key : ${KEYS[j]}
                echo objectname : ${OBJECTS[j]}
                echo object_list_index_oid : ${OBJECT_LIST_INDEX_OID[j]}
                echo object_version_list_index_oid : ${OBJECT_VERSION_LIST_INDEX_OID[j]}
                echo part_list_idx_oid : ${PART_LIST_INDEX_OID[j]}
                echo ""
            else
                break
            fi
        done
        rm /tmp/m0kvprobable.log > /dev/null 2>&1
        echo -e "${green}##########################################################################################${reset}"
    else
        binary_error
    fi
}

get_parts() {
    rm /tmp/m0kvcountpart.log > /dev/null 2>&1
    aws s3api list-parts --bucket $1 --key $2 --upload-id $3 >> /tmp/m0kvcountpart.log
    NO_OF_PARTS=$(grep -wc "PARTS" /tmp/m0kvcountpart.log)
    if [ $NO_OF_PARTS -ne 0 ]
    then
        cd ..
        ./$m0kv_PATH -l $4 -h $5 -p $6 -f $7 index next $8 '0' $NO_OF_PARTS -s >> /tmp/m0kvpart.log
        echo "----------------------------------------------------------------------------------------------------------------" >> /var/log/m0kv_metadata.log
        cd m0kv_metadata_parsing_tool
        echo -e "./$m0kv_PATH -l $4 -h $5 -p $6 -f $7 index next $8 '0' $NO_OF_PARTS -s\n" >> /var/log/m0kv_metadata.log
        cat /tmp/m0kvpart.log >> /var/log/m0kv_metadata.log
        echo "Number of parts : $NO_OF_PARTS"
    else
        echo -e "\n${red}NO PARTS CREATED FOR THIS OBJECT !!${reset}"
    fi
    
}


get_multipart_metadata() {
    declare -a MOTR_PART_OID
    MULTIPART_OBJECTS=()
    
    aws s3api list-multipart-uploads --bucket $2 >> /tmp/countObjects.log
    NO_OF_MULTIPART_OBJECTS=$(grep -wc "INITIATOR" /tmp/countObjects.log)
    rm /tmp/countObjects.log > /dev/null 2>&1
    if [ $NO_OF_MULTIPART_OBJECTS -ne 0 ]
    then 
        rm /tmp/m0kvmultipart.log > /dev/null 2>&1
        declare -a MOTR_PART_OID
        declare -a MOTR_OID
        declare -a UPLOAD_ID
        cd ..
        ./$m0kv_PATH -l $3 -h $4 -p $5 -f $6 index next $1 '0' $NO_OF_MULTIPART_OBJECTS -s >> /tmp/m0kvmultipart.log
        echo "----------------------------------------------------------------------------------------------------------------" >> /var/log/m0kv_metadata.log
        cd m0kv_metadata_parsing_tool
        echo -e "./$m0kv_PATH -l $3 -h $4 -p $5 -f $6 index next $1 '0' $NO_OF_MULTIPART_OBJECTS -s\n" >> /var/log/m0kv_metadata.log
        cat /tmp/m0kvmultipart.log >> /var/log/m0kv_metadata.log
        i=0
        while read data
        do
            MULTIPART_OBJECTS[i]=$(echo $data | cut -c6-)
            i=$((i+1))
        done < <(grep -Ei -o 'KEY: .*([\.])?[a-z0-9]*$' /tmp/m0kvmultipart.log)

        for (( i=0; i<$NO_OF_MULTIPART_OBJECTS; i++ ))
        do
            UPLOAD_ID[i]=$(aws s3api list-multipart-uploads --bucket $2 | grep -o "${MULTIPART_OBJECTS[i]}\s.*" | tr -d " \t\n\r" | cut -c$((${#MULTIPART_OBJECTS[i]}+1))-)
        done
        
        i=0
        for line in $(grep -Ei -o "\"motr_part_oid\":[\"0-9a-zA-Z+=-]*" /tmp/m0kvmultipart.log)
        do
            MOTR_PART_OID[i]=$(./base64_encoder_decoder -d $(echo $line | cut -c18-$((${#line}-1))))
            i=$((i+1))
        done

        i=0
        for line in $(grep -Ei -o "\"motr_oid\":[\"0-9a-zA-Z+=-]*" /tmp/m0kvmultipart.log)
        do
            MOTR_OID[i]=$(./base64_encoder_decoder -d $(echo $line | cut -c13-$((${#line}-1))))
            i=$((i+1))
        done


        
        echo -e "\n${green}#######################-------------------------------------##############################${reset}"
        echo "Multipart Objects metadata in Bucket"
        echo -e "${green}#######################-------------------------------------##############################${reset}\n"
        for j in $(seq 0 $((NO_OF_MULTIPART_OBJECTS-1)))
        do
            echo objectname : ${MULTIPART_OBJECTS[j]}
            echo motr_part_oid : ${MOTR_PART_OID[j]}
            echo motr_oid : ${MOTR_OID[j]}
            echo upload_id : ${UPLOAD_ID[j]}
            get_parts $2 ${MULTIPART_OBJECTS[j]} ${UPLOAD_ID[j]} $3 $4 $5 $6 ${MOTR_PART_OID[j]}
            echo ""
        done
        rm /tmp/m0kvmultipart.log > /dev/null 2>&1
    else
        echo -e "\n${red}NO MULTIPART OBJECTS IN THE BUCKET !!\n${reset}"
    fi
    rm /tmp/countObjects.log > /dev/null 2>&1
}


get_object_metadata() {
    OBJECTS=()
    declare -a MOTR_OIDS
    declare -a OBJECT_VERSION

    NO_OF_OBJECTS=$(aws s3 ls s3://$2 --recursive --summarize | aws s3 ls s3://$2 --recursive --summarize | grep "Total Objects: [0-9]*" | cut -c16-)
    if [ $NO_OF_OBJECTS -ne 0 ]
    then
        rm /tmp/m0kvbucket.log > /dev/null 2>&1
        rm /tmp/m0kvobjver.log > /dev/null 2>&1 
        cd ..
        ./$m0kv_PATH -l $3 -h $5 -p $6 -f $7 index next $1 '0' $NO_OF_OBJECTS -s >> /tmp/m0kvbucket.log
        cd m0kv_metadata_parsing_tool
        echo "----------------------------------------------------------------------------------------------------------------" >> /var/log/m0kv_metadata.log
        echo -e "./$m0kv_PATH -l $3 -h $5 -p $6 -f $7 index next $1 '0' $NO_OF_OBJECTS -s\n" >> /var/log/m0kv_metadata.log
        cat /tmp/m0kvbucket.log >> /var/log/m0kv_metadata.log
        cd ..
        ./$m0kv_PATH -l $3 -h $5 -p $6 -f $7 index next $4 '0' $NO_OF_OBJECTS -s >> /tmp/m0kvobjver.log
        cd m0kv_metadata_parsing_tool
        echo "----------------------------------------------------------------------------------------------------------------" >> /var/log/m0kv_metadata.log
        echo -e "./$m0kv_PATH -l $3 -h $5 -p $6 -f $7 index next $4 '0' $NO_OF_OBJECTS -s\n" >> /var/log/m0kv_metadata.log
        cat /tmp/m0kvobjver.log >> /var/log/m0kv_metadata.log
        i=0
        while read data
        do 
            OBJECTS[i]=$(echo $data | cut -c6-)
            i=$((i+1))
        done < <(grep -Ei -o 'KEY: .*([\.])?[a-z0-9]*$' /tmp/m0kvbucket.log)

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

        echo -e "${green}#######################-------------------------------------##############################${reset}"
        echo "Objects metadata in Bucket"
        echo -e "${green}#######################-------------------------------------##############################${reset}\n"
        for j in $(seq 0 $((NO_OF_OBJECTS-1)))
        do
            echo objectname : ${OBJECTS[j]}
            echo motr_oid : ${MOTR_OIDS[j]}
            echo object_version: ${OBJECT_VERSION[j]}
            echo ""
        done
        rm /tmp/m0kvbucket.log > /dev/null 2>&1
        rm /tmp/m0kvobjver.log > /dev/null 2>&1
    else 
        echo -e "\n${red}NO OBJECTS IN THE BUCKET!!\n${reset}"
    fi
}

get_all_buckets_metadata() {
    cd ..
    m0kv_flag=$(find third_party/motr/utils/ -name m0kv)
    cd m0kv_metadata_parsing_tool
    if [[ ! -z "$m0kv_flag" ]]
    then
        rm /var/log/m0kv_metadata.log > /dev/null 2>&1
        rm /tmp/m0kvtest.log > /dev/null 2>&1
        rm *.[0-9]* -f > /dev/null 2>&1 
        declare -a MOTR_MULTIPART_INDEX_OID
        declare -a MOTR_OBJECT_LIST_INDEX_OID
        declare -a MOTR_OBJECTS_VERSION_LIST_INDEX_OID
        declare -a BUCKETS
        NO_OF_BUCKETS=$(aws s3 ls | wc -l)
        cd ..
        ./$m0kv_PATH -l $1 -h $2 -p $3 -f $4 index next '0x7800000000000000:0x100002' '0' $NO_OF_BUCKETS -s >> /tmp/m0kvtest.log
        cd m0kv_metadata_parsing_tool
        echo "----------------------------------------------------------------------------------------------------------------" >> /var/log/m0kv_metadata.log
        echo -e "./$m0kv_PATH -l $1 -h $2 -p $3 -f $4 index next '0x7800000000000000:0x100002' '0' $NO_OF_BUCKETS -s\n" >> /var/log/m0kv_metadata.log
        cat /tmp/m0kvtest.log >> /var/log/m0kv_metadata.log
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

        echo -e "\nMetadata of all buckets\n"
        green=`tput setaf 2`
        echo -e "${green}##########################################################################################${reset}\n"
        for j in $(seq 1 ${#BUCKETS[@]})
        do
            echo bucketname : ${BUCKETS[j-1]}
            echo motr_multipart_index_oid : ${MOTR_MULTIPART_INDEX_OID[j-1]}
            echo motr_object_list_index_oid : ${MOTR_OBJECT_LIST_INDEX_OID[j-1]}
            echo motr_objects_version_list_index_oid : ${MOTR_OBJECTS_VERSION_LIST_INDEX_OID[j-1]}
            if [ $5 -eq 0 ]
            then
                get_multipart_metadata ${MOTR_MULTIPART_INDEX_OID[j-1]} ${BUCKETS[j-1]} $1 $2 $3 $4
                get_object_metadata ${MOTR_OBJECT_LIST_INDEX_OID[j-1]} ${BUCKETS[j-1]} $1 ${MOTR_OBJECTS_VERSION_LIST_INDEX_OID[j-1]} $2 $3 $4
            else
                echo ""
            fi
            echo -e "${green}##########################################################################################${reset}\n"
        done
        rm /tmp/m0kvtest.log > /dev/null 2>&1
        rm *.[0-9]* -f > /dev/null 2>&1 
    else
        binary_error
    fi
}

get_metadata_of_bucket() {
    cd ..
    m0kv_flag=$(find third_party/motr/utils/ -name m0kv)
    cd m0kv_metadata_parsing_tool
    if [[ ! -z "$m0kv_flag" ]]
    then
        rm /tmp/m0kvtest1.log > /dev/null 2>&1
        rm *.[0-9]* -f > /dev/null 2>&1 
        rm /var/log/m0kv_metadata.log > /dev/null 2>&1
        NO_OF_BUCKETS=$(aws s3 ls | wc -l)
        echo "----------------------------------------------------------------------------------------------------------------" >> /var/log/m0kv_metadata.log
        cd ..
        ./$m0kv_PATH -l $2 -h $3 -p $4 -f $5 index next '0x7800000000000000:0x100002' '0' $NO_OF_BUCKETS -s >> /tmp/m0kvtest1.log
        cd m0kv_metadata_parsing_tool
        echo -e "./$m0kv_PATH -l $2 -h $3 -p $4 -f $5 index next '0x7800000000000000:0x100002' '0' $NO_OF_BUCKETS -s\n" >> /var/log/m0kv_metadata.log
        cat /tmp/m0kvtest1.log >> /var/log/m0kv_metadata.log
        BUCKET_GREP=$(grep -Ei -o "$1[\"].*" /tmp/m0kvtest1.log)
        MOTR_MULTIPART_INDEX_OID_VAL=$(echo $BUCKET_GREP | grep -Ei -o "\"motr_multipart_index_oid\":[\"0-9a-zA-Z+=-]*")
        MOTR_OBJECT_LIST_INDEX_OID_VAL=$(echo $BUCKET_GREP | grep -Ei -o "\"motr_object_list_index_oid\":[\"0-9a-zA-Z+=-]*")
        MOTR_OBJECTS_VERSION_LIST_INDEX_OID_VAL=$(echo $BUCKET_GREP | grep -Ei -o "\"motr_objects_version_list_index_oid\":[\"0-9a-zA-Z+=-]*")

        MOTR_MULTIPART_INDEX_OID=$(./base64_encoder_decoder -d $(echo $MOTR_MULTIPART_INDEX_OID_VAL | cut -c29-$((${#MOTR_MULTIPART_INDEX_OID_VAL}-1))))
        MOTR_OBJECT_LIST_INDEX_OID=$(./base64_encoder_decoder -d $(echo $MOTR_OBJECT_LIST_INDEX_OID_VAL | cut -c31-$((${#MOTR_OBJECT_LIST_INDEX_OID_VAL}-1))))
        MOTR_OBJECTS_VERSION_LIST_INDEX_OID=$(./base64_encoder_decoder -d $(echo $MOTR_OBJECTS_VERSION_LIST_INDEX_OID_VAL | cut -c40-$((${#MOTR_OBJECTS_VERSION_LIST_INDEX_OID_VAL}-1))))
        echo -e "Metadata of $1\n"
        echo -e "${green}##########################################################################################${reset}"
        echo ""
        echo bucketname : $1
        echo motr_multipart_index_oid : $MOTR_MULTIPART_INDEX_OID
        echo motr_object_list_index_oid : $MOTR_OBJECT_LIST_INDEX_OID
        echo motr_objects_version_list_index_oid : $MOTR_OBJECTS_VERSION_LIST_INDEX_OID
        if [ $6 -eq 0 ]
        then
            get_multipart_metadata $MOTR_MULTIPART_INDEX_OID $1 $2 $3 $4 $5
            get_object_metadata $MOTR_OBJECT_LIST_INDEX_OID $1 $2 $MOTR_OBJECTS_VERSION_LIST_INDEX_OID $3 $4 $5
        fi
        echo -e "\n${green}##########################################################################################${reset}"
        rm /tmp/m0kvtest1.log > /dev/null 2>&1
        rm *.[0-9]* -f > /dev/null 2>&1 
    else
        binary_error
        break
    fi
}

while getopts "l:h:p:f:n:b:d:" opt
do
   case "$opt" in
      l ) L="$OPTARG" ;;
      h ) H="$OPTARG" ;;
      p ) P="$OPTARG" ;;
      f ) F="$OPTARG" ;;
      n ) BUCKET_NAME="$OPTARG" ;;
      b ) BUCKET_LEVEL_FLAG="$OPTARG" ;;
      d ) NO_OF_DEL="$OPTARG" ;;
      ? ) usage ;;
   esac 
done

if [[ ! -z "$L" ]] && [[ ! -z "$H" ]] && [[ ! -z "$P" ]] && [[ ! -z "$H" ]] && [[ ! -z "$BUCKET_LEVEL_FLAG" ]] && [[ ! -z "$NO_OF_DEL" ]] && [ -z "$BUCKET_NAME" ]
then
    re='^[0-9]+$'
    if [ $BUCKET_LEVEL_FLAG -le 1 ] && [[ $BUCKET_LEVEL_FLAG =~ $re ]] && [[ $NO_OF_DEL =~ $re ]]
    then
        get_all_buckets_metadata $L $H $P $F $BUCKET_LEVEL_FLAG
        get_probable_delete_index $L $H $P $F $NO_OF_DEL
        rm base64_encoder_decoder -f /dev/null 2>&1
        echo -e "\n${red}Log file :${reset} ${green}/var/log/m0kv_metadata.log${reset}\n"
    else
        usage
        rm base64_encoder_decoder -f /dev/null 2>&1
    fi
elif [[ ! -z "$L" ]] && [[ ! -z "$H" ]] && [[ ! -z "$P" ]] && [[ ! -z "$H" ]] && [[ ! -z "$BUCKET_LEVEL_FLAG" ]] && [[ ! -z "$NO_OF_DEL" ]] && [[ ! -z "$BUCKET_NAME" ]]
then 
    re='^[0-9]+$'
    if [ $BUCKET_LEVEL_FLAG -le 1 ] && [[ $BUCKET_LEVEL_FLAG =~ $re ]] && [[ $NO_OF_DEL =~ $re ]]
    then
        get_metadata_of_bucket $BUCKET_NAME $L $H $P $F $BUCKET_LEVEL_FLAG
        get_probable_delete_index $L $H $P $F $NO_OF_DEL
        rm base64_encoder_decoder -f /dev/null 2>&1
        echo -e "\n${red}Log file :${reset} ${green}/var/log/m0kv_metadata.log${reset}\n"
    else
        usage
        rm base64_encoder_decoder -f /dev/null 2>&1
    fi
else
    usage
    rm base64_encoder_decoder -f /dev/null 2>&1
fi

