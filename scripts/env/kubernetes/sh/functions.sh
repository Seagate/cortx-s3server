# not for execution -- only for using with `source` command.

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

add_separator() {
  ( set +x +e
    echo -e '\n\n\n========================= '"$*"' =========================\n'
  )
}

self_check() {
  ( set +x +e
    add_separator Self-check
    while true; do
      read -p "$* (yes/no) [no] " var
      if test x"$var" = xy -o x"$var" = xyes; then
        true
        return
      fi
      if test x"$var" = x -o x"$var" = xn -o x"$var" = xno; then
        false
        return
      fi
    done
    false
    return
  )
}


hit_enter_when_done() {
  ( set +x +e
    add_separator
    echo "$*"
    read -p 'Hit Enter when done' tmp
  )
}

