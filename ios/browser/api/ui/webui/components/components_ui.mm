// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/ui/webui/components/components_ui.h"
//#include "brave/ios/browser/ui/webui/brave_webui_source.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/values.h"
#include "build/build_config.h"

#include "base/memory/ref_counted_memory.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "brave/ios/browser/api/ui/webui/components/components_handler.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"

#include "components/component_updater/component_updater_service.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#import "ios/web/public/webui/web_ui_ios.h"
#import "ios/web/public/webui/web_ui_ios_data_source.h"
#import "ios/web/public/webui/web_ui_ios_message_handler.h"
#import "ios/web/public/web_state.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/webui/web_ui_util.h"

//#include "chrome/common/chrome_paths.h"
//#include "chrome/common/url_constants.h"
#include "ui/base/webui/resource_path.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"

#include "ios/chrome/grit/ios_resources.h"
#include "ios/chrome/grit/ios_strings.h"

namespace chrome {
const char kChromeUIComponentsHost[] = "components";
}    // namespace chrome

namespace {

void CreateAndAddComponentsUIHTMLSource(ChromeBrowserState* browser_state) {
  web::WebUIIOSDataSource* source = web::WebUIIOSDataSource::Create(chrome::kChromeUIComponentsHost);
  web::WebUIIOSDataSource::Add(browser_state, source);

  static constexpr webui::LocalizedString kStrings[] = {
    {"componentsTitle", IDS_COMPONENTS_TITLE},
    {"componentsNoneInstalled", IDS_COMPONENTS_NONE_INSTALLED},
    {"componentVersion", IDS_COMPONENTS_VERSION},
    {"checkUpdate", IDS_COMPONENTS_CHECK_FOR_UPDATE},
    {"noComponents", IDS_COMPONENTS_NO_COMPONENTS},
    {"statusLabel", IDS_COMPONENTS_STATUS_LABEL},
    {"checkingLabel", IDS_COMPONENTS_CHECKING_LABEL},
  };
  
  source->AddLocalizedStrings(kStrings);

  source->AddBoolean(
      "isGuest",
          browser_state->IsOffTheRecord()
  );
  
  const webui::ResourcePath kComponentsResources[] = {
    {"components.html", IDR_COMPONENTS_COMPONENTS_HTML},
    {"components.css", IDR_COMPONENTS_COMPONENTS_CSS},
    {"components.js", IDR_COMPONENTS_COMPONENTS_JS},
  };

  const size_t kComponentsResourcesSize = std::size(kComponentsResources);
  
  source->UseStringsJs();
  source->AddResourcePaths(
      base::make_span(kComponentsResources, kComponentsResourcesSize));
  source->SetDefaultResource(IDR_COMPONENTS_COMPONENTS_HTML);
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
//
// ComponentsUI
//
///////////////////////////////////////////////////////////////////////////////

ComponentsUI::ComponentsUI(web::WebUIIOS* web_ui,
                           const GURL& url) : web::WebUIIOSController(web_ui, url.host()) {
  
  web_ui->AddMessageHandler(std::make_unique<ComponentsHandler>(
        GetApplicationContext()->GetComponentUpdateService()));
  
  // Set up the chrome://components/ source.
  CreateAndAddComponentsUIHTMLSource(ChromeBrowserState::FromWebUIIOS(web_ui));
}

ComponentsUI::~ComponentsUI() {}
