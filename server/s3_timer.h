/*
 * COPYRIGHT 2016 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 7-Jan-2016
 */

#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_TIMER_H__
#define __MERO_FE_S3_SERVER_S3_TIMER_H__

#include <chrono>

// USAGE:
// S3Timer timer;
// timer.start();
// Do some operations to be timed
// timer.stop();
// cout << "Elapsed time (ms) = " << timer.elapsed_time_in_millisec() << endl;
// cout << "Elapsed time (nanosec) = " << timer.elapsed_time_in_nanosec() <<
// endl << endl;
//

enum class S3TimerState { unknown, started, stopped };

class S3Timer {
  std::chrono::time_point<std::chrono::steady_clock> start_;
  std::chrono::time_point<std::chrono::steady_clock> end_;
  std::chrono::duration<double> difference;

  S3TimerState state;

 public:
  S3Timer() : state(S3TimerState::unknown) {}

  void start() {
    start_ = std::chrono::steady_clock::now();
    state = S3TimerState::started;
  }

  void stop() {
    if (state == S3TimerState::started) {
      end_ = std::chrono::steady_clock::now();
      state = S3TimerState::stopped;
      difference = end_ - start_;
    } else {
      state = S3TimerState::unknown;
    }
  }

  size_t elapsed_time_in_millisec() {
    if (state == S3TimerState::stopped) {
      return std::chrono::duration<double, std::milli>(difference).count();
    }
    return -1;
  }

  size_t elapsed_time_in_nanosec() {
    if (state == S3TimerState::stopped) {
      return std::chrono::duration<double, std::nano>(difference).count();
    }
    return -1;
  }
};

#endif
