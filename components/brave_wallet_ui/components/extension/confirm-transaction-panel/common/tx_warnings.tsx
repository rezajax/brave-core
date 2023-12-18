// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/key-spacing */

import * as React from 'react'

// types
import { BraveWallet } from '../../../../constants/types'

// utils
import { getLocale } from '../../../../../common/locale'

// components
import { TxWarningBanner } from './tx_warning_banner'

// styles
import { FullWidth } from '../../../shared/style'
import {
  WarningCollapse,
  WarningTitle,
  WarningsList
} from './tx_warnings.styles'

export function TransactionWarnings({
  isWarningCollapsed,
  setIsWarningCollapsed,
  warnings
}: {
  warnings: Array<Pick<BraveWallet.BlowfishWarning, 'message' | 'severity'>>
  isWarningCollapsed: boolean
  setIsWarningCollapsed?: React.Dispatch<React.SetStateAction<boolean>>
}): JSX.Element | null {
  // memos
  const hasCriticalWarnings = warnings.some(
    (warning) =>
      warning.severity === BraveWallet.BlowfishWarningSeverity.kCritical
  )

  // no warnings
  if (warnings.length < 1) {
    return null
  }

  // 1 warning
  if (warnings.length === 1) {
    return (
      <FullWidth>
        <TxWarningBanner isCritical={hasCriticalWarnings}>
          {warnings[0].message}
        </TxWarningBanner>
      </FullWidth>
    )
  }

  // multiple warnings
  return (
    <FullWidth>
      <WarningCollapse
        isOpen={!isWarningCollapsed}
        isCritical={hasCriticalWarnings}
        onToggle={
          setIsWarningCollapsed
            ? () => setIsWarningCollapsed((prev) => !prev)
            : undefined
        }
      >
        <WarningTitle
          slot='title'
          isCritical={hasCriticalWarnings}
        >
          {hasCriticalWarnings
            ? getLocale('braveWalletRiskOfLossAction')
            : getLocale('braveWalletFoundIssues').replace(
                '$1',
                warnings.length.toString()
              )}
        </WarningTitle>

        <WarningsList>
          {warnings.map((warning) => (
            <li key={warning.message}>{warning.message}</li>
          ))}
        </WarningsList>
      </WarningCollapse>
    </FullWidth>
  )
}
