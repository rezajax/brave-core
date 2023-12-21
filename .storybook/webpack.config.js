// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const path = require('path')
const webpack = require('webpack')
const fs = require('fs')
const config = require('../build/commands/lib/config')
const genTsConfig = require('../build/commands/lib/genTsConfig')
const { fallback, provideNodeGlobals } = require('../components/webpack/polyfill')
const generatePathMap = require('../components/webpack/path-map')

// Choose which brave-core build directory to look for pre-compiled
// resource dependencies:
// 1. Default for local builds for the actual platform / architecture
// 2. platform / architecture overriden by environment variables
// 3. most recently built - this caters to the common scenario when a
// non-standard target has been built but no arguments are provided to storybook
config.update({
  target_arch: process.env.TARGET_ARCH,
  target_os: process.env.TARGET_OS,
  target_environment: process.env.TARGET_ENVIRONMENT,
  target: process.env.TARGET
})

let outputPath = config.outputDir

function getBuildOuptutPathList() {
  return buildConfigs.flatMap((config) => [
    path.resolve(__dirname, `../../out/${config}`),
    ...extraArchitectures.map((arch) =>
      path.resolve(
        __dirname,
        `../../out/${config}_${arch}`
      )
    )
  ])
}

if (fs.existsSync(config.outputDir)) {
  console.log('Assuming precompiled dependencies can be found at the existing path found from brave-core configuration: ' + outputPath)
} else {
  const outDirectories = getBuildOuptutPathList()
    .filter(a => fs.existsSync(a))
    .sort((a, b) => fs.statSync(b).mtime - fs.statSync(a).mtime)
  if (!outDirectories.length) {
    throw new Error('Cannot find any brave-core build output directories. Have you run a brave-core build yet with the specified (or default) configuration?')
  }
  outputPath = outDirectories[0]
}

const genPath = path.join(outputPath, 'gen')

if (!fs.existsSync(genPath)) {
  throw new Error("Failed to find build output 'gen' folder! Have you run a brave-core build yet with the specified (or default) configuration?")
}
console.log(`Using brave-core generated dependency path of '${genPath}'`)

const basePathMap = generatePathMap(genPath)

// Override the path map we use in the browser with some additional mock
// directories, so that we can replace things in Storybook.
const pathMap = {
  ...basePathMap,
  'chrome://resources': [
    // As we mock some chrome://resources, insert our mock directory as the first
    // place to look.
    path.resolve(__dirname, 'chrome-resources-mock'),
    basePathMap['chrome://resources']
  ]
}

/**
 * Maps a prefix to a corresponding path. We need this as Webpack5 dropped
 * support for scheme prefixes (like chrome://)
 *
 * Note: This prefixReplacer is slightly different from the one we use in proper
 * builds, as some path maps have multiple possible locations (e.g. for mocks).
 *
 * @param {string} prefix The prefix
 * @param {string[] | string} replacements The real path options
 */
const prefixReplacer = (prefix, replacements) => {
  if (!Array.isArray(replacements)) replacements = [replacements]

  const regex = new RegExp(`^${prefix}/(.*)`)
  return new webpack.NormalModuleReplacementPlugin(regex, resource => {
    resource.request = resource.request.replace(regex, (_, subpath) => {
      if (!subpath) {
        throw new Error("Subpath is undefined")
      }

      const match = replacements.find((dir) => fs.existsSync(path.join(dir, subpath))) ?? replacements[0]
      const result = path.join(match, subpath)
      return result
    })
  })
}

// Export a function. Accept the base config as the only param.
module.exports = async ({ config, mode }) => {
  const tsConfigPath = await genTsConfig(genPath, 'tsconfig-storybook.json', genPath, path.resolve(__dirname, '../tsconfig-storybook.json'))
  console.log(`Using generated tsconfig path of '${tsConfigPath}'`)

  const isDevMode = mode === 'development'
  // Make whatever fine-grained changes you need
  config.module.rules.push(
    {
      test: /\.scss$/,
      include: [/\.global\./],
      use: [{ loader: 'style-loader' }, { loader: 'css-loader' }]
    },
    {
      test: /\.scss$/,
      exclude: [/\.global\./, /node_modules/],
      use: [
        { loader: 'style-loader' },
        {
          loader: 'css-loader',
          options: {
            importLoaders: 3,
            sourceMap: false,
            modules: {
              localIdentName: isDevMode
                ? '[path][name]__[local]--[hash:base64:5]'
                : '[hash:base64]'
            }
          }
        },
        { loader: 'sass-loader' }
      ]
    },
    {
      test: /\.(ts|tsx)$/,
      loader: 'ts-loader',
      options: {
        configFile: tsConfigPath,
        getCustomTransformers: path.join(
          __dirname,
          '../components/webpack/webpack-ts-transformers.js'
        )
      }
    },
    {
      test: /\.avif$/,
      loader: 'file-loader'
    }
  )

  config.resolve.alias = pathMap
  config.resolve.fallback = fallback

  config.plugins.push(provideNodeGlobals,
    ...Object.keys(pathMap)
      .filter(prefix => prefix.startsWith('chrome://'))
      .map(prefix => prefixReplacer(prefix, pathMap[prefix])))
  config.resolve.extensions.push('.ts', '.tsx', '.scss')
  return config
}
