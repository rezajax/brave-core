// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_INTERNALS_UI_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_INTERNALS_UI_H_

#include <string>

#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"

// The WebUI for brave://adblock-internals
class BraveAdblockInternalsUI : public web::WebUIIOSController {
 public:
  BraveAdblockInternalsUI(web::WebUIIOS* web_ui, const std::string& host);

  BraveAdblockInternalsUI(const BraveAdblockInternalsUI&) = delete;
  BraveAdblockInternalsUI& operator=(const BraveAdblockInternalsUI&) = delete;

  ~BraveAdblockInternalsUI() override;
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_ADBLOCK_INTERNALS_UI_H_
