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


require 'mixlib/shellout'

class BucketsController < ApplicationController
  def index  # Display all buckets
    s3cmd = Mixlib::ShellOut.new("s3cmd ls --access_key=#{session[:access_key]} --secret_key=#{session[:secret_key]} --access_token=#{session[:access_token]}")
    s3cmd.run_command
    @buckets = []
    s3cmd.stdout.lines do |line|
      case line
      when /^\s*(.*)\s+s3:\/\/(.*)$/
        @buckets.push({type: :content,  createdon: $1, title: $2})
      end
    end
  end

  def new  # Show the create form
  end

  def create  # Handler the POST (create)
    s3cmd = Mixlib::ShellOut.new("s3cmd mb s3://#{params[:bucket][:title]} --access_key=#{session[:access_key]} --secret_key=#{session[:secret_key]} --access_token=#{session[:access_token]}")
    s3cmd.run_command
    redirect_to buckets_path
  end

  def show
    if params[:method] == 'delete'
      destroy
    else
      render plain: "Displaying Bucket " + params[:id].inspect
    end
  end

  def destroy
    s3cmd = Mixlib::ShellOut.new("s3cmd rb s3://#{params[:id]} --access_key=#{session[:access_key]} --secret_key=#{session[:secret_key]} --access_token=#{session[:access_token]}")
    s3cmd.run_command
    redirect_to buckets_path
  end
end
