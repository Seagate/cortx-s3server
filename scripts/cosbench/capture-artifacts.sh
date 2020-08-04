#!/bin/sh
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


set -e
SHORT=h
LONG=help,controller:,workloadID:,output-directory:

CONTROLLER=""
WORKLOAD_ID=""
OUTPUT_DIR=""

usage() {
  echo 'Usage:'
  echo '  ./capture-artifacts.sh --workloadID <workload ID> --output-directory <local directory to copy output files>'
  echo '      --controller <Controller-IP> [--help]'
}

OPTS=$(getopt --options $SHORT --long $LONG  --name "$0" -- "$@")

eval set -- "$OPTS"

invalid_command() {
  printf "\nInvalid command\n"
  usage
  exit 1
}

while true ; do
  case "$1" in
    --controller )
      CONTROLLER="$2"
      shift 2
      ;;
    -h | --help )
      usage
      exit 0
      ;;
    --workloadID)
      WORKLOAD_ID="$2"
      shift 2
      ;;
    --output-directory )
      OUTPUT_DIR="$2"
      shift 2
      ;;
    -- )
      shift
      break
      ;;
    *)
      printf "$1\n"
      invalid_command
      ;;
  esac
done

if [ "$WORKLOAD_ID" = "" ] || [ "$OUTPUT_DIR" = "" ] || [ "$CONTROLLER" = "" ]
then
  invalid_command
fi

if [ ! -d "$OUTPUT_DIR" ]
then
  mkdir -p "$OUTPUT_DIR"
fi

scp "$(whoami)"@"$CONTROLLER":~/cos/archive/"$WORKLOAD_ID"-\*/"$WORKLOAD_ID"-\*workloadtype.csv  "$OUTPUT_DIR"

