
module Fireball where

import Data.Bits

type Point = (Double,Double,Double)

foreign export ccall fireball :: Double -> Double -> Double -> Double

  -- | Function to recreate the fireball from Hypertexture paper.
fireball
  :: Double -- ^ x.
  -> Double -- ^ y.
  -> Double -- ^ z.
  -> Double -- ^ Density.
fireball x y z = softSphere 0.3 0.3 (0.35, 0.3, 0.2) (n*x,n*y,n*z)
  where
    n         = (1 + (turbulence perm 1 8 0.01 True 64 1 (x,y,z)))

-- * Fireball Helper functions:
-- ============================

-- | Function to create a sphere that gets more dense towards its centre
softSphere
  :: Double -- ^ Radius
  -> Double -- ^ Scale
  -> Point  -- ^ Middle
  -> Point  -- ^ Point to find density at.
  -> Double -- ^ Density of point
softSphere r s c p 
  | d < r     = s * (1 - d/r)
  | otherwise = 0
  where
    d = euclideanDist c p

-- | Function that simulates 'turbulence' using 'noise'.
turbulence
  :: [Int]   -- ^ Randomly generated gradients.
  -> Double  -- ^ Scales noise, affecting the steepness of the gradient. [1-n]
  -> Int     -- ^ Squiggliness of noise (produced by brownian noise). [0-n]
  -> Double  -- ^ Density of noise [0-1].
  -> Bool    -- ^ Whether or not noise should be inverted.
  -> Double  -- ^ Scale. Must be a power of 2.
  -> Double  -- ^ Pixel size.
  -> Point   -- ^ Point to find noise at. Given as a value [(-1)-1].
  -> Double  -- ^ Noise value [0-1] at point.
turbulence grads scale squigg density' inverted s p (x,y,z)
  | s < p = 0
  | otherwise
  = abs ((noise grads scale squigg density' inverted (x*s,y*s,z*s)) * 1/s)
  +  (turbulence grads scale squigg density' inverted (s / 2) p (x,y,z))

-- | Returns the noise value of a pixel in a 2d image. Attributes of
--   noise can be adjusted using the arguments.
noise
  :: [Int]  -- ^ Randomly generated gradients.
  -> Double -- ^ Scales noise, affecting the steepness of the gradient. [1-n]
  -> Int    -- ^ Squiggliness of noise (produced by brownian noise). [0-n]
  -> Double -- ^ Density of noise [0-1].
  -> Bool   -- ^ Whether or not noise should be inverted.
  -> Point  -- ^ Point to find noise at.
  -> Double -- ^ Noise value [0-1] at point.
noise grads scale squigg density' inverted p = 
  let 
    i = case inverted of 
      True  -> 0 -- will make it mostly black
      False -> 1 -- will make it mostly white
  in 
    min 1 $ max 0 ((*scale) . realToFrac $ i + brownian3d grads p squigg)
    -- min and max ensure it is in the range even after scaling etc.s

-- | Applies perlin noise n times, making the noise more squiggly.
brownian3d :: [Int] -> Point -> Int -> Double
brownian3d gs (x,y,z) 0 = perlinNoise3d gs (x,y,z) -- noise only once
brownian3d gs (x,y,z) n = (1/m) * perlinNoise3d gs (x*m, y*m, z*m) + brownian3d gs (x,y,z) (n-1) -- smaller noise, scaled to be not too squiggly
  where
    m = 2^n

-- | Function to find the noise value at a specific point in a 3D area.
--   Diagram showing where local variables are:
perlinNoise3d
  :: [Int]  -- ^ Randomly generated gradients.
  -> Point  -- ^ Point to find noise at. (should be in range [0,1])
  -> Double -- ^ Noise value.
perlinNoise3d gs (x,y,z) 
  = lerp fw
      (lerp fv
        (lerp fu 
          (gradient (gs !! (gs !! ((gs !! i0) + j0)) + k0) u v w)      -- aaa
          (gradient (gs !! (gs !! ((gs !! i1) + j0)) + k0) u' v w))    -- baa
        (lerp fu 
          (gradient (gs !! (gs !! ((gs !! i0) + j1)) + k0) u v' w)     -- aba
          (gradient (gs !! (gs !! ((gs !! i1) + j1)) + k0) u' v' w)))  -- bba
      (lerp fv
        (lerp fu 
          (gradient (gs !! (gs !! ((gs !! i0) + j0)) + k1) u v w')     -- aab
          (gradient (gs !! (gs !! ((gs !! i1) + j0)) + k1) u' v w'))   -- bab
        (lerp fu 
          (gradient (gs !! (gs !! ((gs !! i0) + j1)) + k1) u v' w')    -- abb
          (gradient (gs !! (gs !! ((gs !! i1) + j1)) + k1) u' v' w'))) -- bbb
    where
    -- points in big grid
      x0 = (floor x) :: Int
      y0 = (floor y) :: Int
      z0 = (floor z) :: Int
      x1 = x0 + 1 
      y1 = y0 + 1
      z1 = z0 + 1
    -- indexes to extract gradient
      i0 = floor x .&. 255
      j0 = floor y .&. 255
      k0 = floor z .&. 255
      i1 = i0 + 1    
      j1 = j0 + 1
      k1 = k0 + 1  
    -- points in cell
      u  = x - (fromIntegral x0) -- x coordinate in baby square
      v  = y - (fromIntegral y0)
      w  = z - (fromIntegral z0)
      u' = u-1
      v' = v-1
      w' = w-1
    -- weights
      fu = fade u 
      fv = fade v
      fw = fade w

-- | Fade function from "Improving Noise" paper by Ken Perlin. This is used
--   so soften the interpolation.
--
-- >>> fade 2
-- 32.0
--
-- >>> fade 0.5
-- 0.5
--
-- >>> fade 2.5
-- 156.25
--
fade :: Double -> Double
fade t = t * t * t * (t * (t * 6 - 15) + 10)
      
-- | Finds linear interpolation between two values a and b with a weight t.
--   Performs the dot product and weighted mixing from the Perlin Noise
--   algorithm. 
--
-- >>> lerp 0.5 2 5
-- 3.5
--
lerp 
  :: Double -- ^ Weight [0,1] dictation how much of a Vs. b is represented.
  -> Double -- ^ First random vector based on x0.
  -> Double -- ^ Second random vector based on x1.
  -> Double -- ^ Weighted mixture.
lerp t a b = a + t * (b - a)
      
-- | Helps generate deterministically random gradient from element of 
--   perm list and coordinates of point.
gradient :: Int -> Double -> Double -> Double -> Double
gradient hash x y z = case hash .&. 15 of
  0  ->  x + y
  1  -> -x + y
  2  ->  x - y
  3  -> -x - y
  4  ->  x + z
  5  -> -x + z
  6  ->  x - z
  7  -> -x - z
  8  ->  y + z
  9  -> -y + z
  10 ->  y - z
  11 -> -y - z
  12 ->  y + x
  13 -> -y + z
  14 ->  y - x
  15 -> -y - z

-- | Function to find the euclidean distance between two 3D points
--
-- >>> euclideanDist ((-1),2,3) (4,0,(-3))
-- 8.06225774829855
--
euclideanDist :: Point -> Point -> Double
euclideanDist (x1,y1,z1) (x2,y2,z2) = sqrt $ (x1-x2)^2 + (y1-y2)^2 + (z1-z2)^2 

-- | Look up table for gradients
perm :: [Int]
perm = (\p -> p ++ p)
  [ 151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225
  , 140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148
  , 247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32
  ,  57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175
  ,  74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122
  ,  60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54
  ,  65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169
  , 200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64
  ,  52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212
  , 207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213
  , 119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9
  , 129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104
  , 218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241
  ,  81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157
  , 184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93
  , 222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180
  ]