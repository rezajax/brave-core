/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/utils.h"

#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"

#if !BUILDFLAG(IS_IOS)
#include "content/public/browser/browser_context.h"
#endif

namespace ai_chat {

namespace {

bool IsDisabledByPolicy(PrefService* prefs) {
  DCHECK(prefs);
  return prefs->IsManagedPreference(prefs::kEnabledByPolicy) &&
         !prefs->GetBoolean(prefs::kEnabledByPolicy);
}

#if !BUILDFLAG(IS_IOS)
bool IsAllowedForContext(content::BrowserContext* context) {
  return !context->IsTor();
}
#endif

}  // namespace

#if !BUILDFLAG(IS_IOS)
bool IsAIChatEnabled(content::BrowserContext* context) {
  DCHECK(context);
  return IsAllowedForContext(context) &&
         IsAIChatEnabled(user_prefs::UserPrefs::Get(context));
}
#endif

bool IsAIChatEnabled(PrefService* prefs) {
  DCHECK(prefs);
  return features::IsAIChatEnabled() && !IsDisabledByPolicy(prefs);
}

}  // namespace ai_chat
