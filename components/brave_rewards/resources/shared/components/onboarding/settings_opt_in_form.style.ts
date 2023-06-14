/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css'

export const root = styled.div`
  height: 100%;
  width: 100%;
  background-color: ${leo.color.white};
  border-radius: 4px;
  font-family: var(--brave-font-heading);
  text-align: center;
  padding: 44px 128px;
`

export const icon = styled.div`
  .icon {
    height: 190px;
    width: 198px;
  }
`

export const heading = styled.div`
  margin-top: 32px;
  margin-left: auto;
  margin-right: auto;
  max-width: 339px;
  font-weight: 500;
  font-size: 22px;
  line-height: 32px;
  color: ${leo.color.light.text.primary};
`

export const text = styled.div`
  margin-top: 16px;
  margin-left: auto;
  margin-right: auto;
  max-width: 339px;
  color: ${leo.color.light.text.secondary};
  font-weight: 500;
  font-size: 14px;
  line-height: 24px;
`

export const enable = styled.div`
  margin-top: 32px;

  button {
    max-width: 339px;
    color: ${leo.color.white};
    background: ${leo.color.light.interaction.buttonPrimaryBackground};
    border-radius: 48px;
    padding: 12px 24px;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    letter-spacing: 0.03em;
  }
`

export const learnMore = styled.div`
  margin-top: 20px;
  margin-bottom: 8px;
  text-align: center;

  a {
    color: ${leo.color.light.text.interactive};
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    letter-spacing: 0.03em;
    text-decoration: none;
  }
`
