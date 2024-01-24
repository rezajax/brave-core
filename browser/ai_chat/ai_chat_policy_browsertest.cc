/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/ranges/algorithm.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/utils.h"
#include "brave/components/omnibox/browser/leo_provider.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/omnibox_controller.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

namespace policy {

class AIChatPolicyTest : public InProcessBrowserTest,
                         public ::testing::WithParamInterface<bool> {
 public:
  AIChatPolicyTest() = default;
  ~AIChatPolicyTest() override = default;

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    EXPECT_CALL(provider_, IsInitializationComplete(testing::_))
        .WillRepeatedly(testing::Return(true));
    BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
    PolicyMap policies;
    policies.Set(key::kBraveAIChatEnabled, POLICY_LEVEL_MANDATORY,
                 POLICY_SCOPE_USER, POLICY_SOURCE_PLATFORM,
                 base::Value(IsAIChatEnabledTest()), nullptr);
    provider_.UpdateChromePolicy(policies);
  }

  void SetUpOnMainThread() override {
    ui_test_utils::BrowserChangeObserver tor_browser_creation_observer(
        nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
    brave::NewOffTheRecordWindowTor(browser());
    tor_browser_ = tor_browser_creation_observer.Wait();
    ASSERT_TRUE(tor_browser_);
  }

  bool IsAIChatEnabledTest() { return GetParam(); }

  content::WebContents* web_contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::BrowserContext* browser_context() {
    return web_contents()->GetBrowserContext();
  }

  Profile* profile() { return browser()->profile(); }

  Profile* tor_profile() { return tor_browser_->profile(); }

  PrefService* prefs() { return user_prefs::UserPrefs::Get(browser_context()); }

  sidebar::SidebarModel* sidebar_model() const {
    return static_cast<BraveBrowser*>(browser())->sidebar_controller()->model();
  }

  AutocompleteController* GetAutocompleteController() {
    OmniboxView* omnibox =
        browser()->window()->GetLocationBar()->GetOmniboxView();
    return omnibox->controller()->autocomplete_controller();
  }

 protected:
  base::test::ScopedFeatureList feature_list_{ai_chat::features::kAIChat};
  raw_ptr<Browser> tor_browser_ = nullptr;

 private:
  MockConfigurationPolicyProvider provider_;
};

class AIChatPolicyTestFeatureDisabled : public AIChatPolicyTest {
 public:
  AIChatPolicyTestFeatureDisabled() = default;
  ~AIChatPolicyTestFeatureDisabled() override = default;

  void SetUpInProcessBrowserTestFixture() override {
    AIChatPolicyTest::SetUpInProcessBrowserTestFixture();
    feature_list_.Reset();
    feature_list_.InitAndDisableFeature(ai_chat::features::kAIChat);
  }
};

// Testing AIChatEnabled by policy with AIChat feature disabled.
IN_PROC_BROWSER_TEST_P(AIChatPolicyTestFeatureDisabled, IsAIChatEnabled) {
  ASSERT_TRUE(prefs()->FindPreference(ai_chat::prefs::kEnabledByPolicy));
  ASSERT_TRUE(prefs()->IsManagedPreference(ai_chat::prefs::kEnabledByPolicy));
  EXPECT_FALSE(ai_chat::IsAIChatEnabled(profile()));
  EXPECT_FALSE(ai_chat::IsAIChatEnabled(tor_profile()));
  EXPECT_FALSE(ai_chat::IsAIChatEnabled(prefs()));
}

// Testing AIChatEnabled by policy with AIChat feature enabled.
IN_PROC_BROWSER_TEST_P(AIChatPolicyTest, IsAIChatEnabled) {
  ASSERT_TRUE(prefs()->FindPreference(ai_chat::prefs::kEnabledByPolicy));
  ASSERT_TRUE(prefs()->IsManagedPreference(ai_chat::prefs::kEnabledByPolicy));
  if (IsAIChatEnabledTest()) {
    EXPECT_TRUE(prefs()->GetBoolean(ai_chat::prefs::kEnabledByPolicy));
    EXPECT_TRUE(ai_chat::IsAIChatEnabled(profile()));
    EXPECT_FALSE(ai_chat::IsAIChatEnabled(tor_profile()));
    EXPECT_TRUE(ai_chat::IsAIChatEnabled(prefs()));
  } else {
    EXPECT_FALSE(prefs()->GetBoolean(ai_chat::prefs::kEnabledByPolicy));
    EXPECT_FALSE(ai_chat::IsAIChatEnabled(profile()));
    EXPECT_FALSE(ai_chat::IsAIChatEnabled(tor_profile()));
    EXPECT_FALSE(ai_chat::IsAIChatEnabled(prefs()));
  }
}

// Testing if AIChat item exists in sidebar.
IN_PROC_BROWSER_TEST_P(AIChatPolicyTest, SidebarCheck) {
  bool is_in_sidebar = base::ranges::any_of(
      sidebar_model()->GetAllSidebarItems(), [](const auto& item) {
        return item.built_in_item_type ==
               sidebar::SidebarItem::BuiltInItemType::kChatUI;
      });
  if (IsAIChatEnabledTest()) {
    EXPECT_TRUE(is_in_sidebar);
  } else {
    EXPECT_FALSE(is_in_sidebar);
  }
}

// Testing if we insert LeoProvider
IN_PROC_BROWSER_TEST_P(AIChatPolicyTest, Autocomplete) {
  auto* autocomplete_controller = GetAutocompleteController();
  const auto& providers = autocomplete_controller->providers();
  bool is_in_providers =
      base::ranges::any_of(providers, [](const auto& provider) {
        return provider->type() == AutocompleteProvider::TYPE_BRAVE_LEO;
      });
  if (IsAIChatEnabledTest()) {
    EXPECT_TRUE(is_in_providers);
  } else {
    EXPECT_FALSE(is_in_providers);
  }
}

INSTANTIATE_TEST_SUITE_P(
    All,
    AIChatPolicyTest,
    ::testing::Bool(),
    [](const testing::TestParamInfo<AIChatPolicyTest::ParamType>& info) {
      return base::StringPrintf("AIChat_%sByPolicy",
                                info.param ? "Enabled" : "NotEnabled");
    });

INSTANTIATE_TEST_SUITE_P(
    All,
    AIChatPolicyTestFeatureDisabled,
    ::testing::Bool(),
    [](const testing::TestParamInfo<AIChatPolicyTest::ParamType>& info) {
      return base::StringPrintf("AIChat_%sByPolicy",
                                info.param ? "Enabled" : "NotEnabled");
    });

}  // namespace policy
