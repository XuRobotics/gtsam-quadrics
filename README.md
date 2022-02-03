# GTSAM Quadrics: quadric landmarks for GTSAM

<!-- badges -->

[![Best of ACRV Repository](https://img.shields.io/badge/collection-best--of--acrv-%23a31b2a)](https://roboticvision.org/best-of-acrv)
[![License](https://img.shields.io/github/license/best-of-acrv/gtsam-quadrics)](./LICENSE.txt)

This repository contains an extension to the popular [GTSAM](https://github.com/borglab/gtsam) factor graph optimization library. We introduce constrained dual quadrics as GTSAM variables, and support the estimation of the quadric parameters using 2-D bounding box measurements. These tools are available for both C++ and Python and are designed to be used in conjunction with GTSAM. Also provided are a number of C++ and Python examples that demonstrate how quadric landmarks can be used in the SLAM context, alongside doxygen documentation and unit tests.

<p align="center">
<img alt="QuadricSLAM sample output image 1" src=https://github.com/best-of-acrv/gtsam-quadrics/raw/master/doc/quadricslam_still1.png width="400"/>
<img alt="QuadricSLAM sample output image 1" src=https://github.com/best-of-acrv/gtsam-quadrics/raw/master/doc/quadricslam_still2.png width="400"/>
</p>

We expect this repository to be active and continually improved upon. If you have any feature requests or experience any bugs, don't hesitate to let us know. Our code is free to use, and licensed under BSD-3. We simply ask that you [cite our work](#citing-our-work) if you use QuadricSLAM in your own research.

[![@youtube QuadricSLAM demonstration for RA-L](https://github.com/best-of-acrv/gtsam-quadrics/raw/master/doc/quadricslam_video.png)](https://www.youtube.com/watch?v=n-j0DFDFSKU)

## Installation

GTSAM Quadrics contains both C++ libraries, and Python wrappers for use in Python. We offer a number of different install methods, from single step methods to more involved depending on your desired use case.

Python wrappers can be installed via one of the following options:


### Building with CMake

This process will build the library from scratch using CMake. It is very similiar to the "from source" method above, we just use CMake directly to build instead of invoking it as part of the `pip` install process.

Ensure required system dependencies are installed, then clone with the `gtsam` submodule initialised:

```
git clone --recurse-submodules https://github.com/best-of-acrv/gtsam-quadrics
```

Create an out-of-source build directory:

```
cd gtsam_quadrics
mkdir build
cd build
```

Run the configuration and generation CMake steps, optionally building the Python wrapper using the `BUILD_PYTHON_WRAP` variable:

```
cmake -DBUILD_PYTHON_WRAP=ON ..
```

Run the build step:

```
cmake --build . -j$(nproc)
```

Then, install python dependencies and wrapper:
```
pip install pyparsing
pip install numpy
make python-install
```


Update PYTHONPATH (change the directory according to where this repo is cloned on your computer):
```
export PYTHONPATH=${PYTHONPATH}:$HOME/gtsam-quadrics:$HOME/gtsam-quadrics/build
```


Then optionally run any of the other supported targets as described below:

| **Target name** | **Description**                                |
| :-------------- | :--------------------------------------------- |
| check           | compile and run optional unit tests            |
| examples        | compiles the c++ examples                      |
| doc             | generates the doxygen documentation            |
| doc_clean       | removes the doxygen documentation              |
| install         | installs the gtsam_quadrics c++/python library |

_Note: documentation requires Doxygen (`sudo apt install doxygen`) and epstopdf (`sudo apt install texlive-font-utils`)_

For example, to install the library into system paths run:
```
cmake --build . --target install
```



## Planned developments

TODO check these are up-to-date

- High-level SLAM front-end (akin to ORBSLAM2)
- Support for expmap/logmap
- Tools to visualize and evaluate quadric landmarks

## Assorted notes and known issues

**How is the GTSAM dependency handled:** This repository includes GTSAM as a submodule, so you can see precisely which version we build against (currently it is post 4.1rc due to Python wrapper changes). The GTSAM Quadrics library is then built and linked against this copy of GTSAM to ensure compatibility. The Python module is also linked against the GTSAM Python module built from our submodule.

TODO need to check we handle this correctly
**If using GTSAM 4.0.3 or exponential-map rotations:** gtsam 4.0.3 moved to exponential map by default to parametrize rotations. The analytical derivatives we've calculated from this library are based on the cayley transform. Please either select cayley rotations in the gtsam CMakelists or use numerical derivatives (defined in boundingboxfactor.cpp).

**If you plan to use gtsam_quadrics in C++:** You can find the installed C++ headers using the cmake command `find_package(GTSAM_QUADRICS REQUIRED)` which will load `GTSAM_QUADRICS_INCLUDE_DIR`. The default header installation is `/usr/local/include/gtsam_quadrics/`, and by default library is installed to `/usr/local/lib/libgtsam_quadrics.so`.

**Adding Quadrics to `gtsam::Values`:** When using the python interface, ConstrainedDualQuadrics can be added or retrieved from Values using the following. Since GTSAM 4.0 the python interface for Values manually specializes each type. When supported by GTSAM, we plan to derive the gtsam::Values class and add the insert/at methods for ConstrainedDualQuadric.

```Python
quadric.addToValues(values, key)
quadric = gtsam_quadrics.ConstrainedDualQuadric.getFromValues(values, key)
```

TODO ensure this is still relevant?
**`AttributeError: module 'gtsam_quadrics' has no attribute 'ConstrainedDualQuadric'`:** If you import gtsam_quadrics and find it does not contain any attributes, or receive the above, ensure that gtsam_quadrics is built with BUILD_PYTHON_WRAP set ON, the correct python version is used, and that the generated gtsam_quadrics.so shared library is on your PYTHONPATH. I.e, if you have installed gtsam_quadrics, that you have the following line in your ~/.bashrc

```
export PYTHONPATH=$PYTHONPATH:/usr/local/cython/gtsam_quadrics
```

## Citing our work

If you are using this library in academic work, please cite the [publication](https://ieeexplore.ieee.org/document/8440105):

L. Nicholson, M. Milford and N. Sünderhauf, "QuadricSLAM: Dual Quadrics From Object Detections as Landmarks in Object-Oriented SLAM," in IEEE Robotics and Automation Letters, vol. 4, no. 1, pp. 1-8, Jan. 2019, doi: 10.1109/LRA.2018.2866205. [PDF](https://arxiv.org/abs/1804.04011).

```bibtex
@article{nicholson2019,
  title={QuadricSLAM: Dual Quadrics From Object Detections as Landmarks in Object-Oriented SLAM},
  author={Nicholson, Lachlan and Milford, Michael and Sünderhauf, Niko},
  journal={IEEE Robotics and Automation Letters},
  year={2019},
}
```
