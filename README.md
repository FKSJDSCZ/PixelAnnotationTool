PixelAnnotationTool
============================

Software that allows you to manually and quickly annotate images in directories.
The method is pseudo manual because it uses the algorithm [watershed marked](http://docs.opencv.org/3.1.0/d7/d1b/group__imgproc__misc.html#ga3267243e4d3f95165d55a618c65ac6e1) of OpenCV. The general idea is to manually provide the marker with brushes and then to launch the algorithm. If at first pass the segmentation needs to be corrected, the user can refine the markers by drawing new ones on the erroneous areas (as shown on video below).

### Example :

<img src="https://raw.githubusercontent.com/abreheret/PixelAnnotationTool/master/images_test/Abbey_Road.jpg" width="300"/> <img src="https://raw.githubusercontent.com/abreheret/PixelAnnotationTool/master/images_test/Abbey_Road_color_mask.png" width="300"/>

Little example from an user ([tenjumh](https://github.com/tenjumh/Pixel-Annotation-Tool)) of PixelAnnotationTools : https://www.youtube.com/watch?v=tX-xcg5wY4U

----------

### Building Dependencies :
* [Qt](https://www.qt.io/download-open-source/)  >= 6.x
* [CMake](https://cmake.org/download/) >= 2.8.x 
* [OpenCV](http://opencv.org/releases.html) >= 2.4.x 
* For Windows Compiler : Works under Visual Studio >= 2015

### License :

GNU Lesser General Public License v3.0 

Permissions of this copyleft license are conditioned on making available complete source code of licensed works and modifications under the same license or the GNU GPLv3. Copyright and license notices must be preserved. Contributors provide an express grant of patent rights. However, a larger work using the licensed work through interfaces provided by the licensed work may be distributed under different terms and without source code for the larger work.

[more](https://github.com/abreheret/PixelAnnotationTool/blob/master/LICENSE)

### Citation :

```bib
  @MISC{Breheret:2017,
    author = {Amaury Br{\'e}h{\'e}ret},
    title = {{Pixel Annotation Tool}},
    howpublished = "\url{https://github.com/abreheret/PixelAnnotationTool}",
    year = {2017},
  }
```


