module Paths_fractals (
    version,
    getBinDir, getLibDir, getDataDir, getLibexecDir,
    getDataFileName, getSysconfDir
  ) where

import qualified Control.Exception as Exception
import Data.Version (Version(..))
import System.Environment (getEnv)
import Prelude

catchIO :: IO a -> (Exception.IOException -> IO a) -> IO a
catchIO = Exception.catch


version :: Version
version = Version {versionBranch = [0,1,0,0], versionTags = []}
bindir, libdir, datadir, libexecdir, sysconfdir :: FilePath

bindir     = "/home/fe16/rh16216/linux/Desktop/PandA/Week 7/.cabal-sandbox/bin"
libdir     = "/home/fe16/rh16216/linux/Desktop/PandA/Week 7/.cabal-sandbox/lib/x86_64-linux-ghc-7.8.4/fractals-0.1.0.0"
datadir    = "/home/fe16/rh16216/linux/Desktop/PandA/Week 7/.cabal-sandbox/share/x86_64-linux-ghc-7.8.4/fractals-0.1.0.0"
libexecdir = "/home/fe16/rh16216/linux/Desktop/PandA/Week 7/.cabal-sandbox/libexec"
sysconfdir = "/home/fe16/rh16216/linux/Desktop/PandA/Week 7/.cabal-sandbox/etc"

getBinDir, getLibDir, getDataDir, getLibexecDir, getSysconfDir :: IO FilePath
getBinDir = catchIO (getEnv "fractals_bindir") (\_ -> return bindir)
getLibDir = catchIO (getEnv "fractals_libdir") (\_ -> return libdir)
getDataDir = catchIO (getEnv "fractals_datadir") (\_ -> return datadir)
getLibexecDir = catchIO (getEnv "fractals_libexecdir") (\_ -> return libexecdir)
getSysconfDir = catchIO (getEnv "fractals_sysconfdir") (\_ -> return sysconfdir)

getDataFileName :: FilePath -> IO FilePath
getDataFileName name = do
  dir <- getDataDir
  return (dir ++ "/" ++ name)
