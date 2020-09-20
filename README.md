**ꟻLIP: A Difference Evaluator for Alternating Images
High Performance Graphics, 2020.**

by Pontus Andersson, Jim Nilsson, Tomas Akenine-Moller, Magnus Oskarsson, Kalle Astrom, and Mark D. Fairchild

Pointer to the paper: https://research.nvidia.com/publication/2020-07_FLIP

code by Pontus Andersson, Jim Nilsson, and Tomas Akenine-Moller

![Example image](README.png)

# Original README.txt

\#\#\# C++ \#\#\#

- Code written in Visual Studio 2019.
- Allows output either in the form of a full error map (magma color map or grayscale) or histogram.
- See `flip.exe -help` for usage instructions.
- Input images are assumed to be in sRGB space and in the [0,1] range.
- Pooling code is contained in `pooling.h`.
- FLIP is contained in `FLIP.cpp`.
