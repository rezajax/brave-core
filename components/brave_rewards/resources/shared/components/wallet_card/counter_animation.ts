/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

function ease (n: number) {
  return 1 - Math.pow(1 - n, 3)
}

export function useCounterAnimation (targetValue: number, duration: number) {
  duration = Math.max(1, duration)

  const [value, setValue] = React.useState(0)

  React.useEffect(() => {
    if (value === targetValue) {
      return
    }
    const start = Date.now()
    let handle = 0
    const onAnimationFrame = () => {
      const elapsed = Date.now() - start
      if (elapsed < duration) {
        setValue(ease(elapsed / duration) * targetValue)
        handle = requestAnimationFrame(onAnimationFrame)
      } else {
        setValue(targetValue)
      }
    }
    handle = requestAnimationFrame(onAnimationFrame)
    return () => cancelAnimationFrame(handle)
  }, [targetValue])

  return value
}
