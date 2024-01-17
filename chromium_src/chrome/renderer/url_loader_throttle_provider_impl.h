/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_

#define BRAVE_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_                   \
 protected:                                                          \
  static base::PassKey<URLLoaderThrottleProviderImpl> GetPassKey() { \
    return base::PassKey<URLLoaderThrottleProviderImpl>();           \
  }

#include "src/chrome/renderer/url_loader_throttle_provider_impl.h"  // IWYU pragma: export

#undef BRAVE_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_
