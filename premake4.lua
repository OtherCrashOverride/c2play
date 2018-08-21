#!lua
local output = "./build/" .. _ACTION

solution "c2play_solution"
   configurations { "Debug", "Release" }

project "c2play"
   location (output)
   kind "ConsoleApp"
   language "C++"
   includedirs { "src/Media", "src/UI", "src/UI/Fbdev" }
   files { "src/**.h", "src/**.cpp" }
   excludes { "src/UI/X11/**" }
   buildoptions { "-std=c++11" }
   linkoptions { "-lavformat -lavcodec -lavutil -lpthread -lasound -lrt -lEGL -lGLESv2 -lass" }

   configuration "Debug"
      flags { "Symbols" }
      defines { "DEBUG", "MESA_EGL_NO_X11_HEADERS" }

   configuration "Release"
      flags { "Optimize" }
      defines { "" }

project "c2play-x11"
   location (output)
   kind "ConsoleApp"
   language "C++"
   includedirs { "src/Media", "src/UI", "src/UI/X11" }
   files { "src/**.h", "src/**.cpp" }
   excludes { "src/UI/Fbdev/**" }
   buildoptions { "-std=c++11 -Wall" }
   linkoptions { "-lavformat -lavcodec -lavutil -lpthread -lasound -lrt -lX11 -lEGL -lGLESv2 -lass" }
   defines { "X11" }

   configuration "Debug"
      flags { "Symbols" }
      defines { "DEBUG" }

   configuration "Release"
      flags { "Optimize" }
      defines { "" }
