// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { Redirect, useHistory, useParams } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query/react'

// types
import {
  BraveWallet,
  WalletRoutes,
  TokenPriceHistory,
  LineChartIframeData
} from '../../../../constants/types'

// Utils
import { getPriceIdForToken } from '../../../../utils/api-utils'
import { getAssetIdKey } from '../../../../utils/asset-utils'
import { getLocale } from '../../../../../common/locale'
import {
  makeDepositFundsRoute,
  makeFundWalletRoute
} from '../../../../utils/routes-utils'
import { getIsRewardsToken } from '../../../../utils/rewards_utils'
import {
  getStoredPortfolioTimeframe //
} from '../../../../utils/local-storage-utils'
import Amount from '../../../../utils/amount'
import { getBalance } from '../../../../utils/balance-utils'
import { networkSupportsAccount } from '../../../../utils/network-utils'
import { computeFiatAmount } from '../../../../utils/pricing-utils'
import {
  querySubscriptionOptions60s //
} from '../../../../common/slices/constants'

// actions
import { WalletPageActions } from '../../../../page/actions'
import {
  WalletActions //
} from '../../../../common/actions'

// selectors
import { WalletSelectors } from '../../../../common/selectors'

// Components
import {
  LineChartControls //
} from '../../line-chart/line-chart-controls/line-chart-controls'
import { AssetDetailsHeader } from '../../card-headers/asset-details-header'
import {
  TokenDetailsModal //
} from '../portfolio/components/token-details-modal/token-details-modal'
import {
  HideTokenModal //
} from '../portfolio/components/hide-token-modal/hide-token-modal'
import { CoinStats } from '../portfolio/components/coin-stats/coin-stats'

// Hooks
import {
  useIsBuySupported //
} from '../../../../common/hooks/use-multi-chain-buy-assets'
import {
  useSafeWalletSelector //
} from '../../../../common/hooks/use-safe-selector'
import {
  useGetNetworkQuery,
  useGetPriceHistoryQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetCoinMarketQuery,
  useGetTokenSpotPricesQuery
} from '../../../../common/slices/api.slice'
import {
  useAccountsQuery,
  useGetCombinedTokensListQuery
} from '../../../../common/slices/api.slice.extra'
import {
  useScopedBalanceUpdater //
} from '../../../../common/hooks/use-scoped-balance-updater'

// Styled Components
import { Row, Column } from '../../../shared/style'
import { Skeleton } from '../../../shared/loading-skeleton/styles'
import {
  WalletPageWrapper //
} from '../../wallet-page-wrapper/wallet-page-wrapper'
import {
  BuySellBridgeButton,
  ButtonRow,
  StyledWrapper
} from '../portfolio/style'

const emptyPriceList: TokenPriceHistory[] = []

export const MarketAsset = () => {
  // state
  const [showTokenDetailsModal, setShowTokenDetailsModal] =
    React.useState<boolean>(false)
  const [showHideTokenModel, setShowHideTokenModal] =
    React.useState<boolean>(false)
  const [selectedTimeline, setSelectedTimeline] = React.useState<number>(
    getStoredPortfolioTimeframe
  )

  // routing
  const history = useHistory()
  const { coingeckoId } = useParams<{
    coingeckoId?: string
  }>()
  const coingeckoIdLower = coingeckoId?.toLowerCase()

  // redux
  const dispatch = useDispatch()
  const hidePortfolioBalances = useSafeWalletSelector(
    WalletSelectors.hidePortfolioBalances
  )

  // Queries
  const { data: combinedTokensList } = useGetCombinedTokensListQuery()
  const { data: defaultFiat = 'USD' } = useGetDefaultFiatCurrencyQuery()
  const { data: coinMarketData = [] } = useGetCoinMarketQuery({
    limit: 250,
    vsAsset: defaultFiat
  })

  // Params
  const selectedCoinMarket = React.useMemo(() => {
    if (!coingeckoIdLower) {
      return undefined
    }

    return coinMarketData.find(
      (token) => token.id.toLowerCase() === coingeckoIdLower
    )
  }, [coinMarketData, coingeckoIdLower])

  const { selectedAssetFromParams, foundTokens } = React.useMemo(() => {
    if (!selectedCoinMarket) {
      return { selectedAssetFromParams: undefined, foundTokens: [] }
    }

    const marketSymbolLower = selectedCoinMarket.symbol.toLowerCase()

    const foundTokens = combinedTokensList.filter(
      (t) => t.symbol.toLowerCase() === marketSymbolLower
    )

    if (foundTokens.length) {
      return {
        selectedAssetFromParams:
          foundTokens.find((t) => t.coingeckoId === selectedCoinMarket.id) ||
          foundTokens[0],
        foundTokens
      }
    }

    const token = new BraveWallet.BlockchainToken()
    token.coingeckoId = selectedCoinMarket.id
    token.name = selectedCoinMarket.name
    token.contractAddress = ''
    token.symbol = selectedCoinMarket.symbol.toUpperCase()
    token.logo = selectedCoinMarket.image
    return { selectedAssetFromParams: token, foundTokens }
  }, [selectedCoinMarket, combinedTokensList, selectedTimeline])

  const isRewardsToken = getIsRewardsToken(selectedAssetFromParams)

  // queries
  const { accounts } = useAccountsQuery()
  const { data: selectedAssetsNetwork } = useGetNetworkQuery(
    selectedAssetFromParams ?? skipToken
  )

  const assetPriceId = selectedAssetFromParams
    ? getPriceIdForToken(selectedAssetFromParams)
    : undefined

  const {
    data: selectedAssetPriceHistory,
    isFetching: isFetchingPortfolioPriceHistory
  } = useGetPriceHistoryQuery(
    assetPriceId && defaultFiat
      ? {
          tokenParam: assetPriceId,
          timeFrame: selectedTimeline,
          vsAsset: defaultFiat
        }
      : skipToken
  )

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    assetPriceId && defaultFiat
      ? {
          ids: [assetPriceId],
          toCurrency: defaultFiat
        }
      : skipToken,
    querySubscriptionOptions60s
  )

  // custom hooks
  const isAssetBuySupported =
    useIsBuySupported(selectedAssetFromParams) && !isRewardsToken

  // memos / computed
  const isLoadingGraphData =
    !selectedAssetFromParams || isFetchingPortfolioPriceHistory

  const isSelectedAssetDepositSupported = React.useMemo(() => {
    if (!selectedAssetFromParams || isRewardsToken) {
      return false
    }

    return foundTokens.length > 0
  }, [selectedAssetFromParams, isRewardsToken, foundTokens])

  const candidateAccounts = React.useMemo(() => {
    if (!selectedAssetsNetwork) {
      return []
    }

    return accounts.filter((account) =>
      networkSupportsAccount(selectedAssetsNetwork, account.accountId)
    )
  }, [selectedAssetsNetwork, accounts])

  const { data: tokenBalancesRegistry } = useScopedBalanceUpdater(
    selectedAssetFromParams && candidateAccounts.length && selectedAssetsNetwork
      ? {
          network: selectedAssetsNetwork,
          accounts: candidateAccounts,
          tokens: [selectedAssetFromParams]
        }
      : skipToken
  )

  /**
   * This will scrape all the user's accounts and combine the asset balances for
   * a single asset
   */
  const fullAssetBalance = React.useMemo(() => {
    const amounts = candidateAccounts.map((account) =>
      getBalance(
        account.accountId,
        selectedAssetFromParams,
        tokenBalancesRegistry
      )
    )

    // If a user has not yet created a FIL or SOL account,
    // we return 0 until they create an account
    if (amounts.length === 0) {
      return '0'
    }

    return amounts.reduce(function (a, b) {
      return a !== '' && b !== '' ? new Amount(a).plus(b).format() : ''
    })
  }, [candidateAccounts, selectedAssetFromParams, tokenBalancesRegistry])

  const fullAssetFiatBalance = React.useMemo(
    () =>
      selectedAssetFromParams && fullAssetBalance
        ? computeFiatAmount({
            spotPriceRegistry,
            value: fullAssetBalance,
            token: selectedAssetFromParams
          })
        : Amount.empty(),
    [selectedAssetFromParams, fullAssetBalance, spotPriceRegistry]
  )

  // methods
  const goBack = React.useCallback(() => {
    dispatch(WalletPageActions.updateNFTMetadata(undefined))
    dispatch(WalletPageActions.updateNftMetadataError(undefined))
    history.push(WalletRoutes.Market)
  }, [])

  const onCloseTokenDetailsModal = React.useCallback(
    () => setShowTokenDetailsModal(false),
    []
  )

  const onCloseHideTokenModal = React.useCallback(
    () => setShowHideTokenModal(false),
    []
  )

  const onHideAsset = React.useCallback(() => {
    if (!selectedAssetFromParams) return
    dispatch(
      WalletActions.setUserAssetVisible({
        token: selectedAssetFromParams,
        isVisible: false
      })
    )
    dispatch(WalletActions.refreshBalancesAndPriceHistory())
    setShowHideTokenModal(false)
    setShowTokenDetailsModal(false)
    history.push(WalletRoutes.PortfolioAssets)
  }, [selectedAssetFromParams])

  const onSelectBuy = React.useCallback(() => {
    if (foundTokens.length === 1) {
      history.push(
        makeFundWalletRoute(getAssetIdKey(foundTokens[0]), {
          searchText: foundTokens[0].symbol
        })
      )
      return
    }

    if (foundTokens.length > 1) {
      history.push(
        makeFundWalletRoute('', {
          searchText: foundTokens[0].symbol
        })
      )
      return
    }

    if (selectedAssetFromParams) {
      history.push(
        makeFundWalletRoute(getAssetIdKey(selectedAssetFromParams), {
          searchText: selectedAssetFromParams.symbol
        })
      )
    }
  }, [foundTokens, selectedAssetFromParams])

  const onSelectDeposit = React.useCallback(() => {
    if (foundTokens.length === 1) {
      history.push(
        makeDepositFundsRoute(getAssetIdKey(foundTokens[0]), {
          searchText: foundTokens[0].symbol
        })
      )
      return
    }

    if (foundTokens.length > 1) {
      history.push(
        makeDepositFundsRoute('', {
          searchText: foundTokens[0].symbol
        })
      )
      return
    }

    if (selectedAssetFromParams) {
      history.push(
        makeFundWalletRoute('', {
          searchText: selectedAssetFromParams.symbol
        })
      )
    }
  }, [foundTokens, selectedAssetFromParams])

  // token list & market data needs to load before we can find an asset to
  // select from the url params
  if (combinedTokensList.length === 0 || coinMarketData.length === 0) {
    return <Skeleton />
  }

  // asset not found
  if (!selectedAssetFromParams) {
    return <Redirect to={WalletRoutes.PortfolioAssets} />
  }

  const priceData =
    selectedAssetFromParams && selectedAssetPriceHistory
      ? selectedAssetPriceHistory
      : emptyPriceList

  const iframeData: LineChartIframeData = {
    priceData,
    hidePortfolioBalances,
    defaultFiatCurrency: defaultFiat || 'USD'
  }

  const encodedPriceData = encodeURIComponent(JSON.stringify(iframeData))

  // render
  return (
    <WalletPageWrapper
      wrapContentInBox={true}
      noCardPadding={true}
      hideDivider={true}
      cardHeader={
        <AssetDetailsHeader
          selectedTimeline={selectedTimeline}
          selectedAsset={selectedAssetFromParams}
          isShowingMarketData={true}
          onBack={goBack}
          onClickTokenDetails={() => setShowTokenDetailsModal(true)}
          onClickHideToken={() => setShowHideTokenModal(true)}
        />
      }
    >
      <StyledWrapper>
        <Row margin='20px 0px 8px 0px'>
          <LineChartControls
            onSelectTimeline={setSelectedTimeline}
            selectedTimeline={selectedTimeline}
          />
        </Row>

        <iframe
          width={'100%'}
          height={'130px'}
          frameBorder={0}
          src={`chrome-untrusted://line-chart-display${
            isLoadingGraphData ? '' : `?${encodedPriceData}`
          }`}
          sandbox='allow-scripts'
        />
        <Row padding='0px 20px'>
          <ButtonRow>
            {isAssetBuySupported && (
              <BuySellBridgeButton
                onClick={onSelectBuy}
                noBottomMargin={true}
              >
                {getLocale('braveWalletBuy')}
              </BuySellBridgeButton>
            )}
            {isSelectedAssetDepositSupported && (
              <BuySellBridgeButton
                onClick={onSelectDeposit}
                noBottomMargin={true}
              >
                {getLocale('braveWalletAccountsDeposit')}
              </BuySellBridgeButton>
            )}
          </ButtonRow>
        </Row>
        {showTokenDetailsModal &&
          selectedAssetFromParams &&
          selectedAssetsNetwork && (
            <TokenDetailsModal
              onClose={onCloseTokenDetailsModal}
              selectedAsset={selectedAssetFromParams}
              selectedAssetNetwork={selectedAssetsNetwork}
              assetBalance={undefined}
              formattedFiatBalance={fullAssetFiatBalance.formatAsFiat(
                defaultFiat
              )}
              onShowHideTokenModal={() => setShowHideTokenModal(true)}
            />
          )}
        {showHideTokenModel &&
          selectedAssetFromParams &&
          selectedAssetsNetwork && (
            <HideTokenModal
              selectedAsset={selectedAssetFromParams}
              selectedAssetNetwork={selectedAssetsNetwork}
              onClose={onCloseHideTokenModal}
              onHideAsset={onHideAsset}
            />
          )}
        {selectedCoinMarket && (
          <Column
            padding='0px 20px 20px 20px'
            fullWidth={true}
          >
            <CoinStats
              marketCapRank={selectedCoinMarket.marketCapRank}
              volume={selectedCoinMarket.totalVolume}
              marketCap={selectedCoinMarket.marketCap}
            />
          </Column>
        )}
      </StyledWrapper>
    </WalletPageWrapper>
  )
}

export default MarketAsset
