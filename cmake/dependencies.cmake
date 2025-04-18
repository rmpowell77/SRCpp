# dependencies are managed by FetchContent.
# We use the convention of specifying the git tag completely, as that can
# avoid issues when a tag changes.

cmake_minimum_required(VERSION 3.23)

include(FetchContent)

FetchContent_Declare(
  libsamplerate
  GIT_REPOSITORY https://github.com/libsndfile/libsamplerate.git
  GIT_TAG        c96f5e3de9c4488f4e6c97f59f5245f22fda22f7
)
FetchContent_MakeAvailable(libsamplerate)
