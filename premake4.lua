-- A solution contains projects, and defines the available configurations
solution "ckw"
configurations { "Release", "Debug" }

configuration "gmake Debug"
do
    buildoptions { "-g" }
    linkoptions { "-g" }
end

configuration "gmake"
do
  buildoptions { 
      "-Wall", 
  }
  linkoptions {
      --"-Wl,--entry,_wWinMain,--enable-stdcall-fixup",
      --"-s -mwindows -nostartfiles",
  }
end

configuration "gmake windows"
do
  buildoptions { 
      "-U__CYGWIN__", 
  }
end

configuration "vs*"
do
    flags {
        'WinMain',
    }
    linkoptions {}
end

configuration "windows*"
do
    defines {
        'WIN32',
        '_WIN32',
        '_WINDOWS',
    }
end

configuration "Debug"
do
  defines { "DEBUG" }
  flags { "Symbols" }
  targetdir "debug"
end

configuration "Release"
do
  defines { "NDEBUG" }
  flags { "Optimize" }
  targetdir "release"
end

configuration {}

------------------------------------------------------------------------------
-- Project
------------------------------------------------------------------------------
project "ckw"
--language "C"
language "C++"
--kind "StaticLib"
--kind "DynamicLib"
--kind "ConsoleApp"
kind "WindowedApp"
flags {
    "Unicode",
}
files {
    "*.cpp", "*.h", "*.rc",
}
defines {
    "UNICODE",
    "_UNICODE",
    "_WIN32_WINNT=0x0500",
}
includedirs {
}
libdirs {
}
links {
    'Shlwapi',
}

