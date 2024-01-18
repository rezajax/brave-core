/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_BRAVE_WALLET_UTILS_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_BRAVE_WALLET_UTILS_SERVICE_IMPL_H_

#include "brave/components/services/brave_wallet/public/mojom/brave_wallet_utils_service.mojom.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/self_owned_associated_receiver.h"

namespace brave_wallet {

class BraveWalletUtilsServiceImpl : public mojom::BraveWalletUtilsService {
 public:
  explicit BraveWalletUtilsServiceImpl(
      mojo::PendingReceiver<mojom::BraveWalletUtilsService> receiver);

  ~BraveWalletUtilsServiceImpl() override;

  BraveWalletUtilsServiceImpl(const BraveWalletUtilsServiceImpl&) = delete;
  BraveWalletUtilsServiceImpl& operator=(const BraveWalletUtilsServiceImpl&) =
      delete;

  void CreateZCashDecoderService(
      mojo::PendingAssociatedReceiver<mojom::ZCashDecoder> engine_receiver)
      override;

 private:
  mojo::Receiver<mojom::BraveWalletUtilsService> receiver_;
  mojo::SelfOwnedAssociatedReceiverRef<mojom::ZCashDecoder> instance_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_BRAVE_WALLET_UTILS_SERVICE_IMPL_H_
