// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/app_result.h"

#include "base/time/time.h"
#include "ui/app_list/app_list_switches.h"

namespace app_list {

AppResult::AppResult(Profile* profile,
                     const std::string& app_id,
                     AppListControllerDelegate* controller,
                     bool is_recommendation)
    : profile_(profile),
      app_id_(app_id),
      controller_(controller) {
  if (app_list::switches::IsExperimentalAppListEnabled())
    set_display_type(is_recommendation ? DISPLAY_RECOMMENDATION : DISPLAY_TILE);
}

AppResult::~AppResult() {
}

void AppResult::UpdateFromLastLaunched(const base::Time& current_time,
                                       const base::Time& last_launched) {
  base::TimeDelta delta = current_time - last_launched;
  // |current_time| can be before |last_launched| in weird cases such as users
  // playing with their clocks. Handle this gracefully.
  if (current_time < last_launched) {
    set_relevance(1.0);
    return;
  }

  const int kSecondsInWeek = 60 * 60 * 24 * 7;

  // Set the relevance to a value between 0 and 1. This function decays as the
  // time delta increases and reaches a value of 0.5 at 1 week.
  set_relevance(1 / (1 + delta.InSecondsF() / kSecondsInWeek));
}

}  // namespace app_list