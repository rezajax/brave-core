// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/constellation_helper.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/time/time.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/p3a_message.h"
#include "brave/components/p3a/star_randomness_test_util.h"
#include "brave/components/p3a/uploader.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace p3a {

namespace {

constexpr uint8_t kTestSlowEpoch = 2;
constexpr uint8_t kTestTypicalEpoch = 5;
constexpr uint8_t kTestExpressEpoch = 7;
constexpr char kTestSlowNextEpochTime[] = "2086-06-22T18:00:00Z";
constexpr char kTestTypicalNextEpochTime[] = "2086-06-24T18:00:00Z";
constexpr char kTestExpressNextEpochTime[] = "2086-07-01T18:00:00Z";
constexpr char kTestHistogramName[] = "Brave.Test.Histogram";
constexpr char kTestHost[] = "https://localhost:8443";

}  // namespace

class P3AConstellationHelperTest : public testing::Test {
 public:
  P3AConstellationHelperTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

 protected:
  void SetUp() override {
    p3a_config_.disable_star_attestation = true;
    p3a_config_.star_randomness_host = kTestHost;

    ConstellationHelper::RegisterPrefs(local_state_.registry());

    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();

          MetricLogType log_type =
              ValidateURLAndGetMetricLogType(request.url, kTestHost);

          std::string response;
          if (base::EndsWith(request.url.spec(), "/info")) {
            response =
                HandleInfoRequest(request, log_type, GetTestEpoch(log_type),
                                  GetTestNextEpochTime(log_type));
            info_request_made_[log_type] = true;
          } else if (base::EndsWith(request.url.spec(), "/randomness")) {
            response = HandleRandomnessRequest(request, GetTestEpoch(log_type));
            points_request_made_[log_type] = true;
          }
          if (!response.empty()) {
            if (interceptor_send_bad_response_) {
              response = "this is a bad response that is not json";
            }
            url_loader_factory_.AddResponse(request.url.spec(), response);
          }
        }));
  }

  void SetUpHelper() {
    server_info_from_callback_ = nullptr;

    helper_ = std::make_unique<ConstellationHelper>(
        &local_state_, shared_url_loader_factory_,
        base::BindLambdaForTesting(
            [this](std::string histogram_name, MetricLogType log_type,
                   uint8_t epoch,
                   std::unique_ptr<std::string> serialized_message) {
              histogram_name_from_callback_ = histogram_name;
              epoch_from_callback_ = epoch;
              serialized_message_from_callback_ = std::move(serialized_message);
            }),
        base::BindLambdaForTesting(
            [this](MetricLogType log_type, RandomnessServerInfo* server_info) {
              server_info_from_callback_ = server_info;
            }),
        &p3a_config_);
    task_environment_.RunUntilIdle();
  }

  uint8_t GetTestEpoch(MetricLogType log_type) {
    switch (log_type) {
      case MetricLogType::kSlow:
        return kTestSlowEpoch;
      case MetricLogType::kTypical:
        return kTestTypicalEpoch;
      case MetricLogType::kExpress:
        return kTestExpressEpoch;
    }
    NOTREACHED();
  }

  const char* GetTestNextEpochTime(MetricLogType log_type) {
    switch (log_type) {
      case MetricLogType::kSlow:
        return kTestSlowNextEpochTime;
      case MetricLogType::kTypical:
        return kTestTypicalNextEpochTime;
      case MetricLogType::kExpress:
        return kTestExpressNextEpochTime;
    }
    NOTREACHED();
  }

  void CheckInfoRequestMade(MetricLogType log_type) {
    for (MetricLogType check_log_type : kAllMetricLogTypes) {
      if (check_log_type == log_type) {
        ASSERT_TRUE(info_request_made_[check_log_type]);
      } else {
        ASSERT_FALSE(info_request_made_[check_log_type]);
      }
    }
  }

  void CheckPointsRequestMade(MetricLogType log_type) {
    for (MetricLogType check_log_type : kAllMetricLogTypes) {
      if (check_log_type == log_type) {
        ASSERT_TRUE(points_request_made_[check_log_type]);
      } else {
        ASSERT_FALSE(points_request_made_[check_log_type]);
      }
    }
  }

  content::BrowserTaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  P3AConfig p3a_config_;
  std::unique_ptr<ConstellationHelper> helper_;
  TestingPrefServiceSimple local_state_;
  bool interceptor_send_bad_response_ = false;

  raw_ptr<RandomnessServerInfo> server_info_from_callback_ = nullptr;
  std::unique_ptr<std::string> serialized_message_from_callback_;

  std::string histogram_name_from_callback_;
  uint8_t epoch_from_callback_;

  base::flat_map<MetricLogType, bool> info_request_made_;
  base::flat_map<MetricLogType, bool> points_request_made_;
};

TEST_F(P3AConstellationHelperTest, CanRetrieveServerInfo) {
  SetUpHelper();
  for (MetricLogType log_type : kAllMetricLogTypes) {
    uint8_t test_epoch = GetTestEpoch(log_type);
    base::Time exp_next_epoch_time;

    ASSERT_TRUE(base::Time::FromString(GetTestNextEpochTime(log_type),
                                       &exp_next_epoch_time));

    helper_->UpdateRandomnessServerInfo(log_type);
    task_environment_.RunUntilIdle();

    CheckInfoRequestMade(log_type);

    EXPECT_NE(server_info_from_callback_, nullptr);
    EXPECT_EQ(server_info_from_callback_->current_epoch, test_epoch);

    EXPECT_EQ(server_info_from_callback_->next_epoch_time, exp_next_epoch_time);

    // See if cached server info is used on next execution
    info_request_made_[log_type] = false;
    server_info_from_callback_ = nullptr;

    SetUpHelper();
    helper_->UpdateRandomnessServerInfo(log_type);
    task_environment_.RunUntilIdle();

    ASSERT_FALSE(info_request_made_[log_type]);
    EXPECT_NE(server_info_from_callback_, nullptr);
    EXPECT_EQ(server_info_from_callback_->current_epoch, test_epoch);
    EXPECT_EQ(server_info_from_callback_->next_epoch_time, exp_next_epoch_time);
    server_info_from_callback_ = nullptr;
  }
}

TEST_F(P3AConstellationHelperTest, CannotRetrieveServerInfo) {
  SetUpHelper();
  for (MetricLogType log_type : kAllMetricLogTypes) {
    base::Time exp_next_epoch_time;
    ASSERT_TRUE(base::Time::FromString(GetTestNextEpochTime(log_type),
                                       &exp_next_epoch_time));

    interceptor_send_bad_response_ = true;

    helper_->UpdateRandomnessServerInfo(log_type);
    task_environment_.RunUntilIdle();

    CheckInfoRequestMade(log_type);

    // callback should not be executed if info retrieval failed
    EXPECT_EQ(server_info_from_callback_, nullptr);

    // See if info retrieval retry is scheduled
    info_request_made_[log_type] = false;
    task_environment_.FastForwardBy(base::Seconds(6));

    ASSERT_TRUE(info_request_made_[log_type]);
    EXPECT_EQ(server_info_from_callback_, nullptr);
    info_request_made_[log_type] = false;
  }
}

TEST_F(P3AConstellationHelperTest, GenerateBasicMessage) {
  SetUpHelper();
  for (MetricLogType log_type : kAllMetricLogTypes) {
    uint8_t test_epoch = GetTestEpoch(log_type);

    helper_->UpdateRandomnessServerInfo(log_type);
    task_environment_.RunUntilIdle();

    MessageMetainfo meta_info;
    meta_info.Init(&local_state_, "release", "2022-01-01");

    helper_->StartMessagePreparation(
        kTestHistogramName, log_type,
        GenerateP3AConstellationMessage(kTestHistogramName, test_epoch,
                                        meta_info, kP3AUploadType));
    task_environment_.RunUntilIdle();

    CheckPointsRequestMade(log_type);

    EXPECT_NE(serialized_message_from_callback_, nullptr);
    EXPECT_NE(serialized_message_from_callback_->size(), 0U);

    EXPECT_EQ(histogram_name_from_callback_, kTestHistogramName);
    EXPECT_EQ(epoch_from_callback_, test_epoch);
    points_request_made_[log_type] = false;
  }
}

}  // namespace p3a
