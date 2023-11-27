// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { mockSignMessageRequest } from '../../../../stories/mock-data/mock-eth-requests'
import { EthSignTypedData } from './eth_sign_typed_data'

export function _EthSignTypedData() {
  return (
    <EthSignTypedData
      data={mockSignMessageRequest.signData.ethSignTypedData}
      width={'100%'}
      height={'100%'}
    />
  )
}

export default _EthSignTypedData
