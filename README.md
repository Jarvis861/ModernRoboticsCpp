# Modern Robotics:  Mechanics, Planning, and Control
# C++ Library

This repository contains the code library accompanying [_Modern Robotics:
Mechanics, Planning, and Control_](http://modernrobotics.org) (Kevin Lynch
and Frank Park, Cambridge University Press 2017). The
[user manual](https://github.com/NxRLab/ModernRobotics/blob/master/doc/MRlib.pdf) is in the doc directory of [main repository](https://github.com/NxRLab/ModernRobotics/).

The functions are available in:

* C++
* [Python](https://github.com/NxRLab/ModernRobotics/tree/master/packages/Python)
* [MATLAB](https://github.com/NxRLab/ModernRobotics/tree/master/packages/Matlab)
* [Mathematica](https://github.com/NxRLab/ModernRobotics/tree/master/packages/Mathematica)

Each function has a commented section above it explaining the inputs required for its use as well as an example of how it can be used and what the output will be. This repository also contains a pdf document that provides an overview of the available functions using MATLAB syntax. Functions are organized according to the chapter in which they are introduced in the book. Basic functions, such as functions to calculate the magnitude of a vector, normalize a vector, test if the value is near zero, and perform matrix operations such as multiplication and inverses, are not documented here.

The primary purpose of the provided software is to be easy to read and educational, reinforcing the concepts in the book. The code is optimized neither for efficiency nor robustness.

## Installation

### 1. Install Eigen library.
* On Mac
```
brew install eigen
```
* On Linux
```
sudo apt-get install libeigen3-dev
```

### 2. Prepare build
Creat a folder and Open
```
mkdir build && cd build
```

Configure with defaults for only building a library
```
cmake ..
```
Or configure with [googletest](https://github.com/google/googletest.git) test. The catch.hpp has been in the repo.
```
cmake .. -DLIBRARY_TEST=1
```

Build library
```
make all
```

Install library in your local paths (optional)
```
make install
```

## Test the library
The step including googletest is necessary.
```
./lib_test
```