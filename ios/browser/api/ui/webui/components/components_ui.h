// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_UI_WEBUI_COMPONENTS_UI_H_
#define BRAVE_IOS_BROWSER_API_UI_WEBUI_COMPONENTS_UI_H_

#include "ios/web/public/webui/web_ui_ios_controller.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ui/base/resource/resource_scale_factor.h"

namespace base {
class RefCountedMemory;
}    // namespace base


// MARK: - BASED ON: chrome/browser/ui/webui/components/components_ui.h

class ComponentsUI : public web::WebUIIOSController {
 public:
  explicit ComponentsUI(web::WebUIIOS* web_ui, const GURL& url);
  ComponentsUI(const ComponentsUI&) = delete;
  ComponentsUI& operator=(const ComponentsUI&) = delete;
  ~ComponentsUI() override;
  
  static base::RefCountedMemory* GetFaviconResourceBytes(
        ui::ResourceScaleFactor scale_factor);
};

#endif  // BRAVE_IOS_BROWSER_API_UI_WEBUI_COMPONENTS_UI_H_
