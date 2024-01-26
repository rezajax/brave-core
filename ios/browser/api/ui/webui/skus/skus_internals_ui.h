// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_UI_WEBUI_SKUS_SKUS_INTERNALS_UI_H_
#define BRAVE_IOS_BROWSER_API_UI_WEBUI_SKUS_SKUS_INTERNALS_UI_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/skus/common/skus_internals.mojom.h"
#include "components/prefs/pref_service.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"
#include "ios/web/public/webui/web_ui_ios.h"

#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

// MARK: - BASED ON: brave/browser/ui/webui/skus_internals_ui.h

class SkusInternalsUI : public web::WebUIIOSController,
                        public skus::mojom::SkusInternals {
 public:
  explicit SkusInternalsUI(web::WebUIIOS* web_ui, const GURL& url);
  ~SkusInternalsUI() override;
  SkusInternalsUI(const SkusInternalsUI&) = delete;
  SkusInternalsUI& operator=(const SkusInternalsUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<skus::mojom::SkusInternals> pending_receiver);

 private:
  // skus::mojom::SkusInternals overrides:
  void GetEventLog(GetEventLogCallback callback) override;
  void GetSkusState(GetSkusStateCallback callback) override;
  void GetVpnState(GetVpnStateCallback callback) override;
  void ResetSkusState() override;
  void CopySkusStateToClipboard() override;
  void DownloadSkusState() override;

  std::string GetLastVPNConnectionError() const;
  base::Value::Dict GetVPNOrderInfo() const;
  std::string GetSkusStateAsString() const;

  raw_ptr<PrefService> local_state_ = nullptr;
  mojo::Receiver<skus::mojom::SkusInternals> skus_internals_receiver_{this};
};

#endif  // BRAVE_IOS_BROWSER_API_UI_WEBUI_SKUS_SKUS_INTERNALS_UI_H_
