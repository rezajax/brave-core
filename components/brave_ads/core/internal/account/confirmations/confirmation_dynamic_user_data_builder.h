/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_DYNAMIC_USER_DATA_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_DYNAMIC_USER_DATA_BUILDER_H_

#include "brave/components/brave_ads/core/internal/account/user_data/user_data_builder_interface.h"

namespace brave_ads {

class ConfirmationDynamicUserDataBuilder final
    : public UserDataBuilderInterface {
 public:
  void Build(UserDataBuilderCallback callback) const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_DYNAMIC_USER_DATA_BUILDER_H_
