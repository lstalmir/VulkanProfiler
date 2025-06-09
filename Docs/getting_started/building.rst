Building
========

This document describes in detail the layer build process.

.. tip::
    | Build step is optional if a prebuilt package has been downloaded from the `Releases page <https://github.com/lstalmir/VulkanProfiler/releases>`_.  
    | Packages are available for Windows 10 or later, Ubuntu 22.04 and Ubuntu 24.04.

Dependencies
------------

The layer depends on the following software for building:

.. topic:: Windows

    - Git client,
    - Visual Studio 2017 or later,
    - CMake 3.8 or later,
    - Python 3 with pip,
    - Python packages specified in VkLayer_profiler_layer/scripts/requirements.txt.

.. topic:: Linux

    .. note::
        The list below contains packages available on Ubuntu 22.04. The names may differ on other distributions.

    - git,
    - g++-8.3 or later,
    - cmake-3.8 or later,
    - extra-cmake-modules,
    - libxkbcommon-dev,
    - libx11-dev, libxext-dev (for Xlib support),
    - libxcb1-dev, libxcb-shape0-dev (for XCB support),
    - python3 and python3-pip,
    - Python packages specified in VkLayer_profiler_layer/scripts/requirements.txt.

.. topic:: Python packages

    The layer requires following Python packages to build it with full functionality.

    .. literalinclude:: ../../VkLayer_profiler_layer/scripts/requirements.txt
        :caption: VkLayer_profiler_layer/scripts/requirements.txt
        :lines: 21-

.. _building-with-cmake:

Building with CMake
-------------------

The layer uses CMake build management tool, so building it is straightforward.

1.  Clone the repository.

    .. code::

        git clone --recursive https://github.com/lstalmir/VulkanProfiler
        cd VulkanProfiler

2.  Install Python packages.

    .. code::

        python3 -m pip install -r VkLayer_profiler_layer/scripts/requirements.txt

3.  Generate the project files / makefiles in the build directory.

    .. code::

        mkdir cmake_build
        cd cmake_build
        cmake .. -DCMAKE_BUILD_TYPE=Release

4.  Build the layer.

    .. code::

        cmake --build . --config Release

Building with Visual Studio
---------------------------

The repository depends on CMake to generate Visual Studio project files. Running steps 1-3 from :ref:`building-with-cmake` will generate solution file in the build directory for the latest currently installed Visual Studio.

.. tip::
    To select other Visual Studio version, use CMake ``-G`` option and specify the generator, e.g., ``-G "Visual Studio 16 2017"``.

Building with Make
------------------

Unix makefiles is the default CMake generator on Linux systems. Build directory will contain makefiles after running steps 1-3 from :ref:`building-with-cmake`.

Once generated, the project can be built by running make:

.. code::

    make all

Building documentation
----------------------

The documentation is written in reStructuredText and uses Sphinx to generate HTML.

The packages required for building are listed in Docs/requirements.txt:

.. literalinclude:: /requirements.txt
    :caption: Docs/requirements.txt
    :lines: 21-

With the packages installed, sphinx-build tool can be used:

.. code::

    sphinx-build -M html ./Docs ./docs_build
