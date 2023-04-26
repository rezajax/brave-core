/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_BALANCE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_BALANCE_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace brave_rewards::internal::wallet {

class WalletBalance {
 public:
  void Fetch(FetchBalanceCallback callback);

 private:
  void OnGetUnblindedTokens(FetchBalanceCallback callback,
                            std::vector<mojom::UnblindedTokenPtr> tokens);

  void OnFetchExternalWalletBalance(const std::string& wallet_type,
                                    mojom::BalancePtr balance_ptr,
                                    FetchBalanceCallback callback,
                                    mojom::Result result,
                                    double balance);
};

}  // namespace brave_rewards::internal::wallet
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_WALLET_BALANCE_H_
