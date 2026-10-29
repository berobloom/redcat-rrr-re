cmake_minimum_required(VERSION 3.16)
project(redcat C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

# Format core: no SDL, so it can be tested headless.
add_library(rccore STATIC
  src/anims_tab.c
  src/archive.c
  src/gfx8.c
  src/race.c
  src/reken.c
  src/menu.c
  src/podium.c
  src/smd.c
  src/highscore.c
  src/audio.c
  src/rrrmenu_anims_tab.c
  src/reken_screen.c
  src/menu_anims_tab.c
  src/smacker.c
)
target_include_directories(rccore PUBLIC src)
target_link_libraries(rccore PUBLIC m)

add_executable(rc_dump src/rc_dump.c)
target_link_libraries(rc_dump PRIVATE rccore)

add_executable(race_sim src/race_sim.c)
target_link_libraries(race_sim PRIVATE rccore)

add_executable(reken_sim src/reken_sim.c)
target_link_libraries(reken_sim PRIVATE rccore)

# The viewer needs SDL2; skip it (with a note) when it is missing so the core
# and rc_dump still build.
find_package(SDL2 QUIET)
if(SDL2_FOUND)
  add_executable(rcview
    src/main.c
  )
  target_include_directories(rcview PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(rcview PRIVATE rccore ${SDL2_LIBRARIES})

  add_executable(redcat
    src/game.c
  )
  target_include_directories(redcat PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(redcat PRIVATE rccore ${SDL2_LIBRARIES})
else()
  message(STATUS "SDL2 not found: building rccore and rc_dump only")
endif()
