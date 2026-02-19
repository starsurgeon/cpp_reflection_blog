# TODO 

## Test coverage
- integrate test coverage computation and reporting into the build pipeline

## Address sanitizer
- add an address sanitizer into the build pipeline (cmake)

## UB sanitizer
- add an UB sanitizer (cppcheck) into the build pipeline (cmake)

## GitHub
- execpt for the repository owner it shall be impossible to push to the main branch
- all other changes must be integrated using pull requests, provided
  - the code compile without warnings and errors before merging
  - address sanitizer does not report issues
  - UB sanitizer does not report issues
  - the code is properly formatted (using clang-format)

- after merging a pull request a build has to be performed to make sure nothing is broken

## Test fuzzing and mutating
- add FuzzTest support
- add mutate++ support

## other tools
- add include-what-you-use