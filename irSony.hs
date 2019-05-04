{-
 * Copyright (C) 2019  Peter Hercek (phercek@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
-}

import System.IO
import System.Exit
import System.Process
import System.Environment
import Data.Time.Clock

repInterval :: NominalDiffTime
repInterval = fromRational 0.3

main = do
  hSetBuffering stdout NoBuffering
  prgArgs <- getArgs
  fileName <-
    case prgArgs of
      []         -> return "/dev/irSony"
      "--help":_ -> do
                      putStrLn "Usage: irSony [<ttyDevice>]"
                      putStrLn "  The default device is /dev/irSony."
                      exitSuccess
      x:_        -> return x
  fileContent <- readFile fileName
  curTime <- getCurrentTime
  processLines (addUTCTime (-repInterval) curTime)  ("" : lines fileContent)

setVolume s = callProcess "/usr/bin/pactl" ["set-sink-volume", "0", s]

processLines prevTime (prevLine : curLines@(curLine:_)) = do
  curTime <- getCurrentTime
  let difTime = diffUTCTime curTime prevTime
  if difTime < repInterval && prevLine == curLine
    then processLines prevTime curLines
    else do
      case curLine of
        "1:18"  -> putChar '0'      -- volUp -> volUp
        "1:19"  -> putChar '9'      -- volDown -> volDown
        "11:59" -> putChar 'm'      -- effect -> mute
        "11:21" -> putChar 'p'      -- AMP on/off -> pause on/off
        "1:21"  -> putChar 'f'      -- TV on/off -> full screen on/off
        "11:42" -> putChar '#'      -- TV/VTR -> cycle audio tracks
        "11:39" -> putChar 'o'      -- tracking -> toggle OSD states
        "11:69" -> putStr "\ESC[D"  -- tracking ↓ -> 10s back (left arrow)
        "11:68" -> putStr "\ESC[C"  -- tracking ↑ -> 10s forward (right arrow)
        "11:62" -> putStr "\ESC[B"  -- slow - -> 1m back (down arrow)
        "11:61" -> putStr "\ESC[A"  -- slow + -> 1m forward (up arrow)
        "11:87" -> putStr "\ESC[6~" -- AMS « -> 10m back (PgDown)
        "11:86" -> putStr "\ESC[5~" -- AMS » -> 10m forward (PgUp)
        "1:16"  -> setVolume "+2%"  -- next channel -> increase master volume
        "1:17"  -> setVolume "-2%"  -- prev channel -> decrease master volume
        "11:22" | prevLine == "11:29" -> -- record + eject -> quit
          putChar 'q' >> exitSuccess
        _ -> return ()
      processLines curTime curLines
processLines _ _ = return ()
