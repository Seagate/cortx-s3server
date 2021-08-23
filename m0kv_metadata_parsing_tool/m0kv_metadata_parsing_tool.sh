#!/bin/bash

#
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com.
#

red=$(tput setaf 1)
green=$(tput setaf 2)
reset=$(tput sgr0)

readonly probable_delete_index='0x7800000000000000:0x100003'
readonly bucket_list_index='0x7800000000000000:0x100002'

rm base64_encoder_decoder -f > /dev/null 2>&1
rm /var/log/m0kv_metadata.log > /dev/null 2>&1
make > /dev/null 2>&1

##### Finding path of m0kv in cortx-s3server #####
m0kv_PATH=$(find / ! -path /proc -name m0kv -type f | head -n1)

##### this function is invoked when any one of the parameters provided are not correct #####
usage() {
    echo "${red}Usage:${green} $0 -l 'local_addr' -h 'ha_addr' -p 'profile' -f 'proc_fid' -d 'depth' -b 'bucket_name'${reset}"
    echo  for eg.
    echo "${green} $0 -l 10.230.247.176@tcp:12345:33:905 -h 10.230.247.176@tcp:12345:34:1 -p '<0x7000000000000001:0>' -f '<0x7200000000000000:0>' -d 1${reset}" 
    echo "-----------------------------------------------------------------------------------"
    echo "${green}-d represents depth of traversal${reset}"
    echo "${green}-d 1  -> Get metadata of only buckets${reset}"
    echo "${green}-d >1 -> Gets metadata of bucket and object${reset}"
    echo "-----------------------------------------------------------------------------------"
    echo "${green}-b <bucket_name> is an optional arguement to get metadata specific to bucket${reset}"
}


##### this function is invoked when there is no binary(m0kv) present in the "cortx-s3server/third_party_motr/utils/" #####
##### the user need to run third_party/build_motr.sh to generate binary(m0kv) first ####
binary_error() {
    echo "${red}Binary Error : Please run build_motr.sh to generate m0kv file${reset}"
}


##### this function lists down the global probable delete indices upto 100 objects #####
get_probable_delete_index() {
    if [[ ! -z "$m0kv_PATH" ]]
    then
        declare -a KEYS
        declare -a OBJECTS
        declare -a OBJECT_LIST_INDEX_OID
        declare -a OBJECT_VERSION_LIST_INDEX_OID
        declare -a PART_LIST_INDEX_OID
        rm /tmp/m0kvprobable.log > /dev/null 2>&1
        "$m0kv_PATH" -l "$1" -h "$2" -p "$3" -f "$4" index next "$probable_delete_index" '0' 1000 -s >> /tmp/m0kvprobable.log
        ##### m0kv command output #####
        # operation rc: 0
        # [0]:
        #         KEY: vgpIBgAAAAA=-RwAAAAAA8Jk=
        #         VAL: {"create_timestamp":"2020-11-10T04:51:30.000Z","force_delete":"true","global_instance_id":"vgpIBgAAAAA=-AAAAAAAA8Jk=","is_multipart":"true","motr_process_fid":"<0x7200000000000000:0>","object_key_in_index":"temp4.txt","object_layout_id":9,"object_list_index_oid":"vgpIBgAAAHg=-BQAAAAAA8Jk=","objects_version_list_index_oid":"vgpIBgAAAHg=-BgAAAAAA8Jk=","old_oid":"AAAAAAAAAAA=-AAAAAAAAAAA=","part_list_idx_oid":"vgpIBgAAAHg=-SQAAAAAA8Jk="}
        # selected 1 records
        # next done, rc: 0
        # Done, rc:  0
        ###############################

        echo "----------------------------------------------------------------------------------------------------------------" >> /var/log/m0kv_metadata.log

        i=0
        for line in $(grep "KEY: [\"\/0-9a-zA-Z+=-]*" /tmp/m0kvprobable.log | cut -c7-)
        do
            if [ "$line" == "0" ] && [ "$i" == 0  ]
            then
                break
            else
                KEYS[i]=$(./base64_encoder_decoder -decode_oid $(echo "$line"))
                i=$((i+1))
            fi
        done

        GLOBAL_PROBABLE_DELETE_INDEX_COUNT=0
        for j in $(seq 0 ${#KEYS})
        do
            if [[ ! -z ${KEYS[j]} ]]
            then
                GLOBAL_PROBABLE_DELETE_INDEX_COUNT=$((GLOBAL_PROBABLE_DELETE_INDEX_COUNT+1))
            else
                break
            fi
        done
    
        
        
        if [[  "$GLOBAL_PROBABLE_DELETE_INDEX_COUNT" -ne 0 ]]
        then
            echo -e "$m0kv_PATH -l $1 -h $2 -p $3 -f $4 index next $probable_delete_index '0' $GLOBAL_PROBABLE_DELETE_INDEX_COUNT -s\n" >> /var/log/m0kv_metadata.log
            "$m0kv_PATH" -l "$1" -h "$2" -p "$3" -f "$4" index next "$probable_delete_index" '0' "$GLOBAL_PROBABLE_DELETE_INDEX_COUNT" -s >> /var/log/m0kv_metadata.log
            ##### m0kv command output #####
            # operation rc: 0
            # [0]:
            #         KEY: vgpIBgAAAAA=-RwAAAAAA8Jk=
            #         VAL: {"create_timestamp":"2020-11-10T04:51:30.000Z","force_delete":"true","global_instance_id":"vgpIBgAAAAA=-AAAAAAAA8Jk=","is_multipart":"true","motr_process_fid":"<0x7200000000000000:0>","object_key_in_index":"temp4.txt","object_layout_id":9,"object_list_index_oid":"vgpIBgAAAHg=-BQAAAAAA8Jk=","objects_version_list_index_oid":"vgpIBgAAAHg=-BgAAAAAA8Jk=","old_oid":"AAAAAAAAAAA=-AAAAAAAAAAA=","part_list_idx_oid":"vgpIBgAAAHg=-SQAAAAAA8Jk="}
            # selected 1 records
            # next done, rc: 0
            # Done, rc:  0
            ###############################

            i=0
            for line in $(grep -Ei -o "\"object_key_in_index\":[\"0-9a-zA-Z\"]*(.[a-zA-Z0-9]*)?" /tmp/m0kvprobable.log | cut -c24-)
            do
                OBJECTS[i]=$line
                i=$((i+1))
            done

            i=0
            for line in $(grep -Ei -o "\"object_list_index_oid\":\"[0-9a-zA-Z=+-]*" /tmp/m0kvprobable.log | cut -c26-)
            do
                OBJECT_LIST_INDEX_OID[i]=$(./base64_encoder_decoder -decode_oid $(echo "$line"))
                i=$((i+1))
            done

            i=0
            for line in $(grep -Ei -o "\"objects_version_list_index_oid\":\"[0-9a-zA-Z=+-]*" /tmp/m0kvprobable.log | cut -c35-)
            do
                OBJECT_VERSION_LIST_INDEX_OID[i]=$(./base64_encoder_decoder -decode_oid $(echo "$line"))
                i=$((i+1))
            done

            i=0
            for line in $(grep -Ei -o "\"part_list_idx_oid\":\"[0-9a-zA-Z=+-]*" /tmp/m0kvprobable.log | cut -c22-)
            do
                PART_LIST_INDEX_OID[i]=$(./base64_encoder_decoder -decode_oid $(echo "$line"))
                i=$((i+1))
            done

            echo -e "\n${green}##########################################################################################${reset}"
            echo "Probable Delete List Index"
            echo -e "${green}##########################################################################################${reset}\n"
            for j in $(seq 0 ${#KEYS})
            do
                if [[ ! -z ${KEYS[j]} ]]
                then
                    echo "key : ${KEYS[j]}"
                    echo "objectname : ${OBJECTS[j]}"
                    echo "object_list_index_oid : ${OBJECT_LIST_INDEX_OID[j]}"
                    echo "object_version_list_index_oid : ${OBJECT_VERSION_LIST_INDEX_OID[j]}"
                    echo "part_list_idx_oid : ${PART_LIST_INDEX_OID[j]}"
                    echo ""
                else
                    break
                fi
            done
            rm /tmp/m0kvprobable.log > /dev/null 2>&1
            echo -e "${green}##########################################################################################${reset}"
        else
            echo -e "\n${red}NO GLOBAL PROBABLE DELETE LIST INDEX PRESENT!!${reset}"
            rm /tmp/m0kvprobable.log > /dev/null 2>&1
        fi
    else
        binary_error
    fi
}

##### this function is used to get the number of parts of a multipart upload (to be displayed in the final o/p with their upload id's) #####
get_parts() {
    rm /tmp/m0kvcountpart.log > /dev/null 2>&1
    "$m0kv_PATH" -l "$4" -h "$5" -p "$6" -f "$7" index next "$3" '0' 1245 -s >> /tmp/m0kvcountpart.log
    ##### m0kv command output #####
    # operation rc: 0
    # [0]:
    #         KEY: 1
    #         VAL: {"Bucket-Name":"test1","Object-Name":"multipart_object.txt","Part-Num":"1","System-Defined":{"Content-Length":"20971520","Content-MD5":"8f4e33f3dc3e414ff94e5fb6905cba8c","Date":"2020-11-12T06:17:47.000Z","Last-Modified":"2020-11-12T06:17:47.000Z","Upload-ID":"e8a162a1-1fe9-42b2-9841-80a3827f9a57","x-amz-server-side-encryption":"None","x-amz-server-side-encryption-aws-kms-key-id":"","x-amz-server-side-encryption-customer-algorithm":"","x-amz-server-side-encryption-customer-key":"","x-amz-server-side-encryption-customer-key-MD5":"","x-amz-storage-class":"STANDARD","x-amz-version-id":"","x-amz-website-redirect-location":"None"},"Upload-ID":"e8a162a1-1fe9-42b2-9841-80a3827f9a57"}
    ###############################


    i=0
    for line in $(grep 'KEY: [0-9]*[0-9]$' /tmp/m0kvcountpart.log | cut -c7-)
    do
        if [ "$line" == "0" ] && [ "$i" == 0  ]
        then
            break
        else
            i=$((i+1))
        fi
    done
    NO_OF_PARTS="$i"
    
    
    if [ "$NO_OF_PARTS" -ne 0 ]
    then
        rm /tmp/m0kvpart.log > /dev/null 2>&1
        echo "----------------------------------------------------------------------------------------------------------------" >> /var/log/m0kv_metadata.log
        echo -e "$m0kv_PATH -l $4 -h $5 -p $6 -f $7 index next $3 '0' $NO_OF_PARTS -s\n" >> /var/log/m0kv_metadata.log
        "$m0kv_PATH" -l "$4" -h "$5" -p "$6" -f "$7" index next "$3" '0' "$NO_OF_PARTS" -s >> /tmp/m0kvpart.log
        ##### m0kv command output #####
        # operation rc: 0
        # [0]:
        #         KEY: 1
        #         VAL: {"Bucket-Name":"test1","Object-Name":"multipart_object.txt","Part-Num":"1","System-Defined":{"Content-Length":"20971520","Content-MD5":"8f4e33f3dc3e414ff94e5fb6905cba8c","Date":"2020-11-12T06:17:47.000Z","Last-Modified":"2020-11-12T06:17:47.000Z","Upload-ID":"e8a162a1-1fe9-42b2-9841-80a3827f9a57","x-amz-server-side-encryption":"None","x-amz-server-side-encryption-aws-kms-key-id":"","x-amz-server-side-encryption-customer-algorithm":"","x-amz-server-side-encryption-customer-key":"","x-amz-server-side-encryption-customer-key-MD5":"","x-amz-storage-class":"STANDARD","x-amz-version-id":"","x-amz-website-redirect-location":"None"},"Upload-ID":"e8a162a1-1fe9-42b2-9841-80a3827f9a57"}
        ###############################
        cat /tmp/m0kvpart.log >> /var/log/m0kv_metadata.log
        echo "Upload ID : $(grep -o '},[\"]Upload-ID[\"].*' /tmp/m0kvpart.log | cut -c16-51 | head -n1)"
        echo "Number of parts : $NO_OF_PARTS"
        rm /tmp/m0kvpart.log > /dev/null 2>&1
    else
        echo -e "\n${red}NO PARTS UPLOADED FOR THIS OBJECT !!${reset}"
    fi  
    rm /tmp/m0kvcountpart.log > /dev/null 2>&1
}

##### this function is used to get the multipart metadata by passing the multipart_oid to m0kv tool #####
get_multipart_metadata() {
    declare -a MOTR_PART_OID
    MULTIPART_OBJECTS=()
    "$m0kv_PATH" -l "$3" -h "$4" -p "$5" -f "$6" index next "$1" '0' 1245 -s >> /tmp/countObjects.log
    ##### m0kv command output #####
    # operation rc: 0
    # [0]:
    #     KEY: multipart_check
    #     VAL: {"ACL":"PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiIHN0YW5kYWxvbmU9Im5vIj8+PEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+CiA8T3duZXI+CiAgPElEPjY4NjRjN2U2ZTY0YjQ4ZThiYjI0MzRlN2MzZDNhYzhkNDRhYWRiNjZmMWMzNGVmOGEzMmNmZmQ5ZWVlYTAxNTY8L0lEPgogIDxEaXNwbGF5TmFtZT5zM3FhPC9EaXNwbGF5TmFtZT4KIDwvT3duZXI+CiA8QWNjZXNzQ29udHJvbExpc3Q+CiAgPEdyYW50PgogICA8R3JhbnRlZSB4bWxuczp4c2k9Imh0dHA6Ly93d3cudzMub3JnLzIwMDEvWE1MU2NoZW1hLWluc3RhbmNlIiB4c2k6dHlwZT0iQ2Fub25pY2FsVXNlciI+CiAgICA8SUQ+Njg2NGM3ZTZlNjRiNDhlOGJiMjQzNGU3YzNkM2FjOGQ0NGFhZGI2NmYxYzM0ZWY4YTMyY2ZmZDllZWVhMDE1NjwvSUQ+CiAgICA8RGlzcGxheU5hbWU+czNxYTwvRGlzcGxheU5hbWU+CiAgIDwvR3JhbnRlZT4KICAgPFBlcm1pc3Npb24+RlVMTF9DT05UUk9MPC9QZXJtaXNzaW9uPgogIDwvR3JhbnQ+CiA8L0FjY2Vzc0NvbnRyb2xMaXN0Pgo8L0FjY2Vzc0NvbnRyb2xQb2xpY3k+","Bucket-Name":"test3","Object-Name":"multipart_check","Object-URI":"test3\\multipart_check","System-Defined":{"Content-Length":"","Content-MD5":"","Content-Type":"","Date":"2020-11-09T11:27:13.000Z","Last-Modified":"2020-11-09T11:27:13.000Z","Owner-Account":"s3qa","Owner-Account-id":"456451734479","Owner-Canonical-id":"6864c7e6e64b48e8bb2434e7c3d3ac8d44aadb66f1c34ef8a32cffd9eeea0156","Owner-User":"root","Owner-User-id":"9RO8JADzR56oupbZsBiROA","Part-One-Size":"","Upload-ID":"26e8168e-2202-421f-8c9c-2efe4a0de971","x-amz-server-side-encryption":"None","x-amz-server-side-encryption-aws-kms-key-id":"","x-amz-server-side-encryption-customer-algorithm":"","x-amz-server-side-encryption-customer-key":"","x-amz-server-side-encryption-customer-key-MD5":"","x-amz-storage-class":"STANDARD","x-amz-version-id":"","x-amz-website-redirect-location":"None"},"Upload-ID":"26e8168e-2202-421f-8c9c-2efe4a0de971","create_timestamp":"2020-11-09T11:27:13.000Z","layout_id":9,"motr_oid":"vgpIBgAAAAA=-KQAAAAAA8Jk=","motr_old_object_version_id":"","motr_old_oid":"","motr_part_oid":"vgpIBgAAAHg=-KwAAAAAA8Jk=","old_layout_id":0}
    ###############################

    i=0
    for line in $(grep "KEY: .*[0-9a-zA-Z]$" /tmp/countObjects.log | cut -c7-)
    do
        if [ "$line" == "0" ] && [ "$i" == 0  ]
        then
            break
        else
            MULTIPART_OBJECTS[i]=$line
            i=$((i+1))
        fi
    done
    NO_OF_MULTIPART_OBJECTS="$i"

    if [ "$NO_OF_MULTIPART_OBJECTS" -ne "0" ]
    then 
        declare -a MOTR_PART_OID
        declare -a MOTR_OID
        echo "----------------------------------------------------------------------------------------------------------------" >> /var/log/m0kv_metadata.log
        echo -e "$m0kv_PATH -l $3 -h $4 -p $5 -f $6 index next $1 '0' $NO_OF_MULTIPART_OBJECTS -s\n" >> /var/log/m0kv_metadata.log
        "$m0kv_PATH" -l "$3" -h "$4" -p "$5" -f "$6" index next "$1" '0' $NO_OF_MULTIPART_OBJECTS -s >> /var/log/m0kv_metadata.log
        ##### m0kv command output #####
        # operation rc: 0
        # [0]:
        #     KEY: multipart_check
        #     VAL: {"ACL":"PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiIHN0YW5kYWxvbmU9Im5vIj8+PEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+CiA8T3duZXI+CiAgPElEPjY4NjRjN2U2ZTY0YjQ4ZThiYjI0MzRlN2MzZDNhYzhkNDRhYWRiNjZmMWMzNGVmOGEzMmNmZmQ5ZWVlYTAxNTY8L0lEPgogIDxEaXNwbGF5TmFtZT5zM3FhPC9EaXNwbGF5TmFtZT4KIDwvT3duZXI+CiA8QWNjZXNzQ29udHJvbExpc3Q+CiAgPEdyYW50PgogICA8R3JhbnRlZSB4bWxuczp4c2k9Imh0dHA6Ly93d3cudzMub3JnLzIwMDEvWE1MU2NoZW1hLWluc3RhbmNlIiB4c2k6dHlwZT0iQ2Fub25pY2FsVXNlciI+CiAgICA8SUQ+Njg2NGM3ZTZlNjRiNDhlOGJiMjQzNGU3YzNkM2FjOGQ0NGFhZGI2NmYxYzM0ZWY4YTMyY2ZmZDllZWVhMDE1NjwvSUQ+CiAgICA8RGlzcGxheU5hbWU+czNxYTwvRGlzcGxheU5hbWU+CiAgIDwvR3JhbnRlZT4KICAgPFBlcm1pc3Npb24+RlVMTF9DT05UUk9MPC9QZXJtaXNzaW9uPgogIDwvR3JhbnQ+CiA8L0FjY2Vzc0NvbnRyb2xMaXN0Pgo8L0FjY2Vzc0NvbnRyb2xQb2xpY3k+","Bucket-Name":"test3","Object-Name":"multipart_check","Object-URI":"test3\\multipart_check","System-Defined":{"Content-Length":"","Content-MD5":"","Content-Type":"","Date":"2020-11-09T11:27:13.000Z","Last-Modified":"2020-11-09T11:27:13.000Z","Owner-Account":"s3qa","Owner-Account-id":"456451734479","Owner-Canonical-id":"6864c7e6e64b48e8bb2434e7c3d3ac8d44aadb66f1c34ef8a32cffd9eeea0156","Owner-User":"root","Owner-User-id":"9RO8JADzR56oupbZsBiROA","Part-One-Size":"","Upload-ID":"26e8168e-2202-421f-8c9c-2efe4a0de971","x-amz-server-side-encryption":"None","x-amz-server-side-encryption-aws-kms-key-id":"","x-amz-server-side-encryption-customer-algorithm":"","x-amz-server-side-encryption-customer-key":"","x-amz-server-side-encryption-customer-key-MD5":"","x-amz-storage-class":"STANDARD","x-amz-version-id":"","x-amz-website-redirect-location":"None"},"Upload-ID":"26e8168e-2202-421f-8c9c-2efe4a0de971","create_timestamp":"2020-11-09T11:27:13.000Z","layout_id":9,"motr_oid":"vgpIBgAAAAA=-KQAAAAAA8Jk=","motr_old_object_version_id":"","motr_old_oid":"","motr_part_oid":"vgpIBgAAAHg=-KwAAAAAA8Jk=","old_layout_id":0}
        ###############################
        
        i=0
        for line in $(grep -Ei -o "\"motr_part_oid\":[\"0-9a-zA-Z+=-]*" /tmp/countObjects.log)
        do
            MOTR_PART_OID[i]=$(./base64_encoder_decoder -decode_oid $(echo "$line" | cut -c18-$((${#line}-1))))
            i=$((i+1))
        done

        i=0
        for line in $(grep -Ei -o "\"motr_oid\":[\"0-9a-zA-Z+=-]*" /tmp/countObjects.log)
        do
            MOTR_OID[i]=$(./base64_encoder_decoder -decode_oid $(echo "$line" | cut -c13-$((${#line}-1))))
            i=$((i+1))
        done

        echo -e "\n${green}#######################-------------------------------------##############################${reset}"
        echo "Multipart Objects metadata in Bucket"
        echo -e "${green}#######################-------------------------------------##############################${reset}\n"
        for j in $(seq 0 $((NO_OF_MULTIPART_OBJECTS-1)))
        do
            echo "objectname : ${MULTIPART_OBJECTS[j]}"
            echo "motr_part_oid : ${MOTR_PART_OID[j]}"
            echo "motr_oid : ${MOTR_OID[j]}"
            get_parts "$2" "${MULTIPART_OBJECTS[j]}" "${MOTR_PART_OID[j]}" "$3" "$4" "$5" "$6" 
            echo ""
        done
    else
        echo -e "\n${red}NO MULTIPART OBJECTS IN THE BUCKET !!\n${reset}"
    fi
    rm /tmp/countObjects.log > /dev/null 2>&1
}

##### this function is used to gets the object level metadata with object list index #####
get_object_metadata() {
    OBJECTS=()
    declare -a MOTR_OIDS
    declare -a OBJECT_VERSION
    rm /tmp/m0kvbucket.log > /dev/null 2>&1
    rm /tmp/m0kvobjver.log > /dev/null 2>&1 
    "$m0kv_PATH" -l "$3" -h "$5" -p "$6" -f "$7" index next "$1" '0' 65 -s >> /tmp/m0kvbucket.log
    ##### m0kv command output #####
    # operation rc: 0
    # [0]:
    #     KEY: temp2.txt
    #     VAL: {"ACL":"PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiIHN0YW5kYWxvbmU9Im5vIj8+PEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+CiA8T3duZXI+CiAgPElEPjY4NjRjN2U2ZTY0YjQ4ZThiYjI0MzRlN2MzZDNhYzhkNDRhYWRiNjZmMWMzNGVmOGEzMmNmZmQ5ZWVlYTAxNTY8L0lEPgogIDxEaXNwbGF5TmFtZT5zM3FhPC9EaXNwbGF5TmFtZT4KIDwvT3duZXI+CiA8QWNjZXNzQ29udHJvbExpc3Q+CiAgPEdyYW50PgogICA8R3JhbnRlZSB4bWxuczp4c2k9Imh0dHA6Ly93d3cudzMub3JnLzIwMDEvWE1MU2NoZW1hLWluc3RhbmNlIiB4c2k6dHlwZT0iQ2Fub25pY2FsVXNlciI+CiAgICA8SUQ+Njg2NGM3ZTZlNjRiNDhlOGJiMjQzNGU3YzNkM2FjOGQ0NGFhZGI2NmYxYzM0ZWY4YTMyY2ZmZDllZWVhMDE1NjwvSUQ+CiAgICA8RGlzcGxheU5hbWU+czNxYTwvRGlzcGxheU5hbWU+CiAgIDwvR3JhbnRlZT4KICAgPFBlcm1pc3Npb24+RlVMTF9DT05UUk9MPC9QZXJtaXNzaW9uPgogIDwvR3JhbnQ+CiA8L0FjY2Vzc0NvbnRyb2xMaXN0Pgo8L0FjY2Vzc0NvbnRyb2xQb2xpY3k+","Bucket-Name":"test1","Object-Name":"temp2.txt","Object-URI":"test1\\temp2.txt","System-Defined":{"Content-Length":"9","Content-MD5":"27f3f4a6a783c5440ce38ba6c1e52924","Content-Type":"text/plain","Date":"2020-11-09T04:38:04.000Z","Last-Modified":"2020-11-09T04:38:04.000Z","Owner-Account":"s3qa","Owner-Account-id":"456451734479","Owner-Canonical-id":"6864c7e6e64b48e8bb2434e7c3d3ac8d44aadb66f1c34ef8a32cffd9eeea0156","Owner-User":"root","Owner-User-id":"9RO8JADzR56oupbZsBiROA","x-amz-server-side-encryption":"None","x-amz-server-side-encryption-aws-kms-key-id":"","x-amz-server-side-encryption-customer-algorithm":"","x-amz-server-side-encryption-customer-key":"","x-amz-server-side-encryption-customer-key-MD5":"","x-amz-storage-class":"STANDARD","x-amz-version-id":"MTg0NDY3NDI0Njg4MTI4NjcyMzg","x-amz-website-redirect-location":"None"},"create_timestamp":"2020-11-09T04:38:04.000Z","layout_id":1,"motr_oid":"vgpIBgAAAAA=-DAAAAAAA8Jk="}
    ###############################

    i=0
    for line in $(grep -o 'KEY: .*[0-9a-zA-Z]$' /tmp/m0kvbucket.log | cut -c6-)
    do
        if [ "$line" == "0" ] && [ "$i" == 0  ]
        then
            break
        else
            OBJECTS[i]="$line"
            i=$((i+1))
        fi
    done
    NO_OF_OBJECTS="$i"
    if [ "$NO_OF_OBJECTS" -ne 0 ]
    then    
        echo "----------------------------------------------------------------------------------------------------------------" >> /var/log/m0kv_metadata.log
        echo -e "$m0kv_PATH -l $3 -h $5 -p $6 -f $7 index next $1 '0' $NO_OF_OBJECTS -s\n" >> /var/log/m0kv_metadata.log
        "$m0kv_PATH" -l "$3" -h "$5" -p "$6" -f "$7" index next "$1" '0' "$NO_OF_OBJECTS" -s >> /var/log/m0kv_metadata.log
        ##### m0kv command output #####
        # operation rc: 0
        # [0]:
        #     KEY: temp2.txt
        #     VAL: {"ACL":"PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiIHN0YW5kYWxvbmU9Im5vIj8+PEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+CiA8T3duZXI+CiAgPElEPjY4NjRjN2U2ZTY0YjQ4ZThiYjI0MzRlN2MzZDNhYzhkNDRhYWRiNjZmMWMzNGVmOGEzMmNmZmQ5ZWVlYTAxNTY8L0lEPgogIDxEaXNwbGF5TmFtZT5zM3FhPC9EaXNwbGF5TmFtZT4KIDwvT3duZXI+CiA8QWNjZXNzQ29udHJvbExpc3Q+CiAgPEdyYW50PgogICA8R3JhbnRlZSB4bWxuczp4c2k9Imh0dHA6Ly93d3cudzMub3JnLzIwMDEvWE1MU2NoZW1hLWluc3RhbmNlIiB4c2k6dHlwZT0iQ2Fub25pY2FsVXNlciI+CiAgICA8SUQ+Njg2NGM3ZTZlNjRiNDhlOGJiMjQzNGU3YzNkM2FjOGQ0NGFhZGI2NmYxYzM0ZWY4YTMyY2ZmZDllZWVhMDE1NjwvSUQ+CiAgICA8RGlzcGxheU5hbWU+czNxYTwvRGlzcGxheU5hbWU+CiAgIDwvR3JhbnRlZT4KICAgPFBlcm1pc3Npb24+RlVMTF9DT05UUk9MPC9QZXJtaXNzaW9uPgogIDwvR3JhbnQ+CiA8L0FjY2Vzc0NvbnRyb2xMaXN0Pgo8L0FjY2Vzc0NvbnRyb2xQb2xpY3k+","Bucket-Name":"test1","Object-Name":"temp2.txt","Object-URI":"test1\\temp2.txt","System-Defined":{"Content-Length":"9","Content-MD5":"27f3f4a6a783c5440ce38ba6c1e52924","Content-Type":"text/plain","Date":"2020-11-09T04:38:04.000Z","Last-Modified":"2020-11-09T04:38:04.000Z","Owner-Account":"s3qa","Owner-Account-id":"456451734479","Owner-Canonical-id":"6864c7e6e64b48e8bb2434e7c3d3ac8d44aadb66f1c34ef8a32cffd9eeea0156","Owner-User":"root","Owner-User-id":"9RO8JADzR56oupbZsBiROA","x-amz-server-side-encryption":"None","x-amz-server-side-encryption-aws-kms-key-id":"","x-amz-server-side-encryption-customer-algorithm":"","x-amz-server-side-encryption-customer-key":"","x-amz-server-side-encryption-customer-key-MD5":"","x-amz-storage-class":"STANDARD","x-amz-version-id":"MTg0NDY3NDI0Njg4MTI4NjcyMzg","x-amz-website-redirect-location":"None"},"create_timestamp":"2020-11-09T04:38:04.000Z","layout_id":1,"motr_oid":"vgpIBgAAAAA=-DAAAAAAA8Jk="}
        ###############################
        
        "$m0kv_PATH" -l "$3" -h "$5" -p "$6" -f "$7" index next "$4" '0' "$NO_OF_OBJECTS" -s >> /tmp/m0kvobjver.log
        ##### m0kv command output #####
        # operation rc: 0
        # [0]:
        #     KEY: temp2.txt/18446742468812867238
        #     VAL: {"create_timestamp":"2020-11-09T04:38:04.000Z","layout_id":1,"motr_oid":"vgpIBgAAAAA=-DAAAAAAA8Jk="}
        ###############################
        echo "----------------------------------------------------------------------------------------------------------------" >> /var/log/m0kv_metadata.log
        echo -e "$m0kv_PATH -l $3 -h $5 -p $6 -f $7 index next $4 '0' $NO_OF_OBJECTS -s\n" >> /var/log/m0kv_metadata.log
        cat /tmp/m0kvobjver.log >> /var/log/m0kv_metadata.log
        
        i=0
        while read data
        do 
            OBJECT_VERSION[i]=$(echo "$data" | cut -c6-)
            i=$((i+1))
        done < <( grep -Ei -o '[\/][0-9][0-9]*[0-9]' /tmp/m0kvobjver.log | cut -c2-)

        i=0
        for line in $(grep -Ei -o "\"motr_oid\":[\"0-9a-zA-Z+=-]*" /tmp/m0kvbucket.log)
        do
            MOTR_OIDS[i]=$(./base64_encoder_decoder -decode_oid $(echo "$line" | cut -c13-$((${#line}-1))))
            i=$((i+1))
        done

        echo -e "${green}#######################-------------------------------------##############################${reset}"
        echo "Objects metadata in Bucket"
        echo -e "${green}#######################-------------------------------------##############################${reset}\n"
        for j in $(seq 0 $((NO_OF_OBJECTS-1)))
        do
            echo "objectname : ${OBJECTS[j]}"
            echo "motr_oid : ${MOTR_OIDS[j]}"
            echo "object_version: ${OBJECT_VERSION[j]}"
            echo ""
        done
        rm /tmp/m0kvbucket.log > /dev/null 2>&1
        rm /tmp/m0kvobjver.log > /dev/null 2>&1
    else 
        echo -e "\n${red}NO OBJECTS IN THE BUCKET!!\n${reset}"
    fi
}

##### this function is used to get bucket level metadata #####
get_all_buckets_metadata() {
    if [[ ! -z "$m0kv_PATH" ]]
    then
        rm /var/log/m0kv_metadata.log > /dev/null 2>&1
        rm /tmp/m0kvtest.log > /dev/null 2>&1
        rm ./*.[0-9]* -f > /dev/null 2>&1 
        declare -a MOTR_MULTIPART_INDEX_OID
        declare -a MOTR_OBJECT_LIST_INDEX_OID
        declare -a MOTR_OBJECTS_VERSION_LIST_INDEX_OID
        declare -a BUCKETS
        
        "$m0kv_PATH" -l "$1" -h "$2" -p "$3" -f "$4" index next "$bucket_list_index" '0' 1245 -s >> /tmp/m0kvtest.log
        ##### m0kv command output #####
        # operation rc: 0
        # [0]:
        #         KEY: 456451734479/test1
        #         VAL: {"ACL":"PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiIHN0YW5kYWxvbmU9Im5vIj8+PEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+CiA8T3duZXI+CiAgPElEPjY4NjRjN2U2ZTY0YjQ4ZThiYjI0MzRlN2MzZDNhYzhkNDRhYWRiNjZmMWMzNGVmOGEzMmNmZmQ5ZWVlYTAxNTY8L0lEPgogIDxEaXNwbGF5TmFtZT5zM3FhPC9EaXNwbGF5TmFtZT4KIDwvT3duZXI+CiA8QWNjZXNzQ29udHJvbExpc3Q+CiAgPEdyYW50PgogICA8R3JhbnRlZSB4bWxuczp4c2k9Imh0dHA6Ly93d3cudzMub3JnLzIwMDEvWE1MU2NoZW1hLWluc3RhbmNlIiB4c2k6dHlwZT0iQ2Fub25pY2FsVXNlciI+CiAgICA8SUQ+Njg2NGM3ZTZlNjRiNDhlOGJiMjQzNGU3YzNkM2FjOGQ0NGFhZGI2NmYxYzM0ZWY4YTMyY2ZmZDllZWVhMDE1NjwvSUQ+CiAgICA8RGlzcGxheU5hbWU+czNxYTwvRGlzcGxheU5hbWU+CiAgIDwvR3JhbnRlZT4KICAgPFBlcm1pc3Npb24+RlVMTF9DT05UUk9MPC9QZXJtaXNzaW9uPgogIDwvR3JhbnQ+CiA8L0FjY2Vzc0NvbnRyb2xMaXN0Pgo8L0FjY2Vzc0NvbnRyb2xQb2xpY3k+","Bucket-Name":"test1","Policy":"","System-Defined":{"Date":"2020-11-09T04:35:56.000Z","LocationConstraint":"US","Owner-Account":"s3qa","Owner-Account-id":"456451734479","Owner-Canonical-id":"6864c7e6e64b48e8bb2434e7c3d3ac8d44aadb66f1c34ef8a32cffd9eeea0156","Owner-User":"root","Owner-User-id":"9RO8JADzR56oupbZsBiROA"},"create_timestamp":"2020-11-09T04:35:56.000Z","motr_multipart_index_oid":"vgpIBgAAAHg=-AgAAAAAA8Jk=","motr_object_list_index_oid":"vgpIBgAAAHg=-AQAAAAAA8Jk=","motr_objects_version_list_index_oid":"vgpIBgAAAHg=-AwAAAAAA8Jk="}
        ###############################
        i=0
        for line in $(grep "KEY: [0-9]*[\/].*" /tmp/m0kvtest.log | cut -c20-)
        do
            BUCKETS[i]=$line
            i=$((i+1))
        done
        NO_OF_BUCKETS="$i"
        
        echo "----------------------------------------------------------------------------------------------------------------" >> /var/log/m0kv_metadata.log
        echo -e "$m0kv_PATH -l $1 -h $2 -p $3 -f $4 index next $bucket_list_index '0' $NO_OF_BUCKETS -s\n" >> /var/log/m0kv_metadata.log
        "$m0kv_PATH" -l "$1" -h "$2" -p "$3" -f "$4" index next "$bucket_list_index" '0' "$NO_OF_BUCKETS" -s >> /var/log/m0kv_metadata.log
        ##### m0kv command output #####
        # operation rc: 0
        # [0]:
        #         KEY: 456451734479/test1
        #         VAL: {"ACL":"PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiIHN0YW5kYWxvbmU9Im5vIj8+PEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+CiA8T3duZXI+CiAgPElEPjY4NjRjN2U2ZTY0YjQ4ZThiYjI0MzRlN2MzZDNhYzhkNDRhYWRiNjZmMWMzNGVmOGEzMmNmZmQ5ZWVlYTAxNTY8L0lEPgogIDxEaXNwbGF5TmFtZT5zM3FhPC9EaXNwbGF5TmFtZT4KIDwvT3duZXI+CiA8QWNjZXNzQ29udHJvbExpc3Q+CiAgPEdyYW50PgogICA8R3JhbnRlZSB4bWxuczp4c2k9Imh0dHA6Ly93d3cudzMub3JnLzIwMDEvWE1MU2NoZW1hLWluc3RhbmNlIiB4c2k6dHlwZT0iQ2Fub25pY2FsVXNlciI+CiAgICA8SUQ+Njg2NGM3ZTZlNjRiNDhlOGJiMjQzNGU3YzNkM2FjOGQ0NGFhZGI2NmYxYzM0ZWY4YTMyY2ZmZDllZWVhMDE1NjwvSUQ+CiAgICA8RGlzcGxheU5hbWU+czNxYTwvRGlzcGxheU5hbWU+CiAgIDwvR3JhbnRlZT4KICAgPFBlcm1pc3Npb24+RlVMTF9DT05UUk9MPC9QZXJtaXNzaW9uPgogIDwvR3JhbnQ+CiA8L0FjY2Vzc0NvbnRyb2xMaXN0Pgo8L0FjY2Vzc0NvbnRyb2xQb2xpY3k+","Bucket-Name":"test1","Policy":"","System-Defined":{"Date":"2020-11-09T04:35:56.000Z","LocationConstraint":"US","Owner-Account":"s3qa","Owner-Account-id":"456451734479","Owner-Canonical-id":"6864c7e6e64b48e8bb2434e7c3d3ac8d44aadb66f1c34ef8a32cffd9eeea0156","Owner-User":"root","Owner-User-id":"9RO8JADzR56oupbZsBiROA"},"create_timestamp":"2020-11-09T04:35:56.000Z","motr_multipart_index_oid":"vgpIBgAAAHg=-AgAAAAAA8Jk=","motr_object_list_index_oid":"vgpIBgAAAHg=-AQAAAAAA8Jk=","motr_objects_version_list_index_oid":"vgpIBgAAAHg=-AwAAAAAA8Jk="}
        ###############################
    
        i=0
        for line in $(grep -Ei -o "\"motr_multipart_index_oid\":[\"0-9a-zA-Z+=-]*" /tmp/m0kvtest.log)
        do
            MOTR_MULTIPART_INDEX_OID_VAL=$(echo "$line" | cut -c29-$((${#line}-1)))
            MOTR_MULTIPART_INDEX_OID[i]=$(./base64_encoder_decoder -decode_oid "$MOTR_MULTIPART_INDEX_OID_VAL")
            i=$((i+1))
        done

        i=0
        for line in $(grep -Ei -o "\"motr_object_list_index_oid\":[\"0-9a-zA-Z+=-]*" /tmp/m0kvtest.log)
        do
            MOTR_OBJECT_LIST_INDEX_OID_VAL=$(echo "$line" | cut -c31-$((${#line}-1)))
            MOTR_OBJECT_LIST_INDEX_OID[i]=$(./base64_encoder_decoder -decode_oid "$MOTR_OBJECT_LIST_INDEX_OID_VAL")
            i=$((i+1))
        done

        i=0
        for line in $(grep -Ei -o "\"motr_objects_version_list_index_oid\":[\"0-9a-zA-Z+=-]*" /tmp/m0kvtest.log)
        do
            MOTR_OBJECTS_VERSION_LIST_INDEX_OID_VAL=$(echo "$line" | cut -c40-$((${#line}-1)))
            MOTR_OBJECTS_VERSION_LIST_INDEX_OID[i]=$(./base64_encoder_decoder -decode_oid "$MOTR_OBJECTS_VERSION_LIST_INDEX_OID_VAL")
            i=$((i+1))
        done

        echo -e "\nMetadata of all buckets\n"
        echo -e "${green}##########################################################################################${reset}\n"
        for j in $(seq 1 $NO_OF_BUCKETS)
        do
            echo "bucketname : ${BUCKETS[j-1]}"
            echo "motr_multipart_index_oid : ${MOTR_MULTIPART_INDEX_OID[j-1]}"
            echo "motr_object_list_index_oid : ${MOTR_OBJECT_LIST_INDEX_OID[j-1]}"
            echo "motr_objects_version_list_index_oid : ${MOTR_OBJECTS_VERSION_LIST_INDEX_OID[j-1]}"
            if [ "$5" != 1 ]
            then
                get_multipart_metadata "${MOTR_MULTIPART_INDEX_OID[j-1]}" "${BUCKETS[j-1]}" "$1" "$2" "$3" "$4"
                get_object_metadata "${MOTR_OBJECT_LIST_INDEX_OID[j-1]}" "${BUCKETS[j-1]}" "$1" "${MOTR_OBJECTS_VERSION_LIST_INDEX_OID[j-1]}" "$2" "$3" "$4"
            else
                echo ""
            fi
            echo -e "${green}##########################################################################################${reset}\n"
        done
        rm /tmp/m0kvtest.log > /dev/null 2>&1
        rm ./*.[0-9]* -f > /dev/null 2>&1 
    else
        binary_error
    fi
}

##### this function is used to get the metadata of a particular bucket #####
get_metadata_of_bucket() {
    if [[ ! -z "$m0kv_PATH" ]]
    then
        rm /tmp/m0kvtest1.log > /dev/null 2>&1
        rm ./*.[0-9]* -f > /dev/null 2>&1 
        rm /var/log/m0kv_metadata.log > /dev/null 2>&1
        "$m0kv_PATH" -l "$2" -h "$3" -p "$4" -f "$5" index next "$bucket_list_index" '0' 1245 -s >> /tmp/m0kvtest1.log
        ##### m0kv command output #####
        # operation rc: 0
        # [0]:
        #         KEY: 456451734479/test1
        #         VAL: {"ACL":"PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiIHN0YW5kYWxvbmU9Im5vIj8+PEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+CiA8T3duZXI+CiAgPElEPjY4NjRjN2U2ZTY0YjQ4ZThiYjI0MzRlN2MzZDNhYzhkNDRhYWRiNjZmMWMzNGVmOGEzMmNmZmQ5ZWVlYTAxNTY8L0lEPgogIDxEaXNwbGF5TmFtZT5zM3FhPC9EaXNwbGF5TmFtZT4KIDwvT3duZXI+CiA8QWNjZXNzQ29udHJvbExpc3Q+CiAgPEdyYW50PgogICA8R3JhbnRlZSB4bWxuczp4c2k9Imh0dHA6Ly93d3cudzMub3JnLzIwMDEvWE1MU2NoZW1hLWluc3RhbmNlIiB4c2k6dHlwZT0iQ2Fub25pY2FsVXNlciI+CiAgICA8SUQ+Njg2NGM3ZTZlNjRiNDhlOGJiMjQzNGU3YzNkM2FjOGQ0NGFhZGI2NmYxYzM0ZWY4YTMyY2ZmZDllZWVhMDE1NjwvSUQ+CiAgICA8RGlzcGxheU5hbWU+czNxYTwvRGlzcGxheU5hbWU+CiAgIDwvR3JhbnRlZT4KICAgPFBlcm1pc3Npb24+RlVMTF9DT05UUk9MPC9QZXJtaXNzaW9uPgogIDwvR3JhbnQ+CiA8L0FjY2Vzc0NvbnRyb2xMaXN0Pgo8L0FjY2Vzc0NvbnRyb2xQb2xpY3k+","Bucket-Name":"test1","Policy":"","System-Defined":{"Date":"2020-11-09T04:35:56.000Z","LocationConstraint":"US","Owner-Account":"s3qa","Owner-Account-id":"456451734479","Owner-Canonical-id":"6864c7e6e64b48e8bb2434e7c3d3ac8d44aadb66f1c34ef8a32cffd9eeea0156","Owner-User":"root","Owner-User-id":"9RO8JADzR56oupbZsBiROA"},"create_timestamp":"2020-11-09T04:35:56.000Z","motr_multipart_index_oid":"vgpIBgAAAHg=-AgAAAAAA8Jk=","motr_object_list_index_oid":"vgpIBgAAAHg=-AQAAAAAA8Jk=","motr_objects_version_list_index_oid":"vgpIBgAAAHg=-AwAAAAAA8Jk="}
        ###############################
        i=0
        for line in $(grep "KEY: [0-9]*[\/].*" /tmp/m0kvtest1.log | cut -c20-)
        do
            i=$((i+1))
        done
        NO_OF_BUCKETS="$i"

        echo "----------------------------------------------------------------------------------------------------------------" >> /var/log/m0kv_metadata.log
        echo -e "$m0kv_PATH -l $2 -h $3 -p $4 -f $5 index next $bucket_list_index '0' $NO_OF_BUCKETS -s\n" >> /var/log/m0kv_metadata.log
        "$m0kv_PATH" -l "$2" -h "$3" -p "$4" -f "$5" index next "$bucket_list_index" '0' "$NO_OF_BUCKETS" -s >> /var/log/m0kv_metadata.log
        ##### m0kv command output #####
        # operation rc: 0
        # [0]:
        #         KEY: 456451734479/test1
        #         VAL: {"ACL":"PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiIHN0YW5kYWxvbmU9Im5vIj8+PEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+CiA8T3duZXI+CiAgPElEPjY4NjRjN2U2ZTY0YjQ4ZThiYjI0MzRlN2MzZDNhYzhkNDRhYWRiNjZmMWMzNGVmOGEzMmNmZmQ5ZWVlYTAxNTY8L0lEPgogIDxEaXNwbGF5TmFtZT5zM3FhPC9EaXNwbGF5TmFtZT4KIDwvT3duZXI+CiA8QWNjZXNzQ29udHJvbExpc3Q+CiAgPEdyYW50PgogICA8R3JhbnRlZSB4bWxuczp4c2k9Imh0dHA6Ly93d3cudzMub3JnLzIwMDEvWE1MU2NoZW1hLWluc3RhbmNlIiB4c2k6dHlwZT0iQ2Fub25pY2FsVXNlciI+CiAgICA8SUQ+Njg2NGM3ZTZlNjRiNDhlOGJiMjQzNGU3YzNkM2FjOGQ0NGFhZGI2NmYxYzM0ZWY4YTMyY2ZmZDllZWVhMDE1NjwvSUQ+CiAgICA8RGlzcGxheU5hbWU+czNxYTwvRGlzcGxheU5hbWU+CiAgIDwvR3JhbnRlZT4KICAgPFBlcm1pc3Npb24+RlVMTF9DT05UUk9MPC9QZXJtaXNzaW9uPgogIDwvR3JhbnQ+CiA8L0FjY2Vzc0NvbnRyb2xMaXN0Pgo8L0FjY2Vzc0NvbnRyb2xQb2xpY3k+","Bucket-Name":"test1","Policy":"","System-Defined":{"Date":"2020-11-09T04:35:56.000Z","LocationConstraint":"US","Owner-Account":"s3qa","Owner-Account-id":"456451734479","Owner-Canonical-id":"6864c7e6e64b48e8bb2434e7c3d3ac8d44aadb66f1c34ef8a32cffd9eeea0156","Owner-User":"root","Owner-User-id":"9RO8JADzR56oupbZsBiROA"},"create_timestamp":"2020-11-09T04:35:56.000Z","motr_multipart_index_oid":"vgpIBgAAAHg=-AgAAAAAA8Jk=","motr_object_list_index_oid":"vgpIBgAAAHg=-AQAAAAAA8Jk=","motr_objects_version_list_index_oid":"vgpIBgAAAHg=-AwAAAAAA8Jk="}
        ###############################

        BUCKET_GREP=$(grep -Ei -o "$1[\"].*" /tmp/m0kvtest1.log)
        MOTR_MULTIPART_INDEX_OID_VAL=$(echo "$BUCKET_GREP" | grep -Ei -o "\"motr_multipart_index_oid\":[\"0-9a-zA-Z+=-]*")
        MOTR_OBJECT_LIST_INDEX_OID_VAL=$(echo "$BUCKET_GREP" | grep -Ei -o "\"motr_object_list_index_oid\":[\"0-9a-zA-Z+=-]*")
        MOTR_OBJECTS_VERSION_LIST_INDEX_OID_VAL=$(echo "$BUCKET_GREP" | grep -Ei -o "\"motr_objects_version_list_index_oid\":[\"0-9a-zA-Z+=-]*")
        MOTR_MULTIPART_INDEX_OID=$(./base64_encoder_decoder -decode_oid $(echo "$MOTR_MULTIPART_INDEX_OID_VAL" | cut -c29-$((${#MOTR_MULTIPART_INDEX_OID_VAL}-1))))
        MOTR_OBJECT_LIST_INDEX_OID=$(./base64_encoder_decoder -decode_oid $(echo "$MOTR_OBJECT_LIST_INDEX_OID_VAL" | cut -c31-$((${#MOTR_OBJECT_LIST_INDEX_OID_VAL}-1))))
        MOTR_OBJECTS_VERSION_LIST_INDEX_OID=$(./base64_encoder_decoder -decode_oid $(echo "$MOTR_OBJECTS_VERSION_LIST_INDEX_OID_VAL" | cut -c40-$((${#MOTR_OBJECTS_VERSION_LIST_INDEX_OID_VAL}-1))))
        
        echo -e "Metadata of $1\n"
        echo -e "${green}##########################################################################################${reset}"
        echo ""
        echo "bucketname : $1"
        echo "motr_multipart_index_oid : $MOTR_MULTIPART_INDEX_OID"
        echo "motr_object_list_index_oid : $MOTR_OBJECT_LIST_INDEX_OID"
        echo "motr_objects_version_list_index_oid : $MOTR_OBJECTS_VERSION_LIST_INDEX_OID"
        if [ "$6" != 1 ]
        then
            get_multipart_metadata "$MOTR_MULTIPART_INDEX_OID" "$1" "$2" "$3" "$4" "$5"
            get_object_metadata "$MOTR_OBJECT_LIST_INDEX_OID" "$1" "$2" "$MOTR_OBJECTS_VERSION_LIST_INDEX_OID" "$3" "$4" "$5"
        fi
        echo -e "\n${green}##########################################################################################${reset}"
        rm /tmp/m0kvtest1.log > /dev/null 2>&1
        rm ./*.[0-9]* -f > /dev/null 2>&1 
    else
        binary_error
        break
    fi
}

while getopts "l:h:p:f:b:d:" opt
do
   case "$opt" in
      l ) L="$OPTARG" ;;
      h ) H="$OPTARG" ;;
      p ) P="$OPTARG" ;;
      f ) F="$OPTARG" ;;
      b ) BUCKET_NAME="$OPTARG" ;;
      d ) BUCKET_LEVEL_FLAG="$OPTARG" ;;
      ? ) usage ;;
   esac 
done

##### Checks input arguements are null or not #####
if [[ ! -z "$L" ]] && [[ ! -z "$H" ]] && [[ ! -z "$P" ]] && [[ ! -z "$H" ]] && [ "$BUCKET_LEVEL_FLAG" -ge 1 ] 
then
    if [ -z "$BUCKET_NAME" ]
    then
        get_all_buckets_metadata "$L" "$H" "$P" "$F" "$BUCKET_LEVEL_FLAG"
        get_probable_delete_index "$L" "$H" "$P" "$F"
        echo -e "\n${red}Log file :${reset} ${green}/var/log/m0kv_metadata.log${reset}\n"
    else
        get_metadata_of_bucket "$BUCKET_NAME" "$L" "$H" "$P" "$F" "$BUCKET_LEVEL_FLAG"
        get_probable_delete_index "$L" "$H" "$P" "$F"
        echo -e "\n${red}Log file :${reset} ${green}/var/log/m0kv_metadata.log${reset}\n"
    fi
else
    usage
fi
make clean > /dev/null 2>&1

