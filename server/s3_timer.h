/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

#pragma once

#ifndef __S3_SERVER_S3_TIMER_H__
#define __S3_SERVER_S3_TIMER_H__

#include <chrono>

// USAGE:
// S3Timer timer;
// timer.start();
// Do some operations to be timed
// timer.stop();
// Do some other operations
// timer.resume();
// Do some operations to be timed
// timer.stop();
// cout << "Elapsed time (ms) = " << timer.elapsed_time_in_millisec() << endl;
// cout << "Elapsed time (nanosec) = " << timer.elapsed_time_in_nanosec() <<
// endl << endl;
//

class S3Timer {
  enum class S3TimerState {
    unknown,
    started,
    stopped
  };

  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;

  std::chrono::time_point<Clock> start_time;
  std::chrono::time_point<Clock> end_time;
  Duration duration;
  S3TimerState state = S3TimerState::unknown;

 public:

  void start() {
    start_time = Clock::now();
    state = S3TimerState::started;
    duration = Duration(0);
  }

  void stop() {
    if (state == S3TimerState::started) {
      end_time = Clock::now();
      state = S3TimerState::stopped;
      duration += end_time - start_time;
    } else if (state != S3TimerState::stopped) {
      state = S3TimerState::unknown;
    }
  }

  void resume() {
    if (state == S3TimerState::stopped) {
      start_time = Clock::now();
      state = S3TimerState::started;
    } else {
      state = S3TimerState::unknown;
    }
  }

  std::chrono::milliseconds::rep elapsed_time_in_millisec() const {
    if (state == S3TimerState::stopped) {
      return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
          .count();
    }
    return -1;
  }

  std::chrono::nanoseconds::rep elapsed_time_in_nanosec() const {
    if (state == S3TimerState::stopped) {
      return std::chrono::duration_cast<std::chrono::nanoseconds>(duration)
          .count();
    }
    return -1;
  }
};

#endif
